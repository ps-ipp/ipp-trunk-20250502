/* trailfit.c - routines to fit and subtract trails */

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

STATIC double ybuf[MAXSIZE], zbuf[MAXSIZE], wbuf[MAXSIZE], wbuf0[MAXSIZE],xbuf[MAXSIZE];

/****************************************************************/
/* Subtract one rectangle's fit from the data */
STATIC int sub_fit(int nx, int ny, int NX, IMTYPE *buf, OBJBOX *box, 
		   int sign)
{
   int i, j, k;
   int y0, y1, ys, dy, delta;

   if(ny > MAXSIZE) {
      fprintf(stderr, "error: not enough buffer in fit_trail\n");
      return(-1);
   }

   if(box->func != BURN_PWR && box->func != BURN_EXP) {
      fprintf(stderr, "error: unimplemented fit function %d (err %d)\n", 
	      box->func, box->fiterr);
      return(-1);
   }

   if(box->fiterr) return(-1);
   y0 = box->y0;
   y1 = (box->up) ? ny-1 : 0;
   dy = (y0<y1) ? +1 : -1;
   if(box->up) ys = box->sy;
   else        ys = box->ey;

/* Calculate all needed y values */
   for(j=ys; dy*j<=dy*y1; j+=dy) {
      if(box->func == BURN_PWR) {
	 if(dy*(j-y0) <= 0) ybuf[j] = exp(box->slope*log(Y_SCALE*1.0));
	 else ybuf[j] = exp(box->slope*log(Y_SCALE*dy*(j-y0)));
      } else if(box->func == BURN_EXP) {
//	 ybuf[j] = exp(box->slope*Y_SCALE*MAX(0,dy*(j-y0)));
	 ybuf[j] = exp(box->slope*Y_SCALE*dy*(j-y0));
      }
   }
/* Subtract the fits */
//   printf("%3d %3d %3d %1d %1d %8.5f\n", 
//	  box->cx, box->cy, box->y0, box->func, box->up, box->slope);

   for(k=0; k<box->nfit; k++) {
      i = box->xfit[k];
      if(sign > 0) {		/* Subtract */
	 for(j=box->yfit[k]; dy*j<=dy*y1; j+=dy) {
//	 if(k==box->nfit/2) printf("%3d %6d ", j, buf[i+NX*j]);
	    delta = box->zero[k] * ybuf[j] + 0.5;
/* No 16 bit wraparound below zero allowed */
//	    if(delta < buf[i+NX*j]+BZERO) buf[i+NX*j] -= delta;
/* Allow 16 bit wraparound in order to undo it... */
	    buf[i+NX*j] -= delta;
//	 if(k==box->nfit/2) printf("%6.1f %6d\n", 
//			 sign * box->zero[k] * ybuf[j], buf[i+NX*j]);
	 }
      } else {			/* Restore */
	 for(j=box->yfit[k]; dy*j<=dy*y1; j+=dy) {
/* No 16 bit wraparound allowed */
//	    if(delta < buf[i+NX*j]+BZERO) buf[i+NX*j] += delta;
	    delta = box->zero[k] * ybuf[j] + 0.5;
	    buf[i+NX*j] += delta;
	 }
      }
   }

   return(0);
}

/****************************************************************/
/* Tuning parameters for fit_trail */
#define MAXWGT 2.0	/* Maximum allowed weight */
#define MINWGT 0.1	/* Minimum allowed weight */
#define MAXDEV 1.	/* Maximum ln deviation above first pass fit */
#define MAXRMS 4.0	/* Minimum factor of RMS to be chopped */

