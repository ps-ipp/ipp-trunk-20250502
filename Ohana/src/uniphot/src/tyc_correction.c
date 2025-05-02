# include "setastrom.h"

static int Ntycho = 0;
static double *tychoR = NULL;
static double *tychoD = NULL;

// the tycho correction table just lists the tycho stars (R,D) for which the astrometry was messed up
int load_tyc_correction (char *filename) {

  FILE *f = fopen (filename, "r");
  if (!f) myAbort ("file not found");

  double *tychoR, *tychoD;

  int NTYCHO = 10000;

  ALLOCATE (tychoR, double, NTYCHO);
  ALLOCATE (tychoD, double, NTYCHO);

  while (fscanf (f, "%lf %lf", &tychoR[Ntycho], &tychoD[Ntycho]) != EOF) {
    Ntycho ++;

    if (Ntycho >= NTYCHO) {
      NTYCHO += 10000;
      REALLOCATE (tychoR, double, NTYCHO);
      REALLOCATE (tychoD, double, NTYCHO);
    }
  }

  dsortpair (tychoR, tychoD, Ntycho); 

  return TRUE;
}

int get_tyc_correction (double **R, double **D, int *N) {

  *R = NULL;
  *D = NULL;
  *N = 0;

  if (!tychoR) return FALSE;

  *R = tychoR;
  *D = tychoD;
  *N = Ntycho;
  return TRUE;
}

