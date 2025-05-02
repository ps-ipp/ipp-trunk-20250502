# include "astro.h"

# define GRID_SPACING 0.002

# define CHECKELEMENTS						\
  if (N == NELEMENTS) {						\
    NELEMENTS +=200;						\
    REALLOCATE (Xvec.elements.Flt, opihi_flt, NELEMENTS);	\
    REALLOCATE (Yvec.elements.Flt, opihi_flt, NELEMENTS);	\
  }

# define ADD_COORDINATE(RA,DEC)						\
  status = RD_to_XY (&Xvec.elements.Flt[N], &Yvec.elements.Flt[N], (RA), (DEC), &graphmode.coords); \
  if ((Xvec.elements.Flt[N] >= graphmode.xmin) && (Xvec.elements.Flt[N] <= graphmode.xmax) && \
      (Yvec.elements.Flt[N] >= graphmode.ymin) && (Yvec.elements.Flt[N] <= graphmode.ymax) && status) { \
    N++;								\
    CHECKELEMENTS;							\
    OnPic = TRUE;							\
    if (!First) {							\
      Xvec.elements.Flt[N] = Xvec.elements.Flt[N-1];			\
      Yvec.elements.Flt[N] = Yvec.elements.Flt[N-1];			\
      N++;								\
      CHECKELEMENTS;							\
    } else {								\
      if (N > 1) {							\
	Xvec.elements.Flt[N-2] = Xvec.elements.Flt[N-1];		\
	Yvec.elements.Flt[N-2] = Yvec.elements.Flt[N-1];		\
	N--;								\
      }									\
      First = FALSE;							\
    }									\
  } else {								\
    LOnPic = FALSE;							\
    First = TRUE;							\
  }

# define ADD_DEC_LINE(RA)						\
  /* first, DEC increasing */						\
  LOnPic = TRUE;							\
  OnPic = FALSE;							\
  First = TRUE;								\
  for (d = firstDEC; (d < 90 + dD) && (LOnPic || NorthPole || SouthPole || InvalidCorner); d += dD) { \
    D = MAX (-90, MIN(90, d));						\
    ADD_COORDINATE((RA), D);						\
  }									\
  /* next, DEC decreasing */						\
  First = TRUE;								\
  LOnPic = TRUE;							\
  for (d = firstDEC; (d > -90 - dD) && (LOnPic || NorthPole || SouthPole || InvalidCorner); d -= dD) { \
    D = MAX (-90, MIN(90, d));						\
    ADD_COORDINATE((RA), D);						\
  } 

# define ADD_RA_LINE(DEC)						\
  D = MAX (-90, MIN(90, (DEC)));					\
  /* first, RA increasing */						\
  LOnPic = TRUE;							\
  OnPic = FALSE;							\
  First = TRUE;								\
  lastRA = graphmode.coords.crval1 + 180.0;				\
  for (r = firstRA; (r < lastRA + dR) && (LOnPic || NorthPole || SouthPole || InvalidCorner); r += dR) { \
    R = MIN (r, lastRA);					\
    ADD_COORDINATE(R, D);						\
  }									\
  /* next, RA decreasing */						\
  First = TRUE;								\
  LOnPic = TRUE;							\
  lastRA = graphmode.coords.crval1 - 180.0;				\
  for (r = firstRA; (r > lastRA - dR) && (LOnPic || NorthPole || SouthPole || InvalidCorner); r -= dR) { \
    R = MAX (r, lastRA);					\
    ADD_COORDINATE(R, D);						\
  }

