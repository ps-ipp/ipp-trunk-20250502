# include "setgalmodel.h"

int main (int argc, char **argv) {

  int status;

  /* get configuration info, args, lockfile */
  SetSignals ();
  initialize_setgalmodel (argc, argv);

  status = update_dvo_setgalmodel ();

  if (!status) {
    fprintf (stderr, "ERROR: problem running setgalmodel\n");
    exit (1);
  }
  fprintf (stderr, "SUCCESS running setgalmodel\n");
  exit (0);
}
  

/* setgalmodel : set starpar.uRA,uDEC based on starpar.DM

   ** load images
   ** load catalogs
   ** update detection (& averages?)
 */
