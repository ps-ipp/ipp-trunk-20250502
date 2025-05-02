# include "relphot.h"

// These should probably be tunable:
# define FIT_TOLERANCE 1e-4
# define FLT_TOLERANCE 1e-6
# define WEIGHT_THRESHOLD 0.3

# define IS_OLS TRUE

void fit1d_copy_results (double **srcArray, double **tgtArray, int entry, int nterm);
void get_yfit (FitDataSet *dataset, int Npoints);
int fit1d_least_squares (FitDataSet *dataset, FitDataType *data, int isOLS, int Npoints);

// 1D fit with arbitrary order.
// NOTE: the number of points considered for the fit (Npoints)
// may be different from the number of points stored in the array
int fit1d_irls (FitDataSet *dataset, int Npoints) {

  // initialize various things:
  dataset->Nmeas = 0;
  dataset->min = NAN;
  dataset->max = NAN;
  dataset->sigma = NAN;
  dataset->chisq = NAN;
  for (int i = 0; i < dataset->nterm; i++) {
    dataset->bSigma[i] = 0;
  }

  // initial fit with OLS, results in dataset->bArray
  if (!fit1d_least_squares (dataset, dataset->alldata, IS_OLS, Npoints)) return FALSE;
  
  // apply fit to data and save in yFitVector & yOffVector
  get_yfit (dataset, Npoints);

  int converged = FALSE;
  for (int iterations = 0; !converged && (iterations < dataset->MaxIterations); iterations++) {
    
    for (int i = 0; i < Npoints; i++) {
      // we are only including the formal error, not the weight in the definition of wt[]
      dataset->wtIRLS[i] = weight_cauchy (dataset->yOffVector[i] / dataset->alldata->dyVector[i]);
    }
    
    // save this solution to entry 0
    fit1d_copy_results (dataset->bArray, dataset->bSaveArray, 0, dataset->nterm);

    // recalculate the least squares, but now apply the modified weight above (arg 3 = isOLS)
    if (!fit1d_least_squares (dataset, dataset->alldata, !IS_OLS, Npoints)) {
      // restore the saved solution
      fit1d_copy_results (dataset->bSaveArray, dataset->bArray, 0, dataset->nterm);
      break;
    }
    
    // apply fit to data and save in yFitVector & yOffVector (used above to calculate wtIRLS
    get_yfit (dataset, Npoints);

    // has the fit changed in a meaningful way?
    converged = TRUE;
    for (int i = 0; i < dataset->nterm; i++) {
      if ((fabs(dataset->bArray[i][0] - dataset->bSaveArray[i][0]) > FIT_TOLERANCE * fabs(dataset->bArray[i][0])) && 
	  (fabs(dataset->bArray[i][0] - dataset->bSaveArray[i][0]) > FLT_TOLERANCE))
	converged = FALSE;
    }
  }

  // save this solution to entry 0
  fit1d_copy_results (dataset->bArray, dataset->bSaveArray, 0, dataset->nterm);
      
  // calculate the weight thresholds to mask the bad points:
  for (int i = 0; i < Npoints; i++) {
    dataset->wtIRLS[i] = weight_cauchy (dataset->yOffVector[i] / dataset->alldata->dyVector[i]);
    dataset->tmpVector[i] = dataset->wtIRLS[i];
  }
  int midpt = 0.5 * Npoints;
  dsort (dataset->tmpVector, Npoints);
  double WtMedian = (Npoints % 2) ? dataset->tmpVector[midpt] : 0.5*(dataset->tmpVector[midpt] + dataset->tmpVector[midpt-1]);
  double WtThreshold = WEIGHT_THRESHOLD * WtMedian;

  // points with too low weight are ignored in the bootstrap analysis below
  int Nkeep = 0;
  double dChi = 0.0;
  double dSig = 0.0;
  for (int i = 0; i < Npoints; i++) {
    if ((dataset->wtIRLS[i] < WtThreshold) || !isfinite(dataset->alldata->yVector[i])) {
      continue; // skip the masked points
    }
    // save values with sufficient weight for error analysis below (either basic OLS or bootstrap)
    dataset->keepdata->yVector[Nkeep]  = dataset->alldata->yVector[i];
    dataset->keepdata->dyVector[Nkeep] = dataset->alldata->dyVector[i];
    if (dataset->alldata->xVector) {
      dataset->keepdata->xVector[Nkeep] = dataset->alldata->xVector[i]; // only defined if order > 0
    }
    Nkeep ++;
    
    // record min, max, chisq, sigma, Nmeas for the unmasked points
    dataset->min = isfinite(dataset->min) ? MIN(dataset->alldata->yVector[i], dataset->min) : dataset->alldata->yVector[i];
    dataset->max = isfinite(dataset->max) ? MAX(dataset->alldata->yVector[i], dataset->max) : dataset->alldata->yVector[i];
    // fprintf (stderr, "%d : %f %f %f\n", i, dataset->alldata->yVector[i], dataset->min, dataset->max);
    double dValue2 = SQ(dataset->yOffVector[i]);
    dChi += dValue2 / SQ (dataset->alldata->dyVector[i]);
    dSig += dValue2;
  }
  dataset->Nmeas = Nkeep;
  dataset->chisq = dChi / (Nkeep - 1);
  dataset->sigma = sqrt (dSig / (Nkeep - 1));

  if (dataset->Nbootstrap) {
    int Nboot = 0;
    for (int iboot = 0; iboot < dataset->Nbootstrap; iboot++) {
    
      // resample
      for (int i = 0; i < Nkeep; i++) {
	// I need to draw Npoints random entries from 'points' with replacement:
	int N = Nkeep * drand48();
	dataset->sample->yVector[i]  = dataset->keepdata-> yVector[N];
	dataset->sample->dyVector[i] = dataset->keepdata->dyVector[N];
	if (dataset->keepdata->xVector) {
	  dataset->sample->xVector[i] = dataset->keepdata->xVector[N]; // only defined if order > 0
	}
      }

      if (!fit1d_least_squares (dataset, dataset->sample, IS_OLS, Nkeep)) continue;
      // fprintf (stderr, "%d = %f : ", iboot, dataset->bArray[0][0]);

      // save this solution in entry Nboot
      fit1d_copy_results (dataset->bArray, dataset->bBootArray, Nboot, dataset->nterm);
      // fprintf (stderr, "%d = %f : ", Nboot, dataset->bBootArray[0][Nboot]);
      Nboot ++;
    }

    // loop over the nterm values
    for (int i = 0; i < dataset->nterm; i++) {
      // copy the bBootArray values to a tmp array
      for (int j = 0; j < Nboot; j++) {
	dataset->tmpVector[j] = dataset->bBootArray[i][j];
      }
      dsort (dataset->tmpVector, Nboot);
      double Slo = VectorFractionInterpolate (dataset->tmpVector, 0.158655, Nboot);
      double Shi = VectorFractionInterpolate (dataset->tmpVector, 0.841345, Nboot);
      dataset->bSigma[i] = (Shi - Slo) / 2.0;
    }
  }

  // calculate the formal fit error from the covariance matrix.  if we do not do
  // bootstrap, this is the reported parameter error.  Even if we do use bootstrap,
  // bootstrap with few values can sometimes yield an excessively-optimistic result for
  // the error.  Do not let the reported error be smaller than the formal error
  if (fit1d_least_squares (dataset, dataset->keepdata, IS_OLS, Nkeep)) {
    for (int i = 0; i < dataset->nterm; i++) {
      double errvalue = sqrt(dataset->cArray[i][i]);
      dataset->bSigma[i] = MAX (dataset->bSigma[i], errvalue);
    }
  } 
  return TRUE;
}

