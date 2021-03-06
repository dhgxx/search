/*
 * Copyright (c) 2005-2010 Denise H. G. <darcsis@gmail.com>
 * All rights reserved
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

#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <sysexits.h>

#include "search.h"

static __inline void out(const char *);
static void dislink(const char *, NODE);
static int  regexcomp(match_t *);
static int  nodestat(const char *, plan_t *, int (*) (const char *, struct stat *));
static void walk_through(const char *, plan_t *);


int s_getids(const char *, plan_t *);
int s_regex(const char *, plan_t *);
int s_name(const char *, plan_t *);
int s_stat(const char *, plan_t *);
int s_lstat(const char *, plan_t *);
int s_gid(const char *, plan_t *);
int s_uid(const char *, plan_t *);
int s_empty(const char *, plan_t *);
int s_xdev(const char *, plan_t *);
int s_sort(const char *, plan_t *);
int s_delete(const char *, plan_t *);
int s_path(const char *, plan_t *);
int s_nogroup(const char *, plan_t *);
int s_nouser(const char *, plan_t *);
int s_version(const char *, plan_t *);
int s_usage(const char *, plan_t *);

#define NIDS 2048

static struct passwd *pwd;
static struct group *grp;
static struct _ids {
  int uids[NIDS];
  int gids[NIDS];
} ids;

static __inline void
out(const char *s)
{
  if (s == NULL)
	return;
  
  (void)fprintf(stdout, "%s\n", s);
}

int
s_getids(const char *name __unused, plan_t *p __unused)
{
  int i;

  errno = 0;
  
  for (i = 0; i < NIDS; i++) {
	ids.uids[i] = ids.gids[i ] = -1;
  }

  i = 0;
  while ((pwd = getpwent()) != NULL) {
#ifdef _DEBUG_	
	warnx("uid=%d", ids.uids[i]);
#endif
	ids.uids[i] = pwd->pw_uid;
	i++;
  }

  i = 0;
  while ((grp = getgrent()) != NULL) {
#ifdef _DEBUG_
	warnx("gid=%d", ids.gids[i]);
#endif
	ids.gids[i] = grp->gr_gid;
	i++;
  }

  endpwent();
  endgrent();

  return (errno == 0? 0 : -1);
}

static void
dislink(const char *path, NODE type)
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

static int
regexcomp(match_t *mt)
{
  int ret;
  unsigned int plen, mflag;
  static char msg[LINE_MAX];
  static char *pattern;
  static regex_t *fmt;

  if (mt == NULL)
	return (-1);
  
  mflag = mt->mflag;
  plen = strlen(mt->pattern);
    
  bzero(msg, LINE_MAX);
  fmt = &(mt->fmt);

  /* defaults to search all types. */
  if (plen == 0)
	pattern = ".*";
  else
	pattern = mt->pattern;

  ret = regcomp(fmt, pattern, mflag);

  if (ret != 0) {
	if (regerror(ret, fmt, msg, LINE_MAX) > 0) {
	  errx(EX_DATAERR, "-r %s: %s",
		   pattern, msg);
	} else {
	  errx(EX_DATAERR, "-r %s", pattern);
	}
	if (fmt) {
	  regfree(fmt);
	  fmt = NULL;
	}
  }
  
  if (ret == 0)
	return (0);
  else
	return (-1);
}

