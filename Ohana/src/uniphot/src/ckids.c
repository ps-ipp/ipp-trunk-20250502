# include "ckids.h"

int main (int argc, char **argv) {

  int status;

  /* get configuration info, args, lockfile */
  SetSignals ();
  initialize_ckids (argc, argv);

  status = update_dvo_ckids ();

  if (!status) exit (1);
  exit (0);
}
  

/* ckids : check for duplicate IDs for measurements in the db
   ** load catalogs 
   ** count duplicate detections
 */
