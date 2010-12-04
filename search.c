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

static int lookup_options(int, char *[], plan_t *);
static void ftype_err(const char *);
static void cleanup(int);

plan_t plan;
plist_t exec_list;


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

  ret = execute_plan(&plan, &exec_list);

#ifdef _DEBUG_
  warnx("ret=%d\n", ret);
#endif

  cleanup(1);
  return (ret);
}

static int
lookup_options(int argc, char *argv[], plan_t *p)
{
  int ch, ret;
  static int opt_empty;
  static int opt_delete;
  
  static struct option longopts[] = {
	{ "gid",     required_argument, NULL,        2  },
	{ "group",   required_argument, NULL,        3  },
	{ "path",    required_argument, NULL,       'f' },
	{ "name",    required_argument, NULL,       'n' },
	{ "regex",   required_argument, NULL,       'r' },
	{ "type",    required_argument, NULL,       't' },
	{ "user",    required_argument, NULL,        4  },
	{ "uid",     required_argument, NULL,        5  },
	{ "empty",   no_argument,       &opt_empty,  1  },
	{ "delete",  no_argument,       &opt_delete, 1  },
	{ "sort",    no_argument,       NULL,       's' },
	{ "version", no_argument,       NULL,       'v' },
	{ "xdev",    no_argument,       NULL,       'x' },
	{ NULL,      0,                 NULL,        0  }
  };

  if (p == NULL ||
	  p->acq_mt == NULL ||
	  p->acq_args == NULL)
	return (-1);

  ret = 0;

  while ((ch = getopt_long(argc, argv, "EILPsvxf:n:r:t:", longopts, NULL)) != -1)
	switch (ch) {
	case 2:
	case 3:
	  p->acq_flags |= OPT_GRP;
	  bzero(p->acq_args->sgid, LINE_MAX);
	  strncpy(p->acq_args->sgid, optarg, LINE_MAX);
	  break;
	case 4:
	case 5:
	  p->acq_flags |=  OPT_USR;
	  bzero(p->acq_args->suid, LINE_MAX);
	  strncpy(p->acq_args->suid, optarg, LINE_MAX);
	  break;
	case 'f':
	  p->acq_flags |= OPT_PATH;
	  dl_append(optarg, &(p->acq_paths));
	  break;
	case 'n':
	  p->acq_flags |= OPT_NAME;
	  bzero(p->acq_mt->pattern, LINE_MAX);
	  strncpy(p->acq_mt->pattern, optarg, LINE_MAX);
	  break;
	case 'r':
	  p->acq_flags |= OPT_REGEX;
	  bzero(p->acq_mt->pattern, LINE_MAX);
	  strncpy(p->acq_mt->pattern, optarg, LINE_MAX);
	  break;
	case 0:
	  if (opt_empty == 1)
		p->acq_flags |= OPT_EMPTY;
	  if (opt_delete == 1)
		p->acq_flags |=  OPT_DEL;
	  break;
	case 's':
	  p->acq_flags |= OPT_SORT;
	  break;
	case 'v':
	  p->acq_flags |= OPT_VERSION;
	  break;
	case 'x':
	  p->acq_flags |= OPT_XDEV;
	  break;
	case 't':
	  switch (optarg[0]) {
	  case 'p':
		p->acq_args->type = NT_ISFIFO;
		break;
	  case 'c':
		p->acq_args->type = NT_ISCHR;
		break;
	  case 'd':
		p->acq_args->type = NT_ISDIR;
		break;
	  case 'b':
		p->acq_args->type = NT_ISBLK;
		break;
	  case 'l':
		p->acq_args->type = NT_ISLNK;
		break;
	  case 's':
		p->acq_args->type = NT_ISSOCK;
		break;
	  case 'f':
	  case '\0':
		p->acq_args->type = NT_ISREG;
		break;
	  default:
		ftype_err(optarg);
		ret = -1;
		break;
	  }
	  break;
	case 'E':
	  p->acq_mt->mflag |= REG_EXTENDED;
	  break;
	case 'I':
	  p->acq_mt->mflag |= REG_ICASE;
	  break;
	case 'L':
	  p->acq_flags |= OPT_STAT;
	  break;
	case 'P':
	  p->acq_flags |= OPT_LSTAT;
	  break;
	default:
	  p->acq_flags |= OPT_USAGE;
	  break;
	}

  return (ret);
}

static void
ftype_err(const char *s)
{
  if (s == NULL)
	return;

  warnx("--type: %s: unknown type", s);
  return;
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
