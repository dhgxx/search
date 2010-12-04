#ifndef _EXTERN_H_
#define _EXTERN_H_

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

extern int lookup_options(int, char *[], plan_t *);
extern int s_regex(const char *, plan_t *);
extern int s_name(const char *, plan_t *);
extern int s_gettype(const char *, plan_t *);
extern int s_gid(const char *, plan_t *);
extern int s_uid(const char *, plan_t *);
extern int s_empty(const char *, plan_t *);
extern int s_xdev(const char *, plan_t *);
extern int s_sort(const char *, plan_t *);
extern int s_stat(const char *, plan_t *);
extern int s_lstat(const char *, plan_t *);
extern int s_delete(const char *, plan_t *);
extern int s_path(const char *, plan_t *);
extern int s_version(const char *, plan_t *);
extern int s_usage(const char *, plan_t *);
extern void walk_through(const char *, const char *, plan_t *);

extern int init_plan(plan_t *, plist_t *);
extern int find_plan(int, char **, plan_t *);
extern int execute_plan(plan_t *, plist_t *);
extern void add_plan(plan_t *, plist_t *);
extern void free_plan(plist_t *);

#endif	/* _EXTERN_H_ */
