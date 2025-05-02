# include "data.h"

int vroll (int argc, char **argv) {

  int Npix;
  opihi_flt first;
  Vector *vec;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: vroll (vector)\n");
    gprint (GP_ERR, "  roll vector elements (first goes to end)\n");
    return (FALSE);
  }

  if ((vec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  Npix = vec[0].Nelements;
  if (Npix < 2) return (TRUE);

  if (vec[0].type == OPIHI_FLT) {
    first = vec[0].elements.Flt[0];
    memmove (&vec[0].elements.Flt[0], &vec[0].elements.Flt[1], Npix*sizeof(opihi_flt));
    vec[0].elements.Flt[Npix-1] = first;
  } else {
    first = vec[0].elements.Int[0];
    memmove (&vec[0].elements.Int[0], &vec[0].elements.Int[1], Npix*sizeof(opihi_int));
    vec[0].elements.Int[Npix-1] = first;
  }
  return (TRUE);
}
