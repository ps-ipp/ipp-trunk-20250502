# include "Ximage.h"

# define BEZIER_C 0.55191502449

double PDF_SetLineWeight (IOBuffer *buffer, double lweightIn) {
  double lweight = MAX (0, MIN (10, lweightIn));
  PrintIOBuffer (buffer, "%.1f w\n", lweight);
  return lweight;
}

void PDF_SetKapaColor (IOBuffer *buffer, bDrawColor color) {
    PrintIOBuffer (buffer, "%s rg\n", KapaColorRGBString(color));
    PrintIOBuffer (buffer, "%s RG\n", KapaColorRGBString(color));
    return;
}

void PDF_SetScaledColor (IOBuffer *buffer, float *pixel1, float *pixel2, float *pixel3, float value, int Npixels) {
  if (!finite(value)) return;
  int pixel = MIN (Npixels - 2, MAX (0, value*(Npixels - 1)));
  PrintIOBuffer (buffer, "%4.2f %4.2f %4.2f rg\n", pixel1[pixel], pixel2[pixel], pixel3[pixel]);
  PrintIOBuffer (buffer, "%4.2f %4.2f %4.2f RG\n", pixel1[pixel], pixel2[pixel], pixel3[pixel]);
  return;
}

// http://spencermortensen.com/articles/bezier-circle/
/** OK for PDF **/
void PDF_DrawCircle (IOBuffer *buffer, float Xc, float Yc, float R, int isFill) {
  PrintIOBuffer (buffer, " %6.2f %6.2f m\n", Xc, Yc + R);

  PrintIOBuffer (buffer, " %6.2f %6.2f ",    Xc + R*BEZIER_C, Yc + R);
  PrintIOBuffer (buffer, " %6.2f %6.2f ",    Xc + R,          Yc + R*BEZIER_C);
  PrintIOBuffer (buffer, " %6.2f %6.2f c\n", Xc + R,          Yc);

  PrintIOBuffer (buffer, " %6.2f %6.2f ",    Xc + R,          Yc - R*BEZIER_C);
  PrintIOBuffer (buffer, " %6.2f %6.2f ",    Xc + R*BEZIER_C, Yc - R);
  PrintIOBuffer (buffer, " %6.2f %6.2f c\n", Xc,              Yc - R);

  PrintIOBuffer (buffer, " %6.2f %6.2f ",    Xc - R*BEZIER_C, Yc - R);
  PrintIOBuffer (buffer, " %6.2f %6.2f ",    Xc - R,          Yc - R*BEZIER_C);
  PrintIOBuffer (buffer, " %6.2f %6.2f c\n", Xc - R,          Yc);

  PrintIOBuffer (buffer, " %6.2f %6.2f ",    Xc - R,          Yc + R*BEZIER_C);
  PrintIOBuffer (buffer, " %6.2f %6.2f ",    Xc - R*BEZIER_C, Yc + R);
  PrintIOBuffer (buffer, " %6.2f %6.2f c\n", Xc,              Yc + R);

  if (isFill) {
    PrintIOBuffer (buffer, " f\n");
  } else {
    PrintIOBuffer (buffer, " S\n");
  }    
}

void PDF_Bars (KapaGraphWidget *graph, Gobjects *object, IOBuffer *buffer, int mode);

/* DrawRectangle & FillRectangle : take rectangle lower-left corner and widths 
   DrawCircle & FillCircle : take circle center and radius
*/

# define DrawLine(X1,Y1,X2,Y2)    (PrintIOBuffer (buffer, " %6.2f %6.2f m %6.2f %6.2f l S\n", X1, (graphic->dy - Y1), X2, (graphic->dy - Y2)))
# define DrawRectangle(X,Y,dX,dY) (PrintIOBuffer (buffer, " %6.2f %6.2f %6.2f %6.2f re S\n", (X), (graphic->dy-Y), (dX), (dY)))
# define FillRectangle(X,Y,dX,dY) (PrintIOBuffer (buffer, " %6.2f %6.2f %6.2f %6.2f re f\n", (X), (graphic->dy-Y), (dX), (dY)))

