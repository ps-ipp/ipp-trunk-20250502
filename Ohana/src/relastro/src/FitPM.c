# include "relastro.h"

int FitPM_MinChisq (FitAstromResult *fit, FitAstromData *data, FitAstromPoint *points, int Npoints);
int FitPM_SetChisq (FitAstromResult *fit, FitAstromData *data, FitAstromPoint *points, int Npoints);

// These should probably be tunable:
# define MAX_ITERATIONS 10
# define FIT_TOLERANCE 1e-4
# define FLT_TOLERANCE 1e-6
# define WEIGHT_THRESHOLD 0.3

// initial values of *fit are ignored
int FitPM_Basic (FitAstromResult *fit, FitAstromData *data, FitAstromPoint *points, int Npoints) {

  int i;

  // Convert the measurement errors into initial weights.
  for (i = 0; i < Npoints; i++) {
    points[i].Wx = 1.0;
    points[i].Wy = 1.0;
    points[i].Qx = 1.0 / SQ(points[i].dX);
    points[i].Qy = 1.0 / SQ(points[i].dY);
    points[i].qx = points[i].Wx * points[i].Qx; // Wx, Wy start out at 1.0
    points[i].qy = points[i].Wy * points[i].Qy; // Wx, Wy start out at 1.0
  }

  fit->useWeight = FALSE; // Ordinary Least Squares
  if (!FitPM_MinChisq (fit, data, points, Npoints)) return FALSE;
  if (!FitPM_SetChisq (fit, data, points, Npoints)) return FALSE;
  return TRUE;
}

