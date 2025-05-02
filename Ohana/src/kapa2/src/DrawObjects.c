# include "Ximage.h"

/* DrawRectangle & FillRectangle : take rectangle lower-left corner and widths 
   DrawCircle & FillCircle : take circle center and radius
*/

# define DrawLine(X1,Y1,X2,Y2) (XDrawLine (graphic->display, graphic->window, graphic->gc, (int)(X1), (int)(Y1), (int)(X2), (int)(Y2)))
# define DrawRectangle(X,Y,dX,dY) (XDrawRectangle (graphic->display, graphic->window, graphic->gc, (int)(X), (int)(Y), (int)(dX), (int)(dY)))
# define FillRectangle(X,Y,dX,dY) (XFillRectangle (graphic->display, graphic->window, graphic->gc, (int)(X), (int)(Y), (int)(dX), (int)(dY)))
# define DrawCircle(X,Y,R) (XDrawArc (graphic->display, graphic->window, graphic->gc, (int)(X-R), (int)(Y-R), abs(2*R), abs(2*R), 0, 23040))
# define FillCircle(X,Y,R) (XFillArc (graphic->display, graphic->window, graphic->gc, (int)(X-R), (int)(Y-R), abs(2*R), abs(2*R), 0, 23040))

# define XCENTER 0.0
# define YCENTER 0.0
/* # define CAPSTYLE CapButt */
# define CAPSTYLE CapNotLast
# define JOINSTYLE JoinMiter

// XXX this is not thread safe, but that is OK
// static Graphic *graphic;

/* draw all objects for this Graph */
int DrawObjects (KapaGraphWidget *graph) {
  
  int i;
  
  // the functions below use this global value
  Graphic *graphic = GetGraphic();
  
  // this function calls all of the supporting Draw... functions below
  for (i = 0; i < graph[0].Nobjects; i++) {
    if (DEBUG) fprintf (stderr, "object: %d\n", i);
    if (DEBUG) fprintf (stderr, "Npts: %d\n", graph[0].objects[i].Npts);
    DrawObjectN (graphic, graph, &graph[0].objects[i]);
  }    
  XSetLineAttributes (graphic->display, graphic->gc, 0, LineSolid, CAPSTYLE, JOINSTYLE);
  XSetForeground (graphic->display, graphic->gc, graphic->fore);
  return (TRUE);
}

void DrawLineStyle (Graphic *graphic, KapaGraphWidget *graph, Gobjects *object) {

  static char dot[2] = {2,3};
  static char short_dash[2] = {4,4};
  static char long_dash[2] = {8,8};
  static char dot_dash[4] = {2,4,4,4};
  int lweight;
  
  lweight = MAX (1, MIN (10, object[0].lweight));

  /** some notes on drawing lines in X:

      "In general, drawing a thin line will be faster than drawing a wide line of width
      one. However, because of their different drawing algorithms, thin lines may not mix
      well aesthetically with wide lines. If it is desirable to obtain precise and uniform
      results across all displays, a client should always use a line-width of one rather
      than a line-width of zero."  -- http://www.hpc.unimelb.edu.au/nec/g1ae02e/chap7.html

  */

  /* set line type */
  switch (object[0].ltype) {
    case KAPA_LINE_DOT:
      XSetDashes (graphic->display, graphic->gc, 1, dot, 2);
      XSetLineAttributes (graphic->display, graphic->gc, lweight, LineDoubleDash, CAPSTYLE, JOINSTYLE);
      break;
    case KAPA_LINE_DASH_SHORT:
      XSetDashes (graphic->display, graphic->gc, 1, short_dash, 2);
      XSetLineAttributes (graphic->display, graphic->gc, lweight, LineDoubleDash, CAPSTYLE, JOINSTYLE);
      break;
    case KAPA_LINE_DASH_LONG:
      XSetDashes (graphic->display, graphic->gc, 1, long_dash, 2);
      XSetLineAttributes (graphic->display, graphic->gc, lweight, LineDoubleDash, CAPSTYLE, JOINSTYLE);
      break;
    case KAPA_LINE_DOT_DASH:
      XSetDashes (graphic->display, graphic->gc, 1, dot_dash, 4);
      XSetLineAttributes (graphic->display, graphic->gc, lweight, LineDoubleDash, CAPSTYLE, JOINSTYLE);
      break;
    case KAPA_LINE_SOLID:
    default:
      XSetLineAttributes (graphic->display, graphic->gc, lweight, LineSolid, CAPSTYLE, JOINSTYLE);
      break;
  }

  // negative color values mean "scale color with object.z"
  if (object[0].color >= 0) {
    XSetForeground (graphic->display, graphic->gc, graphic->color[object[0].color]);
  }
  return;
}

/* Draw a specific object in the graph */
int DrawObjectN (Graphic *graphic, KapaGraphWidget *graph, Gobjects *object) {
  
  DrawLineStyle (graphic, graph, object);

  switch (object[0].style) {
    case KAPA_PLOT_CONNECT: 
      DrawConnect (graphic, graph, object);
      break;
    case KAPA_PLOT_POLYGON: 
      DrawPolygon (graphic, graph, object);
      break;
    case KAPA_PLOT_POLYFILL: 
      DrawPolyfill (graphic, graph, object);
      break;
    case KAPA_PLOT_HISTOGRAM:
      DrawHistogram (graphic, graph, object);
      break;
    case KAPA_PLOT_BARS_SOLID:
      DrawBars (graphic, graph, object, KAPA_PLOT_BARS_SOLID);
      break;
    case KAPA_PLOT_BARS_OUTLINE:
      DrawBars (graphic, graph, object, KAPA_PLOT_BARS_OUTLINE);
      break;
    case KAPA_PLOT_BARS_OUTFILL:
      DrawBars (graphic, graph, object, KAPA_PLOT_BARS_OUTFILL);
      break;
    case KAPA_PLOT_POINTS:
    default:
      DrawPoints (graphic, graph, object);
      break;
  }
    
  if (object[0].etype & 0x01) {
    DrawYErrors (graphic, graph, object);
  }
  if (object[0].etype & 0x02) {
    DrawXErrors (graphic, graph, object);
  }
  return (TRUE);
}

