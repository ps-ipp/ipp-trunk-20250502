# include "dvomerge.h"

int main (int argc, char **argv) {

  SetSignals ();
  dvomerge_help (argc, argv);
  ConfigInit (&argc, argv);
  dvomerge_args (&argc, argv);

  if ((argc < 4) || (argc > 6)) dvomerge_usage();

  if (argc == 6) {
    if (strcasecmp (argv[2], "and")) dvomerge_usage();
    if (VERIFY) {
      fprintf (stderr, "WARNING / ERROR : dvomerge (input1) and (input2) into (output) : VERIFY mode not implemented\n");
      exit (3);
    }
    if (IMAGES_ONLY) {
      fprintf (stderr, "ERROR : dvomerge (input1) and (input2) into (output) : not compatible with '-images-only'\n");
      exit (5);
    }
    dvomergeCreate (argc, argv);
  }

  if ((argc == 4) || (argc == 5)) {
    if (NTHREADS) {
      dvomergeUpdate_threaded (argc, argv);
    } else {
      dvomergeUpdate (argc, argv);
    }
  }

  dvomerge_args_free ();
  ohana_memcheck (TRUE);
  ohana_memdump (TRUE);
  exit (0);
}

/* we have two major modes of operation:

   Create    : dvomerge (in1) and (in2) to (out) -- create a new db from two input dbs
   Update    : dvomerge (in) into (out)          -- merge a new db into an existing db

   we also have varients on Update:
   Continue  : dvomerge (in) into (out) continue -- merge only unmerged tables into the existing db

   neither of the 2 above modes update the image table
*/
