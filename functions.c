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

static int opt_empty;
static int opt_delete;
static BSTREE *stree;

static void _outprt(const char *);
static void _procnode(const bst_node *);
static int _nstat(const char *);
static int _found(const char *, const char *);

void lookup_option(int, char **);
void comp_regex(reg_t *);
int exec_regex(const char *, reg_t *);
int exec_name(const char *, reg_t *);
void free_regex(reg_t *);
void walk_through(const char *, const char *);
void display_usage(void);
void display_version(void);

void
lookup_option(int argc, char *argv[])
{
  int ch;
  
  static struct option longopts[] = {
	{ "path",    required_argument, NULL,       'f' },
	{ "name",    required_argument, NULL,       'n' },
	{ "regex",   required_argument, NULL,       'r' },
	{ "type",    required_argument, NULL,       't' },
	{ "empty",   no_argument,       &opt_empty,  1  },
	{ "delete",  no_argument,       &opt_delete, 1  },
	{ "sort",    no_argument,       NULL,       's' },
	{ "version", no_argument,       NULL,       'v' },
	{ NULL,      0,                 NULL,        0  }
  };

  while ((ch = getopt_long(argc, argv, "EILPsvf:n:r:t:", longopts, NULL)) != -1)
	switch (ch) {
	case 'f':
	  opts->find_path = 1;
	  bzero(opts->path, MAXPATHLEN);
	  strncpy(opts->path, optarg, MAXPATHLEN);
	  break;
	case 'n':
	  bzero(rep->re_str, LINE_MAX);
	  strncpy(rep->re_str, optarg, LINE_MAX);
	  opts->exec_func = exec_name;
	  break;
	case 'r':
	  bzero(rep->re_str, LINE_MAX);
	  strncpy(rep->re_str, optarg, LINE_MAX);
	  opts->exec_func = exec_regex;
	  break;
	case 0:
	  if (opt_empty == 1)
		opts->find_empty = 1;
	  if (opt_delete == 1)
		opts->delete = 1;
	  break;
	case 's':
	  opts->sort = 1;
	  break;
	case 'v':
	  display_version();
	  break;
	case 't':
	  switch (optarg[0]) {
	  case 'f':
		opts->n_type = NT_ISFIFO;
		break;
	  case 'c':
		opts->n_type = NT_ISCHR;
		break;
	  case 'd':
		opts->n_type = NT_ISDIR;
		break;
	  case 'b':
		opts->n_type = NT_ISBLK;
		break;
	  case 'l':
		opts->n_type = NT_ISLNK;
		opts->stat_func = lstat; 
		break;
	  case 's':
		opts->n_type = NT_ISSOCK;
		break;
#ifndef _OpenBSD_
	  case 'w':
		opts->n_type = NT_ISWHT;
		break;
#endif
	  case 'r':
	  case '\0':
		opts->n_type = NT_ISREG;
		break;
	  default:
		display_usage();
		break;
	  }
	  break;
	case 'E':
	  rep->re_cflag |= REG_EXTENDED;
	  break;
	case 'I':
	  rep->re_cflag |= REG_ICASE;
	  break;
	case 'L':
	  opts->stat_func = stat;
	  break;
	case 'P':
	  break;
	default:
	  display_usage();
	  break;
	}
}

int
exec_name(const char *d_name, reg_t *rep)
{
  int flag, len, matched;
  char *pattern;
  
  pattern = rep->re_str;
  len = strlen(pattern);
  flag = rep->re_cflag;
  matched = FNM_NOMATCH;
  
  if (len == 0)
	pattern = "*";
  
  if ( (flag & REG_ICASE) == 0) {
	flag = 0;
  } else {
	flag = FNM_CASEFOLD | FNM_PERIOD | FNM_PATHNAME | FNM_NOESCAPE;
  }
  
#ifdef __DEBUG__
  (void)fprintf(stderr,
				"pattern=%s, name=%s\n",
				pattern,
				d_name);
#endif
  matched = fnmatch(pattern, d_name, flag);
    
  return ((matched == 0) ? 1 : 0);

}

void
comp_regex(reg_t *rep)
{
  int retval, len, cflag;
  regex_t *fmt;
  char msg[LINE_MAX];
  char *str;
  
  len = strlen(rep->re_str);
  cflag = rep->re_cflag;
  
  bzero(msg, LINE_MAX);
  str = rep->re_str;
  fmt = &(rep->re_fmt);

  if (len == 0)
	str = ".*";		/* defaults to search all types. */

  retval = regcomp(fmt, str, cflag);

  if (retval != 0) {
	if (regerror(retval, fmt, msg, LINE_MAX) > 0) {
	  (void)fprintf(stderr,
					"%s: %s: %s\n",
					opts->prog_name,
					str,
					msg);
	} else {
	  (void)fprintf(stderr,
					"%s: %s: %s\n",
					opts->prog_name,
					str,
					strerror(errno));
	}
	regfree(fmt);
	exit(0);
  }
}


int
exec_regex(const char *d_name, reg_t *rep)
{
  int retval, len, matched;
  regex_t *fmt;
  regmatch_t pmatch;
  char msg[LINE_MAX];
  char *str;

  fmt = &(rep->re_fmt);
  str = rep->re_str;
  len = strlen(d_name);

  bzero(msg, LINE_MAX);
  pmatch.rm_so = 0;
  pmatch.rm_eo = len;
  matched = 0;

  retval = regexec(fmt, d_name, 1, &pmatch, REG_STARTEND);

  if (retval != 0 && retval != REG_NOMATCH) {
	
	if (regerror(retval, fmt, msg, LINE_MAX) > 0) {
	  (void)fprintf(stderr,
					"%s: %s: %s\n",
					opts->prog_name,
					str,
					msg);
	} else {
	  (void)fprintf(stderr,
					"%s: %s: %s\n",
					opts->prog_name,
					str,
					strerror(errno));
	}
	regfree(fmt);
	exit(0);
  }

  matched = retval == 0 && pmatch.rm_so == 0 && pmatch.rm_eo == len;
  return ((matched) ? 1 : 0);
}