/******/
void DrawConnect (Graphic *graphic, KapaGraphWidget *graph, Gobjects *object) {
  
  int i;
  float *x, *y;
  double mxi, mxj, myi, myj, bxi, bxj, byi, byj, bx, by;
  double sx0, sy0, sx1, sy1;
  double X0, X1, Y0, Y1;

  mxi = graph[0].axis[0].dfx / (object[0].x1 - object[0].x0);
  mxj = graph[0].axis[1].dfx / (object[0].y1 - object[0].y0);
  myi = graph[0].axis[0].dfy / (object[0].x1 - object[0].x0);
  myj = graph[0].axis[1].dfy / (object[0].y1 - object[0].y0);
  
  bxi  =  graph[0].axis[0].fx - object[0].x0*graph[0].axis[0].dfx/(object[0].x1 - object[0].x0);
  bxj  =  -object[0].y0*graph[0].axis[1].dfx/(object[0].y1 - object[0].y0);
  byi  =  -object[0].x0*graph[0].axis[0].dfy/(object[0].x1 - object[0].x0);
  byj  =  graph[0].axis[1].fy - object[0].y0*graph[0].axis[1].dfy/(object[0].y1 - object[0].y0);
  
  bx = bxi + bxj;
  by = byi + byj;
  
  X0 = graph[0].axis[0].fx;
  X1 = graph[0].axis[0].fx + graph[0].axis[0].dfx;
  Y0 = graph[0].axis[1].fy;
  Y1 = graph[0].axis[1].fy + graph[0].axis[1].dfy;

  x = object[0].x; y = object[0].y;
  for (i = 0; (i < object[0].Npts) && !(finite(x[i]) && finite(y[i])); i++);
  if (i >= object[0].Npts) return;
  sx0 = x[i]*mxi + y[i]*mxj + bx + XCENTER;
  sy0 = x[i]*myi + y[i]*myj + by + YCENTER;

  for (i++; i < object[0].Npts; i++) {
    if (!(finite(x[i]) && finite(y[i]))) continue;
    sx1 = x[i]*mxi + y[i]*mxj + bx + XCENTER;
    sy1 = x[i]*myi + y[i]*myj + by + YCENTER;
    ClipLine (graphic, sx0, sy0, sx1, sy1, X0, Y0, X1, Y1);
    sx0 = sx1; sy0 = sy1;
  }
}

/******/
void DrawPolygon (Graphic *graphic, KapaGraphWidget *graph, Gobjects *object) {

  float *x, *y;
  double mxi, mxj, myi, myj, bxi, bxj, byi, byj, bx, by;
  double sx0, sy0, sx1, sy1;
  double X0, X1, Y0, Y1;

  mxi = graph[0].axis[0].dfx / (object[0].x1 - object[0].x0);
  mxj = graph[0].axis[1].dfx / (object[0].y1 - object[0].y0);
  myi = graph[0].axis[0].dfy / (object[0].x1 - object[0].x0);
  myj = graph[0].axis[1].dfy / (object[0].y1 - object[0].y0);
  
  bxi  =  graph[0].axis[0].fx - object[0].x0*graph[0].axis[0].dfx/(object[0].x1 - object[0].x0);
  bxj  =  -object[0].y0*graph[0].axis[1].dfx/(object[0].y1 - object[0].y0);
  byi  =  -object[0].x0*graph[0].axis[0].dfy/(object[0].x1 - object[0].x0);
  byj  =  graph[0].axis[1].fy - object[0].y0*graph[0].axis[1].dfy/(object[0].y1 - object[0].y0);
  
  bx = bxi + bxj;
  by = byi + byj;
  
  X0 = graph[0].axis[0].fx;
  X1 = graph[0].axis[0].fx + graph[0].axis[0].dfx;
  Y0 = graph[0].axis[1].fy;
  Y1 = graph[0].axis[1].fy + graph[0].axis[1].dfy;

  x = object[0].x; y = object[0].y;

  // ptype must > 2
  // Npts % ptype must be 0
  // who must validate that?

  // each polygon is made of (N = ptype) points
  // we connect each point and the last one back
  for (int i = 0; i < object[0].Npts; i += object[0].ptype) {
    // first check for any invalid values for this polygon
    int skipObject = FALSE;
    for (int j = 0; (j < object[0].ptype) && !skipObject; j++) {
      int k = i + j;
      if (!(finite(x[k]) && finite(y[k]))) skipObject = TRUE;
    }
    if (skipObject) continue;

    for (int j = 0; (j < object[0].ptype); j++) {
      int k = i + j;
      sx0 = x[k]*mxi + y[k]*mxj + bx + XCENTER;
      sy0 = x[k]*myi + y[k]*myj + by + YCENTER;

      // last point connects to first
      if (j == object[0].ptype - 1) {
	sx1 = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy1 = x[i]*myi + y[i]*myj + by + YCENTER;
      } else {
	sx1 = x[k+1]*mxi + y[k+1]*mxj + bx + XCENTER;
	sy1 = x[k+1]*myi + y[k+1]*myj + by + YCENTER;
      }
      ClipLine (graphic, sx0, sy0, sx1, sy1, X0, Y0, X1, Y1);
    }
  }
}

