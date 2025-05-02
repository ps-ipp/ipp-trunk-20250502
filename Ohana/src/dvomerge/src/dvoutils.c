# include "dvoutils.h"

int main (int argc, char **argv) {

  // check various options
  SetSignals ();
  dvoutils_args (&argc, argv);

  switch (DVOUTILS_OP) {
    
    case DVOUTILS_UNIQ_IMAGES:
      dvoutils_uniq_images(IMAGES_LIST);
      exit (0);

    case DVOUTILS_CHECK_IMAGES:
      dvoutils_check_images();
      exit (0);

    default:
      fprintf (stderr, "ERROR: unknown option\n");
      exit (2);
  }

  exit (2);
}
