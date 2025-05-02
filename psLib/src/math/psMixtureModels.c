/** @file psMixtureModels.c
 *
 *  @brief Mixture Modeling/Clustering tools
 *
 *  Functions are provided to:
 *      Determine N-dimensional K-means separation to data
 *      Determine N-dimensional Gaussian mixture models to data
 *      Provide 1-D simplifications of these methods
 *  @author Chris Waters, IfA
 *
 *  version Based on CZW/brml/mixture_models.c v1.3
 *  format/structure based on psMatrix.c
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/******************************************************************************/
/*  INCLUDE FILES                                                             */
/******************************************************************************/
#include <string.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_cdf.h>

#include "psAbort.h"
#include "psMemory.h"
#include "psError.h"
#include "psImage.h"
#include "psVector.h"
#include "psMatrix.h"
#include "psAssert.h"
#include "psRandom.h"
#include "psStats.h"
#include "psTrace.h"

#include "psMixtureModels.h"


# define TESTING 0
# define TESTING_VERBOSE 0
/////////////////////////////////////////////////////////////////////////////////
// Internal functions                                                          //
/////////////////////////////////////////////////////////////////////////////////


// Calculate Squared Euclidean Norm: ||A - B||^2
// Because of the way psLib handles vectors/matrices, I never use this function in this implementation
#if 0
double psMMVectorNorm(const psVector *A,
		      const psVector *B) {
  // Need to add assertations here, including that we're dealing with F32s

  // Math
  long num = A->n;

  // blas_dnrm2
  double v = 0.0;
  for (long i = 0; i < num; i++) {
    v += A->data.F32[i] * B->data.F32[i];
  }
  return(v);
}
#endif

// Calculate value of N-dimensional Gaussian of mean M and variance S at position X
double psMMNDGaussian(const psVector *X,        //< Vector of length dim
		      const psVector *m,        //< Vector of length dim
		      const psImage  *S) {      //< Matrix of size dim x dim
  // Assertions
  // Are these all strictly necessary?
  PS_ASSERT_VECTOR_NON_NULL(X,false);
  PS_ASSERT_VECTOR_NON_NULL(m,false);
  PS_ASSERT_VECTOR_TYPE(X,PS_TYPE_F32,false);
  PS_ASSERT_VECTOR_TYPE(m,PS_TYPE_F32,false);
  //  PS_ASSERT_VECTOR_SIZE_EQUAL(X,m,false);
  PS_ASSERT_IMAGE_NON_NULL(S,false);
  PS_ASSERT_IMAGE_TYPE(S,PS_TYPE_F32,false);
  //  PS_ASSERT_IMAGE_SIZE(S,X->n,X->n,false);

  // Initialize
  long num = X->n;
  float determinant;
  psImage *invS = psMatrixInvert(NULL,S,&determinant);

  // blas_dtrmv: S^-1 * (x-m)
  psVector *SD = psVectorAlloc(num,PS_TYPE_F32);
  for (int i = 0; i < num; i++) {
    SD->data.F32[i] = 0;
    for (int j = 0; j < num; j++) {
      SD->data.F32[i] += invS->data.F32[i][j] * (X->data.F32[j] - m->data.F32[j]);
    }
  }

  // blas_ddot: (x-m)^T * SD = (x-m)^T * S^-1 * (x-m)
  double v = 0;
  for (int i = 0; i < num; i++) {
    v += SD->data.F32[i] * (X->data.F32[i] - m->data.F32[i]);
  }

  // G = (2 * pi)^(-0.5 * dim) * 1 /sqrt(det) * exp(-0.5 * v)
  // Drop the 2pi term, as we only ever use this in ratios of constant dimension
  double G = exp(-0.5 * v)/sqrt(determinant);

  // Cleanup
  psFree(SD);
  psFree(invS);
  return(G);
}