/******/
void DrawPolyfill (Graphic *graphic, KapaGraphWidget *graph, Gobjects *object) {

  float *x, *y, *z;
  double mxi, mxj, myi, myj, bxi, bxj, byi, byj, bx, by;
  double sx0, sy0;
  // double X0, X1, Y0, Y1;

  mxi = graph[0].axis[0].dfx / (object[0].x1 - object[0].x0);
  mxj = graph[0].axis[1].dfx / (object[0].y1 - object[0].y0);
  myi = graph[0].axis[0].dfy / (object[0].x1 - object[0].x0);
  myj = graph[0].axis[1].dfy / (object[0].y1 - object[0].y0);
  
  bxi  =  graph[0].axis[0].fx - object[0].x0*graph[0].axis[0].dfx/(object[0].x1 - object[0].x0);
  bxj  =  -object[0].y0*graph[0].axis[1].dfx/(object[0].y1 - object[0].y0);
  byi  =  -object[0].x0*graph[0].axis[0].dfy/(object[0].x1 - object[0].x0);
  byj  =  graph[0].axis[1].fy - object[0].y0*graph[0].axis[1].dfy/(object[0].y1 - object[0].y0);
  
  bx = bxi + bxj;
  by = byi + byj;
  
  /*
    window boundary so we can clip objects
  X0 = graph[0].axis[0].fx;
  X1 = graph[0].axis[0].fx + graph[0].axis[0].dfx;
  Y0 = graph[0].axis[1].fy;
  Y1 = graph[0].axis[1].fy + graph[0].axis[1].dfy;
  */

  x = object[0].x; y = object[0].y; z = object[0].z;

  // ptype must > 2
  // Npts % ptype must be 0
  // who must validate that?

  ALLOCATE_PTR (points, XPoint, object[0].ptype);

  // NOTE that LoadObject.c:45 limits the allow
  // object styles which may have negative (scaled) colors
  int scaleColor = (object[0].color < 0);

  // each polygon is made of (N = ptype) points
  // we connect each point and the last one back
  for (int i = 0; i < object[0].Npts; i += object[0].ptype) {
    // first check for any invalid values for this polygon
    int skipObject = FALSE;
    for (int j = 0; (j < object[0].ptype) && !skipObject; j++) {
      int k = i + j;
      if (!(finite(x[k]) && finite(y[k]))) skipObject = TRUE;
    }
    if (skipObject) continue;

    for (int j = 0; (j < object[0].ptype); j++) {
      int k = i + j;
      sx0 = x[k]*mxi + y[k]*mxj + bx + XCENTER;
      sy0 = x[k]*myi + y[k]*myj + by + YCENTER;

      points[j].x = sx0;
      points[j].y = sy0;
    }

    if (scaleColor) {
      if (!finite(z[i])) continue;
      int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
      XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
    }
    XFillPolygon (graphic->display, graphic->window, graphic->gc, points, object[0].ptype, Convex, CoordModeOrigin);
  }

  free (points);
}

void ClipLine (Graphic *graphic, double x0, double y0, double x1, double y1, double X0, double Y1, double X1, double Y0) {

  /* skip line segement if both points are beyond box */
  if ((x0 <= X0) && (x1 <= X0)) return;
  if ((x0 >= X1) && (x1 >= X1)) return;
  if ((y0 <= Y0) && (y1 <= Y0)) return;
  if ((y0 >= Y1) && (y1 >= Y1)) return;

  /* replace x0,y0 if outside box */
  if ((x0 < X0) && (x1 >= X0)) {
    y0 = y0 + (X0 - x0)*(y1 - y0)/(x1 - x0);
    x0 = X0;
  }
  if ((x0 > X1) && (x1 <= X1)) {
    y0 = y0 + (X1 - x0)*(y1 - y0)/(x1 - x0);
    x0 = X1;
  }
  if ((y0 < Y0) && (y1 >= Y0)) {
    x0 = x0 + (Y0 - y0)*(x1 - x0)/(y1 - y0);
    y0 = Y0;
  }
  if ((y0 > Y1) && (y1 <= Y1)) {
    x0 = x0 + (Y1 - y0)*(x1 - x0)/(y1 - y0);
    y0 = Y1;
  }

  /* skip line segement if both points are beyond box */
  if ((x0 <= X0) && (x1 <= X0)) return;
  if ((x0 >= X1) && (x1 >= X1)) return;
  if ((y0 <= Y0) && (y1 <= Y0)) return;
  if ((y0 >= Y1) && (y1 >= Y1)) return;

  /* replace x1,y1 if outside box */
  if ((x1 < X0) && (x0 >= X0)) {
    y1 = y0 + (X0 - x0)*(y1 - y0)/(x1 - x0);
    x1 = X0;
  }
  if ((x1 > X1) && (x0 <= X1)) {
    y1 = y0 + (X1 - x0)*(y1 - y0)/(x1 - x0);
    x1 = X1;
  }
  if ((y1 < Y0) && (y0 >= Y0)) {
    x1 = x0 + (Y0 - y0)*(x1 - x0)/(y1 - y0);
    y1 = Y0;
  }
  if ((y1 > Y1) && (y0 <= Y1)) {
    x1 = x0 + (Y1 - y0)*(x1 - x0)/(y1 - y0);
    y1 = Y1;
  }
  DrawLine (x0, y0, x1, y1);
}
  

/******/
/* simplify the code abit by finding triplets, watch out for a histogram of 2 points */
void DrawHistogram (Graphic *graphic, KapaGraphWidget *graph, Gobjects *object) {

  int i;
  float *x, *y;
  double mxi, mxj, myi, myj, bxi, bxj, byi, byj, bx, by;
  double sx0, sy0, sx1, sy1, sxa, sya, sxo, syo;
  double X0, X1, Y0, Y1;

  mxi = graph[0].axis[0].dfx / (object[0].x1 - object[0].x0);
  mxj = graph[0].axis[1].dfx / (object[0].y1 - object[0].y0);
  myi = graph[0].axis[0].dfy / (object[0].x1 - object[0].x0);
  myj = graph[0].axis[1].dfy / (object[0].y1 - object[0].y0);
  
  bxi  =  graph[0].axis[0].fx - object[0].x0*graph[0].axis[0].dfx/(object[0].x1 - object[0].x0);
  bxj  =  -object[0].y0*graph[0].axis[1].dfx/(object[0].y1 - object[0].y0);
  byi  =  -object[0].x0*graph[0].axis[0].dfy/(object[0].x1 - object[0].x0);
  byj  =  graph[0].axis[1].fy - object[0].y0*graph[0].axis[1].dfy/(object[0].y1 - object[0].y0);
  
  bx = bxi + bxj;
  by = byi + byj;
  
  X0 = graph[0].axis[0].fx;
  X1 = graph[0].axis[0].fx + graph[0].axis[0].dfx;
  Y0 = graph[0].axis[1].fy;
  Y1 = graph[0].axis[1].fy + graph[0].axis[1].dfy;

  /* find the first valid datapoint */
  x = object[0].x; y = object[0].y;
  for (i = 0; (i < object[0].Npts) && !(finite(x[i]) && finite(y[i])); i++);
  if (i >= object[0].Npts) return;

  /* first valid data point */
  sx0 = x[i]*mxi + y[i]*mxj + bx + XCENTER;
  sy0 = x[i]*myi + y[i]*myj + by + YCENTER;
  sx0 = MIN (MAX (sx0, X0), X1);
  sy0 = MAX (MIN (sy0, Y0), Y1);
  
  /* find the second valid datapoint */
  for (i++; (i < object[0].Npts) && !(finite(x[i]) && finite(y[i])); i++);
  if (i >= object[0].Npts) return;

  /* second valid data point */
  sx1 = x[i]*mxi + y[i]*mxj + bx + XCENTER;
  sy1 = x[i]*myi + y[i]*myj + by + YCENTER;
  sx1 = MIN (MAX (sx1, X0), X1);
  sy1 = MAX (MIN (sy1, Y0), Y1);
  
  /* connect first point to second point */
  sxa = MIN (MAX (sx0 - 0.5*(sx1 - sx0), X0), X1);
  sya = MAX (sy0, Y0);
  DrawLine (sx0, sy0, sxa, sy0);
  DrawLine (sxa, sy0, sxa, sya);
  
  /* draw segment equal distance behind first point and down to x-axis */
  sxa = MIN (MAX (0.5*(sx0 + sx1), X0), X1);
  DrawLine (sx0, sy0, sxa, sy0);
  DrawLine (sxa, sy0, sxa, sy1);
  DrawLine (sxa, sy1, sx1, sy1);
  sx0 = sx1;
  sy0 = sy1;
  
  /* continue with rest of points */
  sxo = syo = 0;
  for (i++; i < object[0].Npts; i++) {
    if (!(finite(x[i]) && finite(y[i]))) continue;
    sx1 = x[i]*mxi + y[i]*mxj + bx + XCENTER;
    sy1 = x[i]*myi + y[i]*myj + by + YCENTER;
    sx1 = MIN (MAX (sx1, X0), X1);
    sy1 = MAX (MIN (sy1, Y0), Y1);
    sxa = MIN (MAX (0.5*(sx0 + sx1), X0), X1);
    DrawLine (sx0, sy0, sxa, sy0);
    DrawLine (sxa, sy0, sxa, sy1);
    DrawLine (sxa, sy1, sx1, sy1);
    sxo = sx0; syo = sy0;
    sx0 = sx1; sy0 = sy1;
  }
  
  /* draw segment equal distance after last point and down to x-axis */
  sxa = MIN (MAX (sx1 + 0.5*(sx1 - sxo), X0), X1);
  sya = MAX (sy1, Y0);
  DrawLine (sx1, sy1, sxa, sy1);
  DrawLine (sxa, sy1, sxa, sya);
}

