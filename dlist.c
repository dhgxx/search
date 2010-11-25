#include "dlist.h"

static void ins_after(dl_node **, dl_node **);
static void ins_before(dl_node **, dl_node **);
static int swap(dl_node **, dl_node **);

dl_node *
dl_mknode(const char *str)
{
  dl_node *np;
  
  if (str == NULL)
	return (NULL);
  
  if ((np = (dl_node *)malloc(sizeof(dl_node))) == NULL)
	return (NULL);
  
  bzero(np->ent, DL_ENTSIZ);
  strncpy(np->ent, str, DL_ENTSIZ);
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
dl_empty(DLIST **dl)
{
  DLIST *dlp = *dl;
  
  if (dlp == NULL)
	return (1);

  if ((dlp->head == NULL) &&
	  (dlp->tail == NULL) &&
	  (dlp->cur == NULL) &&
	  (dlp->len == 0)) {
	return (1);
  }

  return (0);
}

int
dl_append(const char *str, DLIST **dl)
{
  dl_node *new;
  DLIST *dlp = *dl;
  
  if (dlp == NULL)
	return (-1);

  new = dl_mknode(str);

  if (dl_empty(&dlp)) {
	dlp->head = new;
  } else {
	dlp->tail->next = new;
	new->pre = dlp->tail;
  }
	
  dlp->cur = new;
  dlp->tail = new;
  dlp->len++;
  return (0);
}

static void
ins_after(dl_node **src, dl_node **dst)
{
  dl_node *np = *src;
  dl_node *new = *dst;
  
  new->pre = np;
  new->next = np->next;
  if (np->next != NULL)
	np->next->pre = new;
  np->next = new;
}

static void
ins_before(dl_node **src, dl_node **dst)
{
  dl_node *np = *src;
  dl_node *new = *dst;
  
  new->next = np;
  if (np->pre != NULL) {	
	new->pre = np->pre;
	np->pre->next = new;
  }
  np->pre = new;
}

static int
swap(dl_node **f, dl_node **r)
{
  dl_node *front = *f;
  dl_node *rear = *r;
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
dl_ins_at_pos(const char *str, int pos, DLIST **dl, const int before)
{
  int i;
  dl_node *new, *np;
  DLIST *dlp = *dl;
  
  if (str == NULL)
	return (-1);

  if (dlp == NULL)
	return (-1);
	
  new = dl_mknode(str);
	
  if (dl_empty(&dlp)) {
	dlp->cur = dlp->head = dlp->tail = new;
	dlp->len++;
  } else {
	dlp->cur = new;
	if (pos <= 0) {
	  new->next = dlp->head;
	  dlp->head->pre = new;
	  dlp->head = new;
	  dlp->len++;
	}

	if (pos > 0 && pos < dlp->len) {
	  np = dlp->head;
	  for (i = 0; i < pos && np; i++)
		np = np->next;

	  if (before == 1)
		ins_before(&np, &new);
	  else
		ins_after(&np, &new);
	  
	  dlp->len++;
	}

	if (pos >= dlp->len) {
	  new->pre = dlp->tail->pre;
	  dlp->tail->next = new;
	  dlp->tail = new;
	  dlp->len++;
	}
  }

  return (0);
}

int
dl_ins_at_val(const char *str, const char *pos, DLIST **dl, const int before)
{
  dl_node *new, *np;
  DLIST *dlp = *dl;
  
  if ((str == NULL) ||
	  (pos == NULL))
	return (-1);

  if (dlp == NULL)
	return (-1);

  dlp->cur = new = dl_mknode(str);
	
  if (dl_empty(&dlp)) {
	dlp->head = dlp->tail = new;
	dlp->len++;
  } else {
	np = dlp->head;
	while (np) {
	  if (0 == strncmp(pos, np->ent, strlen(pos) + 1)) {
		if (before == 1) {
		  ins_before(&np, &new);
		  if (np == dlp->head)
			dlp->head = new;
		} else {
		  ins_after(&np, &new);
		  if (np == dlp->tail)
			dlp->tail = new;
		}
		
		dlp->len++;
		break;
	  }
	  np = np->next;
	}
  }
  return (0);
}

void
dl_sort(DLIST **dl)
{
  int m, ret;
  char *pre, *next;
  dl_node *front, *rear;
  DLIST *dlp = *dl;
  
  if (dlp == NULL)
	return;

  if (dl_empty(&dlp))
	return;

  if (dlp->tail == dlp->head &&
	  dlp->len == 1)
	return;

  rear = dlp->tail;
  front = rear->pre;
  dlp->cur = front;

  while (dlp->cur != NULL) {

	pre = front->ent;
	next = rear->ent;

	if (pre != NULL && next != NULL)
	  m = strncmp(pre, next, strlen(next) + 1);
	else
	  m = -1;

	if (m > 0)
	  ret = swap(&front, &rear);
	else
	  ret = 1;

	if (ret == 0) {
#ifdef _DEBUG_
	  fprintf(stderr, "swaping %s and %s\n", pre, next);
#endif
	  if (dlp->tail == rear)
		dlp->tail = front;
	  if (dlp->head == front)
		dlp->head = rear;
	  front = dlp->cur;
	  rear = front->next;
	} else {
	  front = dlp->cur->pre;
	  rear = dlp->cur;
	}

	dlp->cur = front;
  }
}

int
dl_delete(const char *str, DLIST **dl)
{
  DLIST *dlp = *dl;
  
  if (str == NULL)
	return (-1);

  if (dlp == NULL)
	return (-1);
  
  if (!dl_empty(&dlp)) {
	dlp->cur = dlp->head;
	while (dlp->cur) {
	  if (0 == strncmp(str, dlp->cur->ent, strlen(str) + 1)) {
		dlp->cur->deleted = 1;
		dlp->len--;
		return (0);
	  }
	  dlp->cur = dlp->cur->next;
	}
	return (0);
  }
  return (-1);
}

void
dl_foreach(DLIST **dl, void (*func_p) (dl_node **np))
{
  DLIST *dlp = *dl;
  
  if (dlp == NULL)
	return;

  if (func_p == NULL)
	return;

  if (!dl_empty(&dlp)) {
	dlp->cur = dlp->head;
	while (dlp->cur) {
	  func_p(&(dlp->cur));
	  dlp->cur = dlp->cur->next;
	}
  }
}

void
dl_free(DLIST **dl)
{
  dl_node *np;
  DLIST *dlp;

  dlp = *dl;
  
  if (dlp == NULL)
	return;

  if (dl_empty(&dlp)) {
	free(dlp);
	dlp = NULL;
	return;
  }
	  
  np = dlp->head;
  dlp->cur = dlp->head->next;

  while (np != NULL) {
	free(np);
	np = NULL;
	np = dlp->cur;
	if (dlp->cur != NULL)
	  dlp->cur = dlp->cur->next;
  }

  if (dlp != NULL) {
	free(dlp);
	dlp = NULL;
  }
}