psImage *psMMCensorData(psImage *in,
			long *Ncensored,
			psVector *offsets) {
  // Allocate offset vector
  //  offsets = psVectorRecycle(offsets,in->numRows,PS_TYPE_S32);

  *Ncensored = 0;

  int k = 0;
  for (int i = 0; i < in->numRows; i++) {
    int isCensored = 0;
    for (int j = 0; j < in->numCols; j++) {
      if (!isfinite(in->data.F32[i][j])) {
	isCensored = 1;
      }
    }
    if (isCensored) {
      *Ncensored = *Ncensored + 1;
    }
    else {
      offsets->data.S32[k] = i - k;
      k++;
    }
    //    offsets->data.S32[i] = isCensored;
  }

  psImage *out;
  if (*Ncensored == 0) {
    out = psMemIncrRefCounter(in);
  }
  else {
    out = psImageAlloc(in->numCols,in->numRows - *Ncensored,PS_TYPE_F32);
    for (int i = 0; i < out->numRows; i++) {
      for (int j = 0; j < out->numCols; j++) {
	out->data.F32[i][j] = in->data.F32[i + offsets->data.S32[i]][j];
      }
    }
  }
  return(out);
}

/////////////////////////////////////////////////////////////////////////////////
// External functions                                                          //
/////////////////////////////////////////////////////////////////////////////////

#define MAX_ITERATIONS 100
#define THRESHOLD      1e-3

bool psMMkmeans(psImage *D,      //< Input data to fit.  					 
		int dim,	 //< dimension of the data					 
		long N,		 //< number of data points					 
		psVector *modes, //< Output vector of which mode each data point belongs to	 
		psImage *means,	 //< Matrix containing mode mean values for each dimension	 
		int m,		 //< Number of modes to fit					 
		int *iterations, //< Number of iterations to perform				 
		double *V) {	 //< Deviation value.  Pseudo chi^2                           
  PS_ASSERT_IMAGE_NON_NULL(D,false);
  PS_ASSERT_IMAGE_TYPE(D,PS_TYPE_F32,false);
  // We need to scan the input for invalid d
  long Ncensored;
  psVector *offsets = psVectorAlloc(N,PS_TYPE_S32);
  psImage  *Dcensored = psMMCensorData(D,&Ncensored,offsets);


  bool success;
  if (Ncensored == 0) { // We can directly return the modes and means calculated
    success = psMMkmeansUncensored(Dcensored,
				   dim,
				   N,
				   modes,
				   means,
				   m,
				   iterations,
				   V);
  }
  else {
    psVector *tempModes = psVectorAlloc(N - Ncensored,PS_TYPE_F32);
    success = psMMkmeansUncensored(Dcensored,
				   dim,
				   N - Ncensored,
				   tempModes,
				   means,
				   m,
				   iterations,
				   V);
    
    if (!modes) {
      modes = psVectorAlloc(N,PS_TYPE_F32);
    }
    
    psVectorInit(modes,NAN);
    for (int i = 0; i < N - Ncensored; i++) {
      modes->data.F32[i + offsets->data.S32[i]] = tempModes->data.F32[i];
    }
    psFree(tempModes);
  }
  psFree(offsets);
  psFree(Dcensored);

  return(success);
}


