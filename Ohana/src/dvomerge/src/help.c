# include "dvomerge.h"

void dvomerge_usage (void) {
  fprintf (stderr, "USAGE: dvomerge (input1) and (input2) to (output)\n");
  fprintf (stderr, "   OR: dvomerge (input) into (output)\n");
  fprintf (stderr, "   OR: dvomerge (input) into (output) continue\n");
  fprintf (stderr, "   [-region Rmin Rmax Dmin Dmax]\n");
  exit (2);
}

void dvomerge_client_usage (void) {
  fprintf (stderr, "USAGE: dvomerge_client (input) into (output)\n");
  fprintf (stderr, "   [-region Rmin Rmax Dmin Dmax]\n");
  exit (2);
}

void dvoconvert_usage(void) {

  fprintf (stderr, "USAGE: dvoconvert (input) to (output)\n");

  exit (2);
}

void dvosecfilt_usage(void) {

  fprintf (stderr, "USAGE: dvosecfilt (catdir) (Nsecfilt)\n\n");
  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");

  exit (2);
}

void dvosecfilt_client_usage(void) {

  fprintf (stderr, "USAGE: dvosecfilt_client (catdir) (Nsecfilt)\n\n");
  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");

  exit (2);
}

void dvomerge_help (int argc, char **argv) {

  /* check for help request */
  if (get_argument (argc, argv, "-help")) goto show_help;
  if (get_argument (argc, argv, "-h"))    goto show_help;
  return;

show_help:

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  dvomerge (input1) and (input2) to (output)\n");
  fprintf (stderr, "  dvomerge (input) into (output)\n");
  fprintf (stderr, "  dvomerge (input) into (output) continue\n\n");
  fprintf (stderr, "  dvomerge (input) into (output) from (list)\n");
  fprintf (stderr, "     dvomerge list implies 'continue' : list contains, eg, n1500/1688.00.cpt (one cpt per line)\n\n");
  fprintf (stderr, "  merge DVO databases\n");
  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -verify                  	  : verify merge status of output tables, but do not modify\n");
  fprintf (stderr, "  -check-only              	  : verify merge status of output tables, but do not modify [same as -verify]\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  fprintf (stderr, "  -region Rmin Rmax Dmin Dmax : region to merge\n\n");
  fprintf (stderr, "  -replace                    : replace existing detections of the same photcode\n");
  fprintf (stderr, "                                (this should only be used to merge reference dbs)\n\n");
  exit (2);
}

void dvomerge_client_help (int argc, char **argv) {

  /* check for help request */
  if (get_argument (argc, argv, "-help")) goto show_help;
  if (get_argument (argc, argv, "-h"))    goto show_help;
  return;

show_help:

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  dvomerge_client (input) into (output)\n");
  fprintf (stderr, "  merge DVO databases\n");
  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -verify                  	  : verify merge status of output tables, but do not modify\n");
  fprintf (stderr, "  -check-only              	  : verify merge status of output tables, but do not modify [same as -verify]\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  fprintf (stderr, "  -region Rmin Rmax Dmin Dmax : region to merge\n\n");
  fprintf (stderr, "  -replace                    : replace existing detections of the same photcode\n");
  fprintf (stderr, "                                (this should only be used to merge reference dbs)\n\n");
  exit (2);
}

void dvoconvert_help (int argc, char **argv) {

  /* check for help request */
  if (get_argument (argc, argv, "-help")) goto show_help;
  if (get_argument (argc, argv, "-h"))    goto show_help;
  return;

show_help:

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  dvoconvert (input) to (output)\n\n");

  fprintf (stderr, "  change output format and mode with\n");
  fprintf (stderr, "  -D CATFORMAT (format)\n");
  fprintf (stderr, "  -D CATMODE (mode)\n\n");
 
  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}

void dvosecfilt_help (int argc, char **argv) {

  /* check for help request */
  if (get_argument (argc, argv, "-help")) goto show_help;
  if (get_argument (argc, argv, "-h"))    goto show_help;
  return;

show_help:

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  dvosecfilt (catdir) (Nsecfilt)\n\n");

  fprintf (stderr, "  change number of secfilt entries in photcode table (updates secfilt tables (cps), NSECFILT in cpt files)\n");
  fprintf (stderr, "  NOTE: the user must change the photcode table to reflect the change (use photcode-table -export / -import)\n");
 
  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}

void dvosecfilt_client_help (int argc, char **argv) {

  /* check for help request */
  if (get_argument (argc, argv, "-help")) goto show_help;
  if (get_argument (argc, argv, "-h"))    goto show_help;
  return;

show_help:

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  dvosecfilt_client (catdir) (Nsecfilt)\n\n");

  fprintf (stderr, "  change number of secfilt entries in photcode table (updates secfilt tables (cps), NSECFILT in cpt files)\n");
  fprintf (stderr, "  NOTE: the user must change the photcode table to reflect the change (use photcode-table -export / -import)\n");
 
  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}