// full least-squares fit function including weights
// results are returned to dataset->bArray
int fit1d_least_squares (FitDataSet *dataset, FitDataType *data, int isOLS, int Npoints) { 

  // these are preallocated so repeated evaluations are not slowed down by alloc
  double *s  = dataset->sumVector;
  double **b = dataset->bArray;
  double **c = dataset->cArray;

  int nterm  = dataset->nterm; // number of parameters being fitted, e.g., y = C0 + C1*x -> 1st order, 2 terms
  int mterm  = dataset->mterm; // number of x^n terms calculated (e.g., for 3rd order polynomial, we need x^0 - x^6, i.e., 7 x^n terms)

  double *x  = data->xVector;
  double *y  = data->yVector;
  double *dy = data->dyVector;

  // XXX : the wtIRLS vector has the same sequence as the data vectors
  double *wt = isOLS ? NULL : dataset->wtIRLS; // use IRLS weight if requested

  // initialize summation registers for current pass
  memset (s, 0, mterm*sizeof(double));
  for (int i = 0; i < nterm; i++) {
    memset (c[i], 0, nterm*sizeof(double));
    memset (b[i], 0, sizeof(double));
  }

  // perform linear fit
  for (int i = 0; i < Npoints; i++, y++, dy++) {
    if (!finite(*y)) goto next; // skip invalid (NAN) data
    if (x && !finite(*x)) goto next; // if we are fitting x, skip invalid (NAN) data

    // XXX in the opihi version, I combine weights; in the relphot version I have a second weight vector to apply
    double dY = wt ? *wt / SQ(*dy) : 1.0 / SQ(*dy);

    double  X =  1*dY;
    double  Y = *y*dY;

    for (int j = 0; j < nterm; j++) {
      s[j] += X;
      b[j][0] += Y;
      if (x) {
	// x is NULL is 0-order fit
	X = X * (*x);
	Y = Y * (*x);
      }
    }
    for (int j = nterm; j < mterm; j++) {
      s[j] += X;
      if (x) { X = X * (*x); }
    }
  next:
    // these elements are optional and NULL if not used:
    if (x) { x++; }
    if (wt) { wt++; }
  }
  for (int i = 0; i < nterm; i++) {
    for (int j = 0; j < nterm; j++) {
      c[i][j] = s[i + j];
      if (dataset->bPriorValue && (i == j)) {
	if (!isnan(dataset->bPriorValue[i])) {
	  c[i][i] += 1.0 / SQ(dataset->bPriorSigma[i]);
	  b[i][0] += dataset->bPriorValue[i] / SQ(dataset->bPriorSigma[i]);
	}
      }
    }
  }

  if (!dgaussjordan (c, b, nterm, 1)) {
    gprint (GP_ERR, "failed to fit data : ill-conditioned matrix\n");
    return FALSE;
  }
  return TRUE;
}

