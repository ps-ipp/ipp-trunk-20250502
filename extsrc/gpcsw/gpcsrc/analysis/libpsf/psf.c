/* psf.c: find and fit the brightest star in an image */
/* 081009 v1.0 John Tonry */
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <string.h>
#include <math.h>
#include "psf.h"

#define SNMIN 5.0	/* Minimum S/N to proceed with PSF_MERGE */
#define FWMIN 1.0	/* Minimum FWHM to proceed with PSF_MERGE */
#define FWMAX 8.5	/* Maximum FWHM to proceed with PSF_MERGE */
#define FWMRG 5.5	/* FWHM which is halfway through MARGIN/MOMENT merge */
#define FWLAP 1.0	/* Transition width of MARGIN/MOMENT merge */
#define FWCTE 2.0	/* Maximum yfw/xfw ratio to proceed with PSF_MERGE */
#define EDGE  2.0	/* Minimum edge proximity to proceed with PSF_MERGE */

int psf(int nx,			/* x size of image */
	int ny,			/* y size of image */
	unsigned short *data,	/* Data array */
	PSF_ALGORITHM alg,	/* Choice of PSF algorithm */
	PSF_IMPARAM *imparam,	/* Extra image parameters */
	PSF_PARAM *psfout,	/* Results of fit */
	PSF_EXTRA *psfextra)	/* Extra results from fit */
{
   int err=1;
   int sx=0, sy=0, binx=1, biny=1, bordx=4, bordy=2, extra, fmax, NX=nx;
   double xu, yu, fwhm, xfwhm, yfwhm, bkgnd, ftot, weight, snr;
   int fmax2;
   double xu2, yu2, fwhm2, xfwhm2, yfwhm2, bkgnd2, ftot2, weight2, snr2;
   double frac, f;

/* Load up parameters (with a few sanity checks!) */
   if(imparam != NULL) {
      sx = imparam->sx;
      sy = imparam->sy;
      if(imparam->binx > 0) binx = imparam->binx;
      if(imparam->biny > 0) biny = imparam->biny;
      if(imparam->nodata != 0) {
	 fprintf(stderr, "*NODATA* normally is 0!\n");
/* Ignore imparam->nodata != 0 (the default) for now! */
      }
      if(imparam->NX > 0) NX = imparam->NX;
      if(imparam->bordx >= 0) bordx = imparam->bordx;
      if(imparam->bordy >= 0) bordy = imparam->bordy;
   }

   switch(alg) {
      case PSF_MARGIN:
	 err = psfmargin(sx, sy, binx, biny, bordx, bordy,
			 nx, ny, NX, data, &xu, &yu, &fmax, &fwhm,
			 &xfwhm, &yfwhm, &bkgnd, &ftot, &weight, &snr);
	 break;

      case PSF_MOMENT:
	 err = psfmoment(sx, sy, binx, biny, bordx, bordy,
			 nx, ny, NX, data, &xu, &yu, &fmax, &fwhm,
			 &xfwhm, &yfwhm, &bkgnd, &ftot, &weight, &snr, &extra);
	 break;

      case PSF_BIN:
	 err = psfdonut(sx, sy, binx, biny, bordx, bordy,
			 nx, ny, NX, data, &xu, &yu, &fmax, &fwhm,
			 &xfwhm, &yfwhm, &bkgnd, &ftot, &weight, &snr, &extra);
	 break;

      case PSF_2DIM:
	 err = psf2dim(nx, ny, data, imparam, psfout, psfextra);
	 break;

      case PSF_MERGE:
	 err = psfmoment(sx, sy, binx, biny, bordx, bordy,
			 nx, ny, NX, data, &xu, &yu, &fmax, &fwhm,
			 &xfwhm, &yfwhm, &bkgnd, &ftot, &weight, &snr, &extra);
/* Do we proceed with psfmargin? */
	 if(!err && snr > SNMIN && 
	    fwhm > FWMIN && fwhm < FWMAX && xfwhm < yfwhm*FWCTE &&
	    xu > sx+EDGE && xu < sx+nx*binx-EDGE && 
	    yu > sy+EDGE && yu < sy+ny*biny-EDGE) {
	    err = psfmargin(sx, sy, binx, biny, bordx, bordy,
			    nx, ny, NX, data, &xu2, &yu2, &fmax2, &fwhm2,
			    &xfwhm2, &yfwhm2, &bkgnd2, &ftot2, &weight2, &snr2);
/* Merge the results: frac = fraction of MOMENT to use */
	    if(!err) {
/* Control variable f is a linear combination of MAR/MOM */
	       if(fwhm > FWMRG+FWLAP)      f = fwhm;
	       else if(fwhm < FWMRG-FWLAP) f = fwhm2;
	       else f = (fwhm-FWMRG+FWLAP)/(2*FWLAP)*(fwhm-fwhm2)+fwhm2;
	       f = (f-FWMRG)/(2*FWLAP);
	       if(f >= 1.0) 	     frac = 1.0;
	       else if(f <= -1.0)    frac = 0.0;
	       else 		     frac = 0.5*(sin(f*1.5708)+1);
	       xu = (1-frac)*xu2 + frac*xu;
	       yu = (1-frac)*yu2 + frac*yu;
	       fwhm = (1-frac)*fwhm2 + frac*fwhm;
	       xfwhm = (1-frac)*xfwhm2 + frac*xfwhm;
	       yfwhm = (1-frac)*yfwhm2 + frac*yfwhm;
	       ftot = (1-frac)*ftot2 + frac*ftot;
	       snr = (1-frac)*snr2 + frac*snr;
	       weight = (1-frac)*weight2 + frac*weight;
	    }
	 }

	 break;

      default:
	 fprintf(stderr, "Unknown algorithm! %d\n", alg);
	 return(-1);
   }

/* Load up results (with a few sanity checks!) */
   if(psfout != NULL && alg != PSF_2DIM) {
//      psfout->ix = 0;
//      psfout->iy = 0;
// 091211: JT this is a harmless lie, supposed to be highest pixel
      psfout->ix = xu;
      psfout->iy = yu;
//
      psfout->x0 = xu;
      psfout->y0 = yu;
      psfout->peak = fmax;
      psfout->fwhm = fwhm;
      psfout->bkgnd = bkgnd;
      psfout->flux = ftot;
      psfout->sn = snr;
      psfout->weight = err ? 0.0 : snr;
   }

/* Load up results (with a few sanity checks!) */
   if(psfextra != NULL) {
      if(alg == PSF_BIN || alg == PSF_MOMENT || alg == PSF_MARGIN) {
	 psfextra->xfw = xfwhm;
	 psfextra->yfw = yfwhm;
         psfextra->majfw = 0.0;
         psfextra->minfw = 0.0;
         psfextra->thfw = 0.0;
         psfextra->wpeak = 0.0;
         psfextra->wbkgnd = 0.0;
         psfextra->dflux = 0.0;
         psfextra->dbkgnd = 0.0;
         psfextra->rmsbkgnd = 0.0;
      }
      if(alg == PSF_BIN) psfextra->binfactor = extra;
   }
   return(err);
}


