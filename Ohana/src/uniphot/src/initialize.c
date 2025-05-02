# include "uniphot.h"

void initialize_uniphot (int argc, char **argv) {

  /* are these set correctly? */
  ConfigInit (&argc, argv);
  args_uniphot (argc, argv);

  if ((photcode = GetPhotcodebyName (argv[1])) == NULL) {
    fprintf (stderr, "ERROR: photcode not found in photcode table\n");
    exit (1);
  }
  if ((photcode[0].type == PHOT_DEP) && (photcode[0].type == PHOT_REF)) {
    fprintf (stderr, "photcode must be primary or secondary type\n");
    exit (1);
  }

  IMAGE_BAD = ID_IMAGE_PHOTOM_NOCAL | ID_IMAGE_PHOTOM_POOR | ID_IMAGE_PHOTOM_SKIP | ID_IMAGE_PHOTOM_FEW;

  initstats (STATMODE);
}

void initialize_setfwhm (int argc, char **argv) {

  /* are these set correctly? */
  ConfigInit (&argc, argv);
  args_setfwhm (argc, argv);
}