# define DrawCircle(X1,Y1,R) {PDF_DrawCircle(buffer, X1, graphic->dy - Y1, R, FALSE);}
# define FillCircle(X1,Y1,R) {PDF_DrawCircle(buffer, X1, graphic->dy - Y1, R, TRUE); }

# define FillTriangle(X1,Y1,X2,Y2, X3, Y3) (PrintIOBuffer (buffer, " %6.2f %6.2f m %6.2f %6.2f l %6.2f %6.2f l f\n", (X1), (graphic->dy-Y1), (X2), (graphic->dy-Y2), (X3), (graphic->dy-Y3)))

# define CAPSTYLE 1 /* CapButt */
# define JOINSTYLE 0 /* JoinMiter */

// XXX this is not thread safe, but that is OK
static Graphic *graphic;

/** OK for PDF **/
int PDF_Objects (KapaGraphWidget *graph, IOBuffer *buffer) {
  
  int i;
  
  // the functions below use this global value
  graphic = GetGraphic();

  // this function calls all of the supporting Draw... functions below
  for (i = 0; i < graph[0].Nobjects; i++) {
    PDF_ObjectsN (graph, &graph[0].objects[i], buffer);
  }
  // reset to default color and style
  // PrintIOBuffer (buffer, "[] 0 d\n");
  // PrintIOBuffer (buffer, "0.00 0.00 0.00 rg\n");
  // PrintIOBuffer (buffer, "0.00 0.00 0.00 RG\n");

  return (TRUE);
}

/** OK for PDF **/
void PDF_LineStyle (KapaGraphWidget *graph, Gobjects *object, IOBuffer *buffer) {

  static char short_dash[] = "4 4";
  static char long_dash[] = "8 8";
  static char dot_dash[] = "2 4 4 4";
  static char dot[] = "2 3";
  
  PDF_SetLineWeight (buffer, object->lweight);
  // PrintIOBuffer (buffer, "%d setlinecap %d setlinejoin\n", CAPSTYLE, JOINSTYLE);

  switch (object->ltype) {
    case KAPA_LINE_DOT:
      PrintIOBuffer (buffer, "[%s] 0 d\n", dot);
      break;
    case KAPA_LINE_DASH_SHORT:
      PrintIOBuffer (buffer, "[%s] 0 d\n", short_dash);
      break;
    case KAPA_LINE_DASH_LONG: 
      PrintIOBuffer (buffer, "[%s] 0 d\n", long_dash);
      break;
    case KAPA_LINE_DOT_DASH:
      PrintIOBuffer (buffer, "[%s] 0 d\n", dot_dash);
      break;
    case KAPA_LINE_SOLID: // no need to call 'd' as solid is the default
      PrintIOBuffer (buffer, "[] 0 d\n");
    default:
      break;
  }
    
  if (object->color >= 0) {
    PDF_SetKapaColor (buffer, object->color);
  }
}

/** OK for PDF **/
int PDF_ObjectsN (KapaGraphWidget *graph, Gobjects *object, IOBuffer *buffer) {
  
  // AlphaSet emits a "q" whether or not there is a non-trivial alpha value
  PDF_AlphaSet (object, buffer);
  PDF_LineStyle (graph, object, buffer);

  switch (object->style) {
    case KAPA_PLOT_CONNECT: 
      PDF_Connect (graph, object, buffer);
      break;
    case KAPA_PLOT_HISTOGRAM: 
      PDF_Histogram (graph, object, buffer);
      break;
    case KAPA_PLOT_BARS_SOLID:
      PDF_Bars (graph, object, buffer, KAPA_PLOT_BARS_SOLID);
      break;
    case KAPA_PLOT_BARS_OUTLINE:
      PDF_Bars (graph, object, buffer, KAPA_PLOT_BARS_OUTLINE);
      break;
    case KAPA_PLOT_BARS_OUTFILL:
      PDF_Bars (graph, object, buffer, KAPA_PLOT_BARS_OUTFILL);
      break;
    case KAPA_PLOT_POINTS: 
    default:
      PDF_Points (graph, object, buffer);
      break;
  }

  if (object->etype & 0x01) {
    PDF_YErrors (graph, object, buffer);
  }
  if (object->etype & 0x02) {
    PDF_XErrors (graph, object, buffer);
  }

  // this closes the graphic state generated by PDF_AlphaSet above
  PrintIOBuffer (buffer, "Q\n");

  // reset to default color and style
  // PrintIOBuffer (buffer, "[] 0 d\n");
  // PrintIOBuffer (buffer, "0.00 0.00 0.00 rg\n");
  // PrintIOBuffer (buffer, "0.00 0.00 0.00 RG\n");

  return (TRUE);
}

