/* Routine to fit a linear function */
/* 060304 v1.2 add wlinearfit() */
/* 060130 v1.1 replaced static allocations with calloc's */
/* 040208 v1.0 John Tonry (derived from fortran) */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define ABS(x) ((x)<0?(-(x)):(x))

static int linsolve(int n, double *y, double *a, double *x);

int linearfit(int npt, double *y, int nx, double *x, double *param)
{
/* Y = NPT data being fitted as a linear function of X[*][NX]
 * PARAM = returned parameters of fit:
 *	Y[*] = PARAM[0]*X[*][0],
 *	     + PARAM[1]*X[*][1],
 *		...
 *	     + PARAM[NX-1]*X[*][NX-1]
 *
 */
   int i, j, k;
   double *v, *a;
   v = (double *)calloc(nx, sizeof(double));
   a = (double *)calloc(nx*nx, sizeof(double));

   if(npt < nx) {
      fprintf(stderr, "linearfit: Too few points provided: %d %d\n", npt, nx);
      return(-2);
   }

#if 0		/* Show us data to solve? */
   for(i=0; i<npt; i++) {
      fprintf(stderr, "%3d: %8.1f = ", i, y[i]);
      for(j=0; j<nx; j++) fprintf(stderr, " A%d * %8.1f +", j, x[nx*i+j]);
      fprintf(stderr, "\n");
   }
#endif

/* Zero matrices */
   for(j=0; j<nx; j++) {
      v[j] = 0.0;
      for(i=0; i<nx; i++) a[i+nx*j] = 0.0;
   }

/* Accumulate sums for least squares fit */
   for(i=0; i<npt; i++) {
      for(j=0; j<nx; j++) {
	 v[j] += y[i] * x[nx*i+j];
	 for(k=0; k<=j; k++) a[k+j*nx] += x[nx*i+j] * x[nx*i+k];
      }
   }

/* Fill in symmetrical matrix */
   for(j=0; j<nx-1; j++) {
      for(k=j+1; k<nx; k++) a[k+j*nx] = a[j+k*nx];
   }

/* And solve the matrix */
   if(linsolve(nx, v, a, param)) {
      free(v);
      free(a);
      return(-1);
   }
   free(v);
   free(a);

   return(0);
}

/* linsolve.c - solve a set of linear equations */
/*
 * Solves the matrix equation   Y = A X,   returns X.
 *        where y[j=0,n-1], x[i=0,n-1], a[i+j*n].
 * 
 */
/* 021007 - Rev 1.1 add NY option */
/* 020211 - Rev 1.0 John Tonry */

static int linsolve(int n, double *y, double *a, double *x)
{
   int i, j, k;
   int *rowstatus, *row;
   double rat;

   rowstatus = (int *)calloc(n, sizeof(int));
   row = (int *)calloc(n, sizeof(int));

   for(i=0; i<n; i++) rowstatus[i] = 0;

/* Solve matrix by Gaussian elimination */
   for(j=0; j<n; j++) {		/* j = column to be zero'ed out */

/* Find a good equation to work on (pivot row) */
      for(i=k=0, rat=0.0; i<n; i++) {
	 if(rowstatus[i]) continue;
	 if(ABS(a[j+i*n]) > rat) {
	    k = i;
	    rat = ABS(a[j+k*n]);
	 }
      }
      rowstatus[k] = 1;
      row[j] = k;
      if(rat == 0.0) {
	 fprintf(stderr, "WHOA: singular matrix, iter %d, row %d\n", j, k);
	 free(rowstatus);
	 free(row);
	 return(1);
      }
/* Subtract away matrix below diagonal */
      for(i=0; i<n; i++) {	/* i = subsequent equations */
	 if(rowstatus[i]) continue;
	 rat = a[j+i*n] / a[j+row[j]*n];
	 y[i] -= rat * y[row[j]];
 	 for(k=j+1; k<n; k++) a[k+i*n] -= rat * a[k+row[j]*n];
      }
   }

/* Back substitute into upper diagonal matrix to solve for parameters */
   for(j=n-1; j>=0; j--) {
      x[j] = y[row[j]];
      for(k=j+1; k<n; k++) x[j] -= a[k+row[j]*n] * x[k];
      x[j] /= a[j+row[j]*n];
   }
   free(rowstatus);
   free(row);
   return(0);
}
