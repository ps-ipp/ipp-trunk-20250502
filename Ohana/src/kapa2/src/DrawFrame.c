# include "Ximage.h"

# define DrawLine(X,Y,DX,DY) (XDrawLine (graphic->display, graphic->window, graphic->gc, (int)(X), (int)(Y), (int)(X+DX), (int)(Y+DY)))
  
int DrawFrame (KapaGraphWidget *graph) {
  
  int i, j, Nticks, Px, Py, doffset;
  double fx, fy, dfx, dfy, dx = 0, dy = 0, lweight;
  Graphic *graphic;
  TickMarkData *ticks;
  int color;

  graphic = GetGraphic();

  // P = 0.5 * (1 + 0.25*graph[0].axis[0].lweight) * (hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy) + hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy));
  // P ~ 1.25*(x-axis length)
  Px = 0.5 * (1 + 0.25*graph[0].axis[0].lweight) * (hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy) + hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy));
  Py = 0.5 * (1 + 0.25*graph[0].axis[1].lweight) * (hypot (graph[0].axis[1].dfx, graph[0].axis[1].dfy) + hypot (graph[0].axis[1].dfx, graph[0].axis[1].dfy));
  int P = MIN (Px, Py);
  // fprintf (stderr, "P: %d\n", P);

  /* each axis is drawn independently, but ticks and labels are placed according to perpendicular distance. */
  for (i = 0; i < 4; i++) {
    lweight = MAX (0, MIN (10, graph[0].axis[i].lweight));
    color = MAX (0, MIN (15, graph[0].axis[i].color));

    /* temporarily assume rectilinear axes */
    doffset = ((int)(lweight) % 2) ? 0.5*(lweight - 1) : 0.5*lweight;
    if (i == 0) { dx = doffset; dy = 0.0; }
    if (i == 2) { dx = doffset; dy = 0.0; }
    if (i == 1) { dx = 0.0; dy = doffset; }
    if (i == 3) { dx = 0.0; dy = doffset; }

    fx  = graph[0].axis[i].fx - dx;
    fy  = graph[0].axis[i].fy - dy;
    dfx = graph[0].axis[i].dfx + 2*dx;
    dfy = graph[0].axis[i].dfy + 2*dy;

    // P = hypot (graph[0].axis[(i+1)%2].dfx, graph[0].axis[(i+1)%2].dfy);
    // P *= (1 + 0.25*lweight);

    XSetLineAttributes (graphic->display, graphic->gc, lweight, LineSolid, CapNotLast, JoinMiter);
    XSetForeground (graphic->display, graphic->gc, graphic->color[color]);
    DrawRotTextInit (graphic->display, graphic->window, graphic->gc, graphic->color[color], graphic->back);

    if (graph[0].axis[i].isaxis) {
      DrawLine (fx, fy, dfx, dfy);
    }
    
    if (graph[0].axis[i].areticks) {
      ticks = CreateAxisTicks (&graph[0].axis[i], &Nticks);
      for (j = 0; j < Nticks; j++) {
	DrawTick (graphic, &graph[0].axis[i], P, &ticks[j], i);
      }
      FREE (ticks);
    }
  }
  return (TRUE);
}

void DrawTick (Graphic *graphic, Axis *axis, int P, TickMarkData *tick, int naxis) {
  
  double x, y, dx, dy;
  int pos, dir, fontsize;
  double size, pad;
  char string[64];

  double fx  = axis->fx;
  double fy  = axis->fy;
  double dfx = axis->dfx;
  double dfy = axis->dfy;

  double min = axis->min;
  double max = axis->max;

  double value = tick->value;

  pos = size = 0;

  if (tick->IsMajor) { 
    size = MIN (0.50*P, MAX (0.030 * P, 10.0)); 
  } else {
    size = MIN (0.25*P, MAX (0.015 * P, 5.0)); 
  }
  dir = ((naxis == 0) || (naxis == 3)) ? -1 : +1;

  x = fx + (value - min)*dfx/(max - min);
  y = fy + (value - min)*dfy/(max - min);

  dy = 0;
  dx = 0;

  if ((naxis == 0) || (naxis == 2)) {
    dx = 0;
    dy = dir*size;
    x  = MIN(MAX(x,    fx), fx+dfx);
  }
  if ((naxis == 1) || (naxis == 3)) {
    dx = dir*size;
    dy = 0;
    y  = MAX(MIN(y,    fy), fy+dfy);
  }

  DrawLine (x, y, dx, dy);

  if (tick->IsLabel) {
    int xt, yt;

    // char *fontname is returned by GetRotFont, but not used here
    GetRotFont (&fontsize);
    
    pad = !isnan(axis->ticktextPad) ? axis->ticktextPad*fontsize : fontsize + 4.0;
    // pad = !isnan(axis->ticktextPad) ? axis->ticktextPad*fontsize : 0.8*fontsize + 1.0;
    
    /* temporarily assume rectilinear axes */
    if (naxis == 0) { dx = 0; dy = +1.0*pad; pos = 1; } // center, down justified
    if (naxis == 2) { dx = 0; dy = -1.0*pad; pos = 7; } // center, up justified

    if (naxis == 1) { dy = 0; dx = -0.5*pad; pos = 3; } // left, center justified
    if (naxis == 3) { dy = 0; dx = +0.5*pad; pos = 5; } // right, center justified

    xt = fx + (value - min)*dfx/(max - min) + dx;
    yt = fy + (value - min)*dfy/(max - min) + dy;

    PrintTick (string, tick, min, max);
    DrawRotText (xt, yt, string, pos, 0.0);
  }
}