void
free_regex(reg_t *rep)
{
  regfree(&(rep->re_fmt));
  free(rep);
  rep = NULL;
  return;
}

void
walk_through(const char *n_name, const char *d_name)
{
  int found;
  char tmp_buf[MAXPATHLEN];
  struct dirent *dir;
  DIR *dirp;
  bst_node *np;
  
  if (_nstat(n_name) == NT_ERROR) {
	(void)fprintf(stderr,
				  "%s: %s: %s\n",
				  opts->prog_name,
				  n_name,
				  strerror(errno));
	return;
  }

  if (opts->sort == 1 && stree == NULL)
	stree = bst_init();


  if ((found = _found(n_name, d_name)) == 1) {
	if (opts->sort == 1)
	  bst_ins(n_name, stree, 0);
  }

  if (node_stat->type == NT_ISDIR) {

	if ( (dirp = opendir(n_name)) == NULL) {
	  (void)fprintf(stderr,
					"%s: %s: %s\n",
					opts->prog_name,
					n_name,
					strerror(errno));
	  return;
	}
	
	while ( (dir = readdir(dirp)) != NULL) {
	  if ((strncmp(dir->d_name, ".", 2) != 0) &&
		  (strncmp(dir->d_name, "..", 3) != 0)) {
		bzero(tmp_buf, MAXPATHLEN);
		strncpy(tmp_buf, n_name, MAXPATHLEN);
		
		if (tmp_buf[strlen(tmp_buf) - 1] != '/')
		  strncat(tmp_buf, "/", MAXPATHLEN);
		strncat(tmp_buf, dir->d_name, MAXPATHLEN);

		/* if ((found = _found(tmp_buf, d_name)) == 1) { */
		/*   if (opts->sort == 1) */
		/* 	bst_ins(n_name, stree, 0); */
		/* } */

		walk_through(tmp_buf, dir->d_name);
	  }
	}	
	closedir(dirp);
	
	if (opts->sort == 1) {
	  bst_proc(stree, BST_INORDER, _procnode);
	  bst_free(stree);
	  stree = NULL;
	}
  }
  return;
}

void
display_usage(void)
{
  static const char *usage = "usage:\t%s [-EILPsv]\
 ...\
 [-f|--path ...]\
 [-n|--name ...]\
 [-r|--regex ...]\
 [-t|--type ...]\
 [--empty]\
 [--sort]\n\
 \t%s [-EILPsv]\
 -f | --path ...\
 [...]\
 [-n|--name ...]\
 [-r|--regex ...]\
 [-t|--type ...]\
 [--empty]\
 [--sort]\n";

  (void)fprintf(stderr,
				usage,
				opts->prog_name,
				opts->prog_name);
  exit (0);
}

void
display_version(void)
{  
  (void)fprintf(stderr,
				"%s version %s\n",
				opts->prog_name,
				opts->prog_version);
  exit (0);
}

static void
_outprt(const char *str)
{
  if (str != NULL)
	(void)fprintf(stdout, "%s\n", str);
}

static void
_procnode(const bst_node *np)
{
  if (np != NULL) {
	if (np->node != NULL) {
	  _outprt(np->node);
	}
  }
}

static int
_nstat(const char *d_name)
{
  struct stat stbuf;
  
  if (opts->stat_func(d_name, &stbuf)  < 0 )
	return (NT_ERROR);

  node_stat->empty = 0;

  if (S_ISBLK(stbuf.st_mode))
	node_stat->type = NT_ISBLK;
  if (S_ISCHR(stbuf.st_mode))
	node_stat->type = NT_ISCHR;
  if (S_ISDIR(stbuf.st_mode))
	node_stat->type = NT_ISDIR;
  if (S_ISFIFO(stbuf.st_mode))
	node_stat->type = NT_ISFIFO;
  if (S_ISLNK(stbuf.st_mode))
	node_stat->type = NT_ISLNK;
  if (S_ISREG(stbuf.st_mode))
	node_stat->type = NT_ISREG;
  if (S_ISSOCK(stbuf.st_mode))
	node_stat->type = NT_ISSOCK;
#ifndef _OpenBSD_
  if (S_ISWHT(stbuf.st_mode))
	node_stat->type = NT_ISWHT;
#endif
  
  if (node_stat->type == NT_ISDIR) {
	if ((stbuf.st_nlink <= 2) &&
		(stbuf.st_size <= 2)) {
	  node_stat->empty = 1;
#ifdef _DEBUG_
	  (void)fprintf(stderr,"%s: empty directory.\n", d_name);
#endif
	}
  } else { /* node is not a directory. */
	if (stbuf.st_size == 0) {
	  node_stat->empty = 1;
#ifdef _DEBUG_
	  (void)fprintf(stderr, "%s: empty file.\n", d_name);
#endif
	}
  }
  
  return (NT_UNKNOWN);
}

static int
_found(const char *n_name, const char *d_name)
{
  if ((opts->exec_func(d_name, rep) == 1) &&
	  ((opts->n_type == NT_UNKNOWN) ||
	   (opts->n_type == node_stat->type))) {

	if (opts->find_empty == 1) {
	  if (node_stat->empty == 1) {
		if (opts->sort != 1)
		  (void)fprintf(stdout, "%s\n", n_name);
	  }
	} else {
	  if (opts->sort != 1)
		(void)fprintf(stdout, "%s\n", n_name);
	}
	return (1);
  }
  return (0);
}
