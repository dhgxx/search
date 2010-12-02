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
static int find_plan(int, char **, plan_t *);
static void walk(dl_node **);
static void free_mt(match_t **);
static void cleanup(int);

int
main(int argc, char *argv[])
{
  int i;
  
  extern char *optarg;
  extern int optind;
    
  (void)setlocale(LC_CTYPE, "");
  signal(SIGINT, cleanup);

  if (init_plan(&plan) < 0) {
	warn("init_plan()");
	cleanup(1);
	exit (1);
  }

  if (find_plan(argc, argv, &plan) < 0) {
	warn("find_plan()");
	cleanup(1);
	exit (1);
  }
  
   
  if (plan.acq_flags & OPT_SORT)
	dl_sort(&(plan.acq_paths));
  dl_foreach(&(plan.acq_paths), walk);

  cleanup(1);
  exit(0);
}

static int
init_plan(plan_t *p)
{
  if (p == NULL)
	return (-1);
  
  if ((p->acq_mt = (match_t *)malloc(sizeof(match_t))) == NULL ||
	  (p->acq_args = (args_t *)malloc(sizeof(args_t))) == NULL ||
	  (p->acq_paths = dl_init()) == NULL) {
	cleanup(1);
	return (-1);
  }
    
  return (0);
}

static int
find_plan(int argc, char **argv, plan_t *p)
{
  int i;
  
  if (lookup_options(argc, argv, p) < 0) {
	warn("lookup_options()");
	return (-1);
  }
  
  argc -= optind;
  argv += optind;
    
  if (comp_regex(p->acq_mt) < 0) {
	return (-1);
  }

  if (argc == 0 && dl_empty(&(p->acq_paths))) {
	display_usage();
	return (-1);
  }
  
  if (argc > 0) {
	for (i = 0; i < argc && argv[i]; i++)
	  dl_append(argv[i], &(p->acq_paths));
  }

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
  if (plan.acq_paths != NULL) {
	dl_free(&(plan.acq_paths));
	plan.acq_paths = NULL;
  }
  
  if (plan.acq_mt != NULL) {
	free_mt(&(plan.acq_mt));
	plan.acq_mt = NULL;
  }
  
  if (plan.acq_args != NULL) {
	free(plan.acq_args);
	plan.acq_args = NULL;
  }
  
  if (sig)
	exit(0);
}
