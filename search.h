#ifndef _SEARCH_H_
#define _SEARCH_H_

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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <regex.h>
#include <signal.h>
#include <fnmatch.h>
#include <locale.h>
#include <dlist.h>

#define SEARCH_NAME "search"
#define SEARCH_VERSION "0.3"

typedef enum _node_t {
  NT_UNKNOWN = DT_UNKNOWN,
  NT_ISFIFO = DT_FIFO,
  NT_ISCHR = DT_CHR,
  NT_ISDIR = DT_DIR,
  NT_ISBLK = DT_BLK,
  NT_ISREG = DT_REG,
  NT_ISLNK = DT_LNK,
  NT_ISSOCK = DT_SOCK,
#ifndef _OpenBSD_
  NT_ISWHT = DT_WHT,
#endif
  NT_ERROR = -1
} node_t;

typedef struct _reg_t {
  regex_t re_fmt;
  char re_str[LINE_MAX];
  int re_cflag;
} reg_t;

typedef struct _options_t {
  const char *prog_name;
  const char *prog_version;
  char path[MAXPATHLEN];
  node_t n_type;
  int re_icase;
  unsigned int find_path;
  unsigned int find_empty;
  unsigned int delete;
  unsigned int sort;
  int (*stat_func)(const char *, struct stat *);
  int (*exec_func)(const char *, reg_t *);
} options_t;

typedef struct _node_stat_t {
  node_t type;
  unsigned int empty;
} node_stat_t;

reg_t *rep;
options_t *opts;
node_stat_t *node_stat;

#endif	/* _SEARCH_H_ */
