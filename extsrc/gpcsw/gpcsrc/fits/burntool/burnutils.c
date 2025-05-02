/* burnutils.c - various utility functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "math.h"

#include "burntool.h"
#include "burnparams.h"

/****************************************************************/
/* Slide out the walls of a box until they fall below thresh; bottom fixed */
STATIC int grow_box(int nx, int ny, int NX, DTYPE *data, int thr, OBJBOX *box)
{
   int i, j,i2,j2, is, ie,jlo,jhi,ilo,ihi, left, right;

   left = right = 1;

   if(VERBOSE & VERB_BOXGROW) {
      printf("box: %d %d %d %d\n", box->sx, box->sy, box->ex, box->ey);
   }

   do {
      /* Push the right wall until it's clear */
      jlo = box->sy;
      jhi = box->ey;
      if(right) {
	     for(i=box->ex+1; i<nx; i++) {
	       for(j=box->sy; j<=box->ey; j++) if(data[i+j*NX] >= thr) break;
          if(j > box->ey) break;
          for(j2=box->ey; j2>=box->sy; j2--) if(data[i+j2*NX] >= thr) break;
          if(j2 < box->sy) break;

          /*Since stars are roughly diamonds, the new vertices should be contained within the previous ones*/
          /*If not, there is a discontinuity, and we might have overlapping sources*/
          if(!(j >= jlo-0.1*(jhi-jlo) && j2 <= jhi+0.1*(jhi-jlo))) {break;}

	       if(VERBOSE & VERB_BOXGROW) {
	         printf("right: %8d %8d %8d\n", i, j,data[i+j*NX]);
	       }
          jlo = j;
          jhi = j2;
	     }
	     box->ex = i - 1;
	     right = 0;
      }

      /* Push the left wall until it's clear */
      jlo = box->sy;
      jhi = box->ey;
      if(left) {
	     for(i=box->sx-1; i>=0; i--) {
	       for(j=box->sy; j<=box->ey; j++) if(data[i+j*NX] >= thr) break;
	       if(j > box->ey) break;
          for(j2=box->ey; j2<=box->sy; j2--) if(data[i+j2*NX] >= thr) break;
          if(j2 < box->sy) break;

          /*Since stars are roughly diamonds, the new vertices should be contained within the previous ones*/
          /*If not, there is a discontinuity, and we might have overlapping sources*/
          if(!(j >= jlo-0.1*(jhi-jlo) && j2 <= jhi+0.1*(jhi-jlo))) {break;}

	       if(VERBOSE & VERB_BOXGROW) {
	         printf("left: %8d %8d %8d\n", i, j,data[i+j*NX]);
	       }
          jlo = j;
          jhi = j2;
	     }
	     box->sx = i + 1;
	     left = 0;
      }

      /* Push the top wall until it's clear, catching corners! */
      is = MAX(0, box->sx-1);
      ie = MIN(nx-1, box->ex+1);
      ilo = is;
      ihi = ie;
      for(j=box->ey+1; j<ny; j++) {
	     for(i=is; i<=ie; i++) if(data[i+j*NX] >= thr) break;
        if(i > ie) break;        /* All clear */
        for(i2=ie; i2<=is; i--) if(data[i2+j*NX] >= thr) break;
        if(i2 < is) break;        /* All clear */

	     if(i == is && box->sx>0) left = 1;	/* First point hits thr, so need to redo left */
	     if(data[ie+j*NX] >= thr && box->ex<nx-1) right = 1;  /*Last point hits thr, so redo right */

        /*Since stars are roughly diamonds, the new vertices should be contained within the previous ones*/
        /*If not, there is a discontinuity, and we might have overlapping sources*/
        if(!(i >= ilo-0.1*(ihi-ilo) && i2 <= ihi+0.1*(ihi-ilo))) {break;}

	     if(VERBOSE & VERB_BOXGROW) {
	       printf("top: %8d %8d %8d\n", i, j,data[i+j*NX]);
	     }
        ilo = i;
        ihi = i2;
      }
      box->ey = j - 1;

      /* Push the bottom wall until it's clear, catching corners! */
      is = MAX(0,box->sx-1);
      ie = MIN(nx-1,box->ex+1);
      ilo = is;
      ihi = ie;
      for(j=box->sy-1; j>=0; j--) {
	     for(i=is; i<=ie; i++) if(data[i+j*NX] >= thr) break;
        if(i > ie) break;        /* All clear */
        for(i2=ie; i2<=is; i--) if(data[i2+j*NX] >= thr) break;
        if(i2 < is) break;        /* All clear */

	     if(i == is && box->sx>0) left = 1;	/* Need to redo left */
        if(data[ie+j*NX] >= thr && box->ex<nx-1) right = 1;  /* Redo right */

	     if(data[is+1+j*NX] >= thr) box->y0m = j;
	     if(data[ie-1+j*NX] >= thr) box->y1m = j;

        /*Since stars are roughly diamonds, the new vertices should be contained within the previous ones*/
        /*If not, there is a discontinuity, and we might have overlapping sources*/
        if(!(i >= ilo-0.1*(ihi-ilo) && i2 <= ihi+0.1*(ihi-ilo))) break;

	     if(VERBOSE & VERB_BOXGROW) {
	       printf("bottom: %8d %8d %8d %8d %8d\n", i, j, box->y0m, box->y1m,data[i+j*NX]);
	     }
        ilo = i;
        ihi = i2;
      }
      box->sy = j + 1;

      if(left) box->sx -= 1;
      if(right) box->ex += 1;

   } while(left || right);

/* Find the side saturation points */
   for(j=box->sy; j<box->ey; j++) if(data[box->sx+j*NX] >= thr) break;
   box->y0m = j;

   for(j=box->ey; j>box->sy; j--) if(data[box->sx+j*NX] >= thr) break;
   box->y0p = j;

   for(j=box->sy; j<box->ey; j++) if(data[box->ex+j*NX] >= thr) break;
   box->y1m = j;

   for(j=box->ey; j>box->sy; j--) if(data[box->ex+j*NX] >= thr) break;
   box->y1p = j;

   for(i=box->sx; i<box->ex; i++) if(data[i+box->ey*NX] >= thr) break;
   box->x1m = i;

   for(i=box->ex; i>box->sx; i--) if(data[i+box->ey*NX] >= thr) break;
   box->x1p = i;

   for(i=box->sx; i<box->ex; i++) if(data[i+box->sy*NX] >= thr) break;
   box->x0m = i;

   for(i=box->ex; i>box->sx; i--) if(data[i+box->sy*NX] >= thr) break;
   box->x0p = i;

   if(VERBOSE & VERB_BOXGROW) {
      printf("box: %d %d %d %d\n", box->sx, box->sy, box->ex, box->ey);
      fflush(stdout);
   }

   return(0);
}