static int
nodestat(const char *name, plan_t *p,
	int (*stat_f) (const char *s, struct stat *buf))
{
  int ret;
  static struct stat stbuf;
  static DIR *dirp;
  static struct dirent *dir;

  if (name == NULL)
	return (NT_ERROR);
  if (p == NULL)
	return (NT_ERROR);
  if (stat_f == NULL)
	return (NT_ERROR);
  if (p->nstat == NULL)
	return (NT_ERROR);

  if ((ret = stat_f(name, &stbuf)) < 0) {
	warn("%s", name);
	return (-1);
  }

  p->nstat->empty = 1;
  p->nstat->gid = stbuf.st_gid;
  p->nstat->uid = stbuf.st_uid;
  p->nstat->dev = stbuf.st_dev;

  if (S_ISBLK(stbuf.st_mode))
	p->nstat->type = NT_ISBLK;
  if (S_ISCHR(stbuf.st_mode))
	p->nstat->type = NT_ISCHR;
  if (S_ISDIR(stbuf.st_mode))
	p->nstat->type = NT_ISDIR;
  if (S_ISFIFO(stbuf.st_mode))
	p->nstat->type = NT_ISFIFO;
  if (S_ISLNK(stbuf.st_mode))
	p->nstat->type = NT_ISLNK;
  if (S_ISREG(stbuf.st_mode))
	p->nstat->type = NT_ISREG;
  if (S_ISSOCK(stbuf.st_mode))
	p->nstat->type = NT_ISSOCK;

  if (p->nstat->type != NT_ISDIR &&
      p->nstat->type == NT_ISREG) {
	if (stbuf.st_size != 0)
	  p->nstat->empty = 0;
  } else {
	/* code from from BSD find(1) */
	if ((dirp = opendir(name)) != NULL) {
	  for (dir = readdir(dirp); dir; dir = readdir(dirp))
		if (dir->d_name[0] != '.' ||
			(dir->d_name[1] != '\0' &&
			 (dir->d_name[1] != '.' || dir->d_name[2] != '\0'))) {
		  p->nstat->empty = 0;
		  break;
		}
	  closedir(dirp);
	/* code from FreeBSD find(1). */
    } else {
    /* we can't access the dir, but we'll assume the  */
    /* dir is NOT empty, just like 'find' does.       */
       p->nstat->empty = 0;
    }
  }
  
  return (0);
}

static void
walk_through(const char *name, plan_t *p)
{
  int retval;
  char tmp_buf[MAXPATHLEN];
  static struct dirent *dir;
  static DIR *dirp;
  struct dlist *paths;
  plist_t *pl;

  if (name == NULL ||
	  p == NULL ||
	  p->plans == NULL ||
	  p->nstat == NULL)
	return;

  if ((paths = dl_init()) == NULL)
	return;

  retval = 0;
  
  pl = p->plans;

  pl->cur = pl->start;
  while (pl->cur != NULL) {

	/* bypass s_path() */
	if (pl->cur->exec == 1) {
	  pl->retval = (retval |= pl->cur->s_func(name, p));
#ifdef _DEBUG_
	  warnx("%s: retval=%d", pl->cur->func_name, pl->retval);
#endif
	}

	if (pl->cur)
	  pl->cur = pl->cur->next;
  }

  if (p->args->need_xdev) {
	if (retval != 0) {
	  dl_free(paths);
	  paths = NULL;
	  return;
	}
  }
  
  if (retval == 0) {
	out(name);
  }

  if (p->nstat->type != NT_ISDIR) {
	dl_free(paths);
	paths = NULL;
	return;
  }
  
  if (NULL == (dirp = opendir(name))) {
	warn("%s", name);
	dl_free(paths);
	paths = NULL;
	return;
  }
  
  while (NULL != (dir = readdir(dirp))) {
	
	if ((0 == strncmp(dir->d_name, ".", strlen(dir->d_name) + 1)) ||
		(0 == strncmp(dir->d_name, "..", strlen(dir->d_name) + 1))) {
	  continue;
	}

	bzero(tmp_buf, MAXPATHLEN);
	strlcpy(tmp_buf, name, MAXPATHLEN);
	if ('/' != tmp_buf[strlen(tmp_buf) - 1])
	  strlcat(tmp_buf, "/", MAXPATHLEN);
	strlcat(tmp_buf, dir->d_name, MAXPATHLEN);

	dl_append(tmp_buf, paths);
  }

  closedir(dirp);
  
  if (p->args->need_sort) {
	dl_sort(paths);
  }

  paths->cur = paths->head;
  while (paths->cur != NULL) {
	walk_through(paths->cur->ent, p);
	if (paths->cur)
	  paths->cur = paths->cur->next;
  }
  
  dl_free(paths);
  paths = NULL;
  return;
}

int
s_regex(const char *name, plan_t *p)
{
  int ret, plen, matched;
  static char *pattern, *d_name, msg[LINE_MAX];
  static regex_t *fmt;
  static regmatch_t pmatch;

  if (name == NULL)
	return (-1);
  if (p == NULL)
	return (-1);
  if (p->mt == NULL)
	return (-1);
  if (regexcomp(p->mt) < 0)
	return (-1);

  if ((d_name = basename(name)) == NULL)
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
	  errx(1, "%s: %s",
		   pattern, msg);
	} else {
	  errx(1, "%s", pattern);
	}
  } else {

	matched = ((ret == 0) && (pmatch.rm_so == 0) && (pmatch.rm_eo == plen));
  
#ifdef _DEBUG_
	warnx("exec_regex: pattern=%s, name=%s :%s MATCHED!",
		  pattern, d_name, ((matched == 0) ? "" : "NOT"));
#endif
  }
  
  if (fmt) {
	regfree(fmt);
	fmt = NULL;
  }
  
  return ((matched == 1) ? (0) : (-1));
}

