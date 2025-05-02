# include "setastrom.h"

int main (int argc, char **argv) {

  int status;

  /* get configuration info, args, lockfile */
  SetSignals ();
  initialize_setastrom (argc, argv);

  status = update_dvo_setastrom ();

  if (!status) exit (1);
  exit (0);
}
  

/* setastrom : set the posangle & pltscale for measurements in the db

   ** load images
   ** load catalogs
   ** update detection (& averages?)
 */