/** OK for PDF **/
void PDF_Connect (KapaGraphWidget *graph, Gobjects *object, IOBuffer *buffer) {
  
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
  sx0 = x[i]*mxi + y[i]*mxj + bx;
  sy0 = x[i]*myi + y[i]*myj + by;
  
  for (i++; i < object[0].Npts; i++) {
    if (!(finite(x[i]) && finite(y[i]))) continue;
    sx1 = x[i]*mxi + y[i]*mxj + bx;
    sy1 = x[i]*myi + y[i]*myj + by;
    PDF_ClipLine (sx0, sy0, sx1, sy1, X0, Y0, X1, Y1, buffer);
    /* DrawLine (sx0, sy0, sx1, sy1); */
    sx0 = sx1; sy0 = sy1;
  }
}

/** OK for PDF **/
void PDF_ClipLine (double x0, double y0, double x1, double y1, double X0, double Y1, double X1, double Y0, IOBuffer *buffer) {

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

/** OK for PDF *****/
void PDF_Histogram (KapaGraphWidget *graph, Gobjects *object, IOBuffer *buffer) {

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
  sx0 = x[i]*mxi + y[i]*mxj + bx;
  sy0 = x[i]*myi + y[i]*myj + by;
  sx0 = MIN (MAX (sx0, X0), X1);
  sy0 = MAX (MIN (sy0, Y0), Y1);
  
  /* find the second valid datapoint */
  for (i++; (i < object[0].Npts) && !(finite(x[i]) && finite(y[i])); i++);
  if (i >= object[0].Npts) return;

  /* second valid data point */
  sx1 = x[i]*mxi + y[i]*mxj + bx;
  sy1 = x[i]*myi + y[i]*myj + by;
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
    sx1 = x[i]*mxi + y[i]*mxj + bx;
    sy1 = x[i]*myi + y[i]*myj + by;
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
  double ylow = MAX(symin, symax);					\
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
  double ylow = MAX(symin, symax);					\
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
  double ylow = MAX(symin, symax);					\
  FillRectangle (sxmin, ylow, (DX_VAL), dy);				\
  PrintIOBuffer (buffer, "0.00 0.00 0.00 rg 0.00 0.00 0.00 RG\n");	\
  DrawRectangle (sxmin, ylow, (DX_VAL), dy);				\
  PDF_SetKapaColor (buffer, object->color); }

/** OK for PDF **/
void PDF_Bars (KapaGraphWidget *graph, Gobjects *object, IOBuffer *buffer, int mode) {

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
  // NOTE: Y0 > Y1 (dfy is negative)

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
  // note: Y0 > Y1 : y runs from large on bottom to small on top
  float Xaxis = by;
  Xaxis = MAX (Y1, MIN (Xaxis, Y0));

  for (int i = 0; i < object[0].Npts; i++) {
    if (!(finite(x[i]) && finite(y[i]))) continue;

    // coordinate on the screen of the point:
    double sx1r = x[i]*mxi + y[i]*mxj + bx;

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
    double sx1r = x[n]*mxi + y[n]*mxj + bx;
    double sy1r = x[n]*myi + y[n]*myj + by;

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
    double sx1r = x[n]*mxi + y[n]*mxj + bx;

    n = goodPoint[0];
    double sx0r = x[n]*mxi + y[n]*mxj + bx;
    double sy0r = x[n]*myi + y[n]*myj + by;

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
    double sx2r = x[n]*mxi + y[n]*mxj + bx;

    n = goodPoint[i - 1];
    double sx0r = x[n]*mxi + y[n]*mxj + bx;

    // below we have 2 factors of 0.5 (we want half of the average spacing)
    double dx = 0.5*0.5*object->size*(sx2r - sx0r);

    n = goodPoint[i];
    double sx1r = x[n]*mxi + y[n]*mxj + bx;
    double sy1r = x[n]*myi + y[n]*myj + by;

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
    double sx1r = x[n]*mxi + y[n]*mxj + bx;
    double sy1r = x[n]*myi + y[n]*myj + by;

    n = goodPoint[Ngood - 2];
    double sx0r = x[n]*mxi + y[n]*mxj + bx;

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

/** OK for PDF *****/
void PDF_Points (KapaGraphWidget *graph, Gobjects *object, IOBuffer *buffer) {
 
  int i;
  float *x, *y, *z;
  double mxi, mxj, myi, myj, bxi, bxj, byi, byj, bx, by;
  double sx, sy, sx1, sy1, sx2, sy2, ds, dz, D;
  float *pixel1, *pixel2, *pixel3;

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
  
  // scaled colors use the colormap defined for the graphic
  ALLOCATE (pixel1, float, graphic[0].Npixels);
  ALLOCATE (pixel2, float, graphic[0].Npixels);
  ALLOCATE (pixel3, float, graphic[0].Npixels);

  /** cmap[i].pixel must be defined even if X is not used **/
  for (i = 0; i < graphic[0].Npixels; i++) { /* set up pixel array */
    pixel1[i] = graphic[0].cmap[i].red / (float) 0xffff;
    pixel2[i] = graphic[0].cmap[i].green / (float) 0xffff;
    pixel3[i] = graphic[0].cmap[i].blue / (float) 0xffff;
  }

  /**** point sizes are scaled by object.size, colors by object.color ***/
  int scaleSize = (object[0].size < 0);
  int scaleColor = (object[0].color < 0);

  ds = 0.5 * (graphic->dx + graphic->dy) * 0.003 * object[0].size;
  dz = 0.5 * (graphic->dx + graphic->dy) * 0.010;
  x = object[0].x; y = object[0].y; z = object[0].z;

  switch (object[0].ptype) {
    case KAPA_POINT_BOX_OPEN:	/* open box */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawRectangle (sx - D, sy - D, 2*D, 2*D);
	}
      }
      break;
    case KAPA_POINT_CROSS: /* cross */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx - D, sy, sx + D, sy);
	  DrawLine (sx, sy - D, sx, sy + D);
	}
      }
      break;
    case KAPA_POINT_X:	/* x */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx + D, sy - D, sx - D, sy + D);
	  DrawLine (sx - D, sy - D, sx + D, sy + D);
	}
      }
      break;
    case KAPA_POINT_TRIANGLE_SOLID:	/* filled triangle */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  FillTriangle (sx - D, sy - 0.58*D, sx + D, sy - 0.58*D, sx, sy + 1.15*D);
	}
      }
      break;
    case KAPA_POINT_TRIANGLE_SOLID_DOWN:	/* open triangle */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  FillTriangle (sx - D, sy + 0.58*D, sx + D, sy + 0.58*D, sx, sy - 1.15*D);
	}
      }
      break;
    case KAPA_POINT_TRIANGLE_OPEN:	/* open triangle */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx - D, sy - 0.58*D, sx + D, sy - 0.58*D);
	  DrawLine (sx + D, sy - 0.58*D, sx,     sy + 1.15*D);
	  DrawLine (sx,     sy + 1.15*D, sx - D, sy - 0.58*D);
	}
      }
      break;
    case KAPA_POINT_TRIANGLE_OPEN_DOWN:	/* upside-down open triangle */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx - D, sy + 0.58*D, sx + D, sy + 0.58*D);
	  DrawLine (sx + D, sy + 0.58*D, sx,     sy - 1.15*D);
	  DrawLine (sx,     sy - 1.15*D, sx - D, sy + 0.58*D);
	}
      }
      break;
    case KAPA_POINT_Y:	/* Y */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx, sy, sx - D, sy + 0.58*D);
	  DrawLine (sx, sy, sx + D, sy + 0.58*D);
	  DrawLine (sx, sy, sx,          sy - 1.15*D);
	}
      }
      break;
    case KAPA_POINT_Y_DOWN:	/* upside-down Y */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx, sy, sx - D, sy - 0.58*D);
	  DrawLine (sx, sy, sx + D, sy - 0.58*D);
	  DrawLine (sx, sy, sx,     sy + 1.15*D);
	}
      }
      break;
    case KAPA_POINT_CIRCLE_OPEN: /* 0 */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawCircle (sx, sy, D);
	}
      }
      break;
    case KAPA_POINT_CIRCLE_SOLID: /* filled 0 */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  FillCircle (sx, sy, D);
	}
      }
      break;
    case KAPA_POINT_PENTAGON:	/* pentagon */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx + 0.00*D, sy + 1.00*D, sx + 0.95*D, sy + 0.31*D);
	  DrawLine (sx + 0.95*D, sy + 0.31*D, sx + 0.58*D, sy - 0.81*D);
	  DrawLine (sx + 0.58*D, sy - 0.81*D, sx - 0.58*D, sy - 0.81*D);
	  DrawLine (sx - 0.58*D, sy - 0.81*D, sx - 0.95*D, sy + 0.31*D);
	  DrawLine (sx - 0.95*D, sy + 0.31*D, sx + 0.00*D, sy + 1.00*D);
	}
      }
      break;
    case KAPA_POINT_HEXAGON:	/* hexagon */
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  DrawLine (sx -      D, sy,               sx - 0.50*D, sy + 0.87*D);
	  DrawLine (sx - 0.50*D, sy + 0.87*D, sx + 0.50*D, sy + 0.87*D);
	  DrawLine (sx + 0.50*D, sy + 0.87*D, sx +      D, sy);

	  DrawLine (sx +      D, sy,               sx + 0.50*D, sy - 0.87*D);
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
	sx1 = x[i]*mxi + y[i]*mxj + bx;
	sy1 = x[i]*myi + y[i]*myj + by;
	sx2 = x[i+1]*mxi + y[i+1]*mxj + bx;
	sy2 = x[i+1]*myi + y[i+1]*myj + by;
	PDF_ClipLine (sx1, sy1, sx2, sy2, X0, Y0, X1, Y1, buffer);
      }
      break;
    }
    case KAPA_POINT_BOX_SOLID:	/* filled box */
    default:
      for (i = 0; i < object[0].Npts; i++) {
	if (!(finite(x[i]) && finite(y[i]))) continue;
	sx = x[i]*mxi + y[i]*mxj + bx;
	sy = x[i]*myi + y[i]*myj + by;
	if ((sx > graph[0].axis[0].fx) && (sx < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	    (sy < graph[0].axis[1].fy) && (sy > graph[0].axis[1].fy + graph[0].axis[1].dfy)) {
	  if (scaleColor) {
	    PDF_SetScaledColor (buffer, pixel1, pixel2, pixel3, z[i], graphic->Npixels);
	  }
	  D = scaleSize ? dz*z[i] : ds;
	  FillRectangle (sx - D, sy - D, 2*D, 2*D);
	}
      }
      break;
  }
  free (pixel1);
  free (pixel2);
  free (pixel3);
}
    
