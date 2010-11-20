#ifndef _DLIST_H_
#define _DLIST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DL_ENT_SIZE 256

typedef struct _dl_node {
  char node[DL_ENT_SIZE];
  struct _dl_node *pre;
  struct _dl_node *next;
  unsigned int deleted;
} dl_node;

typedef struct _dlist {
  dl_node *head;
  dl_node *tail;
  dl_node *cur;
  unsigned int len;
} DLIST;

dl_node *dl_mknode(const char *);
DLIST *dl_init(void);
int dl_empty(const DLIST *);
int dl_append(const char *, DLIST *);
int dl_ins_at_pos(const char *, const int, DLIST *, const int);
int dl_ins_at_val(const char *, const char *, DLIST *, const int);
void dl_sort(DLIST *);
int dl_delete(const char *, DLIST *);
void dl_proc(DLIST *, void (*) (const dl_node *));
void dl_free(DLIST **);

#endif /* _DLIST_H_ */
