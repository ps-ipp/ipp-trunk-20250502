# include "Ximage.h"

// things still broken:
// text
// line weight
// line type
// polygon, polyfill not ported to PS (so missing here as well)

# define DrawLine(X1,Y1,DX,DY) (PrintIOBuffer (buffer, " %6.2f %6.2f m %6.2f %6.2f l S\n", X1, Y1, X1+DX, Y1+DY))

void PDF_Tick (Graphic *graphic, Axis *axis, int P, TickMarkData *tick, int naxis, IOBuffer *buffer);

int PDF_Frame (KapaGraphWidget *graph, IOBuffer *buffer) {
  
  int i, j, Nticks, doffset;
  double fx, fy, dfx, dfy, dx = 0, dy = 0;
  Graphic *graphic;
  TickMarkData *ticks;

  graphic = GetGraphic();

  // P = 0.5 * (1 + 0.25*graph[0].axis[0].lweight) * (hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy) + hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy));

  int Px = 0.5 * (1 + 0.25*graph[0].axis[0].lweight) * (hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy) + hypot (graph[0].axis[0].dfx, graph[0].axis[0].dfy));
  int Py = 0.5 * (1 + 0.25*graph[0].axis[1].lweight) * (hypot (graph[0].axis[1].dfx, graph[0].axis[1].dfy) + hypot (graph[0].axis[1].dfx, graph[0].axis[1].dfy));
  int P = MIN (Px, Py);

  /* each axis is drawn independently */
  PDF_SetLineWeight (buffer, 1.0);
  for (i = 0; i < 4; i++) {

    // PDF_SetLineWeight enforces LineWeight limits
    double lweight = PDF_SetLineWeight (buffer, graph[0].axis[i].lweight);
    PDF_SetKapaColor (buffer, graph[0].axis[i].color);

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

    // no need to init rot font

    if (graph[0].axis[i].isaxis) { 
      DrawLine (fx, fy, dfx, dfy); 
    }
    
    if (graph[0].axis[i].areticks) {
      ticks = CreateAxisTicks (&graph[0].axis[i], &Nticks);
      for (j = 0; j < Nticks; j++) {
	PDF_Tick (graphic, &graph[0].axis[i], P, &ticks[j], i, buffer);
      }
      FREE (ticks);
    }      
  }
  return (TRUE);
}

void PDF_Tick (Graphic *graphic, Axis *axis, int P, TickMarkData *tick, int naxis, IOBuffer *buffer) {
  
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
    size = MIN (0.50*P, MAX (0.010 * P, 10.0)); 
  } else {
    size = MIN (0.25*P, MAX (0.005 * P, 5.0)); 
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

    // PrintTick writes the characters into the string 
    PrintTick (string, tick, min, max);
    PDFRotText (buffer, xt, yt, string, pos, 0.0);
  }
}
