# include "astro.h"

int region (int argc, char **argv) {
  
  double Ra, Dec;
  float dx, dy;
  int N, kapa, NoClear, dXpix, dYpix;
  char *name;
  Graphdata graphmode;

  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetGraph (&graphmode, &kapa, name)) return (FALSE);
  FREE (name);

  int channel = 0;
  if ((N = get_argument (argc, argv, "-ch"))) {
    channel = GetKapaChannelFromString (argv[N]);
    if (!channel) return FALSE;
    KiiSetChannel (kapa, channel - 1);
  }

  NoClear = FALSE;
  if ((N = get_argument (argc, argv, "-no-clear"))) {
    remove_argument (N, &argc, argv);
    NoClear = TRUE;
  }

  if ((N = get_argument (argc, argv, "-image"))) {
    remove_argument (N, &argc, argv);
    KapaGetImageCoords (kapa, &graphmode.coords);
    KapaGetImageRange (kapa, &graphmode.xmin, &graphmode.xmax, &graphmode.ymax, &graphmode.ymin, &dXpix, &dYpix);

    set_variable ("XMIN", graphmode.xmin);
    set_variable ("XMAX", graphmode.xmax);
    set_variable ("YMIN", graphmode.ymin);
    set_variable ("YMAX", graphmode.ymax);

    set_variable ("RMIN", Ra  + graphmode.xmin);
    set_variable ("RMAX", Ra  + graphmode.xmax);
    set_variable ("DMIN", Dec + graphmode.ymin);
    set_variable ("DMAX", Dec + graphmode.ymax);

    // if (!NoClear) KapaClearSections (kapa);
    KapaSetLimits (kapa, &graphmode);

    SetGraph (&graphmode);
    return (TRUE);
    // Set Region based on image
  }

  float XSIZE = NAN;
  if ((N = get_argument (argc, argv, "-xsize"))) {
    remove_argument (N, &argc, argv);
    XSIZE = atof (argv[N]);
    remove_argument (N, &argc, argv);
    if (XSIZE <= 0) {
      gprint (GP_ERR, "ERROR: xsize <= 0\n");
      return FALSE;
    }
  }

  float YSIZE = NAN;
  if ((N = get_argument (argc, argv, "-ysize"))) {
    remove_argument (N, &argc, argv);
    YSIZE = atof (argv[N]);
    remove_argument (N, &argc, argv);
    if (YSIZE <= 0) {
      gprint (GP_ERR, "ERROR: xsize <= 0\n");
      return FALSE;
    }
  }

  if ((N = get_argument (argc, argv, "-ew"))) {
    remove_argument (N, &argc, argv);
    graphmode.flipeast = TRUE;
  }

  if ((N = get_argument (argc, argv, "+ew"))) {
    remove_argument (N, &argc, argv);
    graphmode.flipeast = FALSE;
  }

  if ((N = get_argument (argc, argv, "-ns"))) {
    remove_argument (N, &argc, argv);
    graphmode.flipnorth = TRUE;
  }

  if ((N = get_argument (argc, argv, "+ns"))) {
    remove_argument (N, &argc, argv);
    graphmode.flipnorth = FALSE;
  }

  float Angle = 0.0;
  if ((N = get_argument (argc, argv, "-angle"))) {
    remove_argument (N, &argc, argv);
    Angle = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  int RadiusArg, CtypeArg;
  if (!isnan(XSIZE) || !isnan(YSIZE)) {
    RadiusArg = -1;
    CtypeArg = 3;
    if ((argc != 3) && (argc != 4)) {
    gprint (GP_ERR, "USAGE: region Ra Dec [projection] [-xsize deg] [-ysize deg]\n");
    gprint (GP_ERR, "   OR: region Ra Dec Radius [projection]\n");
    gprint (GP_ERR, "  [-image] [-ew] [+ew] [-ns] [+ns] [-no-clear] [-angle theta]\n");
    gprint (GP_ERR, " current: %f %f (%f x %f) (%s)\n", 
	     graphmode.coords.crval1, graphmode.coords.crval2, 
	     fabs(graphmode.xmax - graphmode.xmin), 
	     fabs(graphmode.ymax - graphmode.ymin), 
	     &graphmode.coords.ctype[5]);
    return (FALSE);
    }
  } else {
    RadiusArg = 3;
    CtypeArg = 4;
    if ((argc != 4) && (argc != 5)) {
      gprint (GP_ERR, "USAGE: region Ra Dec Radius [projection]\n");
      gprint (GP_ERR, "   OR: region Ra Dec [projection] [-xsize deg] [-ysize deg]\n");
      gprint (GP_ERR, "  [-image] [-ew] [+ew] [-ns] [+ns] [-no-clear] [-angle theta]\n");
      gprint (GP_ERR, " current: %f %f (%f x %f) (%s)\n", 
	      graphmode.coords.crval1, graphmode.coords.crval2, 
	      fabs(graphmode.xmax - graphmode.xmin), 
	      fabs(graphmode.ymax - graphmode.ymin), 
	      &graphmode.coords.ctype[5]);
      return (FALSE);
    }
  }  
  if (!ohana_str_to_radec (&Ra, &Dec, argv[1], argv[2])) return (FALSE);

  // region 0 0 sin -- should raise an error (radius = 0 or non-numeric)

  // I want to be able to support the old style call in which both of these were valid:
  // region 0 0 90 ait
  // region 0 0 90      <- uses existing, sticky projection type
  // but I also want to be able to use
  // region 0 0 ait -xsize 5
  // region 0 0 ait -ysize 5

  double Radius = NAN;
  if (RadiusArg >= 0) {
    Radius = atof (argv[RadiusArg]);
  }

  InitCoords (&graphmode.coords, "DEC--TAN");
  if (argc == CtypeArg + 1) {
    if (!strcasecmp (argv[CtypeArg], "TAN")) { strcpy (graphmode.coords.ctype, "DEC--TAN"); goto got_ctype; }
    if (!strcasecmp (argv[CtypeArg], "SIN")) { strcpy (graphmode.coords.ctype, "DEC--SIN"); goto got_ctype; }
    if (!strcasecmp (argv[CtypeArg], "ARC")) { strcpy (graphmode.coords.ctype, "DEC--ARC"); goto got_ctype; }
    if (!strcasecmp (argv[CtypeArg], "STG")) { strcpy (graphmode.coords.ctype, "DEC--STG"); goto got_ctype; }
    if (!strcasecmp (argv[CtypeArg], "ZEA")) { strcpy (graphmode.coords.ctype, "DEC--ZEA"); goto got_ctype; }
    if (!strcasecmp (argv[CtypeArg], "AIT")) { strcpy (graphmode.coords.ctype, "DEC--AIT"); goto got_ctype; }
    if (!strcasecmp (argv[CtypeArg], "GLS")) { strcpy (graphmode.coords.ctype, "DEC--GLS"); goto got_ctype; }
    if (!strcasecmp (argv[CtypeArg], "PAR")) { strcpy (graphmode.coords.ctype, "DEC--PAR"); goto got_ctype; }
    if (!strcasecmp (argv[CtypeArg], "MOL")) { strcpy (graphmode.coords.ctype, "DEC--MOL"); goto got_ctype; }
    gprint (GP_ERR, "ERROR: invalid projection type %s\n", argv[CtypeArg]);
    gprint (GP_ERR, "allowed values: TAN, SIN, ARC, STG, ZEA, AIT, GLS, PAR, MOL\n");
    return FALSE;
  }
got_ctype:
  
  // set the rotation
  {
    float pc1_1 = (graphmode.flipeast)  ? -1 : 1;
    float pc2_2 = (graphmode.flipnorth) ? -1 : 1;

    graphmode.coords.pc1_1 =  cos(Angle*RAD_DEG)*pc1_1;
    graphmode.coords.pc1_2 =  sin(Angle*RAD_DEG)*pc2_2;
    graphmode.coords.pc2_1 = -sin(Angle*RAD_DEG)*pc1_1;
    graphmode.coords.pc2_2 =  cos(Angle*RAD_DEG)*pc2_2;
  }

  // determine central pixel for projection:
  {
    // PSEUDOCYL modes need center R,D to be 0,0 and center pixel to be adjusted
    OhanaProjection proj = GetProjection (graphmode.coords.ctype);
    OhanaProjectionMode mode = GetProjectionMode (proj);

    graphmode.coords.crpix1 = 0.0;
    graphmode.coords.crpix2 = 0.0;

    if (mode == PROJ_MODE_PSEUDOCYL) {
      graphmode.coords.crval1 = Ra;
      graphmode.coords.crval2 = 0.0;
    } else {
      graphmode.coords.crval1 = Ra;
      graphmode.coords.crval2 = Dec;
    }

    if (mode == PROJ_MODE_PSEUDOCYL) {
      double Xc, Yc;
      RD_to_XY (&Xc, &Yc, Ra, Dec, &graphmode.coords);
      graphmode.coords.crpix2 = -Yc;
      fprintf (stderr, "center pixel is %f, %f\n", Xc, Yc);
    }
  }

  // ask kapa for coordinate limits, to get the right aspect ratio 
  // dx, dy are the size of the graph region in pixels
  KapaGetLimits (kapa, &dx, &dy);
  dx = fabs (dx);
  dy = fabs (dy); 

  /* define limits for Ra, Dec at center, grid in degrees */
  if (RadiusArg >= 0) {
    // force non-anamorphic projection with Radius set to smaller axis
    if (dy < dx) {
      graphmode.xmin = -(dx/dy)*Radius;
      graphmode.ymin = -Radius;
      graphmode.xmax = (dx/dy)*Radius;
      graphmode.ymax = Radius;
    } else {
      graphmode.xmin = -Radius;
      graphmode.ymin = -(dy/dx)*Radius;
      graphmode.xmax = Radius;
      graphmode.ymax = (dy/dx)*Radius;
    } 
  } else {
    if (isnan(XSIZE)) {
      graphmode.xmin = -(dx/dy)*YSIZE/2.0;
      graphmode.ymin = -YSIZE/2.0;
      graphmode.xmax = (dx/dy)*YSIZE/2.0;
      graphmode.ymax = YSIZE/2.0;
    }
    if (isnan(YSIZE)) {
      graphmode.xmin = -XSIZE/2.0;
      graphmode.ymin = -(dy/dx)*XSIZE/2.0;
      graphmode.xmax = XSIZE/2.0;
      graphmode.ymax = (dy/dx)*XSIZE/2.0;
    }
    // anamorphic projection:
    if (!isnan(XSIZE) && !isnan(YSIZE)) {
      graphmode.xmin = -XSIZE/2.0;
      graphmode.ymin = -YSIZE/2.0;
      graphmode.xmax =  XSIZE/2.0;
      graphmode.ymax =  YSIZE/2.0;
    }
  }

  set_variable ("XMIN", graphmode.xmin);
  set_variable ("XMAX", graphmode.xmax);
  set_variable ("YMIN", graphmode.ymin);
  set_variable ("YMAX", graphmode.ymax);

  set_variable ("RMIN", Ra  + graphmode.xmin);
  set_variable ("RMAX", Ra  + graphmode.xmax);
  set_variable ("DMIN", Dec + graphmode.ymin);
  set_variable ("DMAX", Dec + graphmode.ymax);

  set_int_variable ("EAST_RIGHT", !graphmode.flipeast);
  set_int_variable ("NORTH_UP", !graphmode.flipnorth);

  if (!NoClear) KapaClearSections (kapa);
  KapaSetLimits (kapa, &graphmode);

  /* drop this? */
  // sprintf (string, "%8.4f %8.4f (%f)", Ra, Dec, Radius);
  // KapaSendLabel (kapa, string, 2);

  // XXX is this the right thing to be doing?
  SetGraph (&graphmode);
  return (TRUE);
}