bool psMMkmeansUncensored(psImage *D,       //< Input data to fit.  
			  int dim,          //< dimension of the data
			  long N,           //< number of data points
			  psVector *modes,  //< Output vector of which mode each data point belongs to
			  psImage *means,   //< Matrix containing mode mean values for each dimension
			  int m,            //< Number of modes to fit
			  int *iterations,  //< Number of iterations to perform
			  double *V) {      //< Deviation value.  Pseudo chi^2                           
  // Assertions
  PS_ASSERT_IMAGE_NON_NULL(D,false);
  PS_ASSERT_IMAGE_TYPE(D,PS_TYPE_F32,false);
  //  PS_ASSERT_IMAGE_SIZE(D,N,dim,false);

  
  // Allocate things we can allocate here if they're not already allocated
  if (!modes) {
    modes = psVectorAlloc(N,PS_TYPE_F32);
  }
  if (!means) {
    means = psImageAlloc(dim,m,PS_TYPE_F32);
  }

  PS_ASSERT_VECTOR_NON_NULL(modes,false);
  PS_ASSERT_VECTOR_TYPE(modes,PS_TYPE_F32,false);
  //  PS_ASSERT_VECTOR_SIZE(modes,m,false);

  PS_ASSERT_IMAGE_NON_NULL(means,false);
  PS_ASSERT_IMAGE_TYPE(means,PS_TYPE_F32,false);
  //  PS_ASSERT_IMAGE_SIZE(means,m,dim,false);
  
  // Initialize
  int error = 0;

  int i,j,k;
  psImage *oldMeans;
  psVector *counts;
  double diff;
  // XXX not used: double R;

  // Allocations
  oldMeans = psImageAlloc(dim,m,PS_TYPE_F32);
  counts   = psVectorAlloc(m,PS_TYPE_F32);

  // Initialize means to resonable starting points
  psVector *minimums = psVectorAlloc(dim,PS_TYPE_F32);
  psVector *maximums = psVectorAlloc(dim,PS_TYPE_F32);
  psVector *temp = psVectorAlloc(N,PS_TYPE_F32);
  psStats  *stats = psStatsAlloc(PS_STAT_MIN | PS_STAT_MAX);
  for (j = 0; j < dim; j++) {
    for (k = 0; k < N; k++) {
      temp->data.F32[k] = D->data.F32[k][j];
    }
    psVectorStats(stats,temp,NULL,NULL,0);
    minimums->data.F32[j] = stats->min;
    maximums->data.F32[j] = stats->max;
  }
  psFree(temp);
  psFree(stats);

  if (m == 1) { // In the case of only one mode, set the mean to in the middle of the range
    for (j = 0; j < dim; j++) {
      means->data.F32[0][j] = (maximums->data.F32[j] + minimums->data.F32[j]) / 2;
    }
  }
  else if (m == 2) { // For two modes, set one mode at each end point.
    for (j = 0; j < dim; j++) { 
      means->data.F32[0][j] = minimums->data.F32[j];
      means->data.F32[1][j] = maximums->data.F32[j];
    }
  }
  else { // Otherwise, just throw random points at it until we're done.
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
    for (i = 0; i < m; i++) {
      for (j = 0; j < dim; j++) {
	means->data.F32[i][j] = minimums->data.F32[j] +
	  (psRandomUniform(rng) * (maximums->data.F32[j] - minimums->data.F32[j]));
      }
    }
    psFree(rng);
  }

  psFree(minimums);
  psFree(maximums);

  // Clear assignments
  for (k = 0; k < N; k++) {
    modes->data.F32[k] = -1;
  }
  *iterations = 0;
  do {
      // XXX not used : R = 0.0;

    // Reset counts for each mode
    for (i = 0; i < m; i++) {
      counts->data.F32[i] = 0.0;
    }
    // Assign each datapoint to a mode
    for (k = 0; k < N; k++) {
      double d = 99e99;
      double min_d = 99e99;
      for (i = 0; i < m; i++) {
	// Take the norm  of this point from the mode mean
	d = 0;
	for (j = 0; j < dim; j++) {
	  d += PS_SQR(D->data.F32[k][j] - means->data.F32[i][j]);
	}
	if (d < min_d) {
	  min_d = d;
	  modes->data.F32[k] = i;
	}
      }
      if (modes->data.F32[k] != -1) {
	i = modes->data.F32[k];
	counts->data.F32[i] += 1.0;
      }
    }

    // Clear previous iteration mean values
    for (i = 0; i < m; i++) {
      for (j = 0; j < dim; j++) {
	oldMeans->data.F32[i][j] = means->data.F32[i][j];
	means->data.F32[i][j] = 0.0;
      }
    }

    // Calculate new means
    for (k = 0; k < N; k++) {
      i = modes->data.F32[k];
      if (i != -1) {
	for (j = 0; j < dim; j++) {
	  means->data.F32[i][j] += D->data.F32[k][j];
	}
      }
    }

    diff = 0.0;
    for (i = 0; i < m; i++) {
      for (j = 0; j < dim; j++) {
	if (counts->data.F32[i] > 0.0) {
	  means->data.F32[i][j] /= counts->data.F32[i];
	}
	else {
	  means->data.F32[i][j] = 0.0;
	}
	diff += PS_SQR(means->data.F32[i][j] - oldMeans->data.F32[i][j]);
      }
    }

# if (TESTING)
    fprintf (stderr, "kmeans : %d %f %f %f\n",*iterations,diff,means->data.F32[0][0],means->data.F32[1][0]);
# endif
    
    *iterations += 1;
  } while ((*iterations < MAX_ITERATIONS)&&
	   (diff > THRESHOLD));

# if (TESTING)
  fprintf (stderr, "psMMkmeans diff: %f\n", diff);
# endif

  *V = diff;

  psFree(oldMeans);
  psFree(counts);
  
  return(error); // Checked against stand alone code.  I believe these results. CZW 2013-04-10.
}

