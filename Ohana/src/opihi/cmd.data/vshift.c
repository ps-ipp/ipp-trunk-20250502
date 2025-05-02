# include "data.h"

int vshift (int argc, char **argv) {

  int isPos, N, Npix, Nshift, delta, ROLL;
  Vector *ivec, *ovec;

  ROLL = FALSE;
  if ((N = get_argument (argc, argv, "-roll"))) {
    remove_argument (N, &argc, argv);
    ROLL = TRUE;
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: vshift (input) (output) (delta) [-roll]\n");
    gprint (GP_ERR, "  shift vector by (delta) elements\n");
    gprint (GP_ERR, "  a positive value move element (i) to (i+delta)\n");
    gprint (GP_ERR, "  -roll : move dropped values to the other size (no elements are lost)\n");
    return (FALSE);
  }

  if ((ivec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((ovec = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) return (FALSE);
  delta = atoi(argv[3]);
  isPos = delta > 0;
  delta = abs(delta);
    
  ResetVector (ovec, ivec->type, ivec->Nelements);

  Npix = ivec[0].Nelements;
  if (delta > Npix) {
    if (!ROLL) {
      if (ivec[0].type == OPIHI_FLT) {
	memset (ovec[0].elements.Flt, 0, Npix*sizeof(opihi_flt));
      } else {
	memset (ovec[0].elements.Flt, 0, Npix*sizeof(opihi_flt));
      }
      return TRUE;
    }
    delta = delta % Npix;
  }

  if (ivec[0].type == OPIHI_FLT) {
    memset (ovec[0].elements.Flt, 0, Npix*sizeof(opihi_flt));
    Nshift = Npix - delta;
    if (isPos) {
      memcpy (&ovec[0].elements.Flt[delta], &ivec[0].elements.Flt[0], Nshift*sizeof(opihi_flt));
      if (ROLL) {
	memcpy (&ovec[0].elements.Flt[0], &ivec[0].elements.Flt[Nshift], delta*sizeof(opihi_flt));
      } 
    } else {
      memcpy (&ovec[0].elements.Flt[0], &ivec[0].elements.Flt[delta], Nshift*sizeof(opihi_flt));
      if (ROLL) {
	memcpy (&ovec[0].elements.Flt[Nshift], &ivec[0].elements.Flt[0], delta*sizeof(opihi_flt));
      }
    }
  } else {
    memset (ovec[0].elements.Int, 0, Npix*sizeof(opihi_int));
    Nshift = Npix - delta;
    if (isPos) {
      memcpy (&ovec[0].elements.Int[delta], &ivec[0].elements.Int[0], Nshift*sizeof(opihi_int));
      if (ROLL) {
	memcpy (&ovec[0].elements.Int[0], &ivec[0].elements.Int[Nshift], delta*sizeof(opihi_int));
      } 
    } else {
      memcpy (&ovec[0].elements.Int[0], &ivec[0].elements.Int[delta], Nshift*sizeof(opihi_int));
      if (ROLL) {
	memcpy (&ovec[0].elements.Int[Nshift], &ivec[0].elements.Int[0], delta*sizeof(opihi_int));
      }
    }
  }
  return (TRUE);
}