void get_yfit (FitDataSet *dataset, int Npoints) {

  double *x = dataset->alldata->xVector;
  double *y = dataset->alldata->yVector;

  // XXX: keep this or not?
  // double *yfit = dataset->yFitVector;
  double *yoff = dataset->yOffVector;

  int nterm = dataset->nterm;
  double **b = dataset->bArray;

  /* generate fitted values */
  for (int i = 0; i < Npoints; i++, y++, yoff++) {
    if (x && !finite(*x)) continue;
    double yfit = 0;
    double X = 1;
    for (int j = 0; j < nterm; j++) {
      yfit += b[j][0]*X;
      if (x) { X = X * (*x); }
    }
    *yoff = *y - yfit;
    if (x) { x ++; }
  }
  return;
}

void fit1d_copy_results (double **srcArray, double **tgtArray, int entry, int nterm) {
  for (int i = 0; i < nterm; i++) {
    tgtArray[i][entry] = srcArray[i][0];
  }
  // fprintf (stderr, "%d = %f -> %f: ", entry, srcArray[0][0], tgtArray[0][entry]);
}

// allocate the vectors for this FitDataType entry (if order == 0, xVector = NULL)
FitDataType *FitDataTypeAlloc (int order, int N) {

  ALLOCATE_PTR (data, FitDataType, 1);

  ALLOCATE (data->yVector, double, N);
  ALLOCATE (data->dyVector, double, N);
  if (order) {
    ALLOCATE (data->xVector, double, N);
  } else {
    data->xVector = NULL;
  }
  return data;
}

// allocate the vectors for this FitDataType entry (if order == 0, xVector = NULL)
void FitDataTypeFree (FitDataType *data) {
  if (!data) return;
  FREE (data->yVector);
  FREE (data->dyVector);
  FREE (data->xVector);
  FREE (data);
}

