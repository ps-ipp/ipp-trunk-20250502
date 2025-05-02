# include "ohana.h"
extern double drand48();

# define NFILTER 5
static char filtlist[NFILTER][2] = {"B", "V", "R", "I", "Z"};

int main (int argc, char **argv) {
 
  FILE *f;
  int j, rnd;
  int ccd, state;
  char type[64], filter[64], name[64];
  struct timeval now;
  struct tm *gmt;

  if (argc != 2) {
    fprintf (stderr, "USAGE: mknames (fifo)\n");
    exit (1);
  }

  /* lock fifo */
  f = fsetlockfile (argv[1], 60.0, LCK_XCLD, &state);
  if (f == (FILE *) NULL) {
    fprintf (stderr, "can't open fifo file\n");
    fclearlockfile (argv[1], f, LCK_XCLD, &state);
    exit (1);
  }

  gettimeofday (&now, (void *) NULL);
  gmt = gmtime (&now.tv_sec);
  srand48(now.tv_usec);
  rnd = 1000*drand48();
  sprintf (name, "%04d.%02d.%02d.%03d", gmt[0].tm_year + 1900, gmt[0].tm_mon + 1, gmt[0].tm_mday, rnd);

  for (ccd = 0; ccd < 12; ccd++) {
    strcpy (filter, "X");
    strcpy (type, "bias");
    fprintf (f, "%02d %s %s %s\n", ccd, type, filter, name);
    strcpy (type, "dark");
    fprintf (f, "%02d %s %s %s\n", ccd, type, filter, name);
    strcpy (type, "flat");
    for (j = 0; j < NFILTER; j++) {
      strcpy (filter, filtlist[j]);
      fprintf (f, "%02d %s %s %s\n", ccd, type, filter, name);
    }
  }
  fclearlockfile (argv[1], f, LCK_XCLD, &state);
  exit (0);
}

  /* 
     
     we need to generate a list of entries for 'elixir flips'
     the entries look like:

     (ccd) (type) (filter) (tend) (nameref)

     (ccd)  - ranges from 00 to 11 
     (type) - values are bias, dark, flat, ?
     (filter) - for bias & dark = X
                for flat = B,V,R,I,Z (Hon?, Hoff?)
     (tend) = 'NOW' (probably don't need to generate this!)
     (nameref) = SSSSSS (some string, needs to be unique... based on date?)

  */
		
