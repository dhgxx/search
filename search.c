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


options_t *opts;
match_t *mt;
node_stat_t *node_stat;

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
  
  opts = (options_t *)malloc(sizeof(options_t));
  mt = (match_t *)malloc(sizeof(match_t));
  node_stat = (node_stat_t *)malloc(sizeof(node_stat_t));
  
  if (opts == NULL ||
	  mt == NULL ||
	  node_stat == NULL) {
	(void)fprintf(stderr, "%s: malloc(3): %s.\n",
				  SEARCH_NAME, strerror(errno));
	cleanup(1);
	exit(1);
  }

  opts->n_type = NT_UNKNOWN;
  opts->stat_func = lstat;
  opts->exec_func = exec_name;
  opts->odev = 0;
  opts->flags = OPT_NONE;
  bzero(opts->path, MAXPATHLEN);
  bzero(mt->pattern, LINE_MAX);
  
  lookup_options(argc, argv, &opts, &mt);
  
  argc -= optind;
  argv += optind;

  if ((argc == 0) &&
	  (OPT_PATH != (opts->flags & OPT_PATH)))
	display_usage();
  
  if (comp_regex(&opts, &mt) < 0) {
	cleanup (1);
	exit (1);
  }

  if (opts->flags & OPT_PATH) {
	walk_through(opts->path, opts->path, &opts, &mt, &node_stat);
  }

  if (argc > 0) {
	for (i = 0; i < argc && argv[i]; i++)
	  walk_through(argv[i], argv[i], &opts, &mt, &node_stat);
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
  if (mt != NULL) {
	free_mt(&mt);
	mt = NULL;
  }
  
  if (opts != NULL) {
	free(opts);
	opts = NULL;
  }
  
  if (node_stat != NULL) {
	free(node_stat);
	node_stat = NULL;
  }
  
  if (sig)
	exit(0);
}
