#include "dlist.h"

static void ins_after(dl_node *, dl_node *);
static void ins_before(dl_node *, dl_node *);
static int swap(dl_node *, dl_node *);

dl_node *
dl_mknode(const char *str)
{
  dl_node *np;
  
  if (str == NULL)
	return (NULL);
  
  if ((np = (dl_node *)malloc(sizeof(dl_node))) == NULL)
	return (NULL);
  
  bzero(np->node, DL_ENT_SIZE);
  strncpy(np->node, str, strlen(str) + 1);
  np->deleted = 0;
  np->pre = NULL;
  np->next = NULL;
  return (np);
}

DLIST *
dl_init(void)
{
  DLIST *dl;
  
  if ((dl = (DLIST *)malloc(sizeof(DLIST))) == NULL)
	return (NULL);
  
  dl->head = NULL;
  dl->tail = NULL;
  dl->cur = NULL;
  dl->len = 0;
  return (dl);
}

int
dl_empty(const DLIST *dl)
{
  if (dl == NULL)
	return (1);

  if ((dl->head == NULL) &&
	  (dl->tail == NULL) &&
	  (dl->cur == NULL) &&
	  (dl->len == 0)) {
	return (1);
  }

  return (0);
}

int
dl_append(const char *str, DLIST *dl)
{
  dl_node *new;
  
  if (dl == NULL)
	return (-1);

  new = dl_mknode(str);

  if (dl_empty(dl)) {
	dl->head = new;
  } else {
	dl->tail->next = new;
	new->pre = dl->tail;
  }
	
  dl->cur = new;
  dl->tail = new;
  dl->len++;
  return (0);
}

static void
ins_after(dl_node *np, dl_node *new)
{
  new->pre = np;
  new->next = np->next;
  if (np->next != NULL)
	np->next->pre = new;
  np->next = new;
}

static void
ins_before(dl_node *np, dl_node *new)
{
  new->next = np;
  if (np->pre != NULL) {	
	new->pre = np->pre;
	np->pre->next = new;
  }
  np->pre = new;
}

static int
swap(dl_node *front, dl_node *rear)
{
  dl_node *fp, *rn;
  
  if (front == NULL ||
	  rear == NULL)
	return (-1);

  /* simple: we have only two nodes */
  /* to deal with. */
  if (front->pre == NULL &&
	  rear->next == NULL) {
	front->next = NULL;
	front->pre = rear;
	rear->pre = NULL;
	rear->next = front;
	return (0);
  }

  /* we are at the head. */
  if (front->pre == NULL &&
	  rear->next != NULL) {
	rn = rear->next;

	front->next = rn;
	front->pre = rear;
	rear->next = front;
	rear->pre = NULL;
	rn->pre = front;
	return (0);
  }

  /* we are at the tail. */
  if (front->pre != NULL &&
	  rear->next == NULL) {
	fp = front->pre;

	fp->next = rear;
	front->next = NULL;
	front->pre = rear;
	rear->next = front;
	rear->pre = fp;
	return (0);
  }

  /* hard: the two nodes to swap */
  /* have neighbors. */
  if (front->pre != NULL &&
	  rear->next != NULL) {
	fp = front->pre;
	rn = rear->next;

	rear->pre = fp;
	front->pre = rear;
	rn->pre = front;

	rear->next = front;
	front->next = rn;
	fp->next = rear;
	return (0);
  }
  /* should never reach here! */
  return (-1);
}

int
dl_ins_at_pos(const char *str, int pos, DLIST *dl, const int before)
{
  int i;
  dl_node *new, *np;
  
  if (str == NULL)
	return (-1);

  if (dl == NULL)
	return (-1);
	
  new = dl_mknode(str);
	
  if (dl_empty(dl)) {
	dl->cur = dl->head = dl->tail = new;
	dl->len++;
  } else {
	dl->cur = new;
	if (pos <= 0) {
	  new->next = dl->head;
	  dl->head->pre = new;
	  dl->head = new;
	  dl->len++;
	}

	if (pos > 0 && pos < dl->len) {
	  np = dl->head;
	  for (i = 0; i < pos && np; i++)
		np = np->next;

	  if (before == 1)
		ins_before(np, new);
	  else
		ins_after(np, new);
	  
	  dl->len++;
	}

	if (pos >= dl->len) {
	  new->pre = dl->tail->pre;
	  dl->tail->next = new;
	  dl->tail = new;
	  dl->len++;
	}
  }

  return (0);
}