/****************************************************************/
/* Expand the mask by pushing out boxes by a factor */
STATIC int grow_mask(int nx, int ny, int NX, DTYPE *mask, double bfac, 
		     double rfac, int maskval, int nbox, OBJBOX *box)
{
   int i, j, k, x0, y0, db, dr, r2, i0, i1, j0, j1,boxsize;

   for(k=0; k<nbox; k++) {
/* Push out the box by a distance bfac */
      db = NINT(bfac);
      j0 = MAX(0, box[k].sy-db);
      j1 = MIN(ny-1, box[k].ey+db);
      i0 = MAX(0, box[k].sx-db);
      i1 = MIN(nx-1, box[k].ex+db);

      for(j=j0; j<=j1; j++) {
	     for(i=i0+1; i<i1; i++) {
	      if(mask[i+j*NX] == MASK_NONE) mask[i+j*NX] = maskval;
	      if(mask[i+j*NX] == MASK_NONE) mask[i+j*NX] = maskval;
	     }
      }

/* Push out the center to a diameter rfac*xsize */
      x0 = (box[k].sx + box[k].ex + 1) / 2;
      //y0 = (box[k].y0p + box[k].y1p + box[k].y0m + box[k].y1m + 2) / 4;
      y0 = (box[k].sy + box[k].ey + 1) / 2;
      boxsize = MAX((box[k].ex - box[k].sx + 1),(box[k].ey - box[k].sy + 1));
      dr = NINT(boxsize * 0.5 * rfac);
      for(j=MAX(0,y0-dr); j<=MIN(ny-1, y0+dr); j++) {
	     for(i=MAX(0,x0-dr); i<=MIN(nx-1, x0+dr); i++) {
	       r2 = (i-x0)*(i-x0) + (j-y0)*(j-y0);
	       if(r2 < dr*dr && mask[i+j*NX] == MASK_NONE)
	       mask[i+j*NX] = maskval;
	     }
      }
   }
   return(0);
}

