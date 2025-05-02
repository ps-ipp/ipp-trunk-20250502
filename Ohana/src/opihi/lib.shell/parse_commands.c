# include "opihi.h"
# define D_NARG 10

char **parse_commands (char *line, int *argc) {

  int i, NARG;
  char *c;
  char **argv;

  NARG = D_NARG;
  ALLOCATE (argv, char *, NARG);

  argv[0] = thisword (line);
  if (argv[0] == (char *) NULL) {
    free (argv);
    *argc = 0;
    return ((char **) NULL);
  }

  c = nextword (line);
  for (i = 1; c != (char *) NULL; i++) {
    argv[i] = thisword (c);

    /* if one of the words does not parse (eg, ""), skip it */
    if (argv[i] == NULL) i--;

    c = nextword (c);
    if (i == NARG - 1) {
      NARG += D_NARG;
      REALLOCATE (argv, char *, NARG);
    }
  }
  REALLOCATE (argv, char *, i);
  *argc = i;

  return (argv);

}

  /* parse out the command line into (char **argv).  line looks like:
     command word word word ... */

  /* argv[0] and argv[1..argc-1] are handled differently because
     the syntax of a valid command and a valid word are slighly 
     different (not that this is obviously wanted...) */



/* old code:
      for (j = 0; j < i; j++) 
	free (argv[i]);
      free (argv);
      *argc = 0;
      return (argv);
    }
*/
