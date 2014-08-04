/*
 * Copyright (c) 2005-2010 Denise H. G. <darcsis@gmail.com>
 * All rights reserved
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

#ifndef _SEARCH_H_
#define _SEARCH_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fnmatch.h>
#include <limits.h>
#include <locale.h>
#include <regex.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mi/dlist.h>

#define SEARCH_NAME "search"
#define SEARCH_VERSION "0.6.1"

#define OPT_NONE    0x000000
#define OPT_EMPTY   0x000001
#define OPT_GRP     0x000002
#define OPT_USR     0x000004
#define OPT_XDEV    0x000010
#define OPT_DEL     0x000020
#define OPT_SORT    0x000040
#define OPT_STAT    0x000100
#define OPT_LSTAT   0x000200
#define OPT_NAME    0x000400
#define OPT_REGEX   0x000800
#define OPT_PATH    0x001000
#define OPT_TYPE    0x002000
#define OPT_IDS     0x004000
#define OPT_NGRP    0x008000
#define OPT_NUSR    0x010000
#define OPT_VERSION 0x020000
#define OPT_USAGE   0x040000

typedef enum _node {
  NT_UNKNOWN = DT_UNKNOWN,
  NT_ISFIFO = DT_FIFO,
  NT_ISCHR = DT_CHR,
  NT_ISDIR = DT_DIR,
  NT_ISBLK = DT_BLK,
  NT_ISREG = DT_REG,
  NT_ISLNK = DT_LNK,
  NT_ISSOCK = DT_SOCK,
  NT_ERROR = -1
} NODE;

typedef struct _match_t {
  regex_t fmt;
  char pattern[LINE_MAX];
  unsigned int mflag;
} match_t;

typedef struct _nstat_t {
  NODE type;
  uid_t uid;
  gid_t gid;
  dev_t dev;
  unsigned int empty;
  unsigned int flink;
  unsigned int mtype;
} nstat_t;

typedef struct _args_t {
  NODE type;
  char suid[LINE_MAX];
  char sgid[LINE_MAX];
  dev_t odev;
  unsigned int empty;
  unsigned int need_sort;
  unsigned int need_xdev;
} args_t;

typedef struct _plist_t {
  int retval;
  struct _plan *start;
  struct _plan *cur;
  unsigned int size;
} plist_t;

typedef struct _plan_t {
  unsigned int flags;
  struct _match_t *mt;
  struct _args_t *args;
  struct _plist_t *plans;
  struct _nstat_t *nstat;
  struct dlist *paths;
  /* files to be deleted */
  struct dlist *rfiles;
  /* dirs to be deleted */
  struct dlist *rdirs;
} plan_t;

typedef struct _plan {
  unsigned int exec;
  char *func_name;
  int (*s_func) (const char *, struct _plan_t *);
  struct _plan *next;
} PLAN;

typedef struct flags_t {
  unsigned int opt;
  int (*s_func) (const char *, struct _plan_t *);
  const char *name;
  const unsigned int exec;
} FLAGS;

#endif	/* _SEARCH_H_ */