// uses object->color
# define HISTOGRAM_SOLID(X_VALUE, Y_VALUE, DX_VAL) {			\
  /* histogram bar corners */						\
  double sxmin = (X_VALUE) - 0.5*(DX_VAL);				\
  double sxmax = (X_VALUE) + 0.5*(DX_VAL);				\
  double symin = Xaxis;							\
  double symax = (Y_VALUE);						\
  /* saturated values for corner coords: */				\
  sxmin = MIN (MAX (sxmin, X0), X1);					\
  sxmax = MIN (MAX (sxmax, X0), X1);					\
  symin = MAX (MIN (symin, Y0), Y1);					\
  symax = MAX (MIN (symax, Y0), Y1);					\
  double dy = fabs(symax - symin);					\
  double ylow = MIN(symin, symax);					\
  FillRectangle (sxmin, ylow, (DX_VAL), dy); }

// uses object->color
# define HISTOGRAM_OUTLINE(X_VALUE, Y_VALUE, DX_VAL) {			\
  /* histogram bar corners */						\
  double sxmin = (X_VALUE) - 0.5*(DX_VAL);				\
  double sxmax = (X_VALUE) + 0.5*(DX_VAL);				\
  double symin = Xaxis;							\
  double symax = (Y_VALUE);						\
  /* saturated values for corner coords: */				\
  sxmin = MIN (MAX (sxmin, X0), X1);					\
  sxmax = MIN (MAX (sxmax, X0), X1);					\
  symin = MAX (MIN (symin, Y0), Y1);					\
  symax = MAX (MIN (symax, Y0), Y1);					\
  double dy = fabs(symax - symin);					\
  double ylow = MIN(symin, symax);					\
  DrawRectangle (sxmin, ylow, (DX_VAL), dy); }

# define HISTOGRAM_OUTFILL(X_VALUE, Y_VALUE, DX_VAL) {			\
  /* histogram bar corners */						\
  double sxmin = (X_VALUE) - 0.5*(DX_VAL);				\
  double sxmax = (X_VALUE) + 0.5*(DX_VAL);				\
  double symin = Xaxis;							\
  double symax = (Y_VALUE);						\
  /* saturated values for corner coords: */				\
  sxmin = MIN (MAX (sxmin, X0), X1);					\
  sxmax = MIN (MAX (sxmax, X0), X1);					\
  symin = MAX (MIN (symin, Y0), Y1);					\
  symax = MAX (MIN (symax, Y0), Y1);					\
  double dy = fabs(symax - symin);					\
  double ylow = MIN(symin, symax);					\
  FillRectangle (sxmin, ylow, (DX_VAL), dy);				\
  XSetForeground (graphic->display, graphic->gc, graphic->fore);	\
  DrawRectangle (sxmin, ylow, (DX_VAL), dy);				\
  XSetForeground (graphic->display, graphic->gc, graphic->color[object[0].color]); }

