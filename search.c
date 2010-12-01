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

#include "search.h"
#include "extern.h"

plan_t plan;

static int init_plan(plan_t *);
static void walk(dl_node **);
static void free_mt(match_t **);
static void cleanup(int);

int
main(int argc, char *argv[])
{
  
  extern char *optarg;
  extern int optind;
  
  int i;
  static struct stat *stbuf;
  
  (void)setlocale(LC_CTYPE, "");
  signal(SIGINT, cleanup);

  if (init_plan(&plan) < 0) {
	warnx("init_plan()");
	cleanup(1);
	exit (1);
  }
  
  if (lookup_options(argc, argv, &plan) < 0) {
	warnx("lookup_options()");
	cleanup(1);
	exit (1);
  }
  
  argc -= optind;
  argv += optind;
    
  if (comp_regex(plan.mt) < 0) {
	cleanup (1);
	exit (1);
  }

  if (argc == 0 &&
	  dl_empty(&(plan.paths))) {
	display_usage();
	cleanup(1);
	exit (1);
  }
  
  if (argc > 0) {
	for (i = 0; i < argc && argv[i]; i++)
	  dl_append(argv[i], &(plan.paths));
  }
  
  if (plan.flags & OPT_SORT)
	dl_sort(&(plan.paths));
  dl_foreach(&(plan.paths), walk);

  cleanup(1);
  exit(0);
}

static int
init_plan(plan_t *p)
{
  p->mt = (match_t *)malloc(sizeof(match_t));
  p->stat = (node_stat_t *)malloc(sizeof(node_stat_t));
  p->paths = dl_init();
  
  if (p->mt == NULL ||
	  p->stat == NULL ||
	  p->paths == NULL)
	return (-1);

  p->flags = OPT_NONE;
  p->type = NT_UNKNOWN;
  p->odev = 0;
  p->stat_func = lstat;
  p->exec_func = exec_name;
  p->mt->mflag = REG_BASIC;
  bzero(p->group, LINE_MAX);
  bzero(p->user, LINE_MAX);
  bzero(p->mt->pattern, LINE_MAX);
  
  return (0);
}

static void
walk(dl_node **n)
{
  dl_node *np = *n;
  
  if (np == NULL)
	return;

  walk_through(np->ent, np->ent, &plan);
}

static void
free_mt(match_t **m)
{
  match_t *mt = *m;

  if (mt == NULL)
	return;

  if (&(mt->fmt) != NULL) {
	regfree(&(mt->fmt));
  }
  
  free(mt);
  mt = NULL;
  return;
}

static void
cleanup(int sig)
{
  if (plan.paths != NULL) {
	dl_free(&(plan.paths));
	plan.paths = NULL;
  }
  
  if (plan.mt != NULL) {
	free_mt(&(plan.mt));
	plan.mt = NULL;
  }
  
  if (plan.stat != NULL) {
	free(plan.stat);
	plan.stat = NULL;
  }
  
  if (sig)
	exit(0);
}
