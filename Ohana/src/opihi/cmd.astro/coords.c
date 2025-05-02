# include "astro.h"

enum {NONE, SKY, PIXEL, VECTOR, SCALAR};

int coords (int argc, char **argv) {

  int i, mode, form, N, Quiet;
  double Xin, Yin, Xout, Yout;
  Coords coords, moscoords;
  Buffer *buf, *mosbuffer;
  Vector *xvec = NULL;
  Vector *yvec = NULL;
  char *MOSAIC = NULL;

  if ((N = get_argument (argc, argv, "-copy"))) {
    Buffer *src = NULL;

    remove_argument (N, &argc, argv);
    if ((src = SelectBuffer (argv[N], OLDBUFFER, TRUE)) == NULL) goto escape;

    if (!GetCoords (&coords, &src[0].header)) {
      gprint (GP_ERR, "error getting WCS elements from src buffer %s\n", argv[N]);
      return (FALSE);
    }

    remove_argument (N, &argc, argv);

    if (argc != 2) goto syntax;

    if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) goto escape;
    if (!PutCoords (&coords, &buf[0].header)) {
      gprint (GP_ERR, "error getting WCS elements from src buffer %s\n", argv[1]);
      return (FALSE);
    }
    return TRUE;
  }

  if ((N = get_argument (argc, argv, "-mosaic"))) {
    remove_argument (N, &argc, argv);
    MOSAIC = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  Quiet = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }

  form = NONE;
  mode = NONE;
  if ((N = get_argument (argc, argv, "-p"))) {
    remove_argument (N, &argc, argv);
    mode = SKY;
  }
  if ((N = get_argument (argc, argv, "-c"))) {
    if (mode == SKY) goto syntax;
    remove_argument (N, &argc, argv);
    mode = PIXEL;
  }
  if (mode == NONE) goto syntax;
  if (argc != 4) goto syntax;

  if (SelectScalar (argv[2], &Xin)) {
    if (!SelectScalar (argv[3], &Yin)) {
      gprint (GP_ERR, "syntax error: mixed vector and scalar?\n");
      return (FALSE);
    }
    form = SCALAR;
  } else {
    if ((xvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    if ((yvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    if (xvec[0].Nelements != yvec[0].Nelements) {
      fprintf (stderr, "mis-matched vector lengths\n");
      return (FALSE);
    }
    REQUIRE_VECTOR_FLT (xvec, FALSE); 
    REQUIRE_VECTOR_FLT (yvec, FALSE); 
    form = VECTOR;
  }      

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) goto escape;
  GetCoords (&coords, &buf[0].header);
  if (!strcmp(&coords.ctype[4], "-WRP")) {
    if (MOSAIC == NULL) {
      gprint (GP_ERR, "must supply mosaic for WRP coords\n");
      return (FALSE);
    }
    if ((mosbuffer = SelectBuffer (MOSAIC, OLDBUFFER, TRUE)) == NULL) goto escape;
    GetCoords (&moscoords, &mosbuffer[0].header);
    coords.mosaic = &moscoords;
  }
  
  if (form == SCALAR) {
    if (mode == SKY) {
      XY_to_RD (&Xout, &Yout, Xin, Yin, &coords);
      if (!Quiet) gprint (GP_LOG, "%10.6f %10.6f\n", Xout, Yout);
      set_variable ("RA", Xout);
      set_variable ("DEC", Yout);
      return (TRUE);
    }
    if (mode == PIXEL) {
      Xin = ohana_normalize_angle_to_midpoint (Xin, coords.crval1);
      RD_to_XY (&Xout, &Yout, Xin, Yin, &coords);
      if (!Quiet) gprint (GP_LOG, "%7.2f %7.2f\n", Xout, Yout);
      set_variable ("Xc", Xout);
      set_variable ("Yc", Yout);
      return (TRUE);
    }
  }
  if (mode == SKY) {
    for (i = 0; i < xvec[0].Nelements; i++) {
      double Xin = xvec[0].elements.Flt[i];
      double Yin = yvec[0].elements.Flt[i];
      xvec[0].elements.Flt[i] = NAN;
      yvec[0].elements.Flt[i] = NAN;
      XY_to_RD (&xvec[0].elements.Flt[i], &yvec[0].elements.Flt[i], Xin, Yin, &coords);
    }
    return (TRUE);
  }
  if (mode == PIXEL) {
    for (i = 0; i < xvec[0].Nelements; i++) {
      double Xin = xvec[0].elements.Flt[i];
      double Yin = yvec[0].elements.Flt[i];
      xvec[0].elements.Flt[i] = NAN;
      yvec[0].elements.Flt[i] = NAN;
      Xin = ohana_normalize_angle_to_midpoint (Xin, coords.crval1);
      RD_to_XY (&xvec[0].elements.Flt[i], &yvec[0].elements.Flt[i], Xin, Yin, &coords);
    }
    return (TRUE);
  }
  return (FALSE);

 syntax:
  gprint (GP_ERR, "USAGE: coords [buffer] (-c R D) | (-p X Y)\n");
  gprint (GP_ERR, "USAGE: coords [tgtbuffer] (-copy srcbuffer)\n");
  gprint (GP_ERR, "only one of -p or -c or -copy can be used\n");
  gprint (GP_ERR, " -p : from pixels to ra/dec\n");
  gprint (GP_ERR, " -c : from ra/dec to pixels\n");
  gprint (GP_ERR, " coordinates are in degrees\n");
  gprint (GP_ERR, " -copy : copy coordinate WCS keywords from srcbuffer to tgtbuffer\n");
 escape:
  if (MOSAIC != NULL) free (MOSAIC);
  return (FALSE);
}