void DrawBars (Graphic *graphic, KapaGraphWidget *graph, Gobjects *object, int mode) {

  double mxi = graph[0].axis[0].dfx / (object[0].x1 - object[0].x0); // slope of the x-axis in x-pixels
  double mxj = graph[0].axis[1].dfx / (object[0].y1 - object[0].y0); // slope of the x-axis in y-pixels (always 0 for now)
  double myi = graph[0].axis[0].dfy / (object[0].x1 - object[0].x0); // slope of the x-axis in x-pixels (always 0 for now)
  double myj = graph[0].axis[1].dfy / (object[0].y1 - object[0].y0); // slope of the x-axis in x-pixels
  
  // intercepts of axes
  double bxi  =  graph[0].axis[0].fx - object[0].x0*graph[0].axis[0].dfx/(object[0].x1 - object[0].x0);
  double bxj  =  -object[0].y0*graph[0].axis[1].dfx/(object[0].y1 - object[0].y0);
  double byi  =  -object[0].x0*graph[0].axis[0].dfy/(object[0].x1 - object[0].x0);
  double byj  =  graph[0].axis[1].fy - object[0].y0*graph[0].axis[1].dfy/(object[0].y1 - object[0].y0);
  
  double bx = bxi + bxj;
  double by = byi + byj;
  
  // corner coords
  double X0 = graph[0].axis[0].fx;
  double X1 = graph[0].axis[0].fx + graph[0].axis[0].dfx;
  double Y0 = graph[0].axis[1].fy;
  double Y1 = graph[0].axis[1].fy + graph[0].axis[1].dfy;

  /* find the first valid datapoint */
  float *x = object[0].x;
  float *y = object[0].y;
  
  /* we are drawing bars which are filled rectangles of height y[i] and width 0.5*dx
     one point at a time

     I need to know the distance to the next and prev points to calculate dx
     for the first point, dx is x[1] - x[0]
     for the last point, dx is x[-1] - x[-2] (x[-1] is the last point)
     for the rest, dx is 0.5*(x[i+1] - x[i-1])
    
     rather than working out complex on-the-fly logic, I want to find the first and last
     valid point in an initial pass, then calculate the above for the remainder.
     TBD: make an index vector of only valid points?
  */

  int Ngood = 0;
  ALLOCATE_PTR (goodPoint, int, object[0].Npts);

  // x = ??, y = 0: this assumes the xaxis is parallel to the plot window (myi = 0)
  // since y runs from large on bottom to small on top, this is slightly backwards
  float Xaxis = by + YCENTER;
  Xaxis = MAX (Y1, MIN (Xaxis, Y0)); // MIN & MAX reversed for neg y dir

  for (int i = 0; i < object[0].Npts; i++) {
    if (!(finite(x[i]) && finite(y[i]))) continue;

    // coordinate on the screen of the point:
    double sx1r = x[i]*mxi + y[i]*mxj + bx + XCENTER;

    if (sx1r < X0) {
      // point to the left, skip it
      continue;
    }
    if (sx1r > X1) {
      // point to the right, skip it
      continue;
    }

    goodPoint[Ngood] = i;
    Ngood ++;
  }
    
  if (Ngood == 0) {
    free (goodPoint);
    return;
  }

  // if we only have 1 point, draw bar half the width of the screen
  // this works fine, but the auto limits for 1 value are somewhat silly
  if (Ngood == 1) {
    // coordinate on the screen of the point:
    int n = goodPoint[0];
    double sx1r = x[n]*mxi + y[n]*mxj + bx + XCENTER;
    double sy1r = x[n]*myi + y[n]*myj + by + YCENTER;

    float dx = 0.5*object->size*(X1 - X0);
    
    switch (mode) {
      case KAPA_PLOT_BARS_SOLID:
	HISTOGRAM_SOLID(sx1r, sy1r, dx);
	break;
      case KAPA_PLOT_BARS_OUTLINE:
	HISTOGRAM_OUTLINE(sx1r, sy1r, dx);
	break;
      case KAPA_PLOT_BARS_OUTFILL:
	HISTOGRAM_OUTFILL(sx1r, sy1r, dx);
	break;
      default:
	HISTOGRAM_SOLID(sx1r, sy1r, dx);
	break;
    }

    free (goodPoint);
    return;
  }

  // first point:
  {
    int n;
    n = goodPoint[1];
    double sx1r = x[n]*mxi + y[n]*mxj + bx + XCENTER;

    n = goodPoint[0];
    double sx0r = x[n]*mxi + y[n]*mxj + bx + XCENTER;
    double sy0r = x[n]*myi + y[n]*myj + by + YCENTER;

    double dx = 0.5*object->size*(sx1r - sx0r);
    
    switch (mode) {
      case KAPA_PLOT_BARS_SOLID:
	HISTOGRAM_SOLID(sx0r, sy0r, dx);
	break;
      case KAPA_PLOT_BARS_OUTLINE:
	HISTOGRAM_OUTLINE(sx0r, sy0r, dx);
	break;
      case KAPA_PLOT_BARS_OUTFILL:
	HISTOGRAM_OUTFILL(sx0r, sy0r, dx);
	break;
      default:
	HISTOGRAM_SOLID(sx0r, sy0r, dx);
	break;
    }
  }

  for (int i = 1; i < Ngood - 1; i++) {

    int n;
    n = goodPoint[i + 1];
    double sx2r = x[n]*mxi + y[n]*mxj + bx + XCENTER;

    n = goodPoint[i - 1];
    double sx0r = x[n]*mxi + y[n]*mxj + bx + XCENTER;

    // below we have 2 factors of 0.5 (we want half of the average spacing)
    double dx = 0.5*0.5*object->size*(sx2r - sx0r);

    n = goodPoint[i];
    double sx1r = x[n]*mxi + y[n]*mxj + bx + XCENTER;
    double sy1r = x[n]*myi + y[n]*myj + by + YCENTER;

    switch (mode) {
      case KAPA_PLOT_BARS_SOLID:
	HISTOGRAM_SOLID(sx1r, sy1r, dx);
	break;
      case KAPA_PLOT_BARS_OUTLINE:
	HISTOGRAM_OUTLINE(sx1r, sy1r, dx);
	break;
      case KAPA_PLOT_BARS_OUTFILL:
	HISTOGRAM_OUTFILL(sx1r, sy1r, dx);
	break;
      default:
	HISTOGRAM_SOLID(sx1r, sy1r, dx);
	break;
    }
  }
  
  // last point:
  {
    int n;
    n = goodPoint[Ngood - 1];
    double sx1r = x[n]*mxi + y[n]*mxj + bx + XCENTER;
    double sy1r = x[n]*myi + y[n]*myj + by + YCENTER;

    n = goodPoint[Ngood - 2];
    double sx0r = x[n]*mxi + y[n]*mxj + bx + XCENTER;

    double dx = 0.5*object->size*(sx1r - sx0r);

    switch (mode) {
      case KAPA_PLOT_BARS_SOLID:
	HISTOGRAM_SOLID(sx1r, sy1r, dx);
	break;
      case KAPA_PLOT_BARS_OUTLINE:
	HISTOGRAM_OUTLINE(sx1r, sy1r, dx);
	break;
      case KAPA_PLOT_BARS_OUTFILL:
	HISTOGRAM_OUTFILL(sx1r, sy1r, dx);
	break;
      default:
	HISTOGRAM_SOLID(sx1r, sy1r, dx);
	break;
    }
  }
  free (goodPoint);
  return;
}

