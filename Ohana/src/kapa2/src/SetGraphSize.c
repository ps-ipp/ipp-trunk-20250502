# include "Ximage.h"

/* Set the dimensions of the specific graph based on the current window size.  The graph
   is placed within the window at the fractional position defined by the section */

void SetGraphSize (Section *section) {

  int dXm, dXp, dYm, dYp;
  double x0, y0, x1, y1;
  KapaGraphWidget *graph;
  Graphic *graphic;

  GetGraphBoundary(section, &x0, &y0, &x1, &y1, &dXm, &dXp, &dYm, &dYp);

  if (section == NULL) return;
  graph = section->graph;
  if (graph == NULL) return;

  graphic = GetGraphic ();

  double X0 = graphic[0].dx * section[0].x  + x0;
  double Y0 = graphic[0].dy * section[0].y  + y0;
  double dX = MAX(graphic[0].dx * section[0].dx - x1, 1);
  double dY = MAX(graphic[0].dy * section[0].dy - y1, 1);

  /* define locations of coordinate axes */
  graph[0].axis[0].fx  = X0;
  graph[0].axis[0].fy  = graphic->dy - Y0;
  graph[0].axis[0].dfx = dX;
  graph[0].axis[0].dfy = 0;

  graph[0].axis[1].fx  = X0;
  graph[0].axis[1].fy  = graphic->dy - Y0;
  graph[0].axis[1].dfx = 0;
  graph[0].axis[1].dfy = -dY;

  graph[0].axis[2].fx  = X0;
  graph[0].axis[2].fy  = graphic->dy - Y0 - dY;
  graph[0].axis[2].dfx = dX;
  graph[0].axis[2].dfy = 0;

  graph[0].axis[3].fx  = X0 + dX;
  graph[0].axis[3].fy  = graphic->dy - Y0;
  graph[0].axis[3].dfx = 0;
  graph[0].axis[3].dfy = -dY;

  /* define locations of axis labels */
  graph[0].label[LABELLL].x = graph[0].axis[0].fx;
  graph[0].label[LABELLL].y = graph[0].axis[0].fy + dXm;
  graph[0].label[LABELX0].x = graph[0].axis[0].fx + 0.5*graph[0].axis[0].dfx;
  graph[0].label[LABELX0].y = graph[0].axis[0].fy + dXm;
  graph[0].label[LABELLR].x = graph[0].axis[0].fx + graph[0].axis[0].dfx;
  graph[0].label[LABELLR].y = graph[0].axis[0].fy + dXm;

  graph[0].label[LABELUL].x = graph[0].axis[2].fx;
  graph[0].label[LABELUL].y = graph[0].axis[2].fy - dXp;
  graph[0].label[LABELX1].x = graph[0].axis[2].fx + 0.5*graph[0].axis[2].dfx;
  graph[0].label[LABELX1].y = graph[0].axis[2].fy - dXp;
  graph[0].label[LABELUR].x = graph[0].axis[2].fx + graph[0].axis[2].dfx;
  graph[0].label[LABELUR].y = graph[0].axis[2].fy - dXp;

  graph[0].label[LABELY0].y = graph[0].axis[1].fy + 0.5*graph[0].axis[1].dfy;
  graph[0].label[LABELY0].x = graph[0].axis[1].fx - dYm;

  graph[0].label[LABELY1].y = graph[0].axis[3].fy + 0.5*graph[0].axis[3].dfy;
  graph[0].label[LABELY1].x = graph[0].axis[3].fx + dYp;

  return;
}

