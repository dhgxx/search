#include "search.h"
#include "extern.h"

static const FLAGS flags[] = {
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
  { OPT_PATH,    &s_path    },
  { OPT_VERSION, &s_version },
  { OPT_USAGE,   &s_usage   },
  { OPT_NONE,    NULL       },
};

int init_plan(plan_t *, plist_t *);
int find_plan(int, char **, plan_t *);
int exec_plan(plan_t *, plist_t *);
void add_plan(plan_t *, plist_t *);
void free_plan(plist_t *);

int
init_plan(plan_t *p, plist_t *pl)
{
  if (p == NULL)
	return (-1);
  if (pl == NULL)
	return (-1);
  
  if ((p->acq_mt = (match_t *)malloc(sizeof(match_t))) == NULL ||
	  (p->acq_args = (args_t *)malloc(sizeof(args_t))) == NULL ||
	  (p->nstat = (nstat_t *)malloc(sizeof(nstat_t))) == NULL ||
	  (p->acq_paths = dl_init()) == NULL) {
	return (-1);
  }

  p->acq_flags = OPT_NONE;
  pl->cur = pl->start;
  pl->size = 0;
    
  return (0);
}

int
find_plan(int argc, char **argv, plan_t *p)
{
  int i;

  if (p == NULL)
	return (-1);
  if (p->acq_mt == NULL)
	return (-1);
  if (p->acq_paths == NULL)
	return (-1);

  if (argc == 0) {
	if (p->acq_flags & OPT_VERSION)
	  return (0);
	if (p->acq_flags & OPT_USAGE)
	  return (0);
	if (p->acq_flags & OPT_PATH) {
	  if (dl_empty(&(p->acq_paths)))
		return (-1);
	}
  }
	  
  if (argc > 0) {
	if (p->acq_flags & OPT_NAME) {
	  if (strlen(p->acq_mt->pattern) <= 0)
		return (-1);
	}
	for (i = 0; i < argc && argv[i]; i++)
	  dl_append(argv[i], &(p->acq_paths));
  }

  return (0);
}

void
add_plan(plan_t *p, plist_t *pl)
{
  int i;
  PLAN *new, *tmp;

  if (p == NULL)
	return;
  if (pl == NULL)
	return;

  pl->cur = pl->start;

  if (p->acq_flags & OPT_USAGE) {
	if ((pl->start = (PLAN *)malloc(sizeof(PLAN))) == NULL)
	  return;
	pl->start->s_func = &s_usage;
	pl->start->next = NULL;
	pl->cur = pl->start;
	pl->size = 1;
	return;	
  }
  
  for (i = 0;  flags[i].s_func != NULL; i++) {
	if (p->acq_flags & flags[i].opt) {
	  if ((new = (PLAN *)malloc(sizeof(PLAN))) != NULL) {
		new->next = NULL;
		new->s_func = flags[i].s_func;
		if (pl->start == NULL) {
		  pl->start = new;
		  pl->cur = pl->start;
		  pl->size++;
#ifdef _DEBUG_
		  fprintf(stderr, "add plan(%d)\n", pl->size);
#endif
		} else {
		  while (pl->cur != NULL) {
			tmp = pl->cur;
			pl->cur = pl->cur->next;
		  }
		  tmp->next = new;
		  pl->cur = new;
		  pl->size++;
#ifdef _DEBUG_
		  fprintf(stderr, "add plan(%d)\n", pl->size);
#endif
		}
	  }
	}
  }
}

int
exec_plan(plan_t *p, plist_t *pl)
{
  if (p == NULL)
	return (-1);
  if (pl == NULL)
	return (-1);
  
  if (dl_empty(&(p->acq_paths))) {
	pl->cur = pl->start;
	while (pl->cur != NULL) {
	  pl->retval = pl->cur->retval = pl->cur->s_func(NULL, NULL);
	  pl->cur = pl->cur->next;
	}
	return (pl->retval);
  }
  
  p->acq_paths->cur = p->acq_paths->head;
  while (p->acq_paths->cur != NULL) {
	pl->cur = pl->start;
	while (pl->cur != NULL) {
	  pl->retval = pl->cur->retval = pl->cur->s_func(p->acq_paths->cur->ent, p);
#ifdef _DEBUG_
	  warnx("pl->cur->retval=%d, pl->retval=%d\n",
			pl->cur->retval, pl->retval);
#endif
	  pl->cur = pl->cur->next;
	}
	p->acq_paths->cur = p->acq_paths->cur->next;
  }

  return (pl->retval);
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
	fprintf(stderr, "no plan!\n");
#endif
  }
  
#ifdef _DEBUG_
  warnx("We have %d plan(s) to free!\n", pl->size);
#endif
  
  while (pl->cur != NULL) {
	tmp = pl->cur;
	pl->cur = pl->cur->next;
#ifdef _DEBUG_
	fprintf(stderr, "free plan (%d)\n", i);
#endif
	tmp->s_func = NULL;
	free(tmp);
	tmp = NULL;
	i++;
  }
}
