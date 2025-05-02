# include "Ximage.h"
# define DrawLine(X1,Y1,DX,DY) (fprintf (f, " %6.2f %6.2f %6.2f %6.2f L\n", X1, Y1, X1+DX, Y1+DY))

int PSFrame (KapaGraphWidget *graph, FILE *f) {
  
  int i, j, Nticks, doffset;
  double fx, fy, dfx, dfy, dx = 0, dy = 0, lweight;
  Graphic *graphic;
  TickMarkData *ticks;
  bDrawColor color;

  graphic = GetGraphic();

  // P = 0.5 * (1 + 0.25*graph[0].axis[0].lweight) * (hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy) + hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy));

  int Px = 0.5 * (1 + 0.25*graph[0].axis[0].lweight) * (hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy) + hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy));
  int Py = 0.5 * (1 + 0.25*graph[0].axis[1].lweight) * (hypot (graph[0].axis[1].dfx, graph[0].axis[1].dfy) + hypot (graph[0].axis[1].dfx, graph[0].axis[1].dfy));
  int P = MIN (Px, Py);

  /* each axis is drawn independently */
  fprintf (f, "1 setlinewidth\n");
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
    fy  = graphic->dy - graph[0].axis[i].fy - dy;
    dfx = graph[0].axis[i].dfx + 2*dx;
    dfy = -graph[0].axis[i].dfy + 2*dy;

    // P = hypot (graph[0].axis[(i+1)%2].dfx, graph[0].axis[(i+1)%2].dfy);
    // P *= (1 + 0.25*lweight);

    fprintf (f, "%.1f setlinewidth\n", lweight);
    fprintf (f, "%s setrgbcolor\n", KapaColorRGBString(color));
    // no need to init rot font

    if (graph[0].axis[i].isaxis) { 
      DrawLine (fx, fy, dfx, dfy); 
    }
    
    if (graph[0].axis[i].areticks) {
      ticks = CreateAxisTicks (&graph[0].axis[i], &Nticks);
      for (j = 0; j < Nticks; j++) {
	PSTick (graphic, &graph[0].axis[i], P, &ticks[j], i, f);
      }
      FREE (ticks);
    }      
  }
  return (TRUE);
}

void PSTick (Graphic *graphic, Axis *axis, int P, TickMarkData *tick, int naxis, FILE *f) {
  
  double x, y, dx, dy;
  int pos, dir, fontsize;
  double size, pad;
  char string[64];

  double fx  = axis->fx;
  double fy  = graphic->dy - axis->fy;
  double dfx = axis->dfx;
  double dfy = -axis->dfy;

  double min = axis->min;
  double max = axis->max;

  double value = tick->value;

  pos = size = 0;

  if (tick->IsMajor) { 
    size = MIN (0.50*P, MAX (0.030 * P, 10.0)); 
  } else {
    size = MIN (0.25*P, MAX (0.015 * P, 5.0)); 
  }
  dir = ((naxis == 2) || (naxis == 3)) ? -1 : +1;

  x = fx + (value-min)*dfx/(max - min);
  y = fy + (value-min)*dfy/(max - min);

  dx = 0;
  dy = 0;

  if ((naxis == 0) || (naxis == 2)) {
    dx = 0;
    dy = dir*size;
    x = MIN(MAX(x, fx),fx+dfx);
  }
  if ((naxis == 1) || (naxis == 3)) {
    dx = dir*size;
    dy = 0;
    y = MIN(MAX(y, fy),fy+dfy);
  }
  
  DrawLine (x, y, dx, dy);

  if (tick->IsLabel) {
    int xt, yt;

    // char *fontname is returned by not needed
    GetRotFont (&fontsize);

    pad = !isnan(axis->ticktextPad) ? axis->ticktextPad*fontsize : fontsize + 4.0;
    // pad = !isnan(axis->ticktextPad) ? axis->ticktextPad*fontsize : 0.8*fontsize + 1.0;
    
    /* temporarily assume rectilinear axes */
    if (naxis == 0) { dx = 0; dy = -pad; pos = 1; }
    if (naxis == 2) { dx = 0; dy = +pad; pos = 7; }

    if (naxis == 1) { dy = 0; dx = -pad; pos = 3; }
    if (naxis == 3) { dy = 0; dx = +pad; pos = 5; }

    xt = fx + (value-min)*dfx/(max - min) + dx;
    yt = fy + (value-min)*dfy/(max - min) + dy;

    PrintTick (string, tick, min, max);
    PSRotText (f, xt, yt, string, pos, 0.0);
  }
}

  /* 
  dx = size*dfy*n;	
  dy = size*dfx*n;
  DrawLine (x, y, dx, dy);
  
  dx = -size*dfy*n;	
  dy = -size*dfx*n;
  DrawLine (x, y, dx, dy);
  */
  
# if (0)
void PSTick (Graphic *graphic, Axis *axis, int P, TickMarkData *tick, int naxis, FILE *f) {
  
  double x, y, dx, dy;
  int pos, dir, fontsize;
  double size, n, pad;
  char string[64];

  double fx  = axis->fx;
  double fy  = graphic->dy - axis->fy;
  double dfx = axis->dfx;
  double dfy = -axis->dfy;

  double min = axis->min;
  double max = axis->max;

  double value = tick->value;

  pos = size = 0;

  if (tick->IsMajor) { 
    size = MAX (0.02, 7.0 / P); 
  } else {
    size = MAX (0.01, 4.0 / P); 
  }
  
  n = P / hypot(dfx, dfy);
  x = fx + (value-min)*dfx/(max - min);
  y = fy + (value-min)*dfy/(max - min);

  if ((naxis == 0) || (naxis == 2)) {
    x = MIN(MAX(x, fx),fx+dfx);
  }
  if ((naxis == 1) || (naxis == 3)) {
    y = MIN(MAX(y, fy),fy+dfy);
  }

  dir = ((naxis == 0) || (naxis == 1)) ? +1 : -1;
  dx = dir*size*dfy*n;	
  dy = dir*size*dfx*n;
  
  DrawLine (x, y, dx, dy);

  if (tick->IsLabel) {
    int xt, yt;

    // char *fontname is returned by not needed
    GetRotFont (&fontsize);

    pad = !isnan(axis->ticktextPad) ? axis->ticktextPad*fontsize : 0.8*fontsize + 1.0;
    
    /* temporarily assume rectilinear axes */
    if (naxis == 0) { dx = 0; dy = -pad; pos = 1; }
    if (naxis == 2) { dx = 0; dy = +pad; pos = 7; }

    if (naxis == 1) { dy = 0; dx = -pad; pos = 3; }
    if (naxis == 3) { dy = 0; dx = +pad; pos = 5; }

    xt = fx + (value-min)*dfx/(max - min) + dx;
    yt = fy + (value-min)*dfy/(max - min) + dy;

    PrintTick (string, tick, min, max);
    PSRotText (f, xt, yt, string, pos, 0.0);
  }
}
# endif

