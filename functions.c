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

static unsigned int delete = 0;

static void out(const char *);
static node_t get_type(const char *, plan_t *);
static int cook_entry(const char *, const char *, plan_t *);
static int tell_group(const char *, const gid_t);
static int tell_user(const char *, const uid_t);
static void dislink(const char *, node_t);
static void list_clear(DLIST **);

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
  warnx("exec_name: pattern=%s, name=%s",
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
	  warnx("%s: %s",
			pattern, msg);
	} else {
	  warn("%s", pattern);
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
	  warnx("%s: %s",
			pattern, msg);
	} else {
	  warn("%s", pattern);
	}
	regfree(fmt);
	return (-1);
  }

#ifdef _DEBUG_
  warnx("exec_regex: pattern=%s, name=%s",
		pattern, d_name);
#endif
  
  matched = ((ret == 0) && (pmatch.rm_so == 0) && (pmatch.rm_eo == plen));
  return ((matched == 1) ? (0) : (-1));
}

void
walk_through(const char *n_name, const char *d_name, plan_t *p)
{
  int matched;
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
	warn("%s", n_name);
	return;
  }

  nents = 0;
  dlist = dl_init();
  
  matched = cook_entry(n_name, d_name, p);

  if (matched) {
	if (!(p->opt->flags & OPT_DEL))
	  out(n_name);
	else
	  delete = 1;
  }
  
  if (p->stat->type != NT_ISDIR) {
	if (matched && delete)
	  dislink(n_name, p->stat->type);
	list_clear(&dlist);
	return;
  }

  if (p->opt->flags & OPT_XDEV) {
	if (p->opt->odev == 0)
	  p->opt->odev = p->stat->dev;
	if (p->stat->dev != p->opt->odev) {
	  list_clear(&dlist);
	  return;
	}
  }

  if (NULL == (dirp = opendir(n_name))) {
	warn("%s", n_name);
	list_clear(&dlist);
	return;
  }

  while (NULL != (dir = readdir(dirp))) {
	
	if ((0 == strncmp(dir->d_name, ".", strlen(dir->d_name) + 1)) ||
		(0 == strncmp(dir->d_name, "..", strlen(dir->d_name) + 1))) {
	  continue;
	}
	  
	nents++;
	bzero(tmp_buf, MAXPATHLEN);
	strncpy(tmp_buf, n_name, MAXPATHLEN);
	if ('/' != tmp_buf[strlen(tmp_buf) - 1])
	  strncat(tmp_buf, "/", MAXPATHLEN);
	strncat(tmp_buf, dir->d_name, MAXPATHLEN);
	dl_append(tmp_buf, &dlist);
  }

  /* We have to count dir entries before */
  /* we can tell if or not a directory is */
  /* empty. If it's empty, then display it. */
  if (p->opt->flags & OPT_EMPTY) {
	if (p->opt->o_type == NT_ISDIR ||
		p->opt->o_type == NT_UNKNOWN ||
		p->stat->type == p->opt->o_type) {
	  if (nents == 0) {
		if (!(p->opt->flags & OPT_DEL)) {
		  out(n_name);
		} else {
		  dislink(n_name, p->stat->type);
		}

		list_clear(&dlist);
		closedir(dirp);
		return;
	  }
	}
  }

  /* Sort the search result if */
  /* necessary. */
  if (p->opt->flags & OPT_SORT) {
	dl_sort(&dlist);
  }
  
  dlist->cur = dlist->head;
  while (dlist->cur) {
	if (dlist->cur->ent != NULL) {
	  pbase = basename(dlist->cur->ent);
	  walk_through(dlist->cur->ent, pbase, p);
	}
	dlist->cur = dlist->cur->next;
  }
  
  closedir(dirp);
  list_clear(&dlist);

  if (matched && delete)
	dislink(n_name, p->stat->type);

  return;
}

static void
out(const char *s)
{
  if (s == NULL)
	return;
  
  (void)fprintf(stdout, "%s\n", s);
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
  
  if (stbuf.st_size == 0)
	p->stat->empty = 1;
  else
	p->stat->empty = 0;
  
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
	  if (!(p->stat->empty))
		found = 0;
	}

	if (p->opt->flags & OPT_GRP) {
	  if (0 != tell_group(p->group, p->stat->gid))
		found = 0;
	}
	
	if (p->opt->flags & OPT_USR) {
	  if (0 != tell_user(p->user, p->stat->uid))
		found = 0;
	}
  }

  return (found);
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
	errx(0, "--group: %s: no such group", sgid);
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
	errx(0, "--user: %s: no such user", suid);
  }

  if (pwd->pw_uid == uid)
	return (0);
  
  return (-1);
}

static void
dislink(const char *path, node_t type)
{  
  if (path == NULL)
	return;
  
  if ((0 == strncmp(path, ".", strlen(path) + 1)) ||
	  (0 == strncmp(path, "..", strlen(path) + 1)))
	return;
  
#ifdef _DEBUG_
  warnx("dislink(%s): %s: to be deleted.", path, path);
#endif
  
  if(type == NT_ISDIR) {
	if (rmdir(path) < 0) {
	  warn("--rmdir(%s)", path);
	}
  } else {
	if (unlink(path) < 0) {
	  warn("--unlink(%s)", path);
	}
  }
}

static void
list_clear(DLIST **list)
{
  DLIST *listp = *list;
  
  if (listp == NULL)
	return;

  dl_free(&listp);
  listp = NULL;
}

