# include "data.h"

int limits (int argc, char **argv) {

  int N, dX, dY;
  int kapa;
  Graphdata graphmode;
  Vector *xvec, *yvec;

  xvec = yvec = NULL;

  float minLimitX = NAN;
  float minLimitY = NAN;
  float maxLimitX = NAN;
  float maxLimitY = NAN;
  float delLimitX = NAN;
  float delLimitY = NAN;

  if ((N = get_argument (argc, argv, "-minX"))) {
    remove_argument (N, &argc, argv);
    minLimitX = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-maxX"))) {
    remove_argument (N, &argc, argv);
    maxLimitX = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-delX"))) {
    if (!isnan(minLimitX) || !isnan(maxLimitX)) {
      gprint (GP_ERR, "-minX & -maxX cannot be mixed with -delX\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    delLimitX = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-minY"))) {
    remove_argument (N, &argc, argv);
    minLimitY = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-maxY"))) {
    remove_argument (N, &argc, argv);
    maxLimitY = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-delY"))) {
    if (!isnan(minLimitY) || !isnan(maxLimitY)) {
      gprint (GP_ERR, "-minY & -maxY cannot be mixed with -delY\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    delLimitY = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int APPLY = FALSE;
  if ((N = get_argument (argc, argv, "-a"))) {
    remove_argument (N, &argc, argv);
    APPLY = TRUE;
  }

  char *name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetGraph (&graphmode, &kapa, name)) return (FALSE);
  FREE (name);

  // this is not super intuitive
  if ((N = get_argument (argc, argv, "-boxsize"))) {
    remove_argument (N, &argc, argv);
    float dx, dy;
    // ask kapa for the size of the graph region in pixels
    KapaGetLimits (kapa, &dx, &dy);
    set_variable ("KAPA_XPIX", fabs(dx));
    set_variable ("KAPA_YPIX", fabs(dy));
  }

  // XXX need an option to set the limits based on the current image bounds
  if ((N = get_argument (argc, argv, "-image"))) {
    remove_argument (N, &argc, argv);
    KapaGetImageRange (kapa, &graphmode.xmin, &graphmode.xmax, &graphmode.ymax, &graphmode.ymin, &dX, &dY);

    set_variable ("XMIN", graphmode.xmin);
    set_variable ("XMAX", graphmode.xmax);
    set_variable ("YMIN", graphmode.ymin);
    set_variable ("YMAX", graphmode.ymax);

    set_variable ("KAPA_XMIN", graphmode.xmin);
    set_variable ("KAPA_XMAX", graphmode.xmax);
    set_variable ("KAPA_YMIN", graphmode.ymin);
    set_variable ("KAPA_YMAX", graphmode.ymax);

    set_variable ("KAPA_XPIX", dX);
    set_variable ("KAPA_YPIX", dY);

    // if (!NoClear) KapaClearSections (kapa);
    SetGraph (&graphmode);
    KapaSetLimits (kapa, &graphmode);
    return (TRUE);
    // Set Region based on image
  }

  if (argc == 1) {
    gprint (GP_ERR, "limits: %f %f %f %f [-a] [-n device]\n",
	     graphmode.xmin, graphmode.xmax,
	     graphmode.ymin, graphmode.ymax);
    goto success;
  }

  if (argc == 3) { /* expect to see: limits x y */
    if ((xvec = SelectVector (argv[1], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    if ((yvec = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    goto success;
  }
    
  if (argc == 4) { /* expect to see: limits x num num or limits num num y */
    if (ISNUM(argv[1][0]) && ISNUM(argv[2][0])) {
      if ((yvec = SelectVector (argv[3], OLDVECTOR, FALSE)) == NULL) goto error;
      graphmode.xmin = atof (argv[1]);
      graphmode.xmax = atof (argv[2]);
      goto success;
    }
    if (ISNUM(argv[2][0]) && ISNUM(argv[3][0])) {
      if ((xvec = SelectVector (argv[1], OLDVECTOR, FALSE)) == NULL) goto error;
      graphmode.ymin = atof (argv[2]);
      graphmode.ymax = atof (argv[3]);
      goto success;
    }
    goto error;
  }
  
  if (argc == 5) {
    graphmode.xmin = atof (argv[1]);
    graphmode.xmax = atof (argv[2]);
    graphmode.ymin = atof (argv[3]);
    graphmode.ymax = atof (argv[4]);
    goto success;
  }

  gprint (GP_ERR, "USAGE: limits [xrange] [yrange]\n");
  gprint (GP_ERR, " [range] may be either [min max] or a vector\n");
  return (FALSE);

 error:
  gprint (GP_ERR, "error in vectors\n");
  return (FALSE);

 success:
  SetLimits (xvec, yvec, &graphmode);

  if (!isnan(minLimitX)) graphmode.xmin = MIN (minLimitX, graphmode.xmin);
  if (!isnan(maxLimitX)) graphmode.xmax = MAX (maxLimitX, graphmode.xmax);
  if (!isnan(minLimitY)) graphmode.ymin = MIN (minLimitY, graphmode.ymin);
  if (!isnan(maxLimitY)) graphmode.ymax = MAX (maxLimitY, graphmode.ymax);

  if (!isnan(delLimitX)) {
    float delta = graphmode.xmax - graphmode.xmin;
    if (fabs(delLimitX) > fabs(delta)) {
      float midpt = 0.5*(graphmode.xmax + graphmode.xmin);
      graphmode.xmax = midpt + 0.5*delLimitX;
      graphmode.xmin = midpt - 0.5*delLimitX;
    }
  }
  if (!isnan(delLimitY)) {
    float delta = graphmode.ymax - graphmode.ymin;
    if (fabs(delLimitY) > fabs(delta)) {
      float midpt = 0.5*(graphmode.ymax + graphmode.ymin);
      graphmode.ymax = midpt + 0.5*delLimitY;
      graphmode.ymin = midpt - 0.5*delLimitY;
    }
  }

  if (APPLY) KapaSetLimits (kapa, &graphmode);
  return (TRUE);
}

/* -minX value : the minimum X axis value will be no higher than this value
   -maxX value : the maximum X axis value will be no lower than this value
   -delX value : the range of the X axis will be at least this value

   These can be used to prevent the range from collapsing.  
   These are only used if the -a option is supplied, otherwise the supplied or auto-calculated 
   ranges are used (this seems like an poor choice)

*/


   
