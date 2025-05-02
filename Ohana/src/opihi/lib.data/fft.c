# include "data.h"

// fft based on code by Douglas L. Jones (see note at EOF). modified for Ohana C style
void fft1D (float *x, float *y, int n, int Nbit, int forward) {

  int i,j,k,n1,n2;
  float c,s,e,a,t1,t2;        
  double factor;
         
  // bit-reverse
  j = 0; 
  n2 = n/2;
  for (i = 1; i < n - 1; i++) {
    n1 = n2;
    while ( j >= n1 ) {
      j -= n1;
      n1 /= 2;
    }
    j += n1;
               
    if (i < j) {
      t1 = x[i];
      x[i] = x[j];
      x[j] = t1;
      t1 = y[i];
      y[i] = y[j];
      y[j] = t1;
    }
  }
                                          
  n1 = 0; /* FFT */
  n2 = 1;
                                             
  if (forward) {
    factor = +2.0*M_PI;
  } else {
    factor = -2.0*M_PI;
  }

  for (i=0; i < Nbit; i++) {
    n1 = n2;
    n2 = n2 + n2;
    e = factor/n2;
    a = 0.0;
                                             
    for (j=0; j < n1; j++) {
      c = cos(a);
      s = sin(a);
      a = a + e;
                                            
      for (k=j; k < n; k=k+n2) {
	t1 = c*x[k+n1] - s*y[k+n1];
	t2 = s*x[k+n1] + c*y[k+n1];
	x[k+n1] = x[k] - t1;
	y[k+n1] = y[k] - t2;
	x[k] = x[k] + t1;
	y[k] = y[k] + t2;
      }
    }
  }
                                      
  // re-normalize
  for (i = 0; i < n; i++) {
    x[i] /= n;
    y[i] /= n;
  }

  return;
}                          

// This implementation uses the 1-D fft above for each of the vectors in each dimension.
// This requires 2(Nx*Ny*...) mem copies, but the fft operations are likely to happen in
// cache.
int fftND (float *x, float *y, int Ndim, off_t *Nsize, int forward) {

  int i, nIndex, minor, major, iDim;
  int step, Nmajor, Nminor, Nmax, Ntotal;
  int *Nbit;
  float *tmpX, *tmpY;

  ALLOCATE (Nbit, int, Ndim);

  // find the longest axis and allocate storage for that length
  Nmax = 0;
  Ntotal = 1;
  for (i = 0; i < Ndim; i++) {
    Nmax = MAX(Nmax, Nsize[i]);
    Ntotal *= Nsize[i];
    if (!IsBinary (Nsize[i], &Nbit[i])) {
      free (Nbit);
      return (FALSE);
    }
  }
  ALLOCATE (tmpX, float, Nmax);
  ALLOCATE (tmpY, float, Nmax);
  
  step = 1;
  Nminor = 1;
  Nmajor = Ntotal;
  for (iDim = 0; iDim < Ndim; iDim++) {
    step *= Nsize[iDim];
    Nmajor /= Nsize[iDim];

    // we perform the FFT along all other dimensions 
    for (major = 0; major < Nmajor; major++) {
      for (minor = 0; minor < Nminor; minor++) {
	// nIndex = minor + i*Nminor + major*step;
	// extract the data values to the temp vector
	nIndex = minor + major*step;
	for (i = 0; i < Nsize[iDim]; i++) {
	  tmpX[i] = x[nIndex];
	  tmpY[i] = y[nIndex];
	  nIndex += Nminor;
	}

	fft1D (tmpX, tmpY, Nsize[iDim], Nbit[iDim], forward);

	// replace the result vectors
	nIndex = minor + major*step;
	for (i = 0; i < Nsize[iDim]; i++) {
	  x[nIndex] = tmpX[i];
	  y[nIndex] = tmpY[i];
	  nIndex += Nminor;
	}
      }
    }
    Nminor *= Nsize[iDim];
  }
  free (Nbit);
  free (tmpX);
  free (tmpY);
  return (TRUE);
}

