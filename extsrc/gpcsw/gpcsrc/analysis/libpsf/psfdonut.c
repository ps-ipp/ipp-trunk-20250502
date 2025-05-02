/* psfdonut.c: find and fit the brightest star in an image; donuts OK */

/* Note: this is not as fast as psf() nor as accurate as jtpsf() for
 * flux, width, and position, but it is tolerant of donuts and should
 * provide a pretty good width and center.
 */
/* 080815 v1.0 John Tonry */

#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <math.h>
#include "psf.h"

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

static unsigned short int *binim=NULL;	/* Storaged for binned images */
static int binsize=0;			/* How many pixels? */
#define LOGMAXBIN 4	/* Bin down to 2^LOGMAXBIN, 32=2^5 is a bit much... */
// #define XBORD 2		/* Disregard this border on the x sides */
// #define YBORD 1		/* Disregard this border on the y sides */

// #define DONUT_TEST		/* Test output for donut routine? */

/* Find and report on the brightest star; will work with donuts... */
int psfdonut(int ZFsx, 			/* x offset of pixel 0,0 */
	     int ZFsy,  		/* y offset of pixel 0,0 */
	     int ZFbinx,  		/* x anamorphic compression factor */
	     int ZFbiny,   		/* y anamorphic compression factor */
/* Note: "anamorphic compression" means that the data received have been
 * binned down by that factor.  We *do* correct the positions xu,yu
 * for this factor, and *do* correct the widths for it.  The reasoning, such
 * as it is, returns unbinned positions, correct for offsets, and
 * the widths need to be corrected here because roundness is such an important
 * factor in deciding whether we've got a good fit or not.  Ugh.
 */
	     int bordx,  		/* x border */
	     int bordy,   		/* y border */
	     int ZFnx,   		/* x size of image */
	     int ZFny,    		/* y size of image */
	     int ZFNX,   		/* x stride of image */
	     unsigned short *ZFdata,	/* ushort image data: 0 = *NO_DATA* */
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
	     int *binfactor)		/* Best bin factor used */
{
   int i, j, k;
   int xmax, ymax, big, bbig=0;
   double x0, y0, pfwhm, xpfwhm, ypfwhm, bsize;
   double flux[LOGMAXBIN+1], sky[LOGMAXBIN+1];
   double xp[LOGMAXBIN+1], yp[LOGMAXBIN+1], sn[LOGMAXBIN+1];
   double xfwest[LOGMAXBIN+1], yfwest[LOGMAXBIN+1], fwest[LOGMAXBIN+1];
   double wgt[LOGMAXBIN+1], bestwgt;
   int bin, bestbin=0, err[LOGMAXBIN+1];
   unsigned short int *imptr[LOGMAXBIN+1];
   int sx[LOGMAXBIN+1], sy[LOGMAXBIN+1];
   int nx[LOGMAXBIN+1], ny[LOGMAXBIN+1], NX[LOGMAXBIN+1];
#ifdef DONUT_TEST
   int fd;
   char fname[2880];
#endif

#ifdef DONUT_TEST
   fprintf(stderr, "nx=%d ny=%d sx=%d sy=%d binx=%d biny=%d data=%d\n", 
	  ZFnx, ZFny, ZFsx, ZFsy, ZFbinx, ZFbiny, ZFdata[0]);
#endif

/* Allocate some space for the binned down images */
   if(binsize < (ZFnx*ZFny+2)/3) {
      if(binim == NULL) free(binim);
      binim = (unsigned short int *)calloc((ZFnx*ZFny+2)/3, sizeof(short int));
      binsize = (ZFnx*ZFny+2) / 3;
   }

/* Bin down the image and update the arrays */
   imptr[0] = (unsigned short int *)((ushort *)ZFdata+bordx+bordy*ZFNX);
   sx[0] = ZFsx + bordx;
   sy[0] = ZFsy + bordy;
   nx[0] = ZFnx - 2*bordx;
   ny[0] = ZFny - 2*bordy;
   NX[0] = ZFNX;
   for(bin=1; bin<=LOGMAXBIN; bin++) {
      if(bin == 1) imptr[bin] = binim;
      else	   imptr[bin] = imptr[bin-1] + NX[bin-1]*ny[bin-1];
      sx[bin] = sx[bin-1];
      sy[bin] = sy[bin-1];
      nx[bin] = nx[bin-1]/2;
      ny[bin] = ny[bin-1]/2;
      NX[bin] = nx[bin];
      for(j=0; j<ny[bin]; j++) {
	 for(i=0; i<nx[bin]; i++) {
	    k  = imptr[bin-1][2*i+2*j*NX[bin-1]];
	    k += imptr[bin-1][2*i+1+2*j*NX[bin-1]];
	    k += imptr[bin-1][2*i+(2*j+1)*NX[bin-1]];
	    k += imptr[bin-1][2*i+1+(2*j+1)*NX[bin-1]];
	    imptr[bin][i+j*NX[bin]] = (k+2)/4;
	 }
      }
   }

/* Analyze each one */
   bestwgt = 0.0;
   for(bin=0; bin<=LOGMAXBIN; bin++) {
      bsize = pow(2.0, (double)bin);

#ifdef DONUT_TEST
      sprintf(fname, "/tmp/bin%d.fits", bin);
      fd = creat(fname, 0664);
      sprintf(fname, "SIMPLE  =                    T                                                  BITPIX  =                   16                                                  NAXIS   =                    2                                                  NAXIS1  =                 %4d                                                  NAXIS2  =                 %4d                                                  END                                                                             ", nx[bin], ny[bin]);
      for(i=strlen(fname); i<2880; i++) fname[i] = ' ';
      write(fd, fname, 2880);
      swab(imptr[bin], imptr[bin], 2*NX[bin]*ny[bin]);
      for(j=0; j<ny[bin]; j++) write(fd, imptr[bin]+j*NX[bin], 2*nx[bin]);
      swab(imptr[bin], imptr[bin], 2*NX[bin]*ny[bin]);
      close(fd);
#endif
      err[bin] = psfmargin_guts(1, &xmax,&ymax, &big, &x0,&y0,    
	  &pfwhm, &xpfwhm, &ypfwhm, &sky[bin], &flux[bin], &sn[bin], 
	  nx[bin], ny[bin], NX[bin], imptr[bin]);
      xp[bin] = x0 * bsize * ZFbinx + sx[bin];
      yp[bin] = y0 * bsize * ZFbiny + sy[bin];
      xfwest[bin] = sqrt(MAX(0.01,xpfwhm*xpfwhm - 0.46)) * ZFbinx * bsize;
      yfwest[bin] = sqrt(MAX(0.01,ypfwhm*ypfwhm - 0.46)) * ZFbiny * bsize;
      fwest[bin] = sqrt(xfwest[bin]*yfwest[bin]);
      flux[bin] *= bsize*bsize;
/* Calculate weights for final guesses */
/* Error is bad, virtually fatal */
      wgt[bin] = err[bin] == 0 ? 1.0 : 0.01;
/* Non-round PSF is very bad */
      wgt[bin] *= pow(xfwest[bin]/yfwest[bin], 
		     xfwest[bin] < yfwest[bin] ? +2.0 : -2.0);
/* Ridiculously small or large PSF is bad, ~2.4 is best */
      wgt[bin] *= exp(-0.5*pow((log(MAX(0.1,xpfwhm))-log(2.4))/log(2.0), 2.0));
      wgt[bin] *= exp(-0.5*pow((log(MAX(0.1,ypfwhm))-log(2.4))/log(2.0), 2.0));
/* High S/N is good */
      wgt[bin] *= MAX(0.0, sn[bin]);
/* High flux is good (crudely normalize assuming ~1e/ADU) */
      wgt[bin] *= MAX(0.0, flux[bin]/1e5);

      if(wgt[bin] > bestwgt) {
	 bestwgt = wgt[bin];
	 bestbin = bin;
	 bbig = big;
      }

#ifdef DONUT_TEST
      printf("B%d %2d: %5.1f %5.1f %6.1f %5.1f %4.1f %4.1f %4.1f %4.1f %5d %5.0f %7.0f %6.1f %6.1f\n", 
	     bin, err[bin], x0, y0, xp[bin], yp[bin], pfwhm, xpfwhm, ypfwhm, fwest[bin],
	     big, sky[bin], flux[bin], sn[bin], wgt[bin]);
      drawcirc(xp[bin]/ZFbinx, yp[bin]/ZFbiny, 0.5*fwest[bin], sn[bin]>5?0:1);
#endif
   }

#ifdef DONUT_TEST
   printf("\n");
   drawcirc((xp[bestbin]-sx[bestbin])/ZFbinx+sx[bestbin], 
	    (yp[bestbin]-sy[bestbin])/ZFbiny+sy[bestbin], 
	    0.5*fwest[bestbin]-1, sn[bestbin]>5?0:1);
#endif

/* Return values */
   *xu = xp[bestbin];
   *yu = yp[bestbin];
   *fmax = bbig;
   *fwhm = fwest[bestbin];
   *xfwhm = xfwest[bestbin];
   *yfwhm = yfwest[bestbin];
   *bkgnd = sky[bestbin];
   *ftot = flux[bestbin];
   *weight = wgt[bestbin];
   *snr = sn[bestbin];
   *binfactor = bestbin;

   return(0);
}