int
s_name(const char *name, plan_t *p)
{
  int matched;
  unsigned int plen, mflag;
  char *pattern, *d_name;

  if (name == NULL)
	return (-1);
  if (p == NULL)
	return (-1);
  if (p->mt == NULL)
	return (-1);

  if ((d_name = basename(name)) == NULL)
	return (-1);
  
  mflag = 0;
  pattern = p->mt->pattern;
  plen = strlen(pattern);
  matched = FNM_NOMATCH;
  
  if (plen == 0)
	pattern = "*";
#ifdef _DEBUG_
  else {
	warnx("pattern=%s, plen=%d", pattern, plen);
	}
#endif
  
  if (p->mt->mflag & REG_ICASE) {
	mflag = FNM_CASEFOLD | FNM_PERIOD | FNM_PATHNAME | FNM_NOESCAPE;
  }
  
  matched = fnmatch(pattern, d_name, mflag);
  
#ifdef _DEBUG_
  warnx("name: pattern=%s, name=%s :%s MATCHED!",
		pattern, d_name, ((matched == 0) ? "" : "NOT"));
#endif

  return ((matched == 0) ? (0) : (-1));
}

int
s_stat(const char *name, plan_t *p)
{
  if (name == NULL)
	return (-1);
  if (p == NULL)
	return (-1);

  return (nodestat(name, p, &stat));
}

int
s_lstat(const char *name, plan_t *p)
{
  if (name == NULL)
	return (-1);
  if (p == NULL)
	return (-1);

  return (nodestat(name, p, &lstat));
}

int
s_gid(const char *name __unused, plan_t *p)
{
  gid_t id;
  char *s;
  static struct group *grp;

  if (p == NULL)
	return (-1);
  if (p->args == NULL)
	return (-1);
  if (p->nstat == NULL)
	return (-1);

  id = strtol(p->args->sgid, &s, 0);
  if (s[0] == '\0')
	grp = getgrgid(id);
  else
	grp = getgrnam(s);
  
  if (grp == NULL) {
	errx(EX_NOUSER, "--group: %s: no such group", p->args->sgid);
  } else if (grp->gr_gid != p->nstat->gid) {
	return (-1);
  }
  
  return (0);
}

int
s_uid(const char *name __unused, plan_t *p)
{
  uid_t id;
  char *s;
  static struct passwd *pwd;

  if (p == NULL)
	return (-1);
  if (p->args == NULL)
	return (-1);
  if (p->nstat == NULL)
	return (-1);
  
  id = strtol(p->args->suid, &s, 0);
  if (s[0] == '\0')
	pwd = getpwuid(id);
  else
	pwd = getpwnam(s);
  
  if (pwd == NULL) {
	errx(EX_NOUSER, "--user: %s: no such user", p->args->suid);
  } else if (pwd->pw_uid != p->nstat->uid) {
	return (-1);
  }
  
  return (0);
}

int
s_empty(const char *name __unused, plan_t *p)
{
  if (p == NULL)  
	return (-1);
  if (p->nstat == NULL)
	return (-1);

  if (p->nstat->empty)
	return (0);

  return (-1);
}

int
s_xdev(const char *name, plan_t *p)
{
  if (p == NULL)
	return (-1);
  if (p->args == NULL)
	return (-1);
  if (p->nstat == NULL)
	return (-1);

  if (p->args->odev == 0)
	p->args->odev = p->nstat->dev;
  if (p->nstat->dev != p->args->odev) {
	if (p->nstat->type == NT_ISDIR) {
	  out(name);
	}
	return (-1);
  }

  return (0);
}

int
s_sort(const char *name __unused, plan_t *p) {

  if (p == NULL)
	return (-1);
  if (p->paths == NULL)
	return (-1);

  dl_sort(p->paths);
  return (0);
}