// fft based on code by Douglas L. Jones (see note at EOF). modified for Ohana C style
void dfft1D (double *x, double *y, int n, int Nbit, int forward) {

  int i,j,k,n1,n2;
  double c,s,e,a,t1,t2;        
  double factor;
         
  // bit-reverse
  j = 0; 
  n2 = n/2;
  for (i = 1; i < n - 1; i++) {
    n1 = n2;
    while ( j >= n1 ) {
      j -= n1;
      n1 /= 2;
    }
    j += n1;
               
    if (i < j) {
      t1 = x[i];
      x[i] = x[j];
      x[j] = t1;
      t1 = y[i];
      y[i] = y[j];
      y[j] = t1;
    }
  }
                                          
  n1 = 0; /* FFT */
  n2 = 1;
                                             
  if (forward) {
    factor = +2.0*M_PI;
  } else {
    factor = -2.0*M_PI;
  }

  for (i=0; i < Nbit; i++) {
    n1 = n2;
    n2 = n2 + n2;
    e = factor/n2;
    a = 0.0;
                                             
    for (j=0; j < n1; j++) {
      c = cos(a);
      s = sin(a);
      a = a + e;
                                            
      for (k=j; k < n; k=k+n2) {
	t1 = c*x[k+n1] - s*y[k+n1];
	t2 = s*x[k+n1] + c*y[k+n1];
	x[k+n1] = x[k] - t1;
	y[k+n1] = y[k] - t2;
	x[k] = x[k] + t1;
	y[k] = y[k] + t2;
      }
    }
  }
                                      
  // re-normalize
  for (i = 0; i < n; i++) {
    x[i] /= n;
    y[i] /= n;
  }

  return;
}                          

// This implementation uses the 1-D fft above for each of the vectors in each dimension.
// This requires 2(Nx*Ny*...) mem copies, but the fft operations are likely to happen in
// cache.
int dfftND (double *x, double *y, int Ndim, int *Nsize, int forward) {

  int i, nIndex, minor, major, iDim;
  int step, Nmajor, Nminor, Nmax, Ntotal;
  int *Nbit;
  double *tmpX, *tmpY;

  ALLOCATE (Nbit, int, Ndim);

  // find the longest axis and allocate storage for that length
  Nmax = 0;
  Ntotal = 1;
  for (i = 0; i < Ndim; i++) {
    Nmax = MAX(Nmax, Nsize[i]);
    Ntotal *= Nsize[i];
    if (!IsBinary (Nsize[i], &Nbit[i])) {
      free (Nbit);
      return (FALSE);
    }
  }
  ALLOCATE (tmpX, double, Nmax);
  ALLOCATE (tmpY, double, Nmax);
  
  step = 1;
  Nminor = 1;
  Nmajor = Ntotal;
  for (iDim = 0; iDim < Ndim; iDim++) {
    step *= Nsize[iDim];
    Nmajor /= Nsize[iDim];

    // we perform the FFT along all other dimensions 
    for (major = 0; major < Nmajor; major++) {
      for (minor = 0; minor < Nminor; minor++) {
	// nIndex = minor + i*Nminor + major*step;
	// extract the data values to the temp vector
	nIndex = minor + major*step;
	for (i = 0; i < Nsize[iDim]; i++) {
	  tmpX[i] = x[nIndex];
	  tmpY[i] = y[nIndex];
	  nIndex += Nminor;
	}

	dfft1D (tmpX, tmpY, Nsize[iDim], Nbit[iDim], forward);

	// replace the result vectors
	nIndex = minor + major*step;
	for (i = 0; i < Nsize[iDim]; i++) {
	  x[nIndex] = tmpX[i];
	  y[nIndex] = tmpY[i];
	  nIndex += Nminor;
	}
      }
    }
    Nminor *= Nsize[iDim];
  }
  free (Nbit);
  free (tmpX);
  free (tmpY);
  return (TRUE);
}

// check that a number is binary (2^Nbit).  returns int(log_2(N)) in Nbit
int IsBinary (int N, int *Nbit) {

  int i, Nset;

  if (Nbit != NULL) *Nbit = 0;
  Nset = 0;
  for (i = 0; i < 8*sizeof(int); i++) {
    if (N & 0x01) {
      Nset ++;
      if (Nbit != NULL) *Nbit = i;
    }
    N >>= 1;
  }
  if (Nset > 1) return (FALSE);
  return (TRUE);  
}

/**********************************************************/
/* fft.c                                                  */
/* (c) Douglas L. Jones                                   */
/* University of Illinois at Urbana-Champaign             */
/* January 19, 1992                                       */
/*                                                        */
/*   fft: in-place radix-2 DIT DFT of a complex input     */
/*                                                        */
/*   input:                                               */
/* n: length of FFT: must be a power of two               */
/* m: n = 2**m                                            */
/*   input/output                                         */
/* x: float array of length n with real part of data     */
/* y: float array of length n with imag part of data     */
/*                                                        */
/*   Permission to copy and use this program is granted   */
/*   under a Creative Commons "Attribution" license       */
/*   http://creativecommons.org/licenses/by/1.0/          */
/**********************************************************/

