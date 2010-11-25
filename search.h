#ifndef _SEARCH_H_
#define _SEARCH_H_

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

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <regex.h>
#include <signal.h>
#include <fnmatch.h>
#include <locale.h>

#define SEARCH_NAME "search"
#define SEARCH_VERSION "0.4"

#define OPT_NONE  0x0000
#define OPT_PATH  0x0002
#define OPT_EMPTY 0x0004
#define OPT_GID   0x0006
#define OPT_GRP   0x0008
#define OPT_UID   0x0020
#define OPT_USR   0x0040
#define OPT_XDEV  0x0060
#define OPT_DEL   0x0080
#define OPT_SORT  0x0200
#define OPT_ICAS  0x0400

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
  NT_ISUSR = 18,
  NT_ISGRP = 20,
  NT_ERROR = -1
} node_t;

typedef struct _match_t {
  regex_t fmt;
  char pattern[LINE_MAX];
  unsigned int cflag;
} match_t;

typedef struct _options_t {
  unsigned int flags;
  char path[MAXPATHLEN];
  char user[LINE_MAX];
  char group[LINE_MAX];
  node_t n_type;
#ifndef _OpenBSD_
  __dev_t odev;
#else
  dev_t odev;
#endif
} options_t;

typedef struct _node_stat_t {
  node_t type;
  unsigned int empty;
  uid_t uid;
  gid_t gid;
#ifndef _OpenBSD_
  __dev_t dev;
#else
  dev_t dev;
#endif
} node_stat_t;

typedef struct _plan_t {
  node_stat_t *stat;
  options_t *opt;
  match_t *mt;
  int (*stat_func)(const char *, struct stat *);
  int (*exec_func)(const char *, struct _plan_t *);
} plan_t;

#endif	/* _SEARCH_H_ */
