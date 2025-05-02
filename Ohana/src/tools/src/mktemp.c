# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#ifdef __APPLE__
// mkdtemp is aparently declared here on macos
#include <unistd.h>
#endif

# define FALSE 0
# define TRUE 1

void usage(void);

int main (int argc, char **argv) {

  int i, j, Ntotal;
  char *tmpdir, *template, deftemplate[32], *prefix, defprefix[32], *filename;
  int make_directory, fail_silently, full_path;

  tmpdir = NULL;
  prefix = NULL;
  template = NULL;
  filename = NULL;

  make_directory = FALSE;
  fail_silently = FALSE;
  // int unsafe_mode = FALSE; -u not implemented
  full_path = TRUE;

  for (i = 1; i < argc; i++) {
    // -options must be first
    if (argv[i][0] == '-') {
      if (!strcmp(argv[i], "-V")) {
	fprintf (stdout, "mktemp version Ohana $Revision: $\n");
	exit (0);
      }
      if (!strcmp(argv[i], "-p")) {
	if (argc <= i + 1) usage();
	i++;
	prefix = argv[i];
	full_path = FALSE;
	continue;
      }
      for (j = 1; j < strlen(argv[i]); j++) {
	if (argv[i][j] == 'q') {
	  fail_silently = TRUE;
	  continue;
	}
	if (argv[i][j] == 't') {
	  full_path = FALSE;
	  continue;
	}
	if (argv[i][j] == 'd') {
	  // make directory
	  make_directory = TRUE;
	  continue;
	}
	// -u == --dry-run, not implemented
	// if (argv[i][j] == 'u') {
	//   unsafe_mode = TRUE;
	//   continue;
	// }

	// unknown option
	usage();
      }
      continue;
    }
    // report an error if too many arguments are given
    if (template) usage();
    template = argv[i];
  }

  if (!full_path) {
    // prefix = TMPDIR ? TMPDIR : (prefix ? prefix : /tmp)
    tmpdir = getenv("TMPDIR");
    if (tmpdir) {
      prefix = tmpdir;
    }
    if (!prefix) {
      strcpy (defprefix, "/tmp");
      prefix = defprefix;
    }
    if (template && strchr(template, '/')) usage();
  }

  if (!template) {
    if (full_path) {
      strcpy (deftemplate, "/tmp/tmp.XXXXXXXXXX");
    } else {
      strcpy (deftemplate, "tmp.XXXXXXXXXX");
    }
    template = deftemplate;
  }

  // filename = full_path ? prefix/template : template;
  if (!full_path) {
    Ntotal = strlen(prefix) + strlen(template) + 2;
    filename = (char *) malloc (Ntotal);
    snprintf (filename, Ntotal, "%s/%s", prefix, template);
    template = filename;
  }

  if (make_directory) {
    if (mkdtemp (template) == NULL) {
      if (!fail_silently) fprintf (stderr, "failed to make temp file from %s\n", template);
      exit (1);
    }
  } else {
    if (mkstemp (template) == -1) {
      if (!fail_silently) fprintf (stderr, "failed to make temp file from %s\n", template);
      exit (1);
    }
  }

  fprintf (stdout, "%s\n", template);

  exit (0);
}
 
void usage(void) {
  fprintf (stderr, "Usage: mktemp [-V] | [-dqtu] [-p prefix] [template]\n");
  exit (1);
}
