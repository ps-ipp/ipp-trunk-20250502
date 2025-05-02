#ifndef PS_MIXTUREMODELS_H
#define PS_MIXTUREMODELS_H

// These are the recommended functions to call.  They in turn call code
// that scans the input data matrix/image and censored data points with
// non-finite values.  The censored data is then passed to the next set
// of functions.

bool psMMkmeans(psImage *D,
		int dim,
		long N,
		psVector *modes,
		psImage *means,
		int m,
		int *iterations,
		double *V);

bool psMMGMM(psImage *D,
	     int dim,
	     long N,
	     psVector *modes,
	     psImage *means,
	     psArray *sigma,
	       psVector *pi,
	     psImage *P,
	     int m,
	     int *iterations,
	     double *V);

// These functions assume that the input data matrix/image is cleaned of
// non-finite values.  They should work identically as the ones above on
// data that is well defined.
bool psMMkmeansUncensored(psImage *D,
			  int dim,
			  long N,
			  psVector *modes,
			  psImage *means,
			  int m,
			  int *iterations,
			  double *V);

bool psMMGMMUncensored(psImage *D,
		       int dim,
		       long N,
		       psVector *modes,
		       psImage *means,
		       psArray *sigma,
		       psVector *pi,
		       psImage *P,
		       int m,
		       int *iterations,
		       double *V);

// This function does the classification in five stages:
//  1) pass data to K-means code with m modes for initial values.
//  2) pass data+K-mean values to GMM code with m modes for best values.
//  3) repeat K-means pass with m-1 modes.
//  4) repeat GMM pass with m-1 modes.
//  5) calculate Pvalue that the model with m modes is better than that with m-1
bool psMMClass(psImage *D,
	       int dim,
	       long N,
	       psVector *modes,
	       psImage *means,
	       psArray *sigma,
	       psVector *pi,
	       psImage *P,
	       int m,
	       double *Pval);


// These functions are one dimensional equivalent wrappers of the above functions.
// The psVector data is inserted into the appropriate sized psImage structure to
// avoid having to do so in each program calling this library.  Since this is the
// only special case (2-dim is a matrix, n-dim is matrix as well), no other wrappers
// are needed.
bool psMM1Dkmeans(psVector *D,
		  long N,
		  psVector *modes,
		  psVector *means,
		  int m,
		  int *iterations,
		  double *V);

bool psMM1DGMM(psVector *D,
	       long N,
	       psVector *modes,
	       psVector *means,
	       psVector *S,
	       psVector *pi,
	       psImage  *P,
	       int m,
	       int *iterations,
	       double *V);

bool psMM1DClass(psVector *D,
		 long N,
		 psVector *modes,
		 psVector *means,
		 psVector *S,
		 psVector *pi,
		 psImage  *P,
		 int m,
		 double *Pval);
    
	
#endif