bool psMMGMM(psImage *D,           //< Input data to fit
	     int dim,              //< Dimension of each data vector
	     long N,               //< Number of data vectors
	     psVector *modes,      //< Output vector assigning each data point to a mode
	     psImage *means,       //< Output matrix containing vectors of the means for each mode
	     psArray *sigma,       //< Output array containing covariance matrices for each mode
	     psVector *pi,         //< Output vector contining population fractions for each mode
	     psImage *P,           //< Output matrix containing the probabilities a data point is in each mode
	     int m,                //< Number of modes to fit
	     int *iterations,      //< Number of iterations performed
	     double *V) {          //< Log-likelihood of final solution.

  PS_ASSERT_IMAGE_NON_NULL(D,false);
  PS_ASSERT_IMAGE_TYPE(D,PS_TYPE_F32,false);
  // We need to scan the input for invalid d
  long Ncensored;
  psVector *offsets = psVectorAlloc(N,PS_TYPE_S32);
  psImage  *Dcensored = psMMCensorData(D,&Ncensored,offsets);
  bool success;
  if (Ncensored == 0) { // We can directly return the modes and means calculated
    success = psMMGMMUncensored(Dcensored,
				dim,
				N,
				modes,
				means,
				sigma,
				pi,
				P,
				m,
				iterations,
				V);
  }
  else {
    psVector *tempModes = psVectorAlloc(N - Ncensored,PS_TYPE_F32);
    psImage  *tempP     = psImageAlloc(m,N - Ncensored,PS_TYPE_F32);

    success = psMMGMMUncensored(Dcensored,
				dim,
				N - Ncensored,
				tempModes,
				means,
				sigma,
				pi,
				tempP,
				m,
				iterations,
				V);
      
    
    if (!modes) {
      modes = psVectorAlloc(N,PS_TYPE_F32);
    }
    if (!P) {
      P = psImageAlloc(m,N,PS_TYPE_F32);
    }
    psVectorInit(modes,NAN);
    psImageInit(P,NAN);

    for (int i = 0; i < N - Ncensored; i++) {
      modes->data.F32[i + offsets->data.S32[i]] = tempModes->data.F32[i];
      for (int j = 0; j < dim; j++) {
	P->data.F32[i + offsets->data.S32[i]][j] = tempP->data.F32[i][j];
      }
    }
    psFree(tempModes);
    psFree(tempP);
  }
  psFree(offsets);
  psFree(Dcensored);
  return(success);
}