int
dl_ins_at_val(const char *str, const char *pos, DLIST *dl, const int before)
{
  dl_node *new, *np;
  
  if ((str == NULL) ||
	  (pos == NULL))
	return (-1);

  if (dl == NULL)
	return (-1);

  dl->cur = new = dl_mknode(str);
	
  if (dl_empty(dl)) {
	dl->head = dl->tail = new;
	dl->len++;
  } else {
	np = dl->head;
	while (np) {
	  if (0 == strncmp(pos, np->node, strlen(pos) + 1)) {
		if (before == 1) {
		  ins_before(np, new);
		  if (np == dl->head)
			dl->head = new;
		} else {
		  ins_after(np, new);
		  if (np == dl->tail)
			dl->tail = new;
		}
		
		dl->len++;
		break;
	  }
	  np = np->next;
	}
  }
  return (0);
}

void
dl_sort(DLIST *dl)
{
  int m, ret;
  char *pre, *next;
  dl_node *front, *rear;
  
  if (dl == NULL)
	return;

  if (dl_empty(dl))
	return;

  if (dl->tail == dl->head &&
	  dl->len == 1)
	return;

  rear = dl->tail;
  front = rear->pre;
  dl->cur = front;

  while (dl->cur != NULL) {

	pre = front->node;
	next = rear->node;

	if (pre != NULL && next != NULL)
	  m = strncmp(pre, next, strlen(next) + 1);
	else
	  m = -1;

	if (m > 0)
	  ret = swap(front, rear);
	else
	  ret = 1;

	if (ret == 0) {
#ifdef _DEBUG_
	  fprintf(stderr, "swaping %s and %s\n", pre, next);
#endif
	  if (dl->tail == rear)
		dl->tail = front;
	  if (dl->head == front)
		dl->head = rear;
	  front = dl->cur;
	  rear = front->next;
	} else {
	  front = dl->cur->pre;
	  rear = dl->cur;
	}

	dl->cur = front;
  }
}

int
dl_delete(const char *str, DLIST *dl)
{  
  if (str == NULL)
	return (-1);

  if (dl == NULL)
	return (-1);
  
  if (!dl_empty(dl)) {
	dl->cur = dl->head;
	while (dl->cur) {
	  if (0 == strncmp(str, dl->cur->node, strlen(str) + 1)) {
		dl->cur->deleted = 1;
		dl->len--;
		return (0);
	  }
	  dl->cur = dl->cur->next;
	}
	return (0);
  }
  return (-1);
}

void
dl_proc(DLIST *dl, void (*func_p) (const dl_node *np))
{
  if (dl == NULL)
	return;

  if (func_p == NULL)
	return;

  if (!dl_empty(dl)) {
	dl->cur = dl->head;
	while (dl->cur) {
	  if (dl->cur->deleted != 1)
		func_p(dl->cur);
	  dl->cur = dl->cur->next;
	}
  }
}

void
dl_free(DLIST *dl)
{
  dl_node *np;
  
  if (dl == NULL)
	return;

  if (dl_empty(dl)) {
	free(dl);
	dl = NULL;
	return;
  }
	  
  np = dl->tail;
  dl->cur = dl->tail->pre;
  
  while (np != NULL) {
	free(np);
	np = NULL;
	np = dl->cur;
	if (dl->cur != NULL)
	  dl->cur = dl->cur->pre;
  }

  if (dl != NULL) {
	free(dl);
	dl = NULL;
  }
}
