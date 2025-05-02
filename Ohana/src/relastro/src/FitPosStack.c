# include "relastro.h"

// find the median of the positions and the error associated with the median
int FitPosStack (FitAstromResult *fit, FitStats *fitstats) {

  int i;

  // Convert the measurement errors into initial weights.
  for (i = 0; i < fitstats->Npoints; i++) {
    fitstats-> Xstack[i] = fitstats->points[i]. X;
    fitstats->dXstack[i] = fitstats->points[i].dX;
    fitstats-> Ystack[i] = fitstats->points[i]. Y;
    fitstats->dYstack[i] = fitstats->points[i].dY;
  }

  dsortpair (fitstats->Xstack, fitstats->dXstack, fitstats->Npoints);
  dsortpair (fitstats->Ystack, fitstats->dYstack, fitstats->Npoints);
  int N = (int)(fitstats->Npoints / 2);
  if (fitstats->Npoints % 2) {
    fit->Ro  = fitstats-> Xstack[N];
    fit->dRo = fitstats->dXstack[N];
    fit->Do  = fitstats-> Ystack[N];
    fit->dDo = fitstats->dYstack[N];
  } else {
    fit->Ro  = 0.5*(fitstats-> Xstack[N] + fitstats-> Xstack[N+1]);
    fit->dRo = 0.5*(fitstats->dXstack[N] + fitstats->dXstack[N+1]);
    fit->Do  = 0.5*(fitstats-> Ystack[N] + fitstats-> Ystack[N+1]);
    fit->dDo = 0.5*(fitstats->dYstack[N] + fitstats->dYstack[N+1]);
  }

  return TRUE;
}

