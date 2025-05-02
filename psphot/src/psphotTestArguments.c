# include "psphotInternal.h"
static int usage ();

void psphotTestArguments (int *argc, char **argv) {

  // basic pslib options
  psLogSetFormat ("M");
  psArgumentVerbosity (argc, argv);

  if (*argc != 7) usage ();

  return;
}

static int usage () {

    fprintf (stderr, "USAGE: psphotTest (input.fits) (output.jpg) (zero) (scale) (colormap) (rebin)\n");
    exit (2);
}