/* Fit one rectangle with a power law or exp: up=0/1 for down/up from y0 */
STATIC int fit_trail(int nx, int ny, int NX, 
		    DTYPE *data, MTYPE *mask, OBJBOX *box,
		    int up, int bckgnd, int rms, int fitfunc,char *camera)
{
   int i, j, k, err, nfit,firstfitpix,secondfitpix, xwidth;
   int xs, xe, y0, y1, y2, dy, yfit,x0,x1;
   double slope, zero, zsum, wsum, trial,zfun,xsum,xmean,xstd;
   char fooname[60];
   FILE *fp=stdout;

   if(ny > MAXSIZE) {
      fprintf(stderr, "error: not enough buffer in fit_trail\n");
      return(-1);
   }
   
   /*xs = box->sxfit; */
   /*xe = box->exfit; */
   /*TdB20220222: Do not use s/exfit here, which will fail in the case of positive slopes*/
   xs = box->sx;
   xe = box->ex;

   
   if(up) {
      //y0 = (box->y0m + box->y1m + box->y0p + box->y1p + 2) / 4;
      y0 = (box->sy + box->ey + 1) / 2;
      y1 = box->ey + FIT_EDGE;   /* Start of fit, budged away from the burn */
      y2 = ny - 1;      /* End of fit */
      dy = +1;
      yfit = box->sy;      /* Start of validity, TBD later by col */
      firstfitpix = MAX((box->ey - box->sy)*2,20) ;  /* Number of pixels to include in the initial fit*/
      secondfitpix = 200;  /* Number of pixels to include in the second fit*/
   } else {
      //y0 = (box->y0m + box->y1m + box->y0p + box->y1p + 2) / 4;
      y0 = (box->sy + box->ey + 1) / 2;
      y1 = 0;        /* Start of fit */
      y2 = y0 - FIT_EDGE;  /* End of fit, budged away from the burn */
      dy = -1;
      yfit = box->ey;      /* Start of validity, TBD later by col */
      firstfitpix = 100;  /* Number of pixels to include in the initial fit*/
      secondfitpix = 200;  /* Number of pixels to include in the second fit*/
   }

   if(!strcmp(camera,"gpc2")) {
     /*Use the inner third of the star width for this test. Should be brightest there*/
     xwidth = MAX((box->ex-box->sx)/3,10);	
     x0 = box->sx + (box->ex-box->sx)/2 - xwidth/2;
     x1 = box->sx + (box->ex-box->sx)/2 + xwidth/2;
     /*when fitting a burn, do not want to include part of the star*/
     /*add 20% of box size on top, with a maximum of ten pixels*/
     if(up) y1 = box->ey + MIN((box->ey-box->sy)/5,10);
   } else {
     //regular gpc1 setup
     x0 = box->sx;
     x1 = box->ex;
   }

   if(VERBOSE & VERB_FIT) {
      printf("Fit requested xs,xe,y0,y1,y2,dy,up,f,x0,x1= %d %d %d %d %d %d %d %d %d %d\n", 
	     xs, xe, y0, y1, y2, dy, up, fitfunc,x0,x1);
   }

/* Make sure to alloc memory, even if the fit fails */
   if(box->zero != NULL) free(box->zero);
   if(box->xfit != NULL) free(box->xfit);
   if(box->yfit != NULL) free(box->yfit);
   box->zero = (double *)calloc(xe-xs+1, sizeof(double));
   box->xfit = (int *)calloc(xe-xs+1, sizeof(int));
   box->yfit = (int *)calloc(xe-xs+1, sizeof(int));
   if(box->zero == NULL || box->xfit == NULL || box->yfit == NULL) {
      fprintf(stderr, "\rerror: failed to alloc box memory\n");
      exit(-679);
   }
/* Some defaults */
   box->slope = 0.0;
   box->func = fitfunc;
   box->up = up;
   box->y0 = y0;
   box->eyfit = up?y2:y1;
   for(i=0; i<xe-xs+1; i++) {
      box->zero[i] = 0.0;
      box->xfit[i] = i+xs;
      box->yfit[i] = y0;
   }

/* Burn extends all the way to the top */
   if(up && y1 >= ny-1) {
      box->slope = -1.0;
      box->fiterr = FIT_TOP_ERROR;
      /* No fit here because the burn is to top, but the persist may be fitable */
      if(box->sy > FIT_EDGE) {
	    box->nfit = xe - xs + 1;
       /* However, if burn extends to bottom, persist will not be able to fit */
      } else {
	    box->nfit = 1;		/* Just enough to keep it alive */
	    box->func = BURN_BLASTED;
      }
      return(0);
   }

   if(VERBOSE & VERB_FITPROF) {
      sprintf(fooname, "/tmp/foobt.%d", xs);
      fp = fopen(fooname, "w");
   }

/* Accumulate points to fit */
/* use wbuf0 to select only the first FIRSTFITPIX pixels for first fit */
   nfit = y2 - y1 + 1;
   for(j=y1; j<=y2; j++) {
      ybuf[j] = Y_SCALE * dy * (j-y0);
      zbuf[j]=0.0;
      for(i=x0, k=0; i<=x1; i++) {
	     if(data[i+j*NX] != NODATA && (mask[i+j*NX] == MASK_NONE || mask[i+j*NX] == MASK_SAT_HALO) ) {
	      zbuf[j] += data[i+j*NX];
	      k++;
	     }
        if(mask[i+j*NX] != MASK_NONE) {
        }
      }
      zbuf[j] = zbuf[j] / MAX(1,k) - bckgnd;

/*    Linearize for fit... */
      if(fitfunc == BURN_PWR) ybuf[j] = log(ybuf[j]);
      wbuf[j] = 0;
      wbuf0[j] = 0;
      if(zbuf[j] > 1) {
	     wbuf[j] = MIN(MAXWGT, (zbuf[j] / rms));  /* Quadratic not good */
	     wbuf[j] = MAX(MINWGT, wbuf[j]);
	     zbuf[j] = log(zbuf[j]);
        wbuf0[j] = wbuf[j];
        if(j < (y2-firstfitpix) && !up) wbuf0[j] = 0;
        if(j > (y1+firstfitpix) && up) wbuf0[j] = 0;
        //printf("fit: %4d %.3f %.3f %.3f %.3f\n", j, ybuf[j], zbuf[j], wbuf[j], wbuf0[j]);
      }
   }
/* First pass fit */
   err = wlinearfit(nfit, ybuf+y1, zbuf+y1, wbuf0+y1, &slope, &zero);
   if(VERBOSE & VERB_FIT) {
      printf("nfit= %d  err= %d  slope= %.3f  zero= %.3f\n", 
	     nfit, err, slope, zero);
   }
   if(err) {
      box->fiterr = FIT_ERROR;
      return(-1);
   }

/* Trim away any really big positive deviations */
   for(j=y1; j<=y2; j++) {
      if(abs(zbuf[j]-(ybuf[j]*slope+zero)) > MAXDEV) wbuf[j] = 0;
      if(j < (y2-secondfitpix) && !up) wbuf[j] = 0;
      if(j > (y1+secondfitpix) && up) wbuf[j] = 0;
      if(VERBOSE & VERB_FIT) {
	 printf("trim j : %d %9.4f %9.4f %9.4f %d %9.4f %9.4f\n", j,ybuf[j],zbuf[j],(ybuf[j]*slope+zero),abs(zbuf[j]-(ybuf[j]*slope+zero)), wbuf[j], wbuf0[j]);
      }
   }
/* Second pass fit */
   err = wlinearfit(nfit, ybuf+y1, zbuf+y1, wbuf+y1, &slope, &zero);
   if(err) {
      box->fiterr = FIT_ERROR;
      return(-1);
   }
   box->fiterr = 0;		/* Whew, got a fit */

   if(VERBOSE & VERB_FITPROF) {
      printf("2nd fit nfit, zero, slope: %d  %.3f %.3f\n", nfit, zero, slope);
      fclose(fp);
   }

/* FIXME: sanity check fits */
   if(slope >= FIT_MAX_SLOPE || slope < FIT_MIN_SLOPE) {
      box->slope = slope;
      box->nfit = 0;
      box->fiterr = FIT_SLOPE_ERROR;
/* Check whether it's a significant trail (but with pos slope) or just noise */
      linearrms(nfit, ybuf+y1, zbuf+y1, slope, zero, &trial);
/* 100203 JT: bad idea: appears to be a bug in read/writing ABS(nfit) */

//      if(trial > 2*rms) box->nfit = -box->nfit;
      if(trial > 2*rms) box->func = BURN_POSSLOPE;
      box->sxfit = xs;
      box->exfit = xs - 1;
      return(-1);
   }

/* find where the fitted function becomes less than half the rms and cut the trail there*/
   for(j=(up?y1:y2); dy*j<dy*(up?y2:y1); j+=dy) {
   //for(j=y1; j<=y2; j++) {
      zfun = exp((ybuf[j]*slope)+zero);
      //printf("y zfun thresh : %d %9.4f %9.4f %9.4f\n",j,zfun,NEGLIGIBLE_TRAIL*rms,NEGLIGIBLE_TRAIL*rms*0.5);
      if(up) {
        if(zfun < NEGLIGIBLE_TRAIL*rms*0.5) break;
      } else {
        if(zfun < NEGLIGIBLE_TRAIL*rms) break;      
      }
   }   
   //For burns, just let it go to the top
   //if(j < y2 && !up) box->eyfit = j;
   box->eyfit = j;


   if(!strcmp(camera,"gpc2")) {
     /*Conpute the mean and standard deviation of the trail along x*/
     for(i=xs; i<=xe; i++) {
      xbuf[i] = 0.0;
      for(j=y1, k=0; j<=y2; j++) {
        if(wbuf[j] > 0 && data[i+j*NX] != NODATA && 
        (mask[i+j*NX] == MASK_NONE || mask[i+j*NX] == MASK_SAT_HALO)) {
          xbuf[i] += data[i+j*NX];
          k++;
        }
      }
      xbuf[i] = xbuf[i] / MAX(1,k);
     }
    
     xsum = wsum = 0.;
     for(i=xs; i<=xe; i++) {
      xsum += i*xbuf[i];
      wsum += xbuf[i];
     }
     xmean = xsum / wsum;

     xsum = wsum = 0.;
     for(i=xs; i<=xe; i++) {
      xsum += xbuf[i]*pow(i-xmean,2);
      wsum += xbuf[i];
     }
     xstd = sqrt(xsum / (wsum-1.));
     if(VERBOSE & VERB_FIT) {
       printf("x mean xstd : %9.4f %9.4f %9.4f %9.4f\n",  xmean, xstd,xsum,wsum);  
     }

     /* Update fit ranges */
     if(xstd > 1 && xstd < 50) {
      box->sxfit = round(xmean-(xstd*1.1));
      box->exfit = round(xmean+(xstd*1.1));
     } else {
      box->sxfit = xs;
      box->exfit = xs - 1;
      box->fiterr = FIT_ALL_GONE;
     }
   } else {
     /* Reconstruct the fit for comparison with the data col by col */
     for(j=y1; j<=y2; j++) zbuf[j] = exp((ybuf[j]*slope));
     /* And tag on the extras to determine the end of the fit */
     for(j=yfit; dy*j<dy*(up?y1:y2); j+=dy) {
      if(fitfunc == BURN_EXP) {
  	     zbuf[j] = exp((Y_SCALE*dy*(j-y0)*slope));
      } else {
	     if(dy*(j-y0) <= 0) zbuf[j] = exp((log(Y_SCALE*1.0)*slope));
	     else zbuf[j] = exp((log(Y_SCALE*dy*(j-y0))*slope));
      }
     }

     /* Determine good x-ranges and Save the fit information */
     box->slope = slope;
     box->nfit = 0;
     /* Save individual scaled versions for each column */
     for(i=xs; i<=xe; i++) {
       zsum = wsum = 0.0;
       for(j=y1; j<=y2; j++) {
	     if(wbuf[j] > 0 && data[i+j*NX] != NODATA && 
	     (mask[i+j*NX] == MASK_NONE || mask[i+j*NX] == MASK_SAT_HALO)) {
	       wsum += zbuf[j];
	       zsum += data[i+j*NX] - bckgnd;
	     }
       }
       if(zsum < 0 || wsum <= 0) {
	     zsum = 0.0;
       } else {
	     zsum = zsum / wsum;
       }
       /* FIXME: what's a really good criterion for negligible fit? */
       /* 100 pixels up fit is zsum or ~zsum/e */
       /* 100113: but also evaluate at the end of the fit for stubby trails */
       //printf("scaled fit comparison x zsum zbuf[y2]*zsum thresh: %d %9.4f %9.4f %9.4f %f %d %f\n", i, zsum, zbuf[y2], zbuf[y2]*zsum, NEGLIGIBLE_TRAIL, rms, NEGLIGIBLE_TRAIL*rms);  

       if(zsum > NEGLIGIBLE_TRAIL*rms ||
	   zbuf[y2]*zsum > NEGLIGIBLE_TRAIL*rms) {
        /* Ascertain the starting point of where the fit is good */
//      box->yfit[box->nfit] = yfit;	/* Vanilla, misses center */
//      box->yfit[box->nfit] = y0;	/* Center, no adjust */
	     for(j=yfit; dy*j<=dy*(up?y2:y1); j+=dy) {
	       trial = zsum * zbuf[j];
/*        First time data - fit is closer to sky than is data */
	       if(data[i+j*NX] > bckgnd + 0.5*trial) break;
	     }
	     box->yfit[box->nfit] = j;
	     box->zero[box->nfit] = zsum;
	     box->xfit[box->nfit] = i;
             //printf("y comparison : %d %d %9.4f %9.4f %d %d\n", i, box->yfit[box->nfit], box->zero[box->nfit],zsum, box->xfit[box->nfit],box->nfit);  
	     box->nfit += 1;
       }
     }
     /* Update fit ranges */
     if(box->nfit > 1) {
        box->sxfit = box->xfit[0];
        box->exfit = box->xfit[box->nfit-1];
     } else {
        box->sxfit = xs;
        box->exfit = xs - 1;
        box->fiterr = FIT_ALL_GONE;
     }
   }



   if(VERBOSE & VERB_FIT) {
      printf("Final fit params xsfit,xefit,y1,y2, yfit,slope,nfit = %d %d %d %d %d %.3f %d\n", 
	     box->sxfit, box->exfit, y1, y2,box->eyfit, box->slope, box->nfit);
   }

//   if(box->nfit <= 0) fprintf(stderr, "WHOA, got nfit = 0\n");
   return(0);
}
