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
  
extern char *optarg;
extern int optind;

plan_t plan;
plist_t exec_list;

static void cleanup(int);

int
main(int argc, char *argv[])
{
  int i;
  int ret;
    
  (void)setlocale(LC_CTYPE, "");
  signal(SIGINT, cleanup);

  if (init_plan(&plan, &exec_list) < 0) {
#ifdef _DEBUG_
	warnx("init_plan() failed!\n");
#endif
	cleanup(1);
	exit (1);
  }
  
  if (lookup_options(argc, argv, &plan) < 0) {
#ifdef _DEBUG_
	warnx("lookup_options() failed!\n");
#endif
	cleanup(1);
	exit (1);
  }
  argc -= optind;
  argv += optind;
  
  if (find_plan(argc, argv, &plan) < 0) {
#ifdef _DEBUG_
	warnx("find_plan() failed!\n");
#endif
	cleanup(1);
	exit (1);
  }

  add_plan(&plan, &exec_list);

  ret = exec_plan(&plan, &exec_list);

#ifdef _DEBUG_
  warnx("ret=%d\n", ret);
#endif

  cleanup(1);
  return (ret);
}

static void
cleanup(int sig)
{
  free_plan(&exec_list);
  
  if (plan.acq_paths != NULL) {
	dl_free(&(plan.acq_paths));
	plan.acq_paths = NULL;
  }
  
  if (plan.acq_mt != NULL) {
	free(plan.acq_mt);
	plan.acq_mt = NULL;
  }
  
  if (plan.acq_args != NULL) {
	free(plan.acq_args);
	plan.acq_args = NULL;
  }

  if (plan.nstat != NULL) {
	free(plan.nstat);
	plan.nstat = NULL;
  }
  
  if (sig)
	exit(0);
}