# if (0)
void DrawTick (Graphic *graphic, Axis *axis, int P, TickMarkData *tick, int naxis) {
  
  double x, y, dx, dy;
  int pos, dir, fontsize;
  double size, n, pad;
  char string[64];

  double fx  = axis->fx;
  double fy  = axis->fy;
  double dfx = axis->dfx;
  double dfy = axis->dfy;

  double min = axis->min;
  double max = axis->max;

  double value = tick->value;

  pos = size = 0;

  // size = 4 / P
  // n = P / dfx
  // dy = (4/P)*(P/dfx)*dfx = (0.01 * P) or 4

  // size = 0.01
  // n = P / dfx
  // dy - 0.01 * (P / dfx) * dfx = 0.01*P

  // n ~ 1.25 * x-axis / current-axis
  // n = P / hypot(dfx, dfy);
  n = P / hypot(dfx, dfy);

  if (tick->IsMajor) { 
    // size = MAX (0.02, 7.0 / P); 
    size = MIN (0.50*P, MAX (0.030 * P, 10.0)); 
  } else {
    // size = MAX (0.01, 4.0 / P); 
    size = MIN (0.25*P, MAX (0.015 * P, 5.0)); 
  }
  
  dir = ((naxis == 0) || (naxis == 3)) ? -1 : +1;

  // fdir ~ 1.25 * (4,7) / current-axis

  x = fx + (value-min)*dfx/(max - min);
  y = fy + (value-min)*dfy/(max - min);

  double xs, xe, ys, ye;
  if ((naxis == 0) || (naxis == 2)) {
    dx = 0;
    dy = dir*size;
    xs = MIN(MAX(x,    fx), fx+dfx);
    xe = xs;
    ys = y;
    ye = y+dy;
  }
  if ((naxis == 1) || (naxis == 3)) {
    dx = dir*size;
    dy = 0;
    xs = x;
    xe = x+dx;
    ys = MAX(MIN(y,    fy), fy+dfy);
    ye = ys;
  }

  double dxs = xe - xs;
  double dys = ye - ys;
  
  // dx ~ (4,7)*x-axis 
  // dx = dir*size*dfy*n;	
  // dy = dir*size*dfx*n;

  fprintf (stderr, "axis: %d %f : %f %d %f : %f %f : %f %f : %f %f\n",
	   naxis, value, n, P, size, dfx, dfy, dx, dy, dxs, dys);

  DrawLine (xs, ys, dxs, dys);

  if (tick->IsLabel) {
    int xt, yt;

    // char *fontname is returned by GetRotFont, but not used here
    GetRotFont (&fontsize);
    
    pad = !isnan(axis->ticktextPad) ? axis->ticktextPad*fontsize : fontsize + 4.0;
    // pad = !isnan(axis->ticktextPad) ? axis->ticktextPad*fontsize : 0.8*fontsize + 1.0;
    
    /* temporarily assume rectilinear axes */
    if (naxis == 0) { dx = 0; dy = +1.0*pad; pos = 1; } // center, down justified
    if (naxis == 2) { dx = 0; dy = -1.0*pad; pos = 7; } // center, up justified

    if (naxis == 1) { dy = 0; dx = -0.5*pad; pos = 3; } // left, center justified
    if (naxis == 3) { dy = 0; dx = +0.5*pad; pos = 5; } // right, center justified

    xt = fx + (value - min)*dfx/(max - min) + dx;
    yt = fy + (value - min)*dfy/(max - min) + dy;

    PrintTick (string, tick, min, max);
    DrawRotText (xt, yt, string, pos, 0.0);
  }
}
# endif

