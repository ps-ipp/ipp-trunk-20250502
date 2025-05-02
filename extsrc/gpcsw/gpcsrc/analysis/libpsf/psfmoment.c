/* psfmoment.c: find and fit the brightest star in an image; donuts OK */

/* Note: this is not as fast as psf() nor as accurate as jtpsf() for
 * flux, width, and position, but it is tolerant of donuts and should
 * provide a pretty good width and center.
 */
/* 080815 v1.0 John Tonry */

#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <string.h>
#include <math.h>
#include "psf.h"

//#define TEST

// #define XBORD 4		/* Disregard this border on the x sides */
// #define YBORD 2		/* Disregard this border on the y sides */
#define SIGCLIP 5.0	/* Sky clip level */
#define FWRMS 2.3548	/* Conversion factor: FWHM/RMS */
#define CTRTOL 1.0	/* Tolerance for centroid to move between passes */

/* Find and report on the brightest star; will work with donuts... */
int psfmoment(int sx, 		/* x offset of pixel 0,0 */
	     int sy,  		/* y offset of pixel 0,0 */
	     int binx,  		/* x bin factor */
	     int biny,   		/* y bin factor */
/* The "world coords" and FWHM are derived from pixel coords off of the data by
 *    x_world = *xu = sx + x_data*binx
 *    y_world = *yu = sy + y_data*binx
 * Center of the first pixel is (0.5,0.5)
 */
	     int bordx,  		/* x border */
	     int bordy,   		/* y border */
	     int nx,   			/* x size of image */
	     int ny,    		/* y size of image */
	     int NX,   			/* x size of image */
	     unsigned short *data,	/* ushort image data: 0 = *NO_DATA* */
	     double *xu,		/* Fitted x position */
	     double *yu,		/* Fitted y position */
	     int *fmax,			/* Highest pixel in best bin */
	     double *fwhm,		/* FWHM of psf (binx,y corrected) */
	     double *xfwhm,		/* FWHM in x dir (binx corrected) */
	     double *yfwhm,		/* FWHM in y dir (biny corrected) */
	     double *bkgnd,		/* Sky level */
	     double *ftot,		/* Total flux */
	     double *weight,		/* Composite quality of star: <1 bad,*/
	     				/*    1-10 so-so, 10-30 OK, >30 fine */
	     double *snr,		/* S/N of (big-sky)/noise */
	     int *extra)		/* More return stuff? */
{
   int i, j, k, med=0, lq=0, uq=0, bright=0, bright2=0, flim1, flim2;
   int i0, i1, i2, npix, nbig, pass, biggie, ibig=0, jbig=0;
   double x0=0.0, y0=0.0, xc=0.0, yc=0.0, a4, a5, a6, mxmy_14, fluxsum, sky;
   double x1, y1, x2, y2, mx, my, mxy, flux1, flux2, nearlow, r2;
   double theta, a=0.0;
   double rms, kappa, kappa2, rcut, flux;
   int count[65536];

/* Determine the sky level */
   kappa = 0.99;		/* Fraction of pixels dim enough to ignore */
   kappa2 = 0.75;		/* Fraction of pixels dim enough to ignore */
   bzero(count, 65536*sizeof(int));
   for(j=bordy, flux=0.0; j<ny-bordy; j++) {
      for(i=bordx; i<nx-bordx; i++) {
	 (count[data[i+j*NX]])++;
/* Where's the biggest pixel? */
	 if(data[i+j*NX] > flux) {
	    flux = data[i+j*NX];
	    x0 = i;
	    y0 = j;
	 }
      }
   }
   i0 = 0.5*(nx-2*bordx)*(ny-2*bordy);
   i1 = kappa*(nx-2*bordx)*(ny-2*bordy);
   i2 = kappa2*(nx-2*bordx)*(ny-2*bordy);
   for(i=1, j=0; i<65536; i++) {
      j += count[i];
      if(j < i0/2) lq = i;
      if(j < i0) med = i;
      if(j < (3*i0)/2) uq = i;
      if(j < i2) bright2 = i;
      if(j < i1) bright = i;
   }
/* Get sky as median between median +/- SIGCLIP*quartile */
   i0 = MAX(0,med-SIGCLIP*(med-lq));
   i1 = MIN(65535,med+SIGCLIP*(med-lq));
   for(i=i0, k=0; i<=i1; i++) k += count[i];
   if(k == 0) {
      fprintf(stderr, "psfmoment: No sky pixels at all?\n");
      return(-1);
   }
   for(i=i0, j=0; i<=i1; i++) {
      j += count[i];
      if(j >= k/2) break;
   }
   sky = i;
   if(count[i] > 0) sky += ((double)(j-k/2))/count[i] - 1.0;

   rms = (uq-med) / 0.7;
   if(rms < 0.5) rms = 0.5;

/* subtract sky, clip noise and sum flux and first and moments */
   flim2 = bright - sky;	/* Flux limit for RMS calculation */
   flim1 = MIN(2*rms, flim2);	/* Flux limit for centroid calculation */
   rcut = MAX(binx*nx,biny*ny);	/* Radius limit */

#ifdef TEST
   printf("i0,i1,lq,med,uq,bright= %d %d %d %d %d %d sky=%.1f rms=%.1f flim1=%d flim2=%d\n", 
	  i0,i1,lq,med,uq,bright,sky,rms,flim1,flim2);
#endif

/* Three passes: find center, get decent FWHM, tune it up (if necessary) */
   for(pass=0; pass<3; pass++) {
      fluxsum = 0.0;
      x1 = y1 = flux1 = 0.0;
      x2 = y2 = mx = my = mxy = flux2 = 0.0;
      npix = nbig = biggie = 0;
      nearlow = MAX(nx*nx, ny*ny);
      for(j=bordy; j<ny-bordy; j++) {
	 for(i=bordx; i<nx-bordx; i++) {
	    flux = data[i+j*NX] - sky;
	    r2 = binx*binx*(i+0.5-x0)*(i+0.5-x0) +
	         biny*biny*(j+0.5-y0)*(j+0.5-y0);
/* Where's the nearest pixel which is near sky? */
	    if(flux < rms && r2 < nearlow) nearlow = r2;
/* Sum up moments within a radius of rcut */
	    if(r2 < rcut*rcut) {
/* Very low flux cut to get total flux */
	       if(flux > -2*rms) fluxsum += flux;
/* Modest flux cut to get center */
	       if(flux > flim1) {
		  npix++;
		  flux1 += flux;
		  x1 += (i+0.5) * flux;
		  y1 += (j+0.5) * flux;
	       }
/* Higher flux cut to get moment */
	       if(flux > flim2) {
		  nbig++;
		  flux2 += flux;
		  x2 += (i+0.5) * flux;
		  y2 += (j+0.5) * flux;
		  mx += (i+0.5)*(i+0.5) * flux;
		  my += (j+0.5)*(j+0.5) * flux;
		  mxy += (i+0.5)*(j+0.5) * flux;
		  if(flux > biggie) {
		     ibig = i;
		     jbig = j;
		     biggie = flux;
		  }
	       }
	    }
	 }
      }
//      printf("npix,xc,yc,mx,my,mxy,fluxsum = %d %.1f %.1f %.1f %.1f %.1f %.1f\n", 
//	     npix, xc, yc, mx, my, mxy, fluxsum);
/* Is first moment OK for centroid? */
      if(flux1 > rms*npix) {
	 xc = x1 / flux1;
	 yc = y1 / flux1;
      } else {
	 xc = x0;
	 yc = y0;
      }

/* Is second moment OK for RMS?  Calculate variances about the mean. */
      if(flux2 > rms*nbig) {
	 mx = (mx - x2*x2/flux2) * (binx*binx/flux2);
	 my = (my - y2*y2/flux2) * (biny*biny/flux2);
	 mxy = (mxy - x2*y2/flux2) * (binx*biny/flux2);
      } else {
	 mx = my = mxy = 0.0;
      }
/* Radius from previous center to nearest pixel which is close to sky */
      nearlow = sqrt(nearlow);
#ifdef TEST
      printf("npix,nbig,xc,yc,mx,my,mxy,fluxsum,rcut,nearlow,flim2 = %d %d %5.1f %5.1f %6.1f %6.1f %6.2f %7.0f %5.1f %5.1f %d\n", 
	     npix, nbig, xc, yc, mx, my, mxy, fluxsum, rcut, nearlow, flim2);
#endif

      if(mx > 0 && my > 0) {
	 a4 = sqrt((mx+my)/2.0);
//	 mxmy_14 = sqrt(sqrt(mx*my));
//	 a5 = mxy / mxmy_14 ;
//	 a6 = (mx-my) * mxmy_14 / 2.0 ;
      } else {
	 a4 = mxmy_14 = a5 = a6 = 0.0;
      }

      if(pass > 0 && ABS(x0-xc) < CTRTOL && ABS(y0-yc) < CTRTOL && 
	 a4 > rcut/5) break;

      x0 = xc;
      y0 = yc;
/* Squeeze down rcut to max of 3*RMS or nearlow */
      rcut = MAX(3*a4, 6.0);
      rcut = MAX(rcut, nearlow);
/* Lower flim2 to kappa2 */
      flim2 = bright2 - sky;
   }

/* Ellipse variables: major/minor ~ (1+a) */
   if(a4 > 0) a = sqrt(mxy*mxy+(mx-my)*(mx-my)/4) / ((mx+my)/2);
   theta = 0.5 * atan2(mxy, (mx-my)/2);
#ifdef TEST
   printf("mx,my,mx-my/2,mxy,a, theta = %8.2f %8.2f  %8.2f %8.2f %8.2f %8.2f\n", 
	  mx,my,(mx-my)/2,mxy,a, theta/0.0174533);
#endif

/* Return values */
   *xu = sx + xc*binx;
   *yu = sy + yc*biny;
   *fmax = biggie;
   *fwhm = FWRMS * a4;
   *xfwhm = FWRMS * sqrt(MAX(0.0, mx));
   *yfwhm = FWRMS * sqrt(MAX(0.0, my));
   *bkgnd = sky;
   *ftot = fluxsum;
   *snr = a4 > 0 ? fluxsum / sqrt(k*rms*rms) : 0.0;
/* This could in principle have extra info, e.g. higher order moments */
   *weight = *snr;

#ifdef TEST
   printf("xu,yu,FWHM,snr,x,y,max= %.1f %.1f %.2f %.1f %d %d %d\n", 
	  *xu, *yu, *fwhm, *snr, ibig, jbig, biggie);
#endif


   return(0);
}