int GetGraphBoundary (Section *section, double *x0, double *y0, double *x1, double *y1, int *dXm, int *dXp, int *dYm, int *dYp) {

  int i, Nticks;
  int fontsize, Nc = 0;
  int textpad, textdY, WdY;
  double padXm, padXp, padYm, padYp;
  double minPADx, maxPADx, minPADy;
  double minPAD, maxPAD;
  char string[64];
  KapaGraphWidget *graph;
  Graphic *graphic;
  TickMarkData *ticks;

  *x0 = 0.0;
  *y0 = 0.0;
  *x1 = 0.0;
  *y1 = 0.0;

  *dXm = 0;
  *dXp = 0;
  *dYm = 0;
  *dYp = 0;

  if (section == NULL) return FALSE;
  graph = section->graph;
  if (graph == NULL) return FALSE;

  graphic = GetGraphic ();
  // char *fontname;
  GetRotFont (&fontsize);

  // labelPad (min -> no labels, max -> yes labels)
  // XXX change these defaults
  minPAD = 1.0*fontsize + 0;
  maxPAD = 2.0*fontsize + 4;

  minPADx = 1.5*fontsize;
  maxPADx = 2.5*fontsize + 4.0;

  minPADy = 1.5*fontsize;

  // dXm : offset to label from lower x-axis, dYp : offset to label from right y-axis, etc
  // padXm : padding below x-axis, padYp : padding to right of y-axis, etc

  // offset from lower x-axis to labels and tick values
  // these depend on (a) existence of tick labels and (b) auto-offset or not
  if (isnan(graph[0].axis[0].labelPad)) {
    *dXm = (graph[0].axis[0].islabel) ? maxPAD  : minPAD;
  } else {
    *dXm = graph[0].axis[0].labelPad * fontsize;
  }
  if (isnan(graph[0].axis[0].pad)) {
    padXm = graph[0].axis[0].islabel ? maxPADx : minPADx;
  } else {
    padXm = graph[0].axis[0].pad * fontsize;
  }

  // offset from upper x-axis to labels and tick values
  // these depend on (a) existence of tick labels and (b) auto-offset or not
  if (isnan(graph[0].axis[2].labelPad)) {
    *dXp = (graph[0].axis[2].islabel) ? maxPAD : minPAD;
  } else {
    *dXp = graph[0].axis[2].labelPad * fontsize;
  }
  if (isnan(graph[0].axis[2].pad)) {
    padXp = graph[0].axis[2].islabel ? maxPADx : minPADx;
  } else {
    padXp = graph[0].axis[2].pad * fontsize;
  }

  // offset from left y-axis to labels and tick values
  // these depend on (a) existence of tick labels and (b) auto-offset or not.
  // in the auto-offset case, offset depends on the width of the tick label strings
  // because the y-axis labels are horizontal, the length matters
  if (graph[0].axis[1].islabel && (isnan(graph[0].axis[1].labelPad) || isnan(graph[0].axis[1].pad))) {
    /* check for the max size of the axis label */
    Nc = 0;
    ticks = CreateAxisTicks (&graph[0].axis[1], &Nticks);
    for (i = 0; i < Nticks; i++) {
      if (!ticks[i].IsMajor) continue;
      int Nchar = PrintTick (string, &ticks[i], graph[0].axis[1].min, graph[0].axis[1].max);
      Nc = MAX (Nc, Nchar);
    }
    FREE(ticks);
  }
  if (isnan(graph[0].axis[1].labelPad)) {
    *dYm = (graph[0].axis[1].islabel) ? (0.7*(Nc + 1.5)*fontsize) : minPAD;
  } else {
    *dYm = graph[0].axis[1].labelPad * fontsize;
  }
  if (isnan(graph[0].axis[1].pad)) {
    padYm = graph[0].axis[1].islabel ? (0.7*(Nc + 2.5)*fontsize) : minPADy;
  } else {
    padYm = graph[0].axis[1].pad * fontsize;
  }

  // offset from right y-axis to labels and tick values
  // these depend on (a) existence of tick labels and (b) auto-offset or not.
  // in the auto-offset case, offset depends on the width of the tick label strings
  if (graph[0].axis[3].islabel && (isnan(graph[0].axis[3].labelPad) || isnan(graph[0].axis[3].pad))) {
    Nc = 0;
    ticks = CreateAxisTicks (&graph[0].axis[3], &Nticks);
    for (i = 0; i < Nticks; i++) {
      if (!ticks[i].IsMajor) continue;
      int Nchar = PrintTick (string, &ticks[i], graph[0].axis[3].min, graph[0].axis[3].max);
      Nc = MAX (Nc, Nchar);
    }
    FREE(ticks);
  }
  if (isnan(graph[0].axis[3].labelPad)) {
    *dYp = (graph[0].axis[3].islabel) ? (0.7*(Nc + 1.5)*fontsize) : minPAD;
  } else {
    *dYp = graph[0].axis[3].labelPad * fontsize;
  }
  if (isnan(graph[0].axis[3].pad)) {
    padYp = graph[0].axis[3].islabel ? (0.7*(Nc + 2.5)*fontsize) : minPADy;
  } else {
    padYp = graph[0].axis[3].pad * fontsize;
  }

  /* basic size of the graph in Xwindow coordinates, but measured from lower-left corner */
  *x0 = padYm;
  *y0 = padXm;
  *x1 = padYm + padYp;
  *y1 = padXm + padXp;

  // if we are tied to an image, make mods as needed
  if (section->image) {
    textpad = USE_XWINDOW ? graphic[0].font[0].ascent : 10;
    textdY = 6*textpad + 7*PAD1;
    WdY = MAX (ZOOM_Y, textdY + 2*BUTTON_HEIGHT + PAD1);

    switch (section->image->location) {
      case 0:
	break;
      case 1:
        *y0 = padXm + 2*PAD1 + WdY + 2;
        *y1 = padXm + padXp + 4*PAD1 + 1 + WdY + COLORPAD;
        break;
      case 3:
        *y1 = padXm + padXp + 4*PAD1 + 1 + WdY + COLORPAD;
        break;
      case 2:
        *x0 = padYm + 2*PAD1 + ZOOM_X;
        *x1 = padYm + padYp + 3*PAD1 + ZOOM_X;
	*y1 = padXm + padXp + 2*PAD1 + COLORPAD;
        break;
      case 4:
        *x1 = padYm + padYp + 3*PAD1 + ZOOM_X;
	*y1 = padXm + padXp + 2*PAD1 + COLORPAD;
        break;
    }
  }

  return (TRUE);
}