int FitPM_IRLS (FitAstromResult *fit, FitAstromData *data, FitAstromPoint *points, int Npoints) {

  int i,j;

  int Ndof = 2 * Npoints - data->Nterms;
  
  // Convert the measurement errors into initial weights.
  for (i = 0; i < Npoints; i++) {
    points[i].Wx = 1.0;
    points[i].Wy = 1.0;
    points[i].Qx = 1.0 / SQ(points[i].dX);
    points[i].Qy = 1.0 / SQ(points[i].dY);
    points[i].qx = points[i].Wx * points[i].Qx; // Wx, Wy start out at 1.0
    points[i].qy = points[i].Wy * points[i].Qy; // Wx, Wy start out at 1.0
  }
  
  // Solve OLS equation  
  fit->useWeight = FALSE; // Ordinary Least Squares
  if (!FitPM_MinChisq(fit, data, points, Npoints)) {
    return(FALSE);
  }

  // Calculate r vector of residuals and least squares sigma
  double sigma_ols = 0.0;
  for (i = 0; i < Npoints; i++) {
    points[i].rx = points[i].X - (points[i].T * fit->uR + fit->Ro);
    points[i].ry = points[i].Y - (points[i].T * fit->uD + fit->Do);
    sigma_ols += SQ(points[i].rx) + SQ(points[i].ry);
  }
  sigma_ols = sqrt(sigma_ols / (float)Ndof);

  // Save OLS covariance and Beta (solution vector, which is actually also saved in fit)
  for (i = 0; i < data->Nterms; i++) {
    for (j = 0; j < data->Nterms; j++) {
      data->Cov[i][j] = data->A[i][j];
    }
    data->Beta[i] = data->B[i][0];
  }

  // Iteratively reweight and solve
  // double sigma_hat = 0.0; // save for the error model
  int converged = FALSE;
  int iterations = 0;

  // modify the weight based on the distance from the previous fit.  try up to MAX_ITERATIONS.
  // at the end "fit", has the last fit parameters
  for (iterations = 0; !converged && (iterations < MAX_ITERATIONS); iterations ++) {
    // Save Beta.
    for (i = 0; i < data->Nterms; i ++) {
      data->Beta_prev[i] = data->Beta[i];
    }

    // Assign weights based on the deviation
    for (i = 0; i < Npoints; i++) {
      points[i].Wx = weight_cauchy(points[i].rx / points[i].dX);
      points[i].Wy = weight_cauchy(points[i].ry / points[i].dY);
      points[i].qx = points[i].Wx * points[i].Qx;
      points[i].qy = points[i].Wy * points[i].Qy;
    }    

    // Solve with the new weights
    fit->useWeight = TRUE; // Reweighted Least Squares
    if (!FitPM_MinChisq(fit, data, points, Npoints)) {

      // restore the last solution and break
      fit->Ro = data->Beta_prev[0];
      fit->uR = data->Beta_prev[1];
      fit->Do = data->Beta_prev[2];
      fit->uD = data->Beta_prev[3];
      
      // calculate the residuals:
      for (i = 0; i < Npoints; i++) {
	points[i].rx = points[i].X - (points[i].T * fit->uR + fit->Ro);
	points[i].ry = points[i].Y - (points[i].T * fit->uD + fit->Do);
	points[i].u = sqrt(SQ(points[i].rx / points[i].dX) + SQ(points[i].ry / points[i].dY));
      }
      // sigma_hat = MedianAbsDeviation(points, Npoints) / 0.6745;
      break;
    }

    // store the new Beta.
    for (i = 0; i < data->Nterms; i++) {
      data->Beta[i] = data->B[i][0];
    }

    // calculate the residuals:
    for (i = 0; i < Npoints; i++) {
      points[i].rx = points[i].X - (points[i].T * fit->uR + fit->Ro);
      points[i].ry = points[i].Y - (points[i].T * fit->uD + fit->Do);
      points[i].u = sqrt(SQ(points[i].rx / points[i].dX) + SQ(points[i].ry / points[i].dY));
    }
    // sigma_hat = MedianAbsDeviation(points, Npoints) / 0.6745;
    
    // Check convergence
    converged = TRUE;
    for (i = 0; i < data->Nterms; i++) {
      // if we are within FIT_TOLERANCE as a fractional error or FLT_TOLERANCE as an absolute error, we are good
      if ((fabs(data->Beta[i] - data->Beta_prev[i]) > FIT_TOLERANCE * fabs(data->Beta[i])) && 
	  (fabs(data->Beta[i] - data->Beta_prev[i]) > FLT_TOLERANCE)) {
	converged = FALSE;
      }
    }
  }
  fit->converged = converged;

  // calculate the weight thresholds to mask the bad points:
  double Sum_Wx = 0.0;
  double Sum_Wy = 0.0;
  for (i = 0; i < Npoints; i++) {
    points[i].Wx = weight_cauchy(points[i].rx / points[i].dX);
    points[i].Wy = weight_cauchy(points[i].ry / points[i].dY);
    
    Sum_Wx += points[i].Wx;
    Sum_Wy += points[i].Wy;
  }
  double WxThreshold = WEIGHT_THRESHOLD * Sum_Wx / (1.0 * Npoints);
  double WyThreshold = WEIGHT_THRESHOLD * Sum_Wy / (1.0 * Npoints);

  // set a mask (which can be used by the bootstrap resampling analysis)
  for (i = 0; i < Npoints; i++) {
    // keep if either is above threshold?
    // drop if either is below threshold?
    // points are marked as keep by default
    if ((points[i].Wx < WxThreshold) || (points[i].Wy < WyThreshold)) {
      points[i].mask = 1; // keep point if mask == 0
    }
  }

  // this section calculates the formal error on the weighted fit using the covariance values
  // NOTE EAM: in tests (fitpm.c), they seem to be too large by a factor of ~5.37
  if (data->getError) {
    FitAstromResult fitErrors;
    FitAstromResultInit (&fitErrors);
    fitErrors.useWeight = FALSE; 

    FitPM_MinChisq(&fitErrors, data, points, Npoints);

    // we use the errors from a simple OLS, ignoring masked points
    fit[0].dRo = fitErrors.dRo;
    fit[0].duR = fitErrors.duR;
    fit[0].dDo = fitErrors.dDo;
    fit[0].duD = fitErrors.duD;
  }

  if (!FitPM_SetChisq (fit, data, points, Npoints)) return FALSE;
  return (TRUE);
}

