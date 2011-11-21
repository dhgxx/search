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

#include "search.h"

extern int s_regex(const char *, plan_t *);
extern int s_name(const char *, plan_t *);
extern int s_stat(const char *, plan_t *);
extern int s_lstat(const char *, plan_t *);
extern int s_gid(const char *, plan_t *);
extern int s_uid(const char *, plan_t *);
extern int s_empty(const char *, plan_t *);
extern int s_xdev(const char *, plan_t *);
extern int s_sort(const char *, plan_t *);
extern int s_delete(const char *, plan_t *);
extern int s_path(const char *, plan_t *);
extern int s_type(const char *, plan_t *);
extern int s_version(const char *, plan_t *);
extern int s_usage(const char *, plan_t *);

static const FLAGS flags[] = {
  /* ===== order start ===== */
  { OPT_VERSION, &s_version, "version" },
  { OPT_USAGE,   &s_usage,   "usage" },
  { OPT_SORT,    &s_sort,    "sort" },
  { OPT_PATH,    &s_path,    "path" },
  { OPT_STAT,    &s_stat,    "stat" },
  { OPT_LSTAT,   &s_lstat,   "lstat" },
  /* ===== order end ===== */
  { OPT_EMPTY,   &s_empty,   "empty" },
  { OPT_GRP,     &s_gid,     "gid" },
  { OPT_USR,     &s_uid,     "uid" },
  { OPT_NAME,    &s_name,    "name" },
  { OPT_REGEX,   &s_regex,   "regex" },
  { OPT_TYPE,    &s_type,    "type" },
  /* ===== order start ===== */
  { OPT_XDEV,    &s_xdev,    "xdev" },
  { OPT_DEL,     &s_delete,  "delete" },
  /* ===== order end ===== */
  { OPT_NONE,    NULL,        NULL },
};

static int plan_add(unsigned int *, plist_t *);
static int plan_execute(plan_t *);

int  init_plan(plan_t *);
int  find_plan(int, char **, plan_t *);
int  execute_plan(plan_t *);
int  add_plan(plan_t *);
void free_plan(plist_t **);

int
init_plan(plan_t *p)
{  
  if (p == NULL)
	return (-1);
  
  if ((p->mt = (match_t *)malloc(sizeof(match_t))) == NULL ||
	  (p->args = (args_t *)malloc(sizeof(args_t))) == NULL ||
	  (p->nstat = (nstat_t *)malloc(sizeof(nstat_t))) == NULL ||
	  (p->plans = (plist_t *)malloc(sizeof(plist_t))) == NULL ||
	  (p->paths = dl_init()) == NULL) {
	return (-1);
  }

  bzero(p->mt->pattern, LINE_MAX);
  p->mt->mflag = REG_BASIC;
  p->args->odev = 0;
  p->args->empty = 0;
  p->flags = OPT_NONE | OPT_NAME | OPT_LSTAT;
  
  p->plans->cur = p->plans->start = NULL;
  p->plans->size = 0;
    
  return (0);
}

int
find_plan(int argc, char **argv, plan_t *p)
{
  int i, ret;
  
  if (argv == NULL || p == NULL)
	return (-1);
  
  if (argc == 0) {
	if (dl_empty(p->paths)) {
	  p->flags |= OPT_USAGE;
	}
	return (0);
  }
  
  if (argc > 0) {
	for (i = 0; i < argc && argv[i]; i++) {
#ifdef _DEBUG_
	  warnx("added path %s", argv[i]);
#endif
	  ret = dl_append(argv[i], p->paths);
	}
	p->flags |= OPT_PATH;
  }

  return (ret);
}

int
add_plan(plan_t *p)
{
  if (p == NULL)
	return (-1);

  return (plan_add(&(p->flags), p->plans));
}

int
execute_plan(plan_t *p)
{
  int ret;

  if (p == NULL)
	return (-1);

  return (plan_execute(p));
}

void
free_plan(plist_t **plist)
{
  int i;
  PLAN *tmp;
  plist_t *pl = *plist;
  
  if (pl == NULL)
	return;

  i = 0;
  if ((pl->cur = pl->start) == NULL) {
#ifdef _DEBUG_
	warnx("no plan!");
#endif
  }
  
#ifdef _DEBUG_
  warnx("We have %d plan(s) to free!", pl->size);
#endif
  
  while (pl->cur != NULL) {
	tmp = pl->cur;
	pl->cur = pl->cur->next;
#ifdef _DEBUG_
	warnx("free plan (%d)", i);
#endif
	tmp->s_func = NULL;
	free(tmp);
	tmp = NULL;
	i++;
  }

  if (pl != NULL) {
	free(pl);
	pl = NULL;
  }
}

static int
plan_add(unsigned int *fl, plist_t *pl)
{
  int i;
  PLAN *new, *tmp;

  if (fl == NULL || pl == NULL)
	return (-1);
  
  for (i = 0; flags[i].s_func != NULL; i++) {

	if (*fl & flags[i].opt) {
	  
	  if ((new = (PLAN *)malloc(sizeof(PLAN))) == NULL)
		continue;
	  
	  new->next = NULL;
	  new->s_func = flags[i].s_func;
	  
	  if (pl->start == NULL) {
		pl->cur = pl->start = new;
		pl->size++;
	  } else {
		tmp = pl->cur = pl->start;
		while (pl->cur != NULL) {
		  tmp = pl->cur;
		  pl->cur = pl->cur->next;
		}
		tmp->next = new;
		pl->cur = new;
		pl->size++;
	  }

#ifdef _DEBUG_
	  warnx("added plan(%d): %s", pl->size, flags[i].name);
#endif

	  *fl &= ~(flags[i].opt);
	  if (flags[i].opt & OPT_VERSION)
		return (0);
	  if (flags[i].opt & OPT_USAGE)
		return (0);
	}
  }

  return ((i > 0) ? (0) : (-1));
}

static int
plan_execute(plan_t *p)
{
  if (p == NULL)
	return (-1);

  if ((p->plans->cur = p->plans->start) == NULL)
	return (-1);

  while (p->plans->cur != NULL) {
	p->plans->retval = p->plans->cur->s_func(NULL, p);
	if (p->plans->cur)
	  p->plans->cur = p->plans->cur->next;
  }

  return (p->plans->retval);
}