bool psMMGMMUncensored(psImage *D,           //< Input data to fit
		       int dim,              //< Dimension of each data vector
		       long N,               //< Number of data vectors
		       psVector *modes,      //< Output vector assigning each data point to a mode
		       psImage *means,       //< Output matrix containing vectors of the means for each mode
		       psArray *sigma,       //< Output array containing covariance matrices for each mode
		       psVector *pi,         //< Output vector contining population fractions for each mode
		       psImage *P,           //< Output matrix containing the probabilities a data point is in each mode
		       int m,                //< Number of modes to fit
		       int *iterations,      //< Number of iterations performed
		       double *V) {          //< Log-likelihood of final solution.
  // Assertions
  PS_ASSERT_IMAGE_NON_NULL(D,false);
  PS_ASSERT_IMAGE_TYPE(D,PS_TYPE_F32,false);
  //  PS_ASSERT_IMAGE_SIZE(D,N,dim,false);

  // GMM cannot initialize means from scratch, so require a guess
  PS_ASSERT_IMAGE_NON_NULL(means,false);
  PS_ASSERT_IMAGE_TYPE(means,PS_TYPE_F32,false);
  //  PS_ASSERT_IMAGE_SIZE(means,m,dim,false);

  // Allocate things we can allocate here if they're not already allocated
  if (!modes) {
    modes = psVectorAlloc(N,PS_TYPE_F32);
  }
  if (!pi) {
    pi = psVectorAlloc(m,PS_TYPE_F32);
  }
  if (!P) {
    P = psImageAlloc(m,N,PS_TYPE_F32);
  }
  
  PS_ASSERT_VECTOR_NON_NULL(modes,false);
  PS_ASSERT_VECTOR_TYPE(modes,PS_TYPE_F32,false);
  //  PS_ASSERT_VECTOR_SIZE(modes,m,false);

  PS_ASSERT_VECTOR_NON_NULL(pi,false);
  PS_ASSERT_VECTOR_TYPE(pi,PS_TYPE_F32,false);
  //  PS_ASSERT_VECTOR_SIZE(pi,m,false);

  PS_ASSERT_IMAGE_NON_NULL(P,false);
  PS_ASSERT_IMAGE_TYPE(P,PS_TYPE_F32,false);
  //  PS_ASSERT_IMAGE_SIZE(P,N,m,false);

  // Is this sufficient?  Should we check each mode's image?
  PS_ASSERT_ARRAY_NON_NULL(sigma,false);

  // Initialize 
  int error = 0;

  int i,j,k,jj;
  double logL = -99e99;
  double oldlogL = -99e99;
  psVector *counts;
  psVector *X = psVectorAlloc(dim,PS_TYPE_F32);
  psVector *M = psVectorAlloc(dim,PS_TYPE_F32);

  // Allocations
  counts = psVectorAlloc(m,PS_TYPE_F32);

  X      = psVectorAlloc(dim,PS_TYPE_F32);
  M      = psVectorAlloc(dim,PS_TYPE_F32);
  psVectorInit(counts,0.0);
  psVectorInit(X,0.0);
  psVectorInit(M,0.0);

  // Do the first initialization.  We inherit means.
  // Set pi
  for (k = 0; k < N; k++) {
    i = modes->data.F32[k];
    counts->data.F32[i] += 1.0;
  }
  for (i = 0; i < m; i++) {
    //pi->data.F32[i] = counts->data.F32[i] / (1.0 * N);
    pi->data.F32[i] = 0.5;
  }

  // Set sigma
  for (i = 0; i < m; i++) {
    psImage *S = sigma->data[i];
    psImageInit(S,0.0); // Clear old value
    for (j = 0; j < dim; j++) {
      S->data.F32[j][j] = 1.0; // Doesn't need to be a good guess
    }
  }

  do {
    oldlogL = logL;
    logL = 0.0;
    psImageInit(P,0.0); // Clear old probabilities

    // Calculate P values
    for (k = 0; k < N; k++) {
      double norm_K = 0.0;

      for (j = 0; j < dim; j++) { // VV
	X->data.F32[j] = D->data.F32[k][j];
      }
      for (i = 0; i < m; i++) {
	psImage *S = sigma->data[i];
	for (j = 0; j < dim; j++) { // VV
	  M->data.F32[j] = means->data.F32[i][j];
#if (TESTING_VERBOSE)
	  	  printf("%f %f %f\n", X->data.F32[j],M->data.F32[j], S->data.F32[j][j]);
#endif
	}

	// if log-likelihood becomes too small, we risk numerical overflow 
	float Prob = psMMNDGaussian(X,M,S);
	if (Prob < 1e-6) { Prob = 1e-6; }

	P->data.F32[k][i] = pi->data.F32[i] * Prob;

	logL += log(P->data.F32[k][i]);
#if (TESTING_VERBOSE)
	printf("%d %d %f %f %f %f\n",
		       k,i,
		       pi->data.F32[i],
		       psMMNDGaussian(X,M,S),
		       P->data.F32[k][i],logL);
#endif
	norm_K += P->data.F32[k][i];
      }
      // Nomalize for each mode
      for (i = 0; i < m; i++) {
	if (norm_K != 0.0) {
	  P->data.F32[k][i] /= norm_K;
	}
      }
    }
    
    // Clear old M/S/pi
    psVectorInit(pi,0.0);
    psImageInit(means,0.0);
    for (i = 0; i < m; i++) {
      psImage *S = sigma->data[i];
      psImageInit(S,0.0);
    }

    // Set means

    psVectorInit(counts,0.0);
    for (k = 0; k < N; k++) {

      for (i = 0; i < m; i++) {
	for (j = 0; j < dim; j++) {
	  means->data.F32[i][j] += P->data.F32[k][i] * D->data.F32[k][j];
	}
	counts->data.F32[i] += P->data.F32[k][i];
      }
    }
    double norm_N = 0;
    for (i = 0; i < m; i++) {
      for (j = 0; j < dim; j++) {
	if (counts->data.F32[i] > 0.0) {
	  means->data.F32[i][j] /= counts->data.F32[i];
	}
      }
      norm_N += counts->data.F32[i];
    }

    // Set pi
    for (i = 0; i < m; i++) {
      pi->data.F32[i] = counts->data.F32[i] / N;
    }

    // Set covariance
    for (i = 0; i < m; i++) {
      psImage *S = sigma->data[i];

      for (k = 0; k < N; k++) {
	for (j = 0; j < dim; j++) {
	  for (jj = 0; jj < dim; jj++) {
	    // S = sum (weight^-1 * diff * diff^T)
	    S->data.F32[j][jj] += 
	      (P->data.F32[k][i]) *    // W^-1
	      (D->data.F32[k][j]  - means->data.F32[i][j]) *  // diff
	      (D->data.F32[k][jj] - means->data.F32[i][jj]);  // diff^T
	  }
	}
      }
      for (j = 0; j < dim; j++) {
	for (jj = 0; jj < dim; jj++) {
	  if (counts->data.F32[i] != 0.0) {
	    S->data.F32[j][jj] /= counts->data.F32[i];
	  }
	}
      }
    }

# if (TESTING)
    fprintf (stderr, "GMM: %d %f %f %f %f\n",*iterations,logL,oldlogL,means->data.F32[0][0],means->data.F32[1][0]);
# endif

    *iterations = *iterations + 1;
  } while ((logL - oldlogL > 0)&&
	   (*iterations < MAX_ITERATIONS));

# if (TESTING)
  fprintf (stderr, "psMMGMM logL: %f\n", logL);
# endif

  psFree(counts);
  psFree(X);
  psFree(M);
  *V = logL;
  return(error);
}

