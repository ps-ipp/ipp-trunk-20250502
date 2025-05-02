# include "pantasks.h"

int invalid (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);

  gprint (GP_ERR, "%s is not valid for the pantasks client\n", argv[0]);
  return (TRUE);

}
