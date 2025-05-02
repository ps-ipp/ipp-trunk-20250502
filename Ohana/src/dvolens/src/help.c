# include "dvolens.h"

void dvolens_usage (void) {
  fprintf (stderr, "ERROR: USAGE: dvolens -update-objects\n");
  fprintf (stderr, "  regions:    -region RA RA DEC DEC)\n");
  fprintf (stderr, "       or:    -catalog (name)\n");
  fprintf (stderr, "  use -h for more usage information\n");
  exit (2);
} 

void dvolens_help (int argc, char **argv) {
  /* check for help request */
  if (get_argument (argc, argv, "-help")) goto show_help;
  if (get_argument (argc, argv, "-h"))    goto show_help;
  if (argc == 1) dvolens_usage();
  return;

show_help:
  fprintf (stderr, "ERROR: USAGE: dvolens -update-objects\n");
  fprintf (stderr, "  regions:    -region RA RA DEC DEC)\n");
  fprintf (stderr, "       or:    -catalog (name)\n");
  fprintf (stderr, "  options: \n");
  fprintf (stderr, "  -time (start) (stop)\n");
  fprintf (stderr, "  -v : verbose output\n");
  fprintf (stderr, "  -vv : more verbose output\n");
  fprintf (stderr, "  -statmode (mode)\n");
  fprintf (stderr, "  -n (nloop)\n");
  fprintf (stderr, "  -reset\n");
  fprintf (stderr, "  -update\n");
  fprintf (stderr, "  \n");
  exit (2);
}

void dvolens_client_usage (void) {
  fprintf (stderr, "ERROR: USAGE: dvolens -update-objects -hostID (hostID) -hostdir (hostdir) [options]\n");
  fprintf (stderr, "  use -h for more usage information\n");
  exit (2);
} 

void dvolens_client_help (int argc, char **argv) {

  /* check for help request */
  if (get_argument (argc, argv, "-help")) goto show_help;
  if (get_argument (argc, argv, "-h"))    goto show_help;
  if (argc == 1) dvolens_client_usage();
  return;

show_help:
  fprintf (stderr, "USAGE: dvolens_client -update-objects  : determine average magnitudes for objects\n\n");
  fprintf (stderr, "       db info : -hostID (hostID) -hostdir (hostdir) -catdir (catdir)\n");
  fprintf (stderr, "other options:\n");
  fprintf (stderr, "  -v  : verbose output\n");
  fprintf (stderr, "  -vv : extra verbose output\n");
  fprintf (stderr, "  \n");
  exit (2);
}

