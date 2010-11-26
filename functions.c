/*
 * Copyright (c) 2005-2010 Denise H. G. All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <libgen.h>
#include <pwd.h>
#include <grp.h>

#include "search.h"

static void out(const char *, plan_t *);
static node_t get_type(const char *, plan_t *);
static int cook_entry(const char *, const char *, plan_t *);
static int tell_group(const char *, const gid_t);
static int tell_user(const char *, const uid_t);
static void dislink(dl_node **);
static void list_free(DLIST **);

int comp_regex(plan_t *);
int exec_regex(const char *, plan_t *);
int exec_name(const char *, plan_t *);
void walk_through(const char *, const char *, plan_t *);

int
exec_name(const char *d_name, plan_t *p)
{
  int matched;
  unsigned int plen, mflag;
  char *pattern;

  if (d_name == NULL)
	return (-1);
  if (p == NULL)
	return (-1);
  if (p->mt == NULL)
	return (-1);
  
  mflag = 0;
  pattern = p->mt->pattern;
  plen = strlen(pattern);
  matched = FNM_NOMATCH;
  
  if (plen == 0)
	pattern = "*";
  
  if (p->opt->flags & OPT_ICAS) {
	mflag = FNM_CASEFOLD | FNM_PERIOD | FNM_PATHNAME | FNM_NOESCAPE;
  }
  
#ifdef _DEBUG_
  (void)fprintf(stderr,	"pattern=%s, name=%s\n",
				pattern, d_name);
#endif
  matched = fnmatch(pattern, d_name, mflag);
    
  return ((matched == 0) ? (0) : (-1));
}

int
comp_regex(plan_t *p)
{
  int ret;
  unsigned int plen, mflag;
  char msg[LINE_MAX];
  char *pattern;
  static regex_t *fmt;

  if (p == NULL)
	return (-1);
  if (p->opt == NULL)
	return (-1);
  if (p->mt == NULL)
	return (-1);
  
  mflag = REG_BASIC;
  plen = strlen(p->mt->pattern);
  if (p->opt->flags & OPT_ICAS)
	mflag |= REG_ICASE;
    
  bzero(msg, LINE_MAX);
  fmt = &(p->mt->fmt);

  /* defaults to search all types. */
  if (plen == 0)
	pattern = ".*";
  else
	pattern = p->mt->pattern;

  ret = regcomp(fmt, pattern, mflag);

  if (ret != 0) {
	if (regerror(ret, fmt, msg, LINE_MAX) > 0) {
	  (void)fprintf(stderr, "%s: %s: %s\n",
					SEARCH_NAME, pattern, msg);
	} else {
	  (void)fprintf(stderr, "%s: %s: %s\n",
					SEARCH_NAME, pattern, strerror(errno));
	}
	regfree(fmt);
	return (-1);
  }
  return (0);
}


int
exec_regex(const char *d_name, plan_t *p)
{
  int ret, plen, matched;
  char *pattern, msg[LINE_MAX];
  static regex_t *fmt;
  regmatch_t pmatch;

  if (d_name == NULL)
	return (-1);
  if (p == NULL)
	return (-1);
  if (p->mt == NULL)
	return (-1);

  fmt = &(p->mt->fmt);
  pattern = p->mt->pattern;
  plen = strlen(d_name);
  
  bzero(msg, LINE_MAX);
  pmatch.rm_so = 0;
  pmatch.rm_eo = plen;
  matched = 0;

  ret = regexec(fmt, d_name, 1, &pmatch, REG_STARTEND);

  if (ret != 0 && ret != REG_NOMATCH) {
	if (regerror(ret, fmt, msg, LINE_MAX) > 0) {
	  (void)fprintf(stderr, "%s: %s: %s\n",
					SEARCH_NAME, pattern, msg);
	} else {
	  (void)fprintf(stderr, "%s: %s: %s\n",
					SEARCH_NAME, pattern, strerror(errno));
	}
	regfree(fmt);
	return (-1);
  }
  
  matched = ((ret == 0) && (pmatch.rm_so == 0) && (pmatch.rm_eo == plen));
  return ((matched == 1) ? (0) : (-1));
}

