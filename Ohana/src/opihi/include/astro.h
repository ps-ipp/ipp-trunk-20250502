# include "external.h"
# include "shell.h"
# include "dvomath.h"
# include "convert.h"
# include "display.h"
# include "data.h"

# ifndef ASTRO_H
# define ASTRO_H

void InitAstro (void);
void FreeAstro (void);

typedef struct {
  double *X;
  double *Y;
  double *t;
  double *pX;
  double *pY;
  double *dX;
  double *dY;
  double *Wx; // modified weight based on distance from fit
  double *Wy; // modified weight based on distance from fit
  double *Qx; // raw error-based weight (1/dX^2)
  double *Qy;
  double *qx; // modified error-based weight (1/dX^2) * Wx
  double *qy;
  int *index;
  int Npts;
} PlxFitData;

typedef struct {
  double Ro, dRo;
  double Do, dDo;

  double uR, duR;
  double uD, duD;

  double p, dp;

  double chisq;
  int Nfit;
  int getChisq;
} PlxFit;

int VectorRobustStats (Vector *vector, double *median, double *sigma);
double VectorFractionInterpolate (double *values, float fraction, int Npts);

int PlxSetMeanEpoch (double *R, double *D, double *T, double *Rmean, double *Dmean, double *Tmean, opihi_int *mask, int Ntotal);
int PlxSetEpochPosition (PlxFitData *fitdata, double *R, double *D, double *dR, double *dD, double *T, opihi_int *mask, int Ntotal, Coords *coords, double Tmean);
int PlxOutlierClip (PlxFitData *fitdata, opihi_int *mask, int Noutlier, float dPsigMax, Vector *dPvec, int VERBOSE);

int PlxFitDataAlloc (PlxFitData *data, int N);
void PlxFitDataFree (PlxFitData *data);
int PlxBootstrapResample (PlxFitData *src, PlxFitData *tgt);

int FitPMonly (PlxFit *fit, double *X, double *dX, double *Y, double *dY, double *T, int Npts, int VERBOSE);
int FitPMandPar (PlxFit *fit, double *X, double *dX, double *Y, double *dY, double *T, double *pR, double *pD, int Npts, int VERBOSE);
int sun_ecliptic (double mjd, double *lambda, double *beta, double *epsilon, double *Radius);
int ParFactor (double *pR, double *pD, double RA, double DEC, double Time);
int ParFactor_3d (double *pR, double *pD, double *pZ, double RA, double DEC, double Time);

/***** */

int FitPMonly_IRLS (PlxFit *fit, double *X, double *dX, double *Y, double *dY, double *T, int Npts, double binning_step, int VERBOSE);
int FitPMandPar_IRLS (PlxFit *fit, double *X, double *dX, double *Y, double *dY, double *T, double *pR, double *pD, double *Wx, double *Wy, double *Qx, double *Qy, double *qx, double *qy, int Npts, int max_iterations, double outlier_limit, double binning_step, int VERBOSE);

int weighted_LS_PLX (double *T, double *pR, double *pD, double *X, double *WX, double *Y, double *WY, int Npts, double **A, double **B, int VERBOSE);
int weighted_LS_PM (double *T, double *X, double *WX, double *Y, double *WY, int Npts, double **A, double **B, int VERBOSE);
int bin_points (double *T, double *X, double *WX, double *Y, double *WY, int Npts,
		double **Tbin, double **Xbin, double **WXbin, double **Ybin, double **WYbin, int *Nbins, double binning_step);
int bin_points_PLX (double *T, double *pR, double *pD, double *X, double *WX, double *Y, double *WY, int Npts,
		    double **Tbin, double **pRbin, double **pDbin, double **Xbin, double **WXbin, double **Ybin, double **WYbin, int *Nbins, double binning_step);


double weight_cauchy (double x);
double dpsi_cauchy (double x);
double MedianAbsDeviation(double *in, int N);

# endif
