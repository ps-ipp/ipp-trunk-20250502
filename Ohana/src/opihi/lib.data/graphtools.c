# include "data.h"

// XXX need to select the active vector for the range analysis
// this function accepts either FLT or INT vectors
void SetLimits (Vector *xvec, Vector *yvec, Graphdata *graphmode) {

  double maxX, minX, maxY, minY, range;
  int i;

  if (xvec != NULL) {
    if (xvec->type == OPIHI_FLT) {
      maxX = -DBL_MAX;
      minX = DBL_MAX;
      for (i = 0; i < xvec[0].Nelements; i++) {
	if (!finite(xvec[0].elements.Flt[i])) continue;
	maxX = MAX (maxX, xvec[0].elements.Flt[i]);
	minX = MIN (minX, xvec[0].elements.Flt[i]);
      }
    } else {
      maxX = minX = xvec[0].elements.Int[0];
      for (i = 1; i < xvec[0].Nelements; i++) {
	maxX = MAX (maxX, xvec[0].elements.Int[i]);
	minX = MIN (minX, xvec[0].elements.Int[i]);
      }
    }
    range = maxX - minX;
    if (range == 0) range = 0.001 * maxX;
    if (range == 0) range = 0.001;
    graphmode[0].xmin = minX - 0.05*range;
    graphmode[0].xmax = maxX + 0.05*range;
  }

  if (yvec != NULL) {
    if (yvec->type == OPIHI_FLT) {
      maxY = -DBL_MAX;
      minY = DBL_MAX;
      for (i = 0; i < yvec[0].Nelements; i++) {
	if (!finite(yvec[0].elements.Flt[i])) continue;
	maxY = MAX (maxY, yvec[0].elements.Flt[i]);
	minY = MIN (minY, yvec[0].elements.Flt[i]);
      }
    } else {
      maxY = minY = yvec[0].elements.Int[0];
      for (i = 1; i < yvec[0].Nelements; i++) {
	maxY = MAX (maxY, yvec[0].elements.Int[i]);
	minY = MIN (minY, yvec[0].elements.Int[i]);
      }
    }
    range = maxY - minY;
    if (range == 0) range = 0.0011 * maxY;
    if (range == 0) range = 0.0011;
    graphmode[0].ymin = minY - 0.05*range;
    graphmode[0].ymax = maxY + 0.05*range;
  }
  SetGraph (graphmode);

  set_variable ("XMIN", graphmode[0].xmin);
  set_variable ("XMAX", graphmode[0].xmax);
  set_variable ("YMIN", graphmode[0].ymin);
  set_variable ("YMAX", graphmode[0].ymax);

  set_variable ("KAPA_XMIN", graphmode[0].xmin);
  set_variable ("KAPA_XMAX", graphmode[0].xmax);
  set_variable ("KAPA_YMIN", graphmode[0].ymin);
  set_variable ("KAPA_YMAX", graphmode[0].ymax);
}

void SetLimitsRaw (float *xvec, float *yvec, int Nelements, Graphdata *graphmode) {

  double maxX, minX, maxY, minY, range;
  int i;

  if (xvec != NULL) {
    maxX = minX = xvec[0];
    for (i = 1; i < Nelements; i++) {
      if (!finite(xvec[i])) continue;
      maxX = MAX (maxX, xvec[i]);
      minX = MIN (minX, xvec[i]);
    }
    range = maxX - minX;
    if (range == 0) range = 0.001 * maxX;
    if (range == 0) range = 0.001;
    graphmode[0].xmin = minX - 0.05*range;
    graphmode[0].xmax = maxX + 0.05*range;
  }

  if (yvec != NULL) {
    maxY = minY = yvec[0];
    for (i = 1; i < Nelements; i++) {
      if (!finite(yvec[i])) continue;
      maxY = MAX (maxY, yvec[i]);
      minY = MIN (minY, yvec[i]);
    }
    range = maxY - minY;
    if (range == 0) range = 0.0011 * maxY;
    if (range == 0) range = 0.0011;
    graphmode[0].ymin = minY - 0.05*range;
    graphmode[0].ymax = maxY + 0.05*range;
  }
  SetGraph (graphmode);

  set_variable ("KAPA_XMIN", graphmode[0].xmin);
  set_variable ("KAPA_XMAX", graphmode[0].xmax);
  set_variable ("KAPA_YMIN", graphmode[0].ymin);
  set_variable ("KAPA_YMAX", graphmode[0].ymax);

  set_variable ("XMIN", graphmode[0].xmin);
  set_variable ("XMAX", graphmode[0].xmax);
  set_variable ("YMIN", graphmode[0].ymin);
  set_variable ("YMAX", graphmode[0].ymax);
}