void
walk_through(const char *n_name, const char *d_name, plan_t *p)
{
  int ret;
  unsigned int nents;
  char *pbase, tmp_buf[MAXPATHLEN];
  static struct dirent *dir;
  DIR *dirp;
  DLIST *dlist;

  if (n_name == NULL)
	return;
  if (d_name == NULL)
	return;
  if (p == NULL)
	return;
  if (p->opt == NULL)
	return;
  if (p->stat == NULL)
	return;
    
  if (get_type(n_name, p) == NT_ERROR) {
	(void)fprintf(stderr, "%s: %s: %s\n",
				  SEARCH_NAME, n_name, strerror(errno));
	return;
  }

  nents = 0;
  dlist = dl_init();
  
  ret = cook_entry(n_name, d_name, p);

  if (ret == 0)
	out(n_name, p);
  
  if (p->stat->type == NT_ISDIR) {

	if (p->opt->flags & OPT_XDEV) {
	  if (p->opt->odev == 0) {
		p->opt->odev = p->stat->dev;
#ifdef _DEBUG_
		(void)fprintf(stderr, "%s (dev=%d, odev=%d)\n",
					  d_name, p->stat->dev, p->opt->odev);
#endif
	  }
	  if (p->stat->dev != p->opt->odev) {
		list_free(&dlist);
		return;
	  }
	}

	if (NULL == (dirp = opendir(n_name))) {
	  (void)fprintf(stderr,	"%s: %s: %s\n",
					SEARCH_NAME, n_name, strerror(errno));
	  list_free(&dlist);
	  return;
	}

	while (NULL != (dir = readdir(dirp))) {
	  if ((0 != strncmp(dir->d_name, ".", strlen(dir->d_name) + 1)) &&
		  (0 != strncmp(dir->d_name, "..", strlen(dir->d_name) + 1))) {
		nents++;
		bzero(tmp_buf, MAXPATHLEN);
		strncpy(tmp_buf, n_name, MAXPATHLEN);
		if ('/' != tmp_buf[strlen(tmp_buf) - 1])
		  strncat(tmp_buf, "/", MAXPATHLEN);
		strncat(tmp_buf, dir->d_name, MAXPATHLEN);
		dl_append(tmp_buf, &dlist);
	  }
	}

	/* We have to count dir entries before */
	/* we can tell if or not a directory is */
	/* empty. If it's empty, then display it. */
	if (p->opt->flags & OPT_EMPTY) {
	  if (nents == 0)
		out(n_name, p);
	}

	/* Sort the search result if */
	/* necessary. */
	if (p->opt->flags & OPT_SORT)
	  dl_sort(&dlist);
	dlist->cur = dlist->head;
	while (dlist->cur != NULL) {
	  if (dlist->cur->ent != NULL) {
		pbase = basename(dlist->cur->ent);
		walk_through(dlist->cur->ent, pbase, p);
	  }
	  dlist->cur = dlist->cur->next;
	}
	
	closedir(dirp);
  }
  
  list_free(&dlist);
  return;	
}

static void
out(const char *s, plan_t *p)
{
  if (s == NULL)
	return;
  if (p == NULL)
	return;
  if (p->opt == NULL)
	return;
  if (p->opt->flags & OPT_DEL)
	return;
  
  (void)fprintf(stdout, "%s\n", s);
  return;
}