// This is the expected public function, and as such, requires very little to be defined.
bool psMMClass(psImage *D,
	       int dim,
	       long N,
	       psVector *modes,
	       psImage *means,
	       psArray *sigma,
	       psVector *pi,
	       psImage *P,
	       int m,
	       double *Pval) {
  // Assertions
  PS_ASSERT_IMAGE_NON_NULL(D,false);
  PS_ASSERT_IMAGE_TYPE(D,PS_TYPE_F32,false);
  //  PS_ASSERT_IMAGE_SIZE(D,N,dim,false);
  
  // Is this sufficient?  Should we check each mode's image?
  PS_ASSERT_ARRAY_NON_NULL(sigma,false);
  
  // Initialize
  int iterations;
  double V,V0;

  if (!modes) {
    modes = psVectorAlloc(N,PS_TYPE_F32);
  }

  // Do K-means/GMM iteration for m-1.
  if (m > 1) { // Do a check of the smaller number of modes
    if (!psMMkmeans(D,dim,N,modes,means,m-1,
		    &iterations,&V)) {
      // Error calculating k-means
    }

#if (TESTING)
    for (int i = 0 ; i < m -1; i++) {
      for (int j = 0; j < dim; j++) {
	fprintf(stderr,"KMEANS POST: (%d/%d) (%d/%d) %g\n",
		i,m - 1,j,dim,means->data.F32[i][j]);
      }
    }
#endif
        
    iterations = 0;
    if (!psMMGMM(D,dim,N,modes,means,sigma,pi,P,
		 m-1,&iterations,&V0)) {
      // Error calculating GMM
    }

#if (TESTING)
    for (int i = 0 ; i < m - 1; i++) {
      for (int j = 0; j < dim; j++) {
	fprintf(stderr,"GMM POST: (%d/%d) (%d/%d) %g\n",
		i,m - 1,j,dim,means->data.F32[i][j]);
      }
    }
#endif
  }
  
  // Do K-means/GMM iteration for m.
  
  iterations = 0;
  
  if (!psMMkmeans(D,dim,N,modes,means,m,
		  &iterations,&V)) {
    // Error calculating k-means
  }

#if (TESTING)
  for (int i = 0 ; i < m; i++) {
    for (int j = 0; j < dim; j++) {
      fprintf(stderr,"KMEANS POST: (%d/%d) (%d/%d) %g\n",
	      i,m,j,dim,means->data.F32[i][j]);
    }
  }
#endif
  
  iterations = 0;
  if (!psMMGMM(D,dim,N,modes,means,sigma,pi,P,
	       m,&iterations,&V)) {
    // Error calculating GMM
  }

#if (TESTING)
  for (int i = 0 ; i < m; i++) {
    for (int j = 0; j < dim; j++) {
      fprintf(stderr,"GMM POST: (%d/%d) (%d/%d) %g\n",
	      i,m,j,dim,means->data.F32[i][j]);
    }
  }
#endif
  
  // Calculate utility of m modes over m-1 modes.
  *Pval = gsl_cdf_chisq_Q(-2.0 * (V - V0), 3 * dim);
  return(true);
}

