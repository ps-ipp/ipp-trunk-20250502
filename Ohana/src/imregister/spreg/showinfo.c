# include "imregister.h"
# include "spreg.h"

void showinfo (Spectrum *spec) {

  char *obstime, *regtime;

  fprintf (stderr, "\n");
  fprintf (stderr, "filename: %s\n",   spec[0].filename);
  fprintf (stderr, "pathname: %s\n",   spec[0].pathname);
  fprintf (stderr, "extname:  %s\n\n",   spec[0].extname);

  fprintf (stderr, "instrument: %s\n\n", spec[0].instrument);
  fprintf (stderr, "telescope: %s\n\n", spec[0].telescope);


  fprintf (stderr, "mode: %d, state: %d, flag: %x\n\n", spec[0].mode, spec[0].state, spec[0].flag);

  fprintf (stderr, "exptime: %f, airmass: %f\n", spec[0].exptime, spec[0].airmass);
  fprintf (stderr, "ra: %f, dec: %f\n", spec[0].ra, spec[0].dec);
  fprintf (stderr, "objname: %s\n\n", spec[0].objname);

  fprintf (stderr, "Ws: %f, We: %f, dW: %f\n", spec[0].Ws, spec[0].We, spec[0].dW);
  fprintf (stderr, "Nspec: %d\n", spec[0].Nspec);

  obstime = ohana_sec_to_date (spec[0].obstime);
  regtime = ohana_sec_to_date (spec[0].regtime);

  fprintf (stderr, "obstime: %s\n", obstime);
  fprintf (stderr, "regtime: %s\n", regtime);

  exit (0);

}