static node_t
get_type(const char *d_name, plan_t *p)
{
  static struct stat stbuf;

  if (d_name == NULL)
	return (NT_ERROR);
  if (p == NULL)
	return (NT_ERROR);
  if (p->opt == NULL)
	return (NT_ERROR);
  if (p->stat == NULL)
	return (NT_ERROR);
  if (p->stat_func(d_name, &stbuf)  < 0 )
	return (NT_ERROR);
  
  p->stat->empty = 0;
  p->stat->gid = stbuf.st_gid;
  p->stat->uid = stbuf.st_uid;
  p->stat->dev = stbuf.st_dev;

  if (S_ISBLK(stbuf.st_mode))
	p->stat->type = NT_ISBLK;
  if (S_ISCHR(stbuf.st_mode))
	p->stat->type = NT_ISCHR;
  if (S_ISDIR(stbuf.st_mode))
	p->stat->type = NT_ISDIR;
  if (S_ISFIFO(stbuf.st_mode))
	p->stat->type = NT_ISFIFO;
  if (S_ISLNK(stbuf.st_mode))
	p->stat->type = NT_ISLNK;
  if (S_ISREG(stbuf.st_mode))
	p->stat->type = NT_ISREG;
  if (S_ISSOCK(stbuf.st_mode))
	p->stat->type = NT_ISSOCK;
#ifndef _OpenBSD_
  if (S_ISWHT(stbuf.st_mode))
	p->stat->type = NT_ISWHT;
#endif
  if (p->stat->type != NT_ISDIR) {
	if (stbuf.st_size == 0) {
	  p->stat->empty = 1;
#ifdef _DEBUG_
	  (void)fprintf(stderr, "%s: empty file.\n", d_name);
#endif
	}
  }
  
  return (NT_UNKNOWN);
}

static int
cook_entry(const char *n_name, const char *d_name, plan_t *p)
{
  unsigned int found;

  found = 0;
  
  if ((0 == p->exec_func(d_name, p)) &&
	  ((p->opt->o_type == NT_UNKNOWN) ||
	   (p->opt->o_type == p->stat->type))) {

	found = 1;
	
	if (p->opt->flags & OPT_EMPTY) {
	  if (p->stat->empty != 1)
		found = 0;
	}

	if (p->opt->flags & OPT_GRP) {
	  if (0 != tell_group(p->opt->group, p->stat->gid))
		found = 0;
	}
	
	if (p->opt->flags & OPT_USR) {
	  if (0 != tell_user(p->opt->user, p->stat->uid))
		found = 0;
	}
  }

  return (found == 1 ? (0) : (-1));
}

static int
tell_group(const char *sgid, const gid_t gid)
{
  gid_t id;
  char *p;
  static struct group *grp;
  
  if (sgid == NULL)
	return (-1);

  if (gid < 0)
	return (-1);

  id = strtol(sgid, &p, 0);
  if (p[0] == '\0')
	grp = getgrgid(id);
  else
	grp = getgrnam(p);
  
  if (grp == NULL) {
	(void)fprintf(stderr, "%s: --group: %s: no such group\n",
				  SEARCH_NAME, sgid);
	exit (0);
  }

  if (grp->gr_gid == gid)
	return (0);
  
  return (-1);
}

static int
tell_user(const char *suid, const uid_t uid)
{
  uid_t id;
  char *p;
  static struct passwd *pwd;

  if (suid == NULL)
	return (-1);

  if (uid < 0)
	return (-1);
  
  id = strtol(suid, &p, 0);
  if (p[0] == '\0')
	pwd = getpwuid(id);
  else
	pwd = getpwnam(p);
  
  if (pwd == NULL) {
	(void)fprintf(stderr, "%s: --user: %s: no such user\n",
				  SEARCH_NAME, suid);
	exit (0);
  }

  if (pwd->pw_uid == uid)
	return (0);
  
  return (-1);
}

static void
dislink(dl_node **n)
{
  int ret;
  dl_node *np = *n;
  static struct stat stbuf;
  
  if (np == NULL)
	return;

  if (np->deleted == 1) {
#ifdef _DEBUG_
	(void)fprintf(stderr, "%s: in delete list.\n", np->ent);
#endif
	stat(np->ent, &stbuf);
	
	if(S_ISDIR(stbuf.st_mode)) {
	  ret = rmdir(np->ent);
	  if (ret < 0) {
		(void)fprintf(stderr, "%s: --rmdir(%s): %s\n",
					  SEARCH_NAME, np->ent, strerror(errno));
	  }
	} else {
	  ret = unlink(np->ent);
	  if (ret < 0) {
		(void)fprintf(stderr, "%s: --unlink(%s): %s\n",
					  SEARCH_NAME, np->ent, strerror(errno));
	  }
	}
  }
}

static void
list_free(DLIST **list)
{
  DLIST *listp = *list;
  
  if (listp == NULL)
	return;

  dl_free(&listp);
  listp = NULL;
}

