# include "pclient.h"

int check (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);
  OHANA_UNUSED_PARAM(argv);

  /* force a check */
  CheckChild ();
  return (TRUE);

}
