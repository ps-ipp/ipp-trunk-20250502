# include "Ximage.h"
// bDrawLine is a function, not a macro like DrawLine

int bDrawFrame (bDrawBuffer *buffer, KapaGraphWidget *graph) {
  
  int i, j, Nticks, doffset;
  double fx, fy, dfx, dfy, lweight, dx = 0, dy = 0;
  // Graphic graphic; is not needed
  TickMarkData *ticks;
  bDrawColor color;

  // don't need graphic, unlink DrawFrame

  // P = 0.5 * (1 + 0.25*graph[0].axis[0].lweight) * (hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy) + hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy));
  int Px = 0.5 * (1 + 0.25*graph[0].axis[0].lweight) * (hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy) + hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy));
  int Py = 0.5 * (1 + 0.25*graph[0].axis[1].lweight) * (hypot (graph[0].axis[1].dfx, graph[0].axis[1].dfy) + hypot (graph[0].axis[1].dfx, graph[0].axis[1].dfy));
  int P = MIN (Px, Py);
  // fprintf (stderr, "P: %d\n", P);

  /* each axis is drawn independently */
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

    bDrawSetStyle (buffer, color, lweight, 0, 1.0);
    // function about sets color and weight
    // bDrawRotTextInit does not exist

    if (graph[0].axis[i].isaxis) { 
      bDrawLine (buffer, fx, fy, fx+dfx, fy+dfy); 
    }
    
    if (graph[0].axis[i].areticks) {
      ticks = CreateAxisTicks (&graph[0].axis[i], &Nticks);
      for (j = 0; j < Nticks; j++) {
	bDrawTick (buffer, &graph[0].axis[i], P, &ticks[j], i);

	// XXX DrawTick sets the style to (color, 0, 0) for the font
	// should the font weight stay the same?
	// XXX probably not needed now:
	// bDrawSetStyle (buffer, color, lweight, 0, 1.0);
      }
      FREE (ticks);
    }
  }
  return (TRUE);
}

void bDrawTick (bDrawBuffer *buffer, Axis *axis, int P, TickMarkData *tick, int naxis) {
  
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
    size = MIN (0.50*P, MAX (0.010 * P, 10.0)); 
  } else {
    size = MIN (0.25*P, MAX (0.005 * P, 5.0)); 
  }
  dir = ((naxis == 0) || (naxis == 3)) ? -1 : +1;
  
  x = fx + (value - min)*dfx/(max - min);
  y = fy + (value - min)*dfy/(max - min);

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
    y = MAX(MIN(y, fy),fy+dfy);
  }
  
  bDrawLine (buffer, x, y, x+dx, y+dy);

  if (tick->IsLabel) {
    int xt, yt;

    // char *fontname is returned but ignored
    GetRotFont (&fontsize);

    pad = !isnan(axis->ticktextPad) ? axis->ticktextPad*fontsize : 0.8*fontsize + 1.0;
    
    /* temporarily assume rectilinear axes */
    if (naxis == 0) { dx = 0; dy = +pad; pos = 1; }
    if (naxis == 2) { dx = 0; dy = -pad; pos = 7; }

    if (naxis == 1) { dy = 0; dx = -pad; pos = 3; }
    if (naxis == 3) { dy = 0; dx = +pad; pos = 5; }

    xt = fx + (value - min)*dfx/(max - min) + dx;
    yt = fy + (value - min)*dfy/(max - min) + dy;

    PrintTick (string, tick, min, max);
    bDrawRotText (buffer, xt, yt, string, pos, 0.0);
  }
}

# if (0)
void bDrawTick (bDrawBuffer *buffer, Axis *axis, int P, TickMarkData *tick, int naxis) {
  
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
    y = MAX(MIN(y, fy),fy+dfy);
  }

  dir = ((naxis == 0) || (naxis == 1)) ? -1 : +1;
  dx = dir*size*dfy*n;	
  dy = dir*size*dfx*n;
  
  bDrawLine (buffer, x, y, x+dx, y+dy);

  if (tick->IsLabel) {
    int xt, yt;

    // char *fontname is returned but ignored
    GetRotFont (&fontsize);

    pad = !isnan(axis->ticktextPad) ? axis->ticktextPad*fontsize : 0.8*fontsize + 1.0;
    
    /* temporarily assume rectilinear axes */
    if (naxis == 0) { dx = 0; dy = +pad; pos = 1; }
    if (naxis == 2) { dx = 0; dy = -pad; pos = 7; }

    if (naxis == 1) { dy = 0; dx = -pad; pos = 3; }
    if (naxis == 3) { dy = 0; dx = +pad; pos = 5; }

    xt = fx + (value-min)*dfx/(max - min) + dx;
    yt = fy + (value-min)*dfy/(max - min) + dy;

    PrintTick (string, tick, min, max);
    bDrawRotText (buffer, xt, yt, string, pos, 0.0);
  }
}

# endif
