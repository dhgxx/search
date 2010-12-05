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


static int gettype(const char *, plan_t *);
static void out(const char *);
static void dislink(const char *, NODE);
static int comp_regex(match_t *);
static void walk_through(const char *, plan_t *);

int s_regex(const char *, plan_t *);
int s_name(const char *, plan_t *);
int s_gid(const char *, plan_t *);
int s_uid(const char *, plan_t *);
int s_empty(const char *, plan_t *);
int s_xdev(const char *, plan_t *);
int s_sort(const char *, plan_t *);
int s_stat(const char *, plan_t *);
int s_lstat(const char *, plan_t *);
int s_delete(const char *, plan_t *);
int s_path(const char *, plan_t *);
int s_version(const char *, plan_t *);
int s_usage(const char *, plan_t *);

int
s_regex(const char *name, plan_t *p)
{
  int ret, plen, matched;
  char *pattern, msg[LINE_MAX];
  static regex_t *fmt;
  regmatch_t pmatch;

  if (name == NULL)
	return (-1);
  if (p == NULL)
	return (-1);
  if (p->acq_mt == NULL)
	return (-1);
  if (comp_regex(p->acq_mt) < 0)
	return (-1);

  fmt = &(p->acq_mt->fmt);
  pattern = p->acq_mt->pattern;
  plen = strlen(name);
  
  bzero(msg, LINE_MAX);
  pmatch.rm_so = 0;
  pmatch.rm_eo = plen;
  matched = 0;

  ret = regexec(fmt, name, 1, &pmatch, REG_STARTEND);

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
		pattern, name);
#endif
  
  matched = ((ret == 0) && (pmatch.rm_so == 0) && (pmatch.rm_eo == plen));
  return ((matched == 1) ? (0) : (-1));
}

int
s_name(const char *name, plan_t *p)
{
  int matched;
  unsigned int plen, mflag;
  char *pattern;

  if (name == NULL)
	return (-1);
  if (p == NULL)
	return (-1);
  if (p->acq_mt == NULL)
	return (-1);
  
  mflag = 0;
  pattern = p->acq_mt->pattern;
  plen = strlen(pattern);
  matched = FNM_NOMATCH;
  
  if (plen == 0)
	pattern = "*";
  
  if (p->acq_mt->mflag & REG_ICASE) {
	mflag = FNM_CASEFOLD | FNM_PERIOD | FNM_PATHNAME | FNM_NOESCAPE;
  }

#ifdef _DEBUG_
  warnx("exec_name: pattern=%s, name=%s",
		pattern, name);
#endif
  
  matched = fnmatch(pattern, name, mflag);
  return ((matched == 0) ? (0) : (-1));
}

int
gettype(const char *name, plan_t *p)
{
  int ret;
  static struct stat stbuf;

  if (name == NULL)
	return (NT_ERROR);
  if (p == NULL)
	return (NT_ERROR);
  if (p->nstat == NULL)
	return (NT_ERROR);

  if (p->acq_flags & OPT_STAT)
	ret = stat(name, &stbuf);
  else
	ret = lstat(name, &stbuf);

  if (ret < 0) {
	warn("gettype()");
	return (-1);
  }

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
  
  if (stbuf.st_size == 0)
	p->nstat->empty = 1;
  else
	p->nstat->empty = 0;
  
  return (0);
}


int
s_gid(const char *name __unused, plan_t *p)
{
  gid_t id;
  char *s;
  static struct group *grp;

  if (p == NULL)
	return (-1);
  if (p->acq_args == NULL)
	return (-1);
  if (p->nstat == NULL)
	return (-1);
  if (p->nstat->uid < 0)
	return (-1);
  
  id = strtol(p->acq_args->sgid, &s, 0);
  if (s[0] == '\0')
	grp = getgrgid(id);
  else
	grp = getgrnam(s);
  
  if (grp == NULL) {
	warnx("--group: %s: no such group", p->acq_args->sgid);
	return (-1);
  }

  if (grp->gr_gid == p->nstat->gid)
	return (0);
  
  return (-1);
}

