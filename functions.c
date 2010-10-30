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
node_t tell_type(char *);
void comp_regex(reg_t *);
int exec_regex(char *, reg_t *, int, int);
int exec_name(char *, reg_t *, int, int);
void free_regex(reg_t *);
void walk_through(char *, char *, reg_t *, options_t *);
void display_usage(void);
void display_version(void);

void lookup_option(int argc, char *argv[])
{
  int ch;
  
  static struct option longopts[] = {
	{ "name", required_argument, NULL, 'n' },
	{ "regex", required_argument, NULL, 'r' },
	{ "type", required_argument, NULL, 't' },
	{ "help", 0,				NULL, 'h'	},
	{ "version", 0,				NULL, 'v'	},
	{ NULL,   0,                 NULL,    0 }
  };

  while ((ch = getopt_long(argc, argv, "EILPhvn:r:t:", longopts, NULL)) != -1)
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
			opts->long_help = 1;
			display_usage();
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
	  case 'h':
		opts->long_help = 1;
		break;
	  default:
		display_usage();
		break;
	}
}

node_t tell_type(char *node_name)
{
  struct stat stbuf;
  int tmp;
  
  tmp = opts->stat_func(node_name, &stbuf);
  
  if (tmp < 0 )
	return NT_ERROR;
  else {
	if (S_ISBLK(stbuf.st_mode)) return NT_ISBLK;
	if (S_ISCHR(stbuf.st_mode)) return NT_ISCHR;
	if (S_ISDIR(stbuf.st_mode)) return NT_ISDIR;
	if (S_ISFIFO(stbuf.st_mode)) return NT_ISFIFO;
	if (S_ISLNK(stbuf.st_mode)) return NT_ISLNK;
	if (S_ISREG(stbuf.st_mode)) return NT_ISREG;
	if (S_ISSOCK(stbuf.st_mode)) return NT_ISSOCK;
#ifndef _OpenBSD_
	if (S_ISWHT(stbuf.st_mode)) return NT_ISWHT;
#endif
	return NT_UNKNOWN;
  }
}

int exec_name(char *d_name, reg_t *rep, int n_type, int st_type)
{
  int flag, len, matched;
  char *pattern;
  
  pattern = rep->re_str;
  len = strlen(pattern);
  flag = rep->re_cflag;
  matched = FNM_NOMATCH;
  
  if (len == 0)
	pattern = "*";
  
  if ( (flag & REG_ICASE) == 0)
	flag = 0;
  else
	flag = FNM_CASEFOLD | FNM_PERIOD | FNM_PATHNAME | FNM_NOESCAPE;
#ifdef __DEBUG__
  printf("pattern=%s, name=%s\n", pattern, d_name);
#endif
  matched = fnmatch(pattern, d_name, flag);
  matched = (opts->n_type == 0 || opts->n_type == st_type ) && matched == 0;
  
  return ((matched) ? 1 : 0);

}

void comp_regex(reg_t *rep)
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
	if (regerror(retval, fmt, msg, LINE_MAX) > 0)
	  (void)fprintf(stderr, "%s: %s: %s\n", opts->prog_name, str, msg);
	else
	  (void)fprintf(stderr, "%s: %s: %s\n", opts->prog_name, str, strerror(errno));
	regfree(fmt);
	exit(0);
  }
}


int exec_regex(char *d_name, reg_t *rep, int n_type, int st_type)
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
	if (regerror(retval, fmt, msg, LINE_MAX) > 0)
	  (void)fprintf(stderr, "%s: %s: %s\n", opts->prog_name, str, msg);
	else
	  (void)fprintf(stderr, "%s: %s: %s\n", opts->prog_name, str, strerror(errno));
	regfree(fmt);
	exit(0);
  }

  matched = n_type == 0 || n_type == st_type;
  matched = retval == 0 && pmatch.rm_so == 0 && pmatch.rm_eo == len && matched;

  return ((matched) ? 1 : 0);
}