/******/
void DrawPoints (Graphic *graphic, KapaGraphWidget *graph, Gobjects *object) {

  int i;
  float *x, *y, *z;
  double mxi, mxj, myi, myj, bxi, bxj, byi, byj, bx, by;
  int sx, sy, sx1, sy1, sx2, sy2, ds, dz, D;
  
  mxi = graph[0].axis[0].dfx / (object[0].x1 - object[0].x0);
  mxj = graph[0].axis[1].dfx / (object[0].y1 - object[0].y0);
  myi = graph[0].axis[0].dfy / (object[0].x1 - object[0].x0);
  myj = graph[0].axis[1].dfy / (object[0].y1 - object[0].y0);
  
  bxi  =  graph[0].axis[0].fx - object[0].x0*graph[0].axis[0].dfx/(object[0].x1 - object[0].x0);
  bxj  =  -object[0].y0*graph[0].axis[1].dfx/(object[0].y1 - object[0].y0);
  byi  =  -object[0].x0*graph[0].axis[0].dfy/(object[0].x1 - object[0].x0);
  byj  =  graph[0].axis[1].fy - object[0].y0*graph[0].axis[1].dfy/(object[0].y1 - object[0].y0);
  
  bx = bxi + bxj;
  by = byi + byj;
  
  /**** point sizes are scaled by object.size, colors by object.color ***/
  int scaleSize = (object[0].size < 0);
  int scaleColor = (object[0].color < 0);

  // NOTE that LoadObject.c:45 limits the allow
  // object styles which may have negative (scaled) colors

  ds = 0.5 * (graphic->dx + graphic->dy) * 0.003 * object[0].size;
  dz = 0.5 * (graphic->dx + graphic->dy) * 0.010;
  x = object[0].x; y = object[0].y; z = object[0].z;

  switch (object[0].ptype) {
    case KAPA_POINT_BOX_OPEN:	/* open box */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawRectangle (sx - D, sy - D, 2*D, 2*D);
	}
      }
      break;
    case KAPA_POINT_CROSS: /* cross */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx - D, sy, sx + D + 1, sy);
	  DrawLine (sx, sy - D, sx, sy + D + 1);
	}
      }
      break;
    case KAPA_POINT_X:	/* x */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx - D, sy + D, sx + D + 1, sy - D - 1);
	  DrawLine (sx - D, sy - D, sx + D + 1, sy + D + 1);
	}
      }
      break;
    case KAPA_POINT_TRIANGLE_SOLID:	/* filled triangle */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;

	  XPoint points[4];
	  points[0].x = sx - D;  points[0].y = sy + 0.58*D;  
	  points[1].x = sx + D;  points[1].y = sy + 0.58*D;  
	  points[2].x = sx;      points[2].y = sy - 1.15*D;  
	  points[3].x = sx - D;  points[3].y = sy + 0.58*D;  
	  XFillPolygon (graphic->display, graphic->window, graphic->gc, points, 4, Convex, CoordModeOrigin);
	}
      }
      break;
    case KAPA_POINT_TRIANGLE_SOLID_DOWN: /* filled triangle (down) */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;

	  XPoint points[4];
	  points[0].x = sx - D;  points[0].y = sy - 0.58*D;  
	  points[1].x = sx + D;  points[1].y = sy - 0.58*D;  
	  points[2].x = sx;      points[2].y = sy + 1.15*D;  
	  points[3].x = sx - D;  points[3].y = sy - 0.58*D;  
	  XFillPolygon (graphic->display, graphic->window, graphic->gc, points, 4, Convex, CoordModeOrigin);
	}
      }
      break;
    case KAPA_POINT_TRIANGLE_OPEN:	/* open triangle */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx - D, sy + 0.58*D, sx + D, sy + 0.58*D);
	  DrawLine (sx + D, sy + 0.58*D, sx,     sy - 1.15*D);
	  DrawLine (sx,     sy - 1.15*D, sx - D, sy + 0.58*D);
	}
      }
      break;
    case KAPA_POINT_TRIANGLE_OPEN_DOWN:	/* upside-down open triangle */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx - D, sy - 0.58*D, sx + D, sy - 0.58*D);
	  DrawLine (sx + D, sy - 0.58*D, sx,     sy + 1.15*D);
	  DrawLine (sx,     sy + 1.15*D, sx - D, sy - 0.58*D);
	}
      }
      break;
    case KAPA_POINT_Y:	/* Y */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx, sy, sx - D, sy - 0.58*D);
	  DrawLine (sx, sy, sx + D, sy - 0.58*D);
	  DrawLine (sx, sy, sx,     sy + 1.15*D);
	}
      }
      break;
    case KAPA_POINT_Y_DOWN:	/* upside-down Y */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx, sy, sx - D, sy + 0.58*D);
	  DrawLine (sx, sy, sx + D, sy + 0.58*D);
	  DrawLine (sx, sy, sx,     sy - 1.15*D);
	}
      }
      break;
    case KAPA_POINT_CIRCLE_OPEN: /* 0 */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawCircle (sx, sy, D);
	}
      }
      break;
    case KAPA_POINT_CIRCLE_SOLID: /* filled 0 */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawCircle (sx, sy, D);
	  FillCircle (sx, sy, D);
	}
      }
      break;
    case KAPA_POINT_PENTAGON:	/* pentagon */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx + 0.00*D, sy - 1.00*D, sx + 0.95*D, sy - 0.31*D);
	  DrawLine (sx + 0.95*D, sy - 0.31*D, sx + 0.58*D, sy + 0.81*D);
	  DrawLine (sx + 0.58*D, sy + 0.81*D, sx - 0.58*D, sy + 0.81*D);
	  DrawLine (sx - 0.58*D, sy + 0.81*D, sx - 0.95*D, sy - 0.31*D);
	  DrawLine (sx - 0.95*D, sy - 0.31*D, sx + 0.00*D, sy - 1.00*D);
	}
      }
      break;
    case KAPA_POINT_HEXAGON:	/* hexagon */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx -      D, sy,          sx - 0.50*D, sy + 0.87*D);
	  DrawLine (sx - 0.50*D, sy + 0.87*D, sx + 0.50*D, sy + 0.87*D);
	  DrawLine (sx + 0.50*D, sy + 0.87*D, sx +      D, sy);

	  DrawLine (sx +      D, sy,          sx + 0.50*D, sy - 0.87*D);
	  DrawLine (sx + 0.50*D, sy - 0.87*D, sx - 0.50*D, sy - 0.87*D);
	  DrawLine (sx - 0.50*D, sy - 0.87*D, sx -      D, sy);
	}
      }
      break;
    case KAPA_POINT_PAIR_CONNECT: { /* connect pairs of points */
      double X0 = graph[0].axis[0].fx;
      double X1 = graph[0].axis[0].fx + graph[0].axis[0].dfx;
      double Y0 = graph[0].axis[1].fy;
      double Y1 = graph[0].axis[1].fy + graph[0].axis[1].dfy;

      for (i = 0; i + 1 < object[0].Npts; i+=2) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx1 = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy1 = x[i]*myi + y[i]*myj + by + YCENTER;
	if (!(finite(x[i+1]) && finite(y[i+1]))) continue;
	sx2 = x[i+1]*mxi + y[i+1]*mxj + bx + XCENTER;
	sy2 = x[i+1]*myi + y[i+1]*myj + by + YCENTER;
	ClipLine (graphic, sx1, sy1, sx2, sy2, X0, Y0, X1, Y1);
      }
      break;
    }
    case KAPA_POINT_BOX_SOLID:	/* filled box */
    default:
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx + XCENTER;
	sy = x[i]*myi + y[i]*myj + by + YCENTER;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    if (!finite(z[i])) continue;
	    int pixel = MIN (graphic->Npixels - 2, MAX (0, z[i]*(graphic->Npixels - 1)));
	    XSetForeground (graphic->display, graphic->gc, graphic->cmap[pixel].pixel);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  FillRectangle (sx - D, sy - D, 2*D + 1, 2*D + 1);
	}
      }
      break;
  }
}
    
