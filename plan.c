#include "search.h"

extern int s_regex(const char *, plan_t *);
extern int s_name(const char *, plan_t *);
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

static const FLAGS flags[] = {
  /* order == start == */
  { OPT_VERSION, &s_version },
  { OPT_USAGE,   &s_usage   },
  { OPT_PATH,    &s_path    },
  /* order == end == */
  { OPT_EMPTY,   &s_empty   },
  { OPT_GRP,     &s_gid     },
  { OPT_USR,     &s_uid     },
  { OPT_XDEV,    &s_xdev    },
  { OPT_DEL,     &s_delete  },
  { OPT_SORT,    &s_sort    },
  { OPT_STAT,    &s_stat    },
  { OPT_LSTAT,   &s_lstat   },
  { OPT_NAME,    &s_name    },
  { OPT_REGEX,   &s_regex   },
  { OPT_NONE,    NULL       },
};

static int plan_add(unsigned int *, plist_t *);
static int plan_execute(const char *, plan_t *, plist_t *);

int init_plan(plan_t *, plist_t *);
int find_plan(int, char **, plan_t *);
int execute_plan(plan_t *, plist_t *);
int add_plan(plan_t *, plist_t *);
void free_plan(plist_t *);

int
init_plan(plan_t *p, plist_t *pl)
{
  if (p == NULL || pl == NULL)
	return (-1);
  
  if ((p->acq_mt = (match_t *)malloc(sizeof(match_t))) == NULL ||
	  (p->acq_args = (args_t *)malloc(sizeof(args_t))) == NULL ||
	  (p->nstat = (nstat_t *)malloc(sizeof(nstat_t))) == NULL ||
	  (p->acq_paths = dl_init()) == NULL) {
	return (-1);
  }

  p->acq_mt->mflag = REG_BASIC;
  p->acq_args->odev = 0;
  p->acq_args->empty = 0;
  p->acq_flags = OPT_NONE;
  
  pl->cur = pl->start = NULL;
  pl->size = 0;
    
  return (0);
}

int
find_plan(int argc, char **argv, plan_t *p)
{
  int i, ret;
  
  if (argv == NULL || p == NULL)
	return (-1);
  
  if (argc == 0) {
	if (dl_empty(&(p->acq_paths))) {
	  p->acq_flags |= OPT_USAGE;
	}
	return (0);
  }
  
  if (argc > 0) {
	for (i = 0; i < argc && argv[i]; i++) {
#ifdef _DEBUG_
	  warnx("added path %s\n", argv[i]);
#endif
	  ret = dl_append(argv[i], &(p->acq_paths));
	}
	p->acq_flags |= OPT_PATH;
  }

  return (ret);
}

int
add_plan(plan_t *p, plist_t *pl)
{
  if (p == NULL || pl == NULL)
	return (-1);

  return (plan_add(&(p->acq_flags), pl));
}

int
execute_plan(plan_t *p, plist_t *pl)
{
  int ret;

  if (p == NULL || pl == NULL)
	return (-1);

  if (dl_empty(&(p->acq_paths))) {
	return (plan_execute(NULL, p, pl));
  }
  
  p->acq_paths->cur = p->acq_paths->head;
  while (p->acq_paths->cur != NULL) {
	ret = plan_execute(p->acq_paths->cur->ent, p, pl);
	p->acq_paths->cur = p->acq_paths->cur->next;
  }

  return (ret);
}

void
free_plan(plist_t *pl)
{
  int i;
  PLAN *tmp;
  
  if (pl == NULL)
	return;

  i = 0;
  if ((pl->cur = pl->start) == NULL) {
#ifdef _DEBUG_
	warnx("no plan!\n");
#endif
  }
  
#ifdef _DEBUG_
  warnx("We have %d plan(s) to free!\n", pl->size);
#endif
  
  while (pl->cur != NULL) {
	tmp = pl->cur;
	pl->cur = pl->cur->next;
#ifdef _DEBUG_
	warnx("free plan (%d)\n", i);
#endif
	tmp->s_func = NULL;
	free(tmp);
	tmp = NULL;
	i++;
  }
}

static int
plan_add(unsigned int *fl, plist_t *pl)
{
  int i;
  PLAN *new, *tmp;

  if (fl == NULL || pl == NULL)
	return (-1);
  
  for (i = 0; flags[i].s_func != NULL; i++) {

	if (*fl & flags[i].opt) {
	  
	  if ((new = (PLAN *)malloc(sizeof(PLAN))) == NULL)
		continue;
	  
	  new->next = NULL;
	  new->s_func = flags[i].s_func;
	  
	  if (pl->start == NULL) {
		pl->cur = pl->start = new;
		pl->size++;
#ifdef _DEBUG_
		warnx("null list, add plan(%d)\n", pl->size);
#endif
	  } else {
		tmp = pl->cur = pl->start;
		while (pl->cur != NULL) {
		  tmp = pl->cur;
		  pl->cur = pl->cur->next;
		}
		tmp->next = new;
		pl->cur = new;
		pl->size++;
#ifdef _DEBUG_
		warnx("add plan(%d)\n", pl->size);
#endif
	  }
	  
	  *fl &= ~(flags[i].opt);
	  if (flags[i].opt & OPT_VERSION)
		return (0);
	  if (flags[i].opt & OPT_USAGE)
		return (0);
	}
  }

  return ((i > 0) ? (0) : (-1));
}

static int
plan_execute(const char *s, plan_t *p, plist_t *pl)
{
  if (p == NULL || pl == NULL)
	return (-1);

  pl->cur = pl->start;

  while (pl->cur != NULL) {
	pl->retval |= pl->cur->s_func(s, p);
	pl->cur = pl->cur->next;
#ifdef _DEBUG_
	warnx("execute plan: retval=%d\n", pl->retval);
#endif
  }
  
  return (pl->retval);
}