static float *jtpsfbuf=NULL;
static int jtpsfnpix=0;

int psf2dim(int nx, int ny, unsigned short *data,
	PSF_IMPARAM *imparam,	/* Extra image parameters */
	PSF_PARAM *psfout,	/* Results of fit */
	PSF_EXTRA *psfextra)	/* Extra results from fit */
{
   int err;
   int ix, iy, big;
   int sx=0, sy=0, binx=1, biny=1;
   double x0, y0, fwhm, xfwhm, yfwhm, sn, sky, ftot;
   int i, j, NX=nx;
   int aprad, skyrad, nwpar;
   float wpar[16], flux[5], eadu=1.0;

   if(imparam != NULL) {
      sx = imparam->sx;
      sy = imparam->sy;
      if(imparam->binx > 0) binx = imparam->binx;
      if(imparam->biny > 0) biny = imparam->biny;
      if(imparam->NX > 0) NX = imparam->NX;
   }

/* First run psfmargin_guts to get defaults for jtpsf */
   err = psfmargin_guts(1, &ix, &iy, &big, &x0, &y0, &fwhm, &xfwhm, &yfwhm,
			&sn, &sky, &ftot, nx, ny, NX, data);
   if(err) return(err);

/* Make sure that we have some space allocated for the image */
   if(jtpsfnpix < nx*ny) {
      if(jtpsfbuf != NULL) free(jtpsfbuf);
      jtpsfbuf = (float *)calloc(nx*ny, sizeof(float));
      jtpsfnpix = nx*ny;
   }

/* Copy the array */
   for(j=0; j<ny; j++) {
      for(i=0; i<nx; i++) jtpsfbuf[i+j*nx] = data[i+j*NX];
   }

/* Parameters to run jtpsf_() */
   nwpar = 7;
   wpar[0] = ix;
   wpar[1] = iy;
   aprad = 5*fwhm;
   skyrad = MIN(0.4*nx, 0.4*ny);
   skyrad = MIN(skyrad, 10*fwhm);
//   skyrad = MIN(skyrad, 60);
//   aprad = 32;
//   skyrad = 60;

//   fprintf(stderr, "nx, ny, aprad, skyrad, nwpar, wpar[0], wpar[1] = %d %d %d %d %d %.2f %.2f\n",
//	  nx, ny, aprad, skyrad, nwpar, wpar[0], wpar[1]);

   jtpsf_(&nx, &ny, jtpsfbuf, &eadu, &aprad, &skyrad, &nwpar, wpar, flux, &err);
/* Load up results (with a few sanity checks!) */
   if(psfout != NULL) {
      psfout->ix = ix;
      psfout->iy = iy;
      psfout->x0 = sx + wpar[0]*binx;
      psfout->y0 = sy + wpar[1]*biny;
      psfout->peak = big;
      psfout->fwhm = sqrt(MAX(0.0, wpar[9]*wpar[10]*binx*biny));
      psfout->bkgnd = flux[0];
      psfout->flux = flux[2];
      psfout->sn = flux[2] / MAX(0.0, flux[3]);
      psfout->weight = err ? 0.0 : psfout->sn;
   }

/* Load up extras */
   if(psfextra != NULL) {
/* Sanity check against no error but crazy fit! */
      psfextra->xfw = xfwhm*binx;
      psfextra->yfw = yfwhm*biny;
/* Note that these major/minor things are messed up if binx!=biny */
      psfextra->majfw = wpar[9]*binx;
      psfextra->minfw = wpar[10]*biny;
      psfextra->thfw = wpar[11];
      psfextra->wpeak = wpar[2];
      psfextra->wbkgnd = wpar[3];
      psfextra->dflux = flux[3];
      psfextra->dbkgnd = flux[1];
      psfextra->rmsbkgnd = flux[4];
   }
   return(err);
}

void f77msg_(char *line, int len)
{
   int i;
   for(i=len-2; i>0 && line[i] == ' '; i--);
   line[i+1] = '\0';
   fprintf(stderr, "f77: %s\n", line);
}
