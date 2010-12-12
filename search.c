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

#include <getopt.h>

#include "search.h"
  
extern char *optarg;
extern int optind;

extern int init_plan(plan_t *);
extern int find_plan(int, char **, plan_t *);
extern int execute_plan(plan_t *);
extern int add_plan(plan_t *);
extern void free_plan(plist_t **);

static int opt_empty;
static int opt_delete;

static void ftype_err(const char *);
static void cleanup(int);

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

plan_t plan;

int
main(int argc, char *argv[])
{
  int i, ch;
  int ret;
    
  (void)setlocale(LC_CTYPE, "");
  signal(SIGINT, cleanup);

  if (init_plan(&plan) < 0) {
#ifdef _DEBUG_
	warnx("init_plan() failed!\n");
#endif
	cleanup(0);
	exit (1);
  }

  while ((ch = getopt_long(argc, argv, "EILPsvxf:n:r:t:", longopts, NULL)) != -1)
	switch (ch) {
	case 2:
	case 3:
	  plan.flags |= OPT_GRP;
	  bzero(plan.args->sgid, LINE_MAX);
	  strlcpy(plan.args->sgid, optarg, LINE_MAX);
	  break;
	case 4:
	case 5:
	  plan.flags |=  OPT_USR;
	  bzero(plan.args->suid, LINE_MAX);
	  strlcpy(plan.args->suid, optarg, LINE_MAX);
	  break;
	case 'f':
	  plan.flags |= OPT_PATH;
	  dl_append(optarg, &(plan.paths));
	  break;
	case 'n':
	  if (!(plan.flags & OPT_NAME)) {
		if (plan.flags & OPT_REGEX)
		  plan.flags &= ~OPT_REGEX;
		plan.flags |= OPT_NAME;
	  }
	  bzero(plan.mt->pattern, LINE_MAX);
	  strlcpy(plan.mt->pattern, optarg, LINE_MAX);
	  break;
	case 'r':
	  if (!(plan.flags & OPT_REGEX)) {
		if (plan.flags & OPT_NAME)
		  plan.flags &= ~OPT_NAME;
		plan.flags |= OPT_REGEX;
	  }
	  bzero(plan.mt->pattern, LINE_MAX);
	  strlcpy(plan.mt->pattern, optarg, LINE_MAX);
	  break;
	case 0:
	  if (opt_empty == 1)
		plan.flags |= OPT_EMPTY;
	  if (opt_delete == 1)
		plan.flags |=  OPT_DEL;
	  break;
	case 's':
	  plan.flags |= OPT_SORT;
	  break;
	case 'v':
	  plan.flags |= OPT_VERSION;
	  break;
	case 'x':
	  plan.flags |= OPT_XDEV;
	  break;
	case 't':
	  plan.flags |= OPT_TYPE;
	  switch (optarg[0]) {
	  case 'p':
		plan.args->type = NT_ISFIFO;
		break;
	  case 'c':
		plan.args->type = NT_ISCHR;
		break;
	  case 'd':
		plan.args->type = NT_ISDIR;
		break;
	  case 'b':
		plan.args->type = NT_ISBLK;
		break;
	  case 'l':
		plan.args->type = NT_ISLNK;
		break;
	  case 's':
		plan.args->type = NT_ISSOCK;
		break;
	  case 'f':
	  case '\0':
		plan.args->type = NT_ISREG;
		break;
	  default:
		ftype_err(optarg);
		cleanup(0);
		exit (1);
	  }
	  break;
	case 'E':
	  plan.mt->mflag |= REG_EXTENDED;
	  break;
	case 'I':
	  plan.mt->mflag |= REG_ICASE;
	  break;
	case 'L':
	  if (plan.flags & OPT_LSTAT) {
		plan.flags &= ~OPT_LSTAT;
		plan.flags |= OPT_STAT;
	  }
	  break;
	case 'P':
	  if (plan.flags & OPT_STAT) {
		plan.flags &= ~OPT_STAT;
		plan.flags |= OPT_LSTAT;
	  }
	  break;
	default:
	  plan.flags |= OPT_USAGE;
	  break;
	}

  argc -= optind;
  argv += optind;

  if (find_plan(argc, argv, &plan) < 0) {
#ifdef _DEBUG_
	warnx("find_plan() failed!\n");
#endif
	cleanup(0);
	exit (1);
  }
  
  if (add_plan(&plan) < 0) {
#ifdef _DEBUG_
	warnx("add_plan() failed!\n");
#endif
	cleanup(0);
	exit (1);
  }

  ret = execute_plan(&plan);

#ifdef _DEBUG_
  warnx("ret=%d\n", ret);
#endif

  cleanup(0);
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
  if (plan.plans) {
	free_plan(&(plan.plans));
	plan.plans = NULL;
  }
  
  if (plan.paths) {
	dl_free(&(plan.paths));
	plan.paths = NULL;
  }

  if (plan.mt != NULL) {
	free(plan.mt);
	plan.mt = NULL;
  }
  
  if (plan.args != NULL) {
	free(plan.args);
	plan.args = NULL;
  }

  if (plan.nstat != NULL) {
	free(plan.nstat);
	plan.nstat = NULL;
  }
  
  if (sig)
	exit(0);
}
