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

void lookup_option(int, char **);
static int tell_type(char *);
void comp_regex(reg_t *);
int exec_regex(char *, reg_t *);
int exec_name(char *, reg_t *);
void free_regex(reg_t *);
void walk_through(char *, char *);
void display_usage(void);
void display_version(void);

static int opt_empty;
static int opt_delete;

void
lookup_option(int argc, char *argv[])
{
  int ch;
  
  static struct option longopts[] = {
	{ "name",    required_argument, NULL,       'n' },
	{ "regex",   required_argument, NULL,       'r' },
	{ "type",    required_argument, NULL,       't' },
	{ "empty",   no_argument,       &opt_empty,  1  },
	{ "delete",  no_argument,       &opt_delete, 1  },
	{ "sort",    no_argument,       NULL,       's' },
	{ "version", no_argument,       NULL,       'v' },
	{ NULL,      0,                 NULL,        0  }
  };

  while ((ch = getopt_long(argc, argv, "EILPshvn:r:t:", longopts, NULL)) != -1)
	switch (ch) {
	case 'n':
	  bzero(rep->re_str, LINE_MAX);
	  strlcat(rep->re_str, optarg, LINE_MAX);
	  opts->exec_func = exec_name;
	  break;
	case 'r':
	  bzero(rep->re_str, LINE_MAX);
	  strlcat(rep->re_str, optarg, LINE_MAX);
	  opts->exec_func = exec_regex;
	  break;
	case 0:
	  if (opt_empty)
		opts->find_empty = 1;
	  if (opt_delete)
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
tell_type(char *node_name)
{
  struct stat stbuf;
  int tmp;
  
  tmp = opts->stat_func(node_name, &stbuf);
  
  if (tmp < 0 )
	return NT_ERROR;

  node_stat->size = stbuf.st_size;
  
  if (S_ISBLK(stbuf.st_mode)) node_stat->type = NT_ISBLK;
  if (S_ISCHR(stbuf.st_mode)) node_stat->type = NT_ISCHR;
  if (S_ISDIR(stbuf.st_mode)) node_stat->type = NT_ISDIR;
  if (S_ISFIFO(stbuf.st_mode)) node_stat->type = NT_ISFIFO;
  if (S_ISLNK(stbuf.st_mode)) node_stat->type = NT_ISLNK;
  if (S_ISREG(stbuf.st_mode)) node_stat->type = NT_ISREG;
  if (S_ISSOCK(stbuf.st_mode)) node_stat->type = NT_ISSOCK;
#ifndef _OpenBSD_
  if (S_ISWHT(stbuf.st_mode)) node_stat->type = NT_ISWHT;
#endif
  return NT_UNKNOWN;
}

int
exec_name(char *d_name, reg_t *rep)
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
  printf("pattern=%s, name=%s\n", pattern, d_name);
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
	  (void)fprintf(stderr, "%s: %s: %s\n", opts->prog_name, str, msg);
	} else {
	  (void)fprintf(stderr, "%s: %s: %s\n", opts->prog_name, str, strerror(errno));
	}
	
	regfree(fmt);
	exit(0);
  }
}


int
exec_regex(char *d_name, reg_t *rep)
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
	  (void)fprintf(stderr, "%s: %s: %s\n", opts->prog_name, str, msg);
	} else {
	  (void)fprintf(stderr, "%s: %s: %s\n", opts->prog_name, str, strerror(errno));
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
  return;
}

void
walk_through(char *n_name, char *d_name)
{
  DIR *dirp;
  struct dirent *dir;
  char p_buf[MAXPATHLEN], tmp_buf[MAXPATHLEN];
  int ndir;
  int found;
  
  if (NT_ERROR == tell_type(n_name)) {
	(void)fprintf(stderr, "%s: %s: %s\n", opts->prog_name, n_name, strerror(errno));
	return;
  }

  found = 0;
  bzero(p_buf, MAXPATHLEN);
  strlcat(p_buf, n_name, LINE_MAX);
  
  if (opts->exec_func(d_name, rep) &&
	  (NT_UNKNOWN == opts->n_type ||
	   node_stat->type == opts->n_type)) {

	found = 1;

	if (opts->delete) {
	  
	  if (NT_ISDIR != node_stat->type) {

		if (0 > unlink(p_buf))
		  (void)fprintf(stderr, "%s: --delete: unlink(%s): %s\n", opts->prog_name, n_name, strerror(errno));
	 
	  } else {
		
		if (0 > rmdir(p_buf))
		  (void)fprintf(stderr, "%s: --delete: rmdir(%s): %s\n", opts->prog_name, n_name, strerror(errno));
	  }
	  
	  return;
	}
	  
	
	if (opts->find_empty) {
	  
	  if (0 == node_stat->size &&
		  NT_ISDIR != node_stat->type) {
		if (!opts->sort)
		  (void)fprintf(stdout, "%s\n", p_buf);
		else
		  bst_ins(p_buf, stree, 0);
	  }
	} else {
	  if (!opts->sort)
		(void)fprintf(stdout, "%s\n", p_buf);
	  else
		bst_ins(p_buf, stree, 0);
	}
  }
  
  if (NT_ISDIR == node_stat->type) {

	/* in a new dir, starting to count */
	/* dir entries.                    */
	ndir = 0;
	
	if ( (dirp = opendir(p_buf)) == NULL) {
	  (void)fprintf(stderr, "%s: %s: %s\n", opts->prog_name, p_buf, strerror(errno));
	  return;
	}
	
	while ( (dir = readdir(dirp)) != NULL) {

	  ndir++;
	  
	  if (strncmp(dir->d_name, ".", 2) != 0 &&
		  strncmp(dir->d_name, "..", 3) != 0) {
		bzero(tmp_buf, MAXPATHLEN);
		strlcat(tmp_buf, p_buf, LINE_MAX);
		
		if (p_buf[strlen(p_buf) - 1] != '/')
		  strlcat(tmp_buf, "/", MAXPATHLEN);
		
		strlcat(tmp_buf, dir->d_name, LINE_MAX);
		walk_through(tmp_buf, dir->d_name);
	  }
	}

	if (found) {
	  
	  if (opts->find_empty &&
		  NT_ISDIR == node_stat->type &&
		  2 >= ndir) {
		if (!opts->sort)
		  (void)fprintf(stdout, "%s\n", p_buf);
		else
		  bst_ins(p_buf, stree, 0);
	  }
	}

	closedir(dirp);
  }
  return;
}

void
display_usage(void)
{
  static const char *usage = "usage: %s [-EILPsv] ... [-n|--name ...] [-r|--regex ...] [-t|--type ...] [--empty]\n";

  (void)fprintf(stderr, usage, opts->prog_name);
  
  exit (0);
}

void
display_version(void)
{  
  (void)fprintf(stderr, "%s version %s\n", opts->prog_name, opts->prog_version);
  exit (0);
  
}