/** OK in PDF *****/
void PDF_XErrors (KapaGraphWidget *graph, Gobjects *object, IOBuffer *buffer) {
  
  int i, bar, dz, ds, D;
  float *x, *y, *z, *dxm, *dxp;
  double mxi, mxj, myi, myj, bxi, bxj, byi, byj, bx, by;
  double sx0, sy0, sx1, sy1, sz, sx10, sx11;

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
   
  double X0 = graph[0].axis[0].fx;
  double X1 = graph[0].axis[0].fx + graph[0].axis[0].dfx;
  double Y0 = graph[0].axis[1].fy;
  double Y1 = graph[0].axis[1].fy + graph[0].axis[1].dfy;

  /// XXX NOTE : D should be modified by (mxi,myi) for tilted axes dx = D*(mxi/mx), dy = D*(myi/mx)

  for (i = 0; i < object[0].Npts; i++) {
    // for open circles, only go to the outer radius
    D = 0;
    if (object[0].ptype ==  7) { D = scaleSize ? dz*z[i] : ds; }
    if (object[0].ptype ==  1) { D = scaleSize ? dz*z[i] : ds; }
    if (object[0].ptype ==  5) { D = scaleSize ? 0.66*dz*z[i] : 0.66*ds; }
    if (object[0].ptype == 15) { D = scaleSize ? 0.66*dz*z[i] : 0.66*ds; }
    if (!(finite(x[i]) && finite(y[i]) && finite(dxp[i]))) goto skip_dxp;
    if (D > fabs(dxp[i]*mxi)) goto skip_dxp;
    sx0 = x[i]*mxi + y[i]*mxj + bx + D;
    sy0 = x[i]*myi + y[i]*myj + by;
    sx1 = sx0 + dxp[i]*mxi - D;
    sy1 = sy0 + dxp[i]*myi;
    if (((sx0 > graph[0].axis[0].fx) && (sx0 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy0 < graph[0].axis[1].fy) && (sy0 > graph[0].axis[1].fy + graph[0].axis[1].dfy)) ||
	((sx1 > graph[0].axis[0].fx) && (sx1 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy1 < graph[0].axis[1].fy) && (sy1 > graph[0].axis[1].fy + graph[0].axis[1].dfy)))
      {
	PDF_ClipLine (sx0, sy0, sx1, sy1, X0, Y0, X1, Y1, buffer);
	if (bar) {
	  sx10 = sy1 - sz;
	  sx11 = sy1 + sz;
	  PDF_ClipLine (sx1, sx10, sx1, sx11, X0, Y0, X1, Y1, buffer);
	}
      }
  skip_dxp:
    if (!(finite(x[i]) && finite(y[i]) && finite(dxm[i]))) continue;
    if (D > fabs(dxm[i]*mxi)) continue;
    sx0 = x[i]*mxi + y[i]*mxj + bx - D;
    sy0 = x[i]*myi + y[i]*myj + by;
    sx1 = sx0 - dxm[i]*mxi + D;
    sy1 = sy0 - dxm[i]*myi;
    if (((sx0 > graph[0].axis[0].fx) && (sx0 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy0 < graph[0].axis[1].fy) && (sy0 > graph[0].axis[1].fy + graph[0].axis[1].dfy)) ||
	((sx1 > graph[0].axis[0].fx) && (sx1 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy1 < graph[0].axis[1].fy) && (sy1 > graph[0].axis[1].fy + graph[0].axis[1].dfy)))
      {
	PDF_ClipLine (sx0, sy0, sx1, sy1, X0, Y0, X1, Y1, buffer);
	if (bar) {
	  sx10 = sy1 - sz;
	  sx11 = sy1 + sz;
	  PDF_ClipLine (sx1, sx10, sx1, sx11, X0, Y0, X1, Y1, buffer);
	}
      }
  }
}

    
/** OK in PDF *****/
void PDF_YErrors (KapaGraphWidget *graph, Gobjects *object, IOBuffer *buffer) {
  
  int i, bar, dz, ds, D;
  float *x, *y, *z, *dym, *dyp;
  double mxi, mxj, myi, myj, bxi, bxj, byi, byj, bx, by;
  double sx0, sy0, sx1, sy1, sz, sx10, sx11;
  
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
  
  double X0 = graph[0].axis[0].fx;
  double X1 = graph[0].axis[0].fx + graph[0].axis[0].dfx;
  double Y0 = graph[0].axis[1].fy;
  double Y1 = graph[0].axis[1].fy + graph[0].axis[1].dfy;

  /// XXX NOTE : D should be modified by (mxi,myi) for tilted axes dx = D*(mxi/mx), dy = D*(myi/mx)

  for (i = 0; i < object[0].Npts; i++) {
    // for open circles, only go to the outer radius
    D = 0;
    if (object[0].ptype ==  7) { D = scaleSize ? dz*z[i] : ds; }
    if (object[0].ptype ==  1) { D = scaleSize ? dz*z[i] : ds; }
    if (object[0].ptype ==  5) { D = scaleSize ? 1.15*dz*z[i] : 1.15*ds; }
    if (object[0].ptype == 15) { D = scaleSize ? 0.58*dz*z[i] : 0.58*ds; }
    if (!(finite(x[i]) && finite(y[i]) && finite(dyp[i]))) goto skip_dyp;
    if (D > fabs(dyp[i]*myj)) goto skip_dyp;
    sx0 = x[i]*mxi + y[i]*mxj + bx;
    sy0 = x[i]*myi + y[i]*myj + by - D;
    sx1 = sx0 + dyp[i]*mxj;
    sy1 = sy0 + dyp[i]*myj + D;
    if (((sx0 > graph[0].axis[0].fx) && (sx0 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy0 < graph[0].axis[1].fy) && (sy0 > graph[0].axis[1].fy + graph[0].axis[1].dfy)) ||
	((sx1 > graph[0].axis[0].fx) && (sx1 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy1 < graph[0].axis[1].fy) && (sy1 > graph[0].axis[1].fy + graph[0].axis[1].dfy)))
      {
	PDF_ClipLine (sx0, sy0, sx1, sy1, X0, Y0, X1, Y1, buffer);
	if (bar) {
	  sx10 = sx1 - sz;
	  sx11 = sx1 + sz;
	  PDF_ClipLine (sx10, sy1, sx11, sy1, X0, Y0, X1, Y1, buffer);
	}
      }
  skip_dyp:
    if (!(finite(x[i]) && finite(y[i]) && finite(dym[i]))) continue;
    if (object[0].ptype ==  5) { D = scaleSize ? 0.58*dz*z[i] : 0.58*ds; }
    if (object[0].ptype == 15) { D = scaleSize ? 1.15*dz*z[i] : 1.15*ds; }
    if (D > fabs(dym[i]*myj)) continue;
    sx0 = x[i]*mxi + y[i]*mxj + bx;
    sy0 = x[i]*myi + y[i]*myj + by + D;
    sx1 = sx0 - dym[i]*mxj;
    sy1 = sy0 - dym[i]*myj - D;
    if (((sx0 > graph[0].axis[0].fx) && (sx0 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy0 < graph[0].axis[1].fy) && (sy0 > graph[0].axis[1].fy + graph[0].axis[1].dfy)) ||
	((sx1 > graph[0].axis[0].fx) && (sx1 < graph[0].axis[0].fx + graph[0].axis[0].dfx) &&
	 (sy1 < graph[0].axis[1].fy) && (sy1 > graph[0].axis[1].fy + graph[0].axis[1].dfy)))
      {
	PDF_ClipLine (sx0, sy0, sx1, sy1, X0, Y0, X1, Y1, buffer);
	if (bar) {
	  sx10 = sx1 - sz;
	  sx11 = sx1 + sz;
	  PDF_ClipLine (sx10, sy1, sx11, sy1, X0, Y0, X1, Y1, buffer);
	}
      }
  }
}
