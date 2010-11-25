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
  
  plan.opt = (options_t *)malloc(sizeof(options_t));
  plan.mt = (match_t *)malloc(sizeof(match_t));
  plan.stat = (node_stat_t *)malloc(sizeof(node_stat_t));
  
  if (plan.opt == NULL ||
	  plan.mt == NULL ||
	  plan.stat == NULL) {
	(void)fprintf(stderr, "%s: malloc(3): %s.\n",
				  SEARCH_NAME, strerror(errno));
	cleanup(1);
	exit(1);
  }

  plan.opt->n_type = NT_UNKNOWN;
  plan.stat_func = lstat;
  plan.exec_func = exec_name;
  plan.opt->odev = 0;
  plan.opt->flags = OPT_NONE;
  bzero(plan.opt->path, MAXPATHLEN);
  bzero(plan.mt->pattern, LINE_MAX);
  
  lookup_options(argc, argv, &plan);
  
  argc -= optind;
  argv += optind;

  if ((argc == 0) &&
	  (OPT_PATH != (plan.opt->flags & OPT_PATH)))
	display_usage();
  
  if (comp_regex(&plan) < 0) {
	cleanup (1);
	exit (1);
  }

  if (plan.opt->flags & OPT_PATH) {
	walk_through(plan.opt->path, plan.opt->path, &plan);
  }

  if (argc > 0) {
	for (i = 0; i < argc && argv[i]; i++)
	  walk_through(argv[i], argv[i], &plan);
  }

  cleanup(1);
  exit(0);
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
  if (plan.mt != NULL) {
	free_mt(&(plan.mt));
	plan.mt = NULL;
  }
  
  if (plan.opt != NULL) {
	free(plan.opt);
	plan.opt = NULL;
  }
  
  if (plan.stat != NULL) {
	free(plan.stat);
	plan.stat = NULL;
  }
  
  if (sig)
	exit(0);
}
