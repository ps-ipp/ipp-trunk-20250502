# include "data.h"

int rndseed (int argc, char **argv) {
  
  if (argc < 2) {
    gprint (GP_ERR, "USAGE: rndseed (value)\n");
    return (FALSE);
  }

  /* init srand for rnd numbers elsewhere */
  long A = atol (argv[1]);
  srand48(A);

  return (TRUE);
}

 
  
