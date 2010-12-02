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
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <regex.h>
#include <signal.h>
#include <fnmatch.h>
#include <locale.h>
#include <dlist.h>

#define SEARCH_NAME "search"
#define SEARCH_VERSION "0.4"

#define OPT_NONE  0x0000
#define OPT_EMPTY 0x0002
#define OPT_GRP   0x0004
#define OPT_USR   0x0008
#define OPT_XDEV  0x0020
#define OPT_DEL   0x0040
#define OPT_SORT  0x0080
#define OPT_STAT  0x0200

typedef enum _node_t {
  NT_UNKNOWN = DT_UNKNOWN,
  NT_ISFIFO = DT_FIFO,
  NT_ISCHR = DT_CHR,
  NT_ISDIR = DT_DIR,
  NT_ISBLK = DT_BLK,
  NT_ISREG = DT_REG,
  NT_ISLNK = DT_LNK,
  NT_ISSOCK = DT_SOCK,
  NT_ERROR = -1
} node_t;

typedef struct _match_t {
  regex_t fmt;
  char pattern[LINE_MAX];
  unsigned int mflag;
} match_t;

typedef struct _nstat_t {
  node_t type;
  uid_t uid;
  gid_t gid;
  dev_t dev;
  unsigned int empty;
  unsigned int flink;
  unsigned int mtype;
} nstat_t;

typedef struct _args_t {
  node_t type;
  char suid[LINE_MAX];
  char sgid[LINE_MAX];
  dev_t odev;
  unsigned int empty;
} args_t;

typedef struct _plan {
  int (*s_func) (const char *, struct _args_t *);
  struct _plan *next;
} PLAN;

typedef struct _plan_t {
  unsigned int acq_flags;
  match_t *acq_mt;
  args_t *acq_args;
  DLIST *acq_paths;
  struct _plan *acq_plan;
  struct _plan *acq_cur;
  unsigned int acq_size;
  struct _nstat_t *nstat;
} plan_t;

#endif	/* _SEARCH_H_ */
