#include <getopt.h>

#include "search.h"

extern int exec_name(const char *, plan_t *);
extern int exec_regex(const char *, plan_t *);

static void ftype_err(const char *);

void lookup_options(int, char *[], plan_t *);
void display_usage(void);
void display_version(void);

void
lookup_options(int argc, char *argv[], plan_t *p)
{
  int ch;
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

  while ((ch = getopt_long(argc, argv, "EILPsvxf:n:r:t:", longopts, NULL)) != -1)
	switch (ch) {
	case 2:
	case 3:
	  p->opt->flags |= OPT_GRP;
	  bzero(p->opt->group, LINE_MAX);
	  strncpy(p->opt->group, optarg, LINE_MAX);
	  break;
	case 4:
	case 5:
	  p->opt->flags |=  OPT_USR;
	  bzero(p->opt->user, LINE_MAX);
	  strncpy(p->opt->user, optarg, LINE_MAX);
	  break;
	case 'f':
	  p->opt->flags |= OPT_PATH;
	  bzero(p->opt->path, MAXPATHLEN);
	  strncpy(p->opt->path, optarg, MAXPATHLEN);
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
		p->opt->flags |= OPT_EMPTY;
	  if (opt_delete == 1)
		p->opt->flags |=  OPT_DEL;
	  break;
	case 's':
	  p->opt->flags |= OPT_SORT;
	  break;
	case 'v':
	  display_version();
	  break;
	case 'x':
	  p->opt->flags |= OPT_XDEV;
	  break;
	case 't':
	  switch (optarg[0]) {
	  case 'p':
		p->opt->o_type = NT_ISFIFO;
		break;
	  case 'c':
		p->opt->o_type = NT_ISCHR;
		break;
	  case 'd':
		p->opt->o_type = NT_ISDIR;
		break;
	  case 'b':
		p->opt->o_type = NT_ISBLK;
		break;
	  case 'l':
		p->opt->o_type = NT_ISLNK;
		p->stat_func = lstat; 
		break;
	  case 's':
		p->opt->o_type = NT_ISSOCK;
		break;
	  case 'f':
	  case '\0':
		p->opt->o_type = NT_ISREG;
		break;
	  default:
		ftype_err(optarg);
		break;
	  }
	  break;
	case 'E':
	  p->mt->cflag |= REG_EXTENDED;
	  break;
	case 'I':
	  p->opt->flags |= OPT_ICAS;
	  break;
	case 'L':
	  p->stat_func = stat;
	  break;
	case 'P':
	  break;
	default:
	  display_usage();
	  break;
	}
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
  exit (0);
}

void
display_version(void)
{  
  (void)fprintf(stderr,	"%s version %s\n",
				SEARCH_NAME, SEARCH_VERSION);
  exit (0);
}

static void
ftype_err(const char *s)
{
  if (s == NULL)
	return;

  (void)fprintf(stderr, "%s: --type: %s: unknown type\n",
				SEARCH_NAME, s);
  exit (0);
}
