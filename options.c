#include <getopt.h>

#include "search.h"

extern int exec_name(const char *, options_t **, match_t **);
extern int exec_regex(const char *, options_t **, match_t **);

void lookup_options(int, char *[], options_t **, match_t **);
void display_usage(void);
void display_version(void);

void
lookup_options(int argc, char *argv[], options_t **o, match_t **m)
{
  int ch;
  static unsigned int opt_empty;
  static unsigned int opt_delete;
  options_t *opt = *o;
  match_t *mt = *m;
  
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
	  opt->flags |= OPT_GRP;
	  bzero(opt->group, LINE_MAX);
	  strncpy(opt->group, optarg, LINE_MAX);
	  break;
	case 4:
	case 5:
	  opt->flags |=  OPT_USR;
	  bzero(opt->user, LINE_MAX);
	  strncpy(opt->user, optarg, LINE_MAX);
	  break;
	case 'f':
	  opt->flags |= OPT_PATH;
	  bzero(opt->path, MAXPATHLEN);
	  strncpy(opt->path, optarg, MAXPATHLEN);
	  break;
	case 'n':
	  bzero(mt->pattern, LINE_MAX);
	  strncpy(mt->pattern, optarg, LINE_MAX);
	  opt->exec_func = exec_name;
	  break;
	case 'r':
	  bzero(mt->pattern, LINE_MAX);
	  strncpy(mt->pattern, optarg, LINE_MAX);
	  opt->exec_func = exec_regex;
	  break;
	case 0:
	  if (opt_empty == 1)
		opt->flags |= OPT_EMPTY;
	  if (opt_delete == 1)
		opt->flags |=  OPT_DEL;
	  break;
	case 's':
	  opt->flags |= OPT_SORT;
	  break;
	case 'v':
	  display_version();
	  break;
	case 'x':
	  opt->flags |= OPT_XDEV;
	  break;
	case 't':
	  switch (optarg[0]) {
	  case 'f':
		opt->n_type = NT_ISFIFO;
		break;
	  case 'c':
		opt->n_type = NT_ISCHR;
		break;
	  case 'd':
		opt->n_type = NT_ISDIR;
		break;
	  case 'b':
		opt->n_type = NT_ISBLK;
		break;
	  case 'l':
		opt->n_type = NT_ISLNK;
		opt->stat_func = lstat; 
		break;
	  case 's':
		opt->n_type = NT_ISSOCK;
		break;
#ifndef _OpenBSD_
	  case 'w':
		opt->n_type = NT_ISWHT;
		break;
#endif
	  case 'r':
	  case '\0':
		opt->n_type = NT_ISREG;
		break;
	  default:
		display_usage();
		break;
	  }
	  break;
	case 'E':
	  mt->cflag |= REG_EXTENDED;
	  break;
	case 'I':
	  opt->flags |= OPT_ICAS;
	  break;
	case 'L':
	  opt->stat_func = stat;
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
  static const char *usage = "usage:\t%s [-EILPsv]\
 ...\
 [-f|--path ...]\
 [-n|--name ...]\
 [-r|--regex ...]\
 [-t|--type ...]\
 [...]\n\
 \t%s [-EILPsv]\
 -f | --path ...\
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