/******/
void DrawXErrors (Graphic *graphic, KapaGraphWidget *graph, Gobjects *object) {
  
  int i, bar, dz, ds, D;
  float *x, *y, *z, *dxm, *dxp;
  double mxi, mxj, myi, myj, bxi, bxj, byi, byj, bx, by;
  double sx0, sy0, sx1, sy1, sz, sx10, sx11, X0, X1, Y0, Y1;

  int scaleSize = (object[0].size < 0);

  mxi = graph[0].axis[0].dfx / (object[0].x1 - object[0].x0);
  mxj = graph[0].axis[1].dfx / (object[0].y1 - object[0].y0);
  myi = graph[0].axis[0].dfy / (object[0].x1 - object[0].x0);
  myj = graph[0].axis[1].dfy / (object[0].y1 - object[0].y0);
  
  bxi  =  graph[0].axis[0].fx - object[0].x0*graph[0].axis[0].dfx/(object[0].x1 - object[0].x0);
  bxj  =  -object[0].y0*graph[0].axis[1].dfx/(object[0].y1 - object[0].y0);
  byi  =  -object[0].x0*graph[0].axis[0].dfy/(object[0].x1 - object[0].x0);
  byj  =  graph[0].axis[1].fy - object[0].y0*graph[0].axis[1].dfy/(object[0].y1 - object[0].y0);
  
  bx = bxi + bxj;
  by = byi + byj;
  
  ds = 0.5 * (graphic->dx + graphic->dy) * 0.003 * object[0].size;
  dz = 0.5 * (graphic->dx + graphic->dy) * 0.010;

  x = object[0].x; y = object[0].y; dxp = object[0].dxp; dxm = object[0].dxm; z = object[0].z;
  bar = object[0].ebar; sz = object[0].size*graph[0].axis[1].dfy*0.03;
  
  X0 = graph[0].axis[0].fx;
  X1 = graph[0].axis[0].fx + graph[0].axis[0].dfx;
  Y0 = graph[0].axis[1].fy;
  Y1 = graph[0].axis[1].fy + graph[0].axis[1].dfy;

  /// XXX NOTE : D should be modified by (mxi,myi) for tilted axes dx = D*(mxi/mx), dy = D*(myi/mx)

  for (i = 0; i < object[0].Npts; i++) {
    // for open circles, only go to the outer radius
    D = 0;
    if (object[0].ptype == KAPA_POINT_CIRCLE_OPEN  	) { D = scaleSize ? dz*z[i] : ds; }
    if (object[0].ptype == KAPA_POINT_BOX_OPEN     	) { D = scaleSize ? dz*z[i] : ds; }
    if (object[0].ptype == KAPA_POINT_TRIANGLE_OPEN	) { D = scaleSize ? 0.66*dz*z[i] : 0.66*ds; }
    if (object[0].ptype == KAPA_POINT_TRIANGLE_OPEN_DOWN) { D = scaleSize ? 0.66*dz*z[i] : 0.66*ds; }
    if (!(finite(x[i]) && finite(y[i]) && finite(dxp[i]))) goto skip_dxp;
    if (D > fabs(dxp[i]*mxi)) goto skip_dxp;
    sx0 = x[i]*mxi + y[i]*mxj + bx + XCENTER + D;
    sy0 = x[i]*myi + y[i]*myj + by + YCENTER;
    sx1 = sx0 + dxp[i]*mxi - D;
    sy1 = sy0 + dxp[i]*myi;
    if (((sx0 > graph[0].axis[0].fx) && (sx0 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy0 < graph[0].axis[1].fy) && (sy0 > graph[0].axis[1].fy + graph[0].axis[1].dfy)) ||
	((sx1 > graph[0].axis[0].fx) && (sx1 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy1 < graph[0].axis[1].fy) && (sy1 > graph[0].axis[1].fy + graph[0].axis[1].dfy))) 
    {
      ClipLine (graphic, sx0, sy0, sx1, sy1, X0, Y0, X1, Y1);
      if (bar) {
	sx10 = sy1 - sz;
	sx11 = sy1 + sz;
	ClipLine (graphic, sx1, sx10, sx1, sx11, X0, Y0, X1, Y1);
      }
    }
  skip_dxp:
    if (!(finite(x[i]) && finite(y[i]) && finite(dxm[i]))) continue;
    if (D > fabs(dxm[i]*mxi)) continue;
    sx0 = x[i]*mxi + y[i]*mxj + bx + XCENTER - D;
    sy0 = x[i]*myi + y[i]*myj + by + YCENTER;
    sx1 = sx0 - dxm[i]*mxi + D;
    sy1 = sy0 - dxm[i]*myi;
    if (((sx0 > graph[0].axis[0].fx) && (sx0 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy0 < graph[0].axis[1].fy) && (sy0 > graph[0].axis[1].fy + graph[0].axis[1].dfy)) ||
	((sx1 > graph[0].axis[0].fx) && (sx1 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy1 < graph[0].axis[1].fy) && (sy1 > graph[0].axis[1].fy + graph[0].axis[1].dfy)))
    {
      ClipLine (graphic, sx0, sy0, sx1, sy1, X0, Y0, X1, Y1);
      if (bar) {
	sx10 = sy1 - sz;
	sx11 = sy1 + sz;
	ClipLine (graphic, sx1, sx10, sx1, sx11, X0, Y0, X1, Y1);
      }
    }
  }
}
    