int
s_uid(const char *name __unused, plan_t *p)
{
  uid_t id;
  char *s;
  static struct passwd *pwd;

  if (p == NULL)
	return (-1);
  if (p->acq_args == NULL)
	return (-1);
  if (p->nstat == NULL)
	return (-1);
  
  id = strtol(p->acq_args->suid, &s, 0);
  if (s[0] == '\0')
	pwd = getpwuid(id);
  else
	pwd = getpwnam(s);
  
  if (pwd == NULL) {
	warnx("--user: %s: no such user", p->acq_args->suid);
	return (-1);
  }

  if (pwd->pw_uid == p->nstat->uid)
	return (0);
  
  return (-1);
}

int s_empty(const char *name __unused, plan_t *p)
{
  if (p == NULL)  
	return (-1);
  if (p->nstat == NULL)
	return (-1);

  if (p->nstat->empty)
	return (0);

  return (-1);
}

int s_xdev(const char *name __unused, plan_t *p)
{
  if (p == NULL)
	return (-1);
  if (p->acq_args == NULL)
	return (-1);
  if (p->nstat == NULL)
	return (-1);

  if (p->acq_args->odev == 0)
	p->acq_args->odev = p->nstat->dev;
  if (p->nstat->dev != p->acq_args->odev)
	return (-1);

  return (0);
}

int s_sort(const char *name __unused, plan_t *p) {

  if (p == NULL)
	return (-1);
  if (p->acq_paths == NULL)
	return (-1);
  if (dl_empty(&(p->acq_paths)))
	return (-1);

  dl_sort(&(p->acq_paths));
  return (0);
}

int s_stat(const char *name, plan_t *p) {
  return (-1);
}

int s_lstat(const char *name, plan_t *p) {
  return (-1);
}

int s_delete(const char *name, plan_t *p) {
  return (-1);
}

int s_path(const char *name __unused, plan_t *p)
{
  if (p == NULL ||
	  p->acq_paths == NULL ||
	  dl_empty(&(p->acq_paths)))
	return (-1);

  p->acq_paths->cur = p->acq_paths->head;
  while (p->acq_paths->cur != NULL) {
	walk_through(p->acq_paths->cur->ent, p);
	if (p->acq_paths->cur != NULL) {
	  p->acq_paths->cur = p->acq_paths->cur->next;
	}
	dl_clear(&(p->acq_paths));
  }

  return (0);
}

static void
walk_through(const char *name, plan_t *p)
{
  char tmp_buf[MAXPATHLEN];
  static struct dirent *dir;
  DIR *dirp;

  if (name == NULL ||
	  p == NULL ||
	  p->nstat == NULL ||
	  dl_empty(&(p->acq_paths)))
	return;
    
  if (gettype(name, p) < 0) {
	warn("%s", name);
	return;
  }

  if (p->nstat->type != NT_ISDIR) {
	dl_append(name, &(p->acq_paths));
	return;
  }
  
  if (NULL == (dirp = opendir(name))) {
	warn("%s", name);
	return;
  }
  
  while (NULL != (dir = readdir(dirp))) {
	
	if ((0 == strncmp(dir->d_name, ".", strlen(dir->d_name) + 1)) ||
		(0 == strncmp(dir->d_name, "..", strlen(dir->d_name) + 1))) {
	  continue;
	}

	bzero(tmp_buf, MAXPATHLEN);
	strncpy(tmp_buf, name, MAXPATHLEN);
	if ('/' != tmp_buf[strlen(tmp_buf) - 1])
	  strncat(tmp_buf, "/", MAXPATHLEN);
	strncat(tmp_buf, dir->d_name, MAXPATHLEN);

	out(tmp_buf);
	dl_append(tmp_buf, &(p->acq_paths));
  }
  
  closedir(dirp);

  return;
}

static int
comp_regex(match_t *mt)
{
  int ret;
  unsigned int plen, mflag;
  char msg[LINE_MAX];
  char *pattern;
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

static void
out(const char *s)
{
  if (s == NULL)
	return;
  
  (void)fprintf(stdout, "%s\n", s);
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