void free_regex(reg_t *rep)
{
  regfree(&(rep->re_fmt));
  free(rep);
  return;
}

void walk_through(char *n_name, char *d_name, reg_t *rep, options_t *opts)
{
  DIR *dirp;
  struct dirent *dir;
  char p_buf[MAXPATHLEN], tmp_buf[MAXPATHLEN];
  node_t n_type, st_type;
  
  if ( (st_type = tell_type(n_name)) == NT_ERROR) {
	(void)fprintf(stderr, "%s: %s: %s\n", opts->prog_name, n_name, strerror(errno));
		return;
  }
  
  n_type = opts->n_type;
  
  bzero(p_buf, MAXPATHLEN);
  strlcat(p_buf, n_name, LINE_MAX);

  if (opts->exec_func(d_name, rep, n_type, st_type))
	(void)fprintf(stdout, "%s\n", p_buf);
  
  if (st_type == NT_ISDIR) {
	
	if ( (dirp = opendir(p_buf)) == NULL) {
	  (void)fprintf(stderr, "%s: %s: %s\n", opts->prog_name, p_buf, strerror(errno));
	  return;
	}
	
	while ( (dir = readdir(dirp)) != NULL) {
	  if (strncmp(dir->d_name, ".", 2) != 0 &&
		strncmp(dir->d_name, "..", 3) != 0) {
		bzero(tmp_buf, MAXPATHLEN);
		strlcat(tmp_buf, p_buf, LINE_MAX);
		if (p_buf[strlen(p_buf) - 1] != '/')
		  strlcat(tmp_buf, "/", MAXPATHLEN);
		strlcat(tmp_buf, dir->d_name, LINE_MAX);
		walk_through(tmp_buf, dir->d_name, rep, opts);
	  }
	}
	closedir(dirp);
  }
  return;
}

void display_usage(void)
{
  static const char *s_help="usage: %s [-E|-I|-L|-P|-h] [path] [-n|-r ...] [-t ...]\n";
  static const char *l_help="\npossible options:\n\n \
 -h: this long help.\n\n											 \
 -E: use modern regular expressions rather than `basic' (obsolete)\n \
     regular expressions which is the default. See the man page of\n \
     `re_format(7)' for more information.\n\n \
 -I: case insensitive search, otherwise the specified pattern is\n	\
     interpreted as case sensitive.\n\n								\
 -P: do not follow symbolic links, but return the information the\n \
     symbolic links themselves, this is the default behaviour.\n\n \
 -L: follow symbolic links and return the information of the files\n \
     they reference. It is an error if the referenced files do not\n \
     exist.\n\n \
 -n: specify a pattern to match the search results. Special shell\n \
     pattern matching characters may be used as part of the pattern.\n \
     Also available as `--name'. See the man page of `fnmatch(3)' for\n \
     more information.\n\n \
 -r: specify a regular expression to match the search results. Unless\n \
     the option `-E' is explicitly specified, the regular expression\n \
     will only be interpreted as `basic' (obsolete) regular expressions.\n \
     Also available as `--regex'. See the man page of `re_format(7)' for\n \
     more information.\n\n \
 -t: specify a file type to match the search results. Also available\n \
     as `--type'. The possible types are as follows:\n\n \
       `b': block device\n \
       `c': character special\n \
       `d': directory\n \
       `f': fifo\n \
       `l': symbolic link\n \
       `r': regular file\n \
       `s': socket\n";

  (void)fprintf(stderr, s_help, opts->prog_name);

  if (opts->long_help) {
	(void)fprintf(stderr, l_help);
#ifndef _OpenBSD_
	(void)fprintf(stderr, "        `w': white out\n");
#endif
  }

  exit (0);
}

void display_version(void)
{
  static const char *version = SEARCH_VERSION;
  static const char *name = SEARCH_NAME;

  (void)fprintf(stderr, "%s version %s\n", name, version);
  exit (0);
  
}