/******/
void DrawYErrors (Graphic *graphic, KapaGraphWidget *graph, Gobjects *object) {

  int i, bar, dz, ds, D;
  float *x, *y, *z, *dym, *dyp;
  double mxi, mxj, myi, myj, bxi, bxj, byi, byj, bx, by;
  double sx0, sy0, sx1, sy1, sz, sx10, sx11, X0, X1, Y0, Y1;

  int scaleSize = (object[0].size < 0);

  mxi = graph[0].axis[0].dfx / (object[0].x1 - object[0].x0);
  mxj = graph[0].axis[1].dfx / (object[0].y1 - object[0].y0);
  myi = graph[0].axis[0].dfy / (object[0].x1 - object[0].x0);
  myj = graph[0].axis[1].dfy / (object[0].y1 - object[0].y0);
  
  bxi  =  graph[0].axis[0].fx - object[0].x0*graph[0].axis[0].dfx/(object[0].x1 - object[0].x0);
  bxj  =  -object[0].y0*graph[0].axis[1].dfx/(object[0].y1 - object[0].y0);
  byi  =  -object[0].x0*graph[0].axis[0].dfy/(object[0].x1 - object[0].x0);
  byj  =  graph[0].axis[1].fy - object[0].y0*graph[0].axis[1].dfy/(object[0].y1 - object[0].y0);
  
  bx = bxi + bxj;
  by = byi + byj;
  
  ds = 0.5 * (graphic->dx + graphic->dy) * 0.003 * object[0].size;
  dz = 0.5 * (graphic->dx + graphic->dy) * 0.010;

  x = object[0].x; y = object[0].y; dyp = object[0].dyp; dym = object[0].dym; z = object[0].z;
  bar = object[0].ebar; sz = object[0].size*graph[0].axis[0].dfx*0.03;
  
  X0 = graph[0].axis[0].fx;
  X1 = graph[0].axis[0].fx + graph[0].axis[0].dfx;
  Y0 = graph[0].axis[1].fy;
  Y1 = graph[0].axis[1].fy + graph[0].axis[1].dfy;

  /// XXX NOTE : D should be modified by (mxi,myi) for tilted axes dx = D*(mxi/mx), dy = D*(myi/mx)

  for (i = 0; i < object[0].Npts; i++) {
    // for open circles, only go to the outer radius
    D = 0;
    if (object[0].ptype == KAPA_POINT_CIRCLE_OPEN  	) { D = scaleSize ? dz*z[i] : ds; }
    if (object[0].ptype == KAPA_POINT_BOX_OPEN     	) { D = scaleSize ? dz*z[i] : ds; }
    if (object[0].ptype == KAPA_POINT_TRIANGLE_OPEN	) { D = scaleSize ? 1.15*dz*z[i] : 1.15*ds; }
    if (object[0].ptype == KAPA_POINT_TRIANGLE_OPEN_DOWN) { D = scaleSize ? 0.58*dz*z[i] : 0.58*ds; }
    if (!(finite(x[i]) && finite(y[i]) && finite(dyp[i]))) goto skip_dyp;
    if (D > fabs(dyp[i]*myj)) goto skip_dyp;
    sx0 = x[i]*mxi + y[i]*mxj + bx + XCENTER;
    sy0 = x[i]*myi + y[i]*myj + by + YCENTER - D;
    sx1 = sx0 + dyp[i]*mxj;
    sy1 = sy0 + dyp[i]*myj + D;
    if (((sx0 > graph[0].axis[0].fx) && (sx0 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy0 < graph[0].axis[1].fy) && (sy0 > graph[0].axis[1].fy + graph[0].axis[1].dfy)) ||
	((sx1 > graph[0].axis[0].fx) && (sx1 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy1 < graph[0].axis[1].fy) && (sy1 > graph[0].axis[1].fy + graph[0].axis[1].dfy)))
    {
      ClipLine (graphic, sx0, sy0, sx1, sy1, X0, Y0, X1, Y1);
      if (bar) {
	sx10 = sx1 - sz;
	sx11 = sx1 + sz;
	ClipLine (graphic, sx10, sy1, sx11, sy1, X0, Y0, X1, Y1);
      }
    }
  skip_dyp:
    if (!(finite(x[i]) && finite(y[i]) && finite(dym[i]))) continue;
    if (object[0].ptype == KAPA_POINT_TRIANGLE_OPEN	) { D = scaleSize ? 0.58*dz*z[i] : 0.58*ds; }
    if (object[0].ptype == KAPA_POINT_TRIANGLE_OPEN_DOWN) { D = scaleSize ? 1.15*dz*z[i] : 1.15*ds; }
    if (D > fabs(dym[i]*myj)) continue;
    sx0 = x[i]*mxi + y[i]*mxj + bx + XCENTER;
    sy0 = x[i]*myi + y[i]*myj + by + YCENTER + D;
    sx1 = sx0 - dym[i]*mxj;
    sy1 = sy0 - dym[i]*myj - D;
    if (((sx0 > graph[0].axis[0].fx) && (sx0 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy0 < graph[0].axis[1].fy) && (sy0 > graph[0].axis[1].fy + graph[0].axis[1].dfy)) ||
	((sx1 > graph[0].axis[0].fx) && (sx1 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy1 < graph[0].axis[1].fy) && (sy1 > graph[0].axis[1].fy + graph[0].axis[1].dfy)))
    {
      ClipLine (graphic, sx0, sy0, sx1, sy1, X0, Y0, X1, Y1);
      if (bar) {
	sx10 = sx1 - sz;
	sx11 = sx1 + sz;
	ClipLine (graphic, sx10, sy1, sx11, sy1, X0, Y0, X1, Y1);
      }
    }
  }
}