int cgrid (int argc, char **argv) {
  
  double range, minor, major;
  double firstRA, lastRA, firstDEC, minorRA, minorDEC;
  double r, d, dR, dD, R, D;
  double x, y;
  Vector Xvec, Yvec;
  int kapa, NorthPole, SouthPole, N, OnPic, LOnPic, status, NELEMENTS;
  int First, RAbyHour, Labels;
  Graphdata graphmode;

  if ((N = get_argument (argc, argv, "-h"))) goto usage;
  if ((N = get_argument (argc, argv, "--help"))) goto usage;

  RAbyHour = FALSE;
  if ((N = get_argument (argc, argv, "-ra-by-hour"))) {
    remove_argument (N, &argc, argv);
    RAbyHour = TRUE;
  }

  Labels = FALSE;
  if ((N = get_argument (argc, argv, "-labels"))) {
    remove_argument (N, &argc, argv);
    Labels = TRUE;
  }

  int MinorSpacing = TRUE;
  if ((N = get_argument (argc, argv, "-major-spacing"))) {
    remove_argument (N, &argc, argv);
    MinorSpacing = FALSE;
  }

  minorRA = NAN;
  if ((N = get_argument (argc, argv, "-ra-spacing"))) {
    remove_argument (N, &argc, argv);
    minorRA = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  minorDEC = NAN;
  if ((N = get_argument (argc, argv, "-dec-spacing"))) {
    remove_argument (N, &argc, argv);
    minorDEC = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  int JustifyRA = 8;
  int JustifyDEC = 2;
  if ((N = get_argument (argc, argv, "-justify-ra"))) {
    remove_argument (N, &argc, argv);
    JustifyRA = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-justify-dec"))) {
    remove_argument (N, &argc, argv);
    JustifyDEC = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  double LabelRA = NAN;
  double LabelDEC = NAN;
  if ((N = get_argument (argc, argv, "-label-ra"))) {
    remove_argument (N, &argc, argv);
    LabelRA = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-label-dec"))) {
    remove_argument (N, &argc, argv);
    LabelDEC = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  int LabelColor = KapaColorByName ("black");
  if ((N = get_argument (argc, argv, "-label-color"))) {
    remove_argument (N, &argc, argv);
    LabelColor = KapaColorByName (argv[N]);
    if (LabelColor == -1) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  if (argc != 1) goto usage;

  /* are we plotting one of the poles? */
  NorthPole = SouthPole = FALSE;
  status = RD_to_XY (&x, &y, 0.0, 90.0, &graphmode.coords);
  if ((x >= graphmode.xmin) && (x <= graphmode.xmax) && 
      (y >= graphmode.ymin) && (y <= graphmode.ymax) && status)
    NorthPole = TRUE;
  status = RD_to_XY (&x, &y, 0.0, -90.0, &graphmode.coords);
  if ((x >= graphmode.xmin) && (x <= graphmode.xmax) && 
      (y >= graphmode.ymin) && (y <= graphmode.ymax) && status)
    SouthPole = TRUE;

  // PSEUDOCYL modes need center R,D to be 0,0 and center pixel to be adjusted
  OhanaProjection proj = GetProjection (graphmode.coords.ctype);
  OhanaProjectionMode mode = GetProjectionMode (proj);
  int InvalidCorner = FALSE;
  if (mode == PROJ_MODE_PSEUDOCYL) {
    // for PSEUDOCYL, the ra range is 360 if the corners are invalid 
    InvalidCorner |= XY_to_RD (&r, &d, graphmode.xmin, graphmode.ymin, &graphmode.coords); 
    InvalidCorner |= XY_to_RD (&r, &d, graphmode.xmax, graphmode.ymin, &graphmode.coords); 
    InvalidCorner |= XY_to_RD (&r, &d, graphmode.xmax, graphmode.ymax, &graphmode.coords); 
    InvalidCorner |= XY_to_RD (&r, &d, graphmode.xmin, graphmode.ymax, &graphmode.coords); 
  }    

  range = MIN (fabs(graphmode.coords.cdelt1*(graphmode.xmax-graphmode.xmin)), fabs(graphmode.coords.cdelt2*(graphmode.ymax-graphmode.ymin)));
  if (NorthPole || SouthPole || InvalidCorner) range = 360;
  dR = range * GRID_SPACING;
  dD = range * GRID_SPACING;

  /* set spacings for RA */
  if (isnan(minorRA)) {
    if (RAbyHour) {
      SetGridScales (&major, &minor, range / 15.0);
      minorRA = MinorSpacing ? minor * 15.0 : major * 15.0;
    } else {
      SetGridScales (&major, &minor, range);
      minorRA = MinorSpacing ? minor : major;
    }
  }

  
  /* set spacings for DEC */
  if (isnan(minorDEC)) {
    SetGridScales (&major, &minor, range);
    minorDEC = MinorSpacing ? minor : major;
  }

  /* choose a starting point */
  if ((int)(graphmode.coords.crval1/minorRA) == (graphmode.coords.crval1/minorRA)) {
    firstRA = graphmode.coords.crval1;
  } else {
    firstRA = minorRA + minorRA*((int)(graphmode.coords.crval1/minorRA));
  }
  if ((int)(graphmode.coords.crval2/minorDEC) == (graphmode.coords.crval2/minorDEC)) {
    firstDEC = graphmode.coords.crval2;
  } else {
    firstDEC = minorDEC + minorDEC*((int)(graphmode.coords.crval2/minorDEC));
  }
  if (SouthPole) firstDEC = -90;
  if (NorthPole) firstDEC = 90;
  
  /* prepare vectors to hold data */
  N = 0;
  NELEMENTS = 200;
  SetVector (&Xvec, OPIHI_FLT, NELEMENTS);
  SetVector (&Yvec, OPIHI_FLT, NELEMENTS);
  
  { // sanity check
    float Nelem;
    Nelem = 180.0 / minorRA;
    if (!isfinite(Nelem) || (fabs(Nelem) > 10000)) { fprintf (stderr, "absurd cgrid spacing %f\n", minorRA); return FALSE; }
    Nelem = 180.0 / minorDEC;
    if (!isfinite(Nelem) || (fabs(Nelem) > 10000)) { fprintf (stderr, "absurd cgrid spacing %f\n", minorDEC); return FALSE; }
  }

  /***  do consecutive RA lines, first increasing **/
  OnPic = TRUE;
  lastRA = graphmode.coords.crval1 + 180.0;
  for (r = firstRA; (r <= lastRA) && (OnPic); r += minorRA) {
    ADD_DEC_LINE (r);
  }
  if (r != lastRA) {
    ADD_DEC_LINE (lastRA);
  }

  /***  do consecutive RA lines, decreasing **/
  OnPic = TRUE;
  lastRA = graphmode.coords.crval1 - 180.0;
  for (r = firstRA; (r >=  lastRA) && (OnPic); r -= minorRA) {
    ADD_DEC_LINE (r);
  }
  if (r != lastRA) {
    ADD_DEC_LINE (lastRA);
  }

  /***  do consecutive DEC lines, first increasing **/
  OnPic = TRUE;
  for (d = firstDEC; (d < 90 + dD) && (OnPic); d += minorDEC) {
    ADD_RA_LINE (d);
  }

  /***  do consecutive DEC lines, decreasing **/
  OnPic = TRUE;
  for (d = firstDEC; (d > -90 - dD) && (OnPic); d -= minorDEC) {
    ADD_RA_LINE (d);
  }
  
  // add labels for center lines:
  if (Labels) { 
    char line[16], format[16];
    double xt, yt, frac;
    // dx = +0.01 * (graphmode.xmax - graphmode.xmin);
    // dy = -0.02 * (graphmode.ymax - graphmode.ymin);

    if (isnan(LabelRA)) LabelRA = graphmode.coords.crval1;
    if (isnan(LabelDEC)) LabelDEC = graphmode.coords.crval2;
    for (r = firstRA; r <= graphmode.coords.crval1 + 180.0; r += minorRA) {
      status = RD_to_XY (&xt, &yt, r, LabelDEC, &graphmode.coords);
      if (!status) continue;
      if (xt < graphmode.xmin) continue;
      if (xt > graphmode.xmax) continue;
      if (yt < graphmode.ymin) continue;
      if (yt > graphmode.ymax) continue;
      frac = -1.0 * log10(minorRA);
      if (frac != (int)frac) {
	frac += 1.0;
      }
      if (frac <= 0.0) frac = 0.0;
      if (RAbyHour) {
	snprintf (format, 16, "%%.%df^h", (int) frac);
	snprintf (line, 16, format, r / 15.0);
      } else {
	snprintf (format, 16, "%%.%df^o", (int) frac);
	snprintf (line, 16, format, r);
      }
      KapaSendTextline (kapa, line, xt, yt, 0.0, JustifyRA, LabelColor);
    }
    for (r = firstRA; r >= graphmode.coords.crval1 - 180.0; r -= minorRA) {
      status = RD_to_XY (&xt, &yt, r, LabelDEC, &graphmode.coords);
      if (!status) continue;
      if (xt < graphmode.xmin) continue;
      if (xt > graphmode.xmax) continue;
      if (yt < graphmode.ymin) continue;
      if (yt > graphmode.ymax) continue;
      frac = -1.0 * log10(minorRA);
      if (frac != (int)frac) {
	frac += 1.0;
      }
      if (frac <= 0.0) frac = 0.0;
      if (RAbyHour) {
	snprintf (format, 16, "%%.%df^h", (int) frac);
	snprintf (line, 16, format, r / 15.0);
      } else {
	snprintf (format, 16, "%%.%df^o", (int) frac);
	snprintf (line, 16, format, r);
      }
      KapaSendTextline (kapa, line, xt, yt, 0.0, JustifyRA, LabelColor);
    }
    for (d = firstDEC; d <= graphmode.coords.crval2 + 90.0; d += minorDEC) {
      status = RD_to_XY (&xt, &yt, LabelRA, d, &graphmode.coords);
      if (!status) continue;
      if (xt < graphmode.xmin) continue;
      if (xt > graphmode.xmax) continue;
      if (yt < graphmode.ymin) continue;
      if (yt > graphmode.ymax) continue;
      frac = -1.0 * log10(minorDEC);
      if (frac != (int)frac) {
	frac += 1.0;
      }
      if (frac <= 0.0) frac = 0.0;
      snprintf (format, 16, "%%.%df^o", (int) frac);
      snprintf (line, 16, format, d);
      KapaSendTextline (kapa, line, xt, yt, 0.0, JustifyDEC, LabelColor);
    }
    for (d = firstDEC; d >= graphmode.coords.crval2 - 90.0; d -= minorDEC) {
      status = RD_to_XY (&xt, &yt, LabelRA, d, &graphmode.coords);
      if (!status) continue;
      if (xt < graphmode.xmin) continue;
      if (xt > graphmode.xmax) continue;
      if (yt < graphmode.ymin) continue;
      if (yt > graphmode.ymax) continue;
      frac = -1.0 * log10(minorDEC);
      if (frac != (int)frac) {
	frac += 1.0;
      }
      if (frac <= 0.0) frac = 0.0;
      snprintf (format, 16, "%%.%df^o", (int) frac);
      snprintf (line, 16, format, d);
      KapaSendTextline (kapa, line, xt, yt, 0.0, JustifyDEC, LabelColor);
    }
  }

  /* send the line segments as connect-points */
  Xvec.Nelements = Yvec.Nelements = N;
  graphmode.style = KAPA_PLOT_POINTS; /* points */
  graphmode.ptype = KAPA_POINT_PAIR_CONNECT; /* connect pairs of points */
  graphmode.etype = 0;
  PlotVectorPair (kapa, &Xvec, &Yvec, NULL, &graphmode);

  free (Xvec.elements.Ptr);
  free (Yvec.elements.Ptr);
  return (TRUE);

 usage:

  gprint (GP_ERR, "USAGE: cgrid [style] [options]\n");
  gprint (GP_ERR, "  options:\n");
  gprint (GP_ERR, "  -h, --help: show this list\n");
  gprint (GP_ERR, "  -ra-by-hour : RA grid lines will be space on rounded hour lines (default is degrees)\n");
  gprint (GP_ERR, "  -labels : add RA & dec coordinates\n");
  gprint (GP_ERR, "  -major-spacing : grid lines drawn at major tickmarks, not minor tickmarks\n");
  gprint (GP_ERR, "  -ra-spacing : specify size of RA grid steps in degrees (ignores -ra-by-hour)\n");
  gprint (GP_ERR, "  -dec-spacing : specify size of dec grid steps in degrees\n");
  gprint (GP_ERR, "  -justify-ra : choose how RA labels are justified (see below)\n");
  gprint (GP_ERR, "  -justify-dec : choose how dec labels are justified (see below)\n");
  gprint (GP_ERR, "  -label-ra : RA coordinate of the Dec-line labels\n");
  gprint (GP_ERR, "  -label-dec : Dec coordinate of the RA-line labels\n");
  gprint (GP_ERR, "  -label-color : color for the labels (independent of grid lines)\n");
  gprint (GP_ERR, "  text justification: text is justified horizontally and vertically based on the following numbers:\n");
  gprint (GP_ERR, "   6 7 8\n");
  gprint (GP_ERR, "   3 4 5\n");
  gprint (GP_ERR, "   0 1 2\n");
  return (FALSE);
}

