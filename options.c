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

  if (p == NULL ||
	  p->acq_mt == NULL ||
	  p->acq_args == NULL ||
	  p->acq_plan == NULL)
	return (-1);

  ret = 0;

  while ((ch = getopt_long(argc, argv, "EILPsvxf:n:r:t:", longopts, NULL)) != -1)
	switch (ch) {
	case 2:
	case 3:
	  p->acq_flags |= OPT_GRP;
	  bzero(p->acq_args->sgid, LINE_MAX);
	  strncpy(p->acq_args->sgid, optarg, LINE_MAX);
	  break;
	case 4:
	case 5:
	  p->acq_flags |=  OPT_USR;
	  bzero(p->acq_args->suid, LINE_MAX);
	  strncpy(p->acq_args->suid, optarg, LINE_MAX);
	  break;
	case 'f':
	  dl_append(optarg, &(p->acq_paths));
	  break;
	case 'n':
	  bzero(p->acq_mt->pattern, LINE_MAX);
	  strncpy(p->acq_mt->pattern, optarg, LINE_MAX);
	  break;
	case 'r':
	  bzero(p->acq_mt->pattern, LINE_MAX);
	  strncpy(p->acq_mt->pattern, optarg, LINE_MAX);
	  break;
	case 0:
	  if (opt_empty == 1)
		p->acq_flags |= OPT_EMPTY;
	  if (opt_delete == 1)
		p->acq_flags |=  OPT_DEL;
	  break;
	case 's':
	  p->acq_flags |= OPT_SORT;
	  break;
	case 'v':
	  display_version();
	  ret = -1;
	  break;
	case 'x':
	  p->acq_flags |= OPT_XDEV;
	  break;
	case 't':
	  switch (optarg[0]) {
	  case 'p':
		p->acq_args->type = NT_ISFIFO;
		break;
	  case 'c':
		p->acq_args->type = NT_ISCHR;
		break;
	  case 'd':
		p->acq_args->type = NT_ISDIR;
		break;
	  case 'b':
		p->acq_args->type = NT_ISBLK;
		break;
	  case 'l':
		p->acq_args->type = NT_ISLNK;
		break;
	  case 's':
		p->acq_args->type = NT_ISSOCK;
		break;
	  case 'f':
	  case '\0':
		p->acq_args->type = NT_ISREG;
		break;
	  default:
		ftype_err(optarg);
		ret = -1;
		break;
	  }
	  break;
	case 'E':
	  p->acq_mt->mflag |= REG_EXTENDED;
	  break;
	case 'I':
	  p->acq_mt->mflag |= REG_ICASE;
	  break;
	case 'L':
	  p->acq_flags |= OPT_STAT;
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