bool psMM1Dkmeans(psVector *D,
		  long N,
		  psVector *modes,
		  psVector *means,
		  int m,
		  int *iterations,
		  double *V) {
  int i,k;
  psImage *DD = psImageAlloc(1,N,PS_TYPE_F32);
  for (k = 0; k < N; k++) {
    DD->data.F32[k][0] = D->data.F32[k];
  }
  psImage *MM = psImageAlloc(m,1,PS_TYPE_F32);
  if (!psMMkmeans(DD,1,N,modes,MM,m,iterations,V)) {
    // Error
  }
  psFree(DD);
  for (i = 0; i < N; i++) {
    means->data.F32[i] = MM->data.F32[i][0];
  }
  psFree(MM);
  return(true);
}

bool psMM1DGMM(psVector *D,
	       long N,
	       psVector *modes,
	       psVector *means,
	       psVector *S,
	       psVector *pi,
	       psImage  *P,
	       int m,
	       int *iterations,
	       double *V) {
  int i,k;
  psImage *DD = psImageAlloc(1,N,PS_TYPE_F32);
  for (k = 0; k < N; k++) {
    DD->data.F32[k][0] = D->data.F32[k];
  }
  psImage *MM = psImageAlloc(1,m,PS_TYPE_F32);
  
  psArray *sigma = psArrayAlloc(0);
  for (k = 0; k < m; k++) {
    psImage *Stmp = psImageAlloc(2,2,PS_TYPE_F32);
    Stmp->data.F32[0][0] = S->data.F32[k];
    psArrayAdd(sigma,0,S);
  }
  if (!psMMGMM(DD,1,N,modes,MM,sigma,pi,P,m,iterations,V)) {
    // Error
  }
  psFree(DD);
  for (i = 0; i < N; i++) {
    means->data.F32[i] = MM->data.F32[i][0];
  }
  psFree(MM);
  psFree(sigma);
  return(true);
}

bool psMM1DClass(psVector *D,
		 long N,
		 psVector *modes,
		 psVector *means,
		 psVector *S,
		 psVector *pi,
		 psImage  *P,
		 int m,
		 double *Pval) {
  int i,k;
  psImage *DD = psImageAlloc(1,N,PS_TYPE_F32);
  for (k = 0; k < N; k++) {
    DD->data.F32[k][0] = D->data.F32[k];
  }
  psImage *MM = psImageAlloc(1,m,PS_TYPE_F32);
  
  psArray *sigma = psArrayAlloc(0);
  for (i = 0; i < m; i++) {
    psImage *Stmp = psImageAlloc(1,1,PS_TYPE_F32);
    if (S->data.F32[i] != 0.0) {
      Stmp->data.F32[0][0] = S->data.F32[i];
    }
    else {
      Stmp->data.F32[0][0] = 1.0;
    }
    psArrayAdd(sigma,0,Stmp);
  }
  if (!psMMClass(DD,1,N,modes,MM,sigma,pi,P,m,Pval)) {
    // Error
  }
  psFree(DD);
  for (i = 0; i < m; i++) {
    psImage *Stmp = sigma->data[i];
    means->data.F32[i] = MM->data.F32[i][0];
    S->data.F32[i]     = sqrt(Stmp->data.F32[0][0]); // GMM actually calculates the covariance, so take sqrt
  }
  psFree(MM);
  psFree(sigma);
  return(true);
}