void FitDataSetAlloc (FitDataSet *dataset, int Nmax, int order, int Nbootstrap) {

  dataset->order = order;
  dataset->nterm = order + 1;
  dataset->mterm = 2*order + 1;
  dataset->Nbootstrap = Nbootstrap;
  dataset->MaxIterations = 10;

  // arrays for actual data values
  dataset->alldata  = FitDataTypeAlloc (order, Nmax);

  // subset arrays to calculate formal errors
  dataset->keepdata = FitDataTypeAlloc (order, Nmax);

  dataset->sample   = NULL;  
  if (Nbootstrap) {
    // arrays for bootstrap samples
    dataset->sample   = FitDataTypeAlloc (order, Nmax);
  } 

  // allocate internal vectors and arrays 
  ALLOCATE (dataset->bSigma,     double,   dataset->nterm);
  ALLOCATE (dataset->sumVector,  double,   dataset->mterm);
  ALLOCATE (dataset->cArray,     double *, dataset->nterm);
  ALLOCATE (dataset->bArray,     double *, dataset->nterm);
  ALLOCATE (dataset->bSaveArray, double *, dataset->nterm);
  ALLOCATE (dataset->bBootArray, double *, dataset->nterm);
  for (int i = 0; i < dataset->nterm; i++) {
    ALLOCATE (dataset->cArray[i],      double, dataset->nterm);
    ALLOCATE (dataset->bArray[i],      double, 1);
    ALLOCATE (dataset->bSaveArray[i],  double, 1);
    ALLOCATE (dataset->bBootArray[i],  double, Nbootstrap);
  }

  ALLOCATE (dataset->yOffVector,  double, Nmax);
  ALLOCATE (dataset->tmpVector,   double, MAX(Nmax, Nbootstrap));
  ALLOCATE (dataset->wtIRLS,      double, Nmax);

  // set to NULL; user can activate
  dataset->bPriorValue = NULL;
  dataset->bPriorSigma = NULL;
}

void FitDataSetFree (FitDataSet *dataset) {

  FREE (dataset->bPriorValue);
  FREE (dataset->bPriorSigma);
  FREE (dataset->wtIRLS);
  FREE (dataset->tmpVector);
  FREE (dataset->yOffVector);

  FitDataTypeFree (dataset->alldata);
  FitDataTypeFree (dataset->keepdata);
  if (dataset->Nbootstrap) {
    FitDataTypeFree (dataset->sample);
  }

  // allocate internal vectors and arrays 
  for (int i = 0; i < dataset->nterm; i++) {
    FREE (dataset->cArray[i]);
    FREE (dataset->bArray[i]);
    FREE (dataset->bSaveArray[i]);
    FREE (dataset->bBootArray[i]);
  }
  FREE (dataset->sumVector);
  FREE (dataset->cArray);
  FREE (dataset->bArray);
  FREE (dataset->bSigma);
  FREE (dataset->bSaveArray);
  FREE (dataset->bBootArray);
}  

void FitDataSetAddPriors (FitDataSet *dataset) {

  ALLOCATE (dataset->bPriorValue, double, dataset->nterm);
  ALLOCATE (dataset->bPriorSigma, double, dataset->nterm);

  for (int i = 0; i < dataset->nterm; i++) {
    dataset->bPriorValue[i] = NAN;
    dataset->bPriorSigma[i] = NAN;
  }
}

StatType FitDataSetSoften (FitDataSet *dataset, int Nvalues) {

  // use liststats to find the 20-pct, median, 80-pct points
  StatType stats;
  liststats_setmode (&stats, "MEDIAN");
  liststats (dataset->alldata->yVector, NULL, NULL, Nvalues, &stats);

  double altSigma = (stats.Upper80 - stats.Lower20) / 1.6;  // 20% to 80% encompasses 60% of the values, corresponds to the range (-0.85 sigma : +0.85 sigma)

  // soften the individual errors with 10% of the scatter above
  for (int j = 0; j < Nvalues; j++) {
    double newSigma = hypot(dataset->alldata->dyVector[j], 0.1*altSigma);
    dataset->alldata->dyVector[j] = newSigma;
  }
  
  return stats;
}
