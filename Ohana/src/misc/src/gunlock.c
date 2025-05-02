# include <stdio.h>

main (int argc, char **argv) {

  if (argc != 2) {
    fprintf (stderr, "USAGE: gunlock (filename)\n");
    exit (1);
  }

  if (!clearlockfile (argv[1], -1, 1)) {
    fprintf (stdout, "LOCK NOT REMOVED\n");
    exit (1);
  }
  
  fprintf (stdout, "LOCK REMOVED\n");
  exit (0);

}
