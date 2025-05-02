# include "data.h"

int peak (int argc, char **argv) {
  
  int i, N, imax, QUIET;
  double start, end, xmax, ymax;
  Vector *vecx, *vecy;

  QUIET = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    QUIET = TRUE;
    remove_argument (N, &argc, argv);
  }

  if ((argc != 5) && (argc != 3)) {
    gprint (GP_ERR, "USAGE: peak <x> <y> [start end]\n");
    return (FALSE);
  }

  if ((vecx = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecy = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);

  if (argc == 5) {
    start = atof (argv[3]);
    end   = atof (argv[4]);
  } else {
    start = (vecx[0].type == OPIHI_FLT) ? vecx[0].elements.Flt[0] : vecx[0].elements.Int[0];
    end   = (vecx[0].type == OPIHI_FLT) ? vecx[0].elements.Flt[vecx[0].Nelements - 1] : vecx[0].elements.Int[vecx[0].Nelements - 1];
  }

  imax = -1;
  xmax = -HUGE_VAL;
  ymax = -HUGE_VAL;

  if ((vecx[0].type == OPIHI_FLT) && (vecy[0].type == OPIHI_FLT)) {
    opihi_flt *X = vecx[0].elements.Flt;
    opihi_flt *Y = vecy[0].elements.Flt;
    for (i = 0; i < vecx[0].Nelements; i++, X++, Y++) {
      if (*X < start) continue;
      if (*X > end) continue;
      if (!isfinite(*Y)) continue;
      if (*Y < ymax) continue;
      xmax = *X;
      ymax = *Y;
      imax = i;
    }      
  }
  if ((vecx[0].type == OPIHI_FLT) && (vecy[0].type == OPIHI_INT)) {
    opihi_flt *X = vecx[0].elements.Flt;
    opihi_int *Y = vecy[0].elements.Int;
    for (i = 0; i < vecx[0].Nelements; i++, X++, Y++) {
      if (*X < start) continue;
      if (*X > end) continue;
      if (*Y < ymax) continue;
      xmax = *X;
      ymax = *Y;
      imax = i;
    }      
  }
  if ((vecx[0].type == OPIHI_INT) && (vecy[0].type == OPIHI_FLT)) {
    opihi_int *X = vecx[0].elements.Int;
    opihi_flt *Y = vecy[0].elements.Flt;
    for (i = 0; i < vecx[0].Nelements; i++, X++, Y++) {
      if (*X < start) continue;
      if (*X > end) continue;
      if (!isfinite(*Y)) continue;
      if (*Y < ymax) continue;
      xmax = *X;
      ymax = *Y;
      imax = i;
    }      
  }
  if ((vecx[0].type == OPIHI_INT) && (vecy[0].type == OPIHI_INT)) {
    opihi_int *X = vecx[0].elements.Int;
    opihi_int *Y = vecy[0].elements.Int;
    for (i = 0; i < vecx[0].Nelements; i++, X++, Y++) {
      if (*X < start) continue;
      if (*X > end) continue;
      if (*Y < ymax) continue;
      xmax = *X;
      ymax = *Y;
      imax = i;
    }      
  }

  set_variable ("peakval", ymax);
  set_variable ("peakpos", xmax);
  set_variable ("peaknum", imax);

  if (!QUIET) gprint (GP_LOG, "peak %f @ %f (%d)\n", ymax, xmax, imax);

  return (TRUE);
}