#if 0
/****************************************************************/
/* Expand the mask by pushing out boxes by a factor */
STATIC int grow_mask(int nx, int ny, int NX, DTYPE *mask, 
		     double xfac, double yfac, int nbox, OBJBOX *box)
{
   int i, j, k, x0, y0, dx, dy;

   for(k=0; k<nbox; k++) {
      x0 = (box[k].sx + box[k].ex + 1) / 2;
      y0 = (box[k].sy + box[k].ey + 1) / 2;
      dx = NINT((box[k].ex - box[k].sx + 1) * 0.5 * xfac);
      dy = NINT((box[k].ey - box[k].sy + 1) * 0.5 * yfac);
      for(j=MAX(0,y0-dy); j<MIN(ny, y0+dy); j++) {
	 for(i=MAX(0,x0-dx); i<MIN(nx, x0+dx); i++) {
	    if(mask[i+j*NX] == MASK_NONE) mask[i+j*NX] = MASK_HALO;
	 }
      }
   }
   return(0);
}
#endif

/****************************************************************/
/* Test to see whether a spot is a not-so-local maximum */
STATIC int local_max(int i, int j, int r, int nx, int ny, int NX, DTYPE *data)
{
   int k, l;
   if(i < r || i > nx-1-r || j < r || j > ny-1-r) return(0);
   if(data[i+j*NX] <= data[i+1+j*NX] || 
      data[i+j*NX] <  data[i-1+j*NX] ||
      data[i+j*NX] <= data[i+(j+1)*NX] ||
      data[i+j*NX] <  data[i+(j-1)*NX]) return(0);
   for(l=-r; l<=r; l++) {
      for(k=-r; k<=r; k++) {
	 if(data[i+j*NX] < data[i+k+(j+l)*NX]) return(0);
      }
   }
   return(1);
}

#ifndef INSERT_MEDIAN	/* Faster quick sort */
/****************************************************************/
STATIC int int_median(int n, int *key)
{
   if(n == 0) return(0);
   qsort_int(n, key);
   return((key[n/2]+key[(n-1)/2]) / 2);
}

/****************************************************************/
STATIC double double_median(int n, double *key)
{
   if(n == 0) return(0.0);
   qsort_dbl(n, key);
   return( 0.5*(key[n/2]+key[(n-1)/2]) );
}

#else	/* Slower sort by insertion */
/****************************************************************/
STATIC int int_median(int n, int *key)
{
   int i, j, k;
   int tmp;

   if(n == 0) return(0);
   for(j=n-2; j>=0; j--) {
      for(i=j+1, k=j; i<n; i++) {
	 if(key[j] <= key[i]) break;
	 k = i;
      }
      if(k == j) continue;
      tmp = key[j];
      for(i=j+1; i<=k; i++) {
	 key[i-1] = key[i];
      }
      key[k] = tmp;
   }
   i = (key[n/2]+key[(n-1)/2]) / 2;
   return(i);
}

/****************************************************************/
STATIC double double_median(int n, double *key)
{
   int i, j, k;
   double tmp;

   if(n == 0) return(0.0);
   for(j=n-2; j>=0; j--) {
      for(i=j+1, k=j; i<n; i++) {
	 if(key[j] <= key[i]) break;
	 k = i;
      }
      if(k == j) continue;
      tmp = key[j];
      for(i=j+1; i<=k; i++) {
	 key[i-1] = key[i];
      }
      key[k] = tmp;
   }
   return( 0.5*(key[n/2]+key[(n-1)/2]) );
}
#endif

/****************************************************************/
/* Fit y = ax + b */
STATIC int wlinearfit(int npt, double *x, double *y, double *w, 
		      double *a, double *b)
{
   int i;
   double v0=0.0, v1=0.0, m00=0.0, m01=0.0, m11=0.0, det;

/* Accumulate sums for least squares fit */
   for(i=0; i<npt; i++) {
      v0 += y[i] * w[i];
      v1 += y[i] * x[i] * w[i];
      m00 += w[i];
      m01 += x[i] * w[i];
      m11 += x[i] * x[i] * w[i];
   }

/* And solve the matrix */
   det = m00 * m11 - m01 * m01;
   if(det == 0.0) {
      *a = *b = 0.0;
      return(-1);
   }
   *b = (v0*m11-v1*m01) / det;
   *a = (v1*m00-v0*m01) / det;
   return(0);
}

/****************************************************************/
/* Return of RMS relative to Fit y = ax + b */
STATIC int linearrms(int npt, double *x, double *y, double a, double b, 
		     double *rms)
{
   *rms = 0.0;
   while(--npt >= 0) *rms += (y[npt]-a*x[npt]-b)*(y[npt]-a*x[npt]-b);
   if(*rms > 0) *rms = sqrt(*rms);
   return(0);
}