int FitPM_MinChisq (FitAstromResult *fit, FitAstromData *data, FitAstromPoint *points, int Npoints) {

  myAssert (data->Nterms == 4, "invalid fit arrays");

  int i;
  double wx, wy, Wx, Wy, Tx, Ty, Tx2, Ty2, Xs, Ys, XT, YT;
  Wx = Wy = Tx = Ty = Tx2 = Ty2 = Xs = Ys = XT = YT = 0.0;

  int Nfit = 0;
  for (i = 0; i < Npoints; i++) {
    if (points[i].mask) continue; // respect the mask if set
    Nfit ++;

    if (fit->useWeight) {
      wx = points[i].qx;
      wy = points[i].qy;
    } else {
      wx = points[i].Qx;
      wy = points[i].Qy;
    }

    Wx += wx;
    Wy += wy;

    double TWx = points[i].T*wx;
    double TWy = points[i].T*wy;

    Tx += TWx;
    Ty += TWy;
    
    Tx2 += points[i].T*TWx;
    Ty2 += points[i].T*TWy;
    
    Xs += points[i].X*wx;
    Ys += points[i].Y*wy;

    XT += points[i].X*TWx;
    YT += points[i].Y*TWy;
  }
  if (Nfit < 3) return FALSE;

  // X^T W X
  data->A[0][0] = Wx;
  data->A[0][1] = Tx;
  data->A[0][2] = 0.0;
  data->A[0][3] = 0.0;

  data->A[1][0] = Tx;
  data->A[1][1] = Tx2;
  data->A[1][2] = 0.0;
  data->A[1][3] = 0.0;

  data->A[2][0] = 0.0;
  data->A[2][1] = 0.0;
  data->A[2][2] = Wy;
  data->A[2][3] = Ty;

  data->A[3][0] = 0.0;
  data->A[3][1] = 0.0;
  data->A[3][2] = Ty;
  data->A[3][3] = Ty2;

  // X^T W Y
  data->B[0][0] = Xs;
  data->B[1][0] = XT;
  data->B[2][0] = Ys;
  data->B[3][0] = YT;

  if (!dgaussjordan (data->A, data->B, 4, 1)) {
    return FALSE;
  }

  fit->Ro = data->B[0][0];
  fit->uR = data->B[1][0];
  fit->Do = data->B[2][0];
  fit->uD = data->B[3][0];
  fit->p  = 0.0;

  fit->dRo = sqrt(data->A[0][0]);
  fit->duR = sqrt(data->A[1][1]);
  fit->dDo = sqrt(data->A[2][2]);
  fit->duD = sqrt(data->A[3][3]);
  fit->dp  = 0.0;

  return TRUE;
}

int FitPM_SetChisq (FitAstromResult *fit, FitAstromData *data, FitAstromPoint *points, int Npoints) {

  int i;

  // count unmasked points and (optionally) add up the chi square for the fit
  double chisq = 0.0;
  fit[0].Nfit = 0;
  for (i = 0; i < Npoints; i++) {
    if (points[i].mask) continue;
    fit[0].Nfit ++;
      
    if (data->getChisq) {
      double Xf = fit[0].Ro + fit[0].uR*points[i].T;
      double Yf = fit[0].Do + fit[0].uD*points[i].T;
      double wx = 1.0 / SQ(points[i].dX);
      double wy = 1.0 / SQ(points[i].dY);
      chisq += SQ(points[i].X - Xf) * wx;
      chisq += SQ(points[i].Y - Yf) * wy;
    }
  }
    
  if (fit[0].Nfit < 3) {
    fit[0].chisq = NAN;
    return FALSE;
  }

  // the reduced chisq is divided by (Ndof = 2*Nfit - Nterms)
  fit[0].chisq = chisq / (2.0*fit[0].Nfit - data->Nterms);
  return TRUE;
}