int
s_delete(const char *name, plan_t *p)
{
  if (name == NULL)
	return (-1);
  if (p == NULL)
	return (-1);
  if (p->plans == NULL)
	return (-1);

  if (p->plans->retval != 0) {
	return (-1);
  }

  if ((0 == strncmp(name, ".", strlen(name) + 1)) ||
      (0 == strncmp(name, "..", strlen(name) + 1))) {
	return (-1);
  }

  if (p->nstat->type == NT_ISDIR) {
	dl_append(name, p->rdirs);
#ifdef _DEBUG_	
	warnx("directory: `%s' to be deleted", name);
#endif	
  } else {
    dl_append(name, p->rfiles);
#ifdef _DEBUG_	
	warnx("file: `%s' to be deleted", name);
#endif	
  }
  /* upon -1, the results will not be printed out */
  return (-1);
}

int
s_path(const char *name __unused, plan_t *p)
{  
  if (p == NULL) {
	return (-1);
  }
  
  if (p->paths == NULL ||
	  dl_empty(p->paths))
	return (-1);
 
  p->paths->cur = p->paths->head; 
  while (p->paths->cur != NULL) {
#ifdef _DEBUG_
	warnx("walking through: %s", p->paths->cur->ent);
#endif
	walk_through(p->paths->cur->ent, p);
	if (p->paths->cur)
	  p->paths->cur = p->paths->cur->next;
  }
  
 /*  --- delete files ---
  *  must be here. because we have to wait for
  *  walk_through() to terminate.
  */
  if (p->rfiles && !dl_empty(p->rfiles)) {
	p->rfiles->cur = p->rfiles->head;
	while (p->rfiles->cur != NULL) {
#ifdef _DEBUG_
	  warnx("%s deleted!", p->rfiles->cur->ent);
#endif
	  /* to delete files, we have no need to specify its file type */
	  dislink(p->rfiles->cur->ent, NT_UNKNOWN);
	  if (p->rfiles->cur)
		p->rfiles->cur = p->rfiles->cur->next;
	}
  }

  /* delete dirs */
  if (p->rdirs) {
	p->rdirs->cur = p->rdirs->head;
	while (p->rdirs->cur != NULL) {
#ifdef _DEBUG_		
	  warnx("%s deleted!", p->rdirs->cur->ent);
#endif
	  dislink(p->rdirs->cur->ent, NT_ISDIR);
	  if (p->rdirs->cur)
		p->rdirs->cur = p->rdirs->cur->next;
	}
  }

  return (0);
}

int
s_type(const char *s __unused, plan_t *p)
{
  if (p == NULL ||
	  p->args == NULL ||
	  p->nstat == NULL)
	return (-1);
#ifdef _DEBUG_
  warnx("want: %d, actual: %d\n",
		p->args->type, p->nstat->type);
#endif
  return ((p->args->type == p->nstat->type) ? (0) : (-1));
}


int
s_nogroup(const char *name __unused, plan_t *p)
{
  int i;

  if (p == NULL)
	return (-1);
  if (p->nstat == NULL)
	return (-1);

  for (i = 0; ids.gids[i] >= 0; i++) {
	if (p->nstat->gid == ids.gids[i])
	  return (-1);
  }
  
  return (0);
}

int
s_nouser(const char *name __unused, plan_t *p)
{
  int i;

  if (p == NULL)
	return (-1);
  if (p->nstat == NULL)
	return (-1);

  for (i = 0; ids.uids[i] >= 0; i++) {
	if (p->nstat->uid == ids.uids[i])
	  return (-1);
  }

  return (0);
}

int
s_usage(const char *s __unused, plan_t *p __unused)
{  
  static const char *usage = "usage:\t%s [-EILPsxv]\
 ...\
 [-f|--path ...]\
 [-n|--name ...]\
 [-r|--regex ...]\
 [-t|--type ...]\
 [...]\n\
 \t%s [-EILPsxv]\
 -f|--path ...\
 [...]\
 [-n|--name ...]\
 [-r|--regex ...]\
 [-t|--type ...]\
 [...]\n";

  (void)fprintf(stderr,	usage,
				SEARCH_NAME, SEARCH_NAME);
  return (0);
}

int
s_version(const char *s __unused, plan_t *p __unused)
{  
  (void)fprintf(stderr,	"%s version %s\n",
				SEARCH_NAME, SEARCH_VERSION);
  return (0);
}
