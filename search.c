/*
 * Copyright (c) 2005-2007 Denise H. G. All rights reserved
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

static void cleanup(int);

int
main(int argc, char *argv[])
{
  
  extern char *optarg;
  extern int optind;
  
  int i;
  
  (void)setlocale(LC_CTYPE, "");
  signal(SIGINT, cleanup);
  
  rep = (reg_t *)malloc(sizeof(reg_t));
  opts = (options_t *)malloc(sizeof(options_t));
  node_stat = (node_stat_t *)malloc(sizeof(node_stat_t));
  
  if (rep == NULL ||
	  opts == NULL ||
	  node_stat == NULL) {
	(void)fprintf(stderr, "malloc(3): %s.\n", strerror(errno));
	exit(0);
  }

  opts->prog_name = SEARCH_NAME;
  opts->prog_version = SEARCH_VERSION;
  opts->n_type = 0;
  opts->stat_func = lstat;
  opts->exec_func = NULL;
  opts->find_path = 0;
  opts->find_empty = 0;
  opts->delete = 0;
  opts->sort = 0;

  bzero(opts->path, MAXPATHLEN);
  bzero(rep->re_str, LINE_MAX);
  rep->re_cflag =  REG_BASIC;
  
  lookup_option(argc, argv);
  
  argc -= optind;
  argv += optind;
  
  if (argc == 0 &&
	  opts->find_path == 0)
	display_usage();

  if (opts->exec_func == NULL)
	opts->exec_func = exec_name;
  
  comp_regex(rep);

  if (opts->find_path == 1)
	walk_through(opts->path, opts->path);

  if (argc > 0) {
	for (i = 0; i < argc && argv[i]; i++)
	  walk_through(argv[i], argv[i]);
  }
  
  free_regex(rep);
  rep = NULL;
  free(opts);
  opts = NULL;
  free(node_stat);
  node_stat = NULL;
  exit(0);
}

static void
cleanup(int sig)
{
  (void)fprintf(stderr, "\nUser interrupted, cleaning up...\n");
  
  if (rep != NULL) {
	free_regex(rep);
	rep = NULL;
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
	exit(1);

}

