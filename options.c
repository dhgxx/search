#include <getopt.h>

#include "search.h"
#include "extern.h"

static void ftype_err(const char *);

int lookup_options(int, char *[], plan_t *);
void display_usage(void);
void display_version(void);

int
lookup_options(int argc, char *argv[], plan_t *p)
{
  int ch, ret;
  static int opt_empty;
  static int opt_delete;
  
  static struct option longopts[] = {
	{ "gid",     required_argument, NULL,        2  },
	{ "group",   required_argument, NULL,        3  },
	{ "path",    required_argument, NULL,       'f' },
	{ "name",    required_argument, NULL,       'n' },
	{ "regex",   required_argument, NULL,       'r' },
	{ "type",    required_argument, NULL,       't' },
	{ "user",    required_argument, NULL,        4  },
	{ "uid",     required_argument, NULL,        5  },
	{ "empty",   no_argument,       &opt_empty,  1  },
	{ "delete",  no_argument,       &opt_delete, 1  },
	{ "sort",    no_argument,       NULL,       's' },
	{ "version", no_argument,       NULL,       'v' },
	{ "xdev",    no_argument,       NULL,       'x' },
	{ NULL,      0,                 NULL,        0  }
  };

  ret = 0;

  while ((ch = getopt_long(argc, argv, "EILPsvxf:n:r:t:", longopts, NULL)) != -1)
	switch (ch) {
	case 2:
	case 3:
	  p->flags |= OPT_GRP;
	  bzero(p->group, LINE_MAX);
	  strncpy(p->group, optarg, LINE_MAX);
	  break;
	case 4:
	case 5:
	  p->flags |=  OPT_USR;
	  bzero(p->user, LINE_MAX);
	  strncpy(p->user, optarg, LINE_MAX);
	  break;
	case 'f':
	  dl_append(optarg, &(p->paths));
	  break;
	case 'n':
	  bzero(p->mt->pattern, LINE_MAX);
	  strncpy(p->mt->pattern, optarg, LINE_MAX);
	  p->exec_func = exec_name;
	  break;
	case 'r':
	  bzero(p->mt->pattern, LINE_MAX);
	  strncpy(p->mt->pattern, optarg, LINE_MAX);
	  p->exec_func = exec_regex;
	  break;
	case 0:
	  if (opt_empty == 1)
		p->flags |= OPT_EMPTY;
	  if (opt_delete == 1)
		p->flags |=  OPT_DEL;
	  break;
	case 's':
	  p->flags |= OPT_SORT;
	  break;
	case 'v':
	  display_version();
	  ret = -1;
	  break;
	case 'x':
	  p->flags |= OPT_XDEV;
	  break;
	case 't':
	  switch (optarg[0]) {
	  case 'p':
		p->type = NT_ISFIFO;
		break;
	  case 'c':
		p->type = NT_ISCHR;
		break;
	  case 'd':
		p->type = NT_ISDIR;
		break;
	  case 'b':
		p->type = NT_ISBLK;
		break;
	  case 'l':
		p->type = NT_ISLNK;
		p->stat_func = lstat; 
		break;
	  case 's':
		p->type = NT_ISSOCK;
		break;
	  case 'f':
	  case '\0':
		p->type = NT_ISREG;
		break;
	  default:
		ftype_err(optarg);
		ret = -1;
		break;
	  }
	  break;
	case 'E':
	  p->mt->mflag |= REG_EXTENDED;
	  break;
	case 'I':
	  p->mt->mflag |= REG_ICASE;
	  break;
	case 'L':
	  p->stat_func = stat;
	  break;
	case 'P':
	  break;
	default:
	  display_usage();
	  ret = -1;
	  break;
	}

  return (ret);
}

void
display_usage(void)
{  
  static const char *usage = "usage:\t%s [-EILPsxv]\
 ...\
 [-f|--path ...]\
 [-n|--name ...]\
 [-r|--regex ...]\
 [-t|--type ...]\
 [...]\n\
 \t%s [-EILPsxv]\
 -f|--path ...\
 [...]\
 [-n|--name ...]\
 [-r|--regex ...]\
 [-t|--type ...]\
 [...]\n";

  (void)fprintf(stderr,	usage,
				SEARCH_NAME, SEARCH_NAME);
  return;
}

void
display_version(void)
{  
  (void)fprintf(stderr,	"%s version %s\n",
				SEARCH_NAME, SEARCH_VERSION);
  return;
}

static void
ftype_err(const char *s)
{
  if (s == NULL)
	return;

  warnx("--type: %s: unknown type", s);
  return;
}
