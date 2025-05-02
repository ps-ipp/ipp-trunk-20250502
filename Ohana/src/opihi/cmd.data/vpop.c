# include "data.h"

int vpop (int argc, char **argv) {

  int Npix;
  Vector *vec;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: vpop (vector)\n");
    gprint (GP_ERR, "  remove first element of vector\n");
    return (FALSE);
  }

  if ((vec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  Npix = vec[0].Nelements;
  if (Npix < 1) return (TRUE);

  if (Npix > 1) {
    if (vec[0].type == OPIHI_FLT) {
      memmove (&vec[0].elements.Flt[0], &vec[0].elements.Flt[1], Npix*sizeof(opihi_flt));
    } else {
      memmove (&vec[0].elements.Int[0], &vec[0].elements.Int[1], Npix*sizeof(opihi_int));
    }
  }
  vec[0].Nelements = Npix - 1;
  return (TRUE);
}
