/* psfstamp.c - identify and write PSF stars */

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
#include "pscoords/pscoords.h"

#define PIXEL_FWHM 0.68	/* FWHM of one pixel: sqrt(ln(256)/12) */

/****************************************************************/
/* psf_select(): Choose worthy stars and copy a postage stamp */
STATIC int psf_select(int nx, int ny, int NX, MTYPE *mask, DTYPE *data,
		      int nbox, OBJBOX *box, int size, int sky)
{
   int i, j, k, sum;
   int x0, x1, y0, y1, xm, xp, ym, yp;

   for(k=0; k<nbox; k++) {
      box[k].func = FUNC_NONE;

/* Too small? */
      if(VERBOSE & VERB_PSFSEL) {
	 printf("Testing star at %d %d for max, %d\n", 
		box[k].cx, box[k].cy, box[k].max);
      }
      if(box[k].max < PSF_THRESH) continue;

/* If the max is very decentered in the box, choose the box */
      if(ABS((box[k].sx+box[k].ex+1)/2 - box[k].cx) < PSF_CTR_TOL &&
	 ABS((box[k].sy+box[k].ey+1)/2 - box[k].cy) < PSF_CTR_TOL) {
	 x0 = box[k].cx - (size-1)/2;
	 x1 = box[k].cx + size/2;
	 y0 = box[k].cy - (size-1)/2;
	 y1 = box[k].cy + size/2;
	 box[k].up = box[k].cx;		/* Coopt for box center */
	 box[k].y0 = box[k].cy;		/* Coopt for box center */
      } else {
	 x0 = (box[k].sx+box[k].ex+1)/2 - (size-1)/2;
	 x1 = (box[k].sx+box[k].ex+1)/2 + size/2;
	 y0 = (box[k].sy+box[k].ey+1)/2 - (size-1)/2;
	 y1 = (box[k].sy+box[k].ey+1)/2 + size/2;
	 box[k].up = (box[k].sx+box[k].ex+1)/2;		/* Coopt */
	 box[k].y0 = (box[k].sy+box[k].ey+1)/2;		/* Coopt */
      }

/* Too close to the edge? */
      if(VERBOSE & VERB_PSFSEL) {
	 printf("testing for edge %d %d %d %d\n", x0, y0, x1, y1);
      }
      if(x0 < 0 || x1 > nx-1 || y0 < 0 || y1 > ny-1) continue;

/* Box too big (blobby, extended crap)? */
      if(VERBOSE & VERB_PSFSEL) {
	 printf("testing for too big %d %d > %d ?\n", 
	      box[k].ex-box[k].sx+1, box[k].ey-box[k].sy+1, size);
      }
      if(box[k].ex-box[k].sx+1 > size || 
	 box[k].ey-box[k].sy+1 > size) continue;
/* Box too small (CR?)? */
      if(VERBOSE & VERB_PSFSEL) {
	 printf("testing for too small %d %d < %d ?\n", 
	      box[k].ex-box[k].sx+1, box[k].ey-box[k].sy+1, MIN_PSF_SIZE);
      }
      if(box[k].ex-box[k].sx+1 < MIN_PSF_SIZE || 
	 box[k].ey-box[k].sy+1 < MIN_PSF_SIZE) continue;
/* Overlap with mask?  Run around edge, outside of star box... */
      ym = MIN(y0, box[k].sy-1);
      yp = MAX(y1, box[k].ey+1);
      if(ym < 0 || yp >= ny-1) {
	 if(VERBOSE & VERB_PSFSEL) 
	    printf("No edge below or above y %d %d\n", ym, yp);
	 continue;
      }
      for(i=x0; i<=x1; i++) {
	 if(mask[i+ym*NX] > MASK_STAR_HALO) break;
	 if(mask[i+yp*NX] > MASK_STAR_HALO) break;
      }
      if(VERBOSE & VERB_PSFSEL) {
	 printf("testing for too much mask overlap x %d < %d ?\n", i, x1);
      }
      if(i <= x1) continue;
      xm = MIN(x0, box[k].sx-1);
      xp = MAX(x1, box[k].ex+1);
      if(xm < 0 || xp >= nx-1) {
	 if(VERBOSE & VERB_PSFSEL) 
	    printf("No edge left or right x %d %d\n", xm, xp);
	 continue;
      }
      for(j=y0+1; j<y1; j++) {
	 if(mask[xm+j*NX] > MASK_STAR_HALO) break;
	 if(mask[xp+j*NX] > MASK_STAR_HALO) break;
      }
      if(VERBOSE & VERB_PSFSEL) {
	 printf("testing for too much mask overlap y, %d < %d ?\n", j, y1);
      }
      if(j < y1) continue;

/* Good star */
      if(VERBOSE & VERB_PSFSEL) {
	 printf("good star at %d %d...\n", box[k].up, box[k].y0);
      }
      box[k].func = PSF_STAR;

/* Make a postage stamp for it */
      if( (box[k].stamp = (IMTYPE *)calloc(size*size, sizeof(IMTYPE))) == NULL) {
	 fprintf(stderr, "\rerror: failed to stamp buffer\n");
	 exit(-675);
      }

      sum = 0;
      for(j=y0; j<=y1; j++) {
	 for(i=x0; i<=x1; i++) {
	    box[k].stamp[(i-x0)+(j-y0)*size] = data[i+j*NX] - sky +
	       USHORT_BIAS - BZERO;
	    sum += data[i+j*NX] - sky;
	 }
      }
      box[k].diff = sum;
      box[k].slope = MAX(0.1, sum) / box[k].max;
   }

   return(0);
}


/****************************************************************/
/* psf_write() writes all the postage stamps to a 3D FITS file */
STATIC int psf_write(int nx, int ny, CELL ota[], int otanum, 
		     const char *psffile)
{
   int i, k, l, nstar, fdout, otacx, otacy, xid, yid, ntot=0, sumax;
   int cellcount, ota_xid, ota_yid;
   double scale, phi, fwhm[3], q[7], qt, xfp, yfp, pi=4*atan(1.0);
   IMTYPE *median_image;
   CELL *cell;
   OBJBOX *box;

/* How many do we have to write? */
   nstar = 0;
   for(k=0; k<MAXCELL; k++) {
      cell = ota + k;
      for(l=cellcount=0; l<cell->nstar; l++) {
	 box = cell->star+l;
	 if(box->func != PSF_STAR) continue;
         if(cellcount++ > MAX_PSF_PER_CELL) break;
	 if(nstar < nmedian_buf) median_buf[nstar] = box->slope;
	 nstar++;
      }
   }
   if(nstar == 0) {
      printf("N= %d\n", nstar);
      return(0);
   }

/* Use the wicked crude cheapfits.c */
   if((fdout=creat(psffile, 0644)) < 0) {
      fprintf(stderr,"error: cannot open output PSF file %s\n", psffile);
      return(-1);
   }

   if(!CONCAT_FITS) write_3dhdr(nx, ny, nstar+1, 0, 0, fdout);

/* Median sum/max */
   sumax = int_median(MIN(nstar, nmedian_buf), median_buf);
//   printf("sumax = %d\n", sumax);

/* Create a median image and write as the first one */
   if( (median_image = (IMTYPE *)calloc(nx*ny, sizeof(IMTYPE))) == NULL) {
      fprintf(stderr, "\rerror: failed to alloc median PSF image\n");
      exit(-676);
   }
   for(i=0; i<nx*ny; i++) {
      nstar = 0;
      for(k=0; k<MAXCELL; k++) {
	 cell = ota + k;
	 for(l=cellcount=0; l<cell->nstar; l++) {
	    box = cell->star+l;
	    if(box->func != PSF_STAR) continue;
	    if(cellcount++ > MAX_PSF_PER_CELL) break;
	    scale = 1e4 * sumax / box->diff;
	    median_buf[nstar++] = (box->stamp[i]-USHORT_BIAS+BZERO) * scale;
	 }
      }
      median_image[i] = int_median(nstar, median_buf) + 
	 USHORT_BIAS - BZERO;
   }

   i = psf_stats(nx, ny, median_image, USHORT_BIAS-BZERO, fwhm, q);

/* Subtract out one pixel in quadrature from median */
   if(fwhm[0] > PIXEL_FWHM && fwhm[1] > PIXEL_FWHM) {
      fwhm[0] = sqrt(fwhm[0]*fwhm[0] - PIXEL_FWHM*PIXEL_FWHM);
      fwhm[1] = sqrt(fwhm[1]*fwhm[1] - PIXEL_FWHM*PIXEL_FWHM);
   }

   ota_xid = otanum % 8;
   ota_yid = otanum / 8;
   psc_pixel_to_fp(ota_xid, ota_yid, 2423.0, 2434.0, &xfp, &yfp);
   phi = atan2(yfp, xfp);
/* Change so that + means tangential, - means radial */
   qt = -q[1] * cos(2*phi) - q[2] * sin(2*phi);

   printf("N= %d PSFmaj= %.2f min= %.2f theta= %.1f m2= %.2f q+= %.3f qx= %.3f qt= %.3f q3c= %.3f q3s= %.3f\n",
	  nstar, fwhm[0], fwhm[1], fwhm[2]*180/pi, q[0], q[1], q[2], qt,
	  q[3], q[4]);

   if(CONCAT_FITS) {
      write_2dfits(nx, ny, 0, 0, median_image, fdout);
   } else {
      write_2ddata(nx, ny, &ntot, median_image, fdout);
   }

/* Write each stamp as concatenated or 3D FITS file */
   nstar = 0;
   for(k=0; k<MAXCELL; k++) {
      cell = ota + k;
      xid = k % 8;
      yid = k / 8;
      for(l=cellcount=0; l<cell->nstar; l++) {
	 box = cell->star+l;
	 if(box->func != PSF_STAR) continue;
	 if(cellcount++ > MAX_PSF_PER_CELL) break;
	 nstar++;
/* Note that x flip required the bigger side offset when nx is even */
	 otacx = (PSC_HCELL - box->up) + xid*(PSC_HCELL+PSC_VSTREET);
	 otacy = box->y0 + yid*(PSC_VCELL+PSC_HSTREET);
	 if(CONCAT_FITS) {
	    write_2dfits(nx, ny, otacx, otacy, box->stamp, fdout);
	 } else {
/* Nahhh, nobody likes this but me... */
//	    box->stamp[0] = otacx-BZERO;	/* Origin is first 2 pixels */
//	    box->stamp[1] = otacy-BZERO;
	    write_2ddata(nx, ny, &ntot, box->stamp, fdout);
	 }

      }
   }
   if(!CONCAT_FITS) write_3dend(&ntot, fdout);

   close(fdout);
   free(median_image);
   return(0);
}

#define MAXPSFMEDIAN (20*64)		/* Max PSF's per OTA */
#define MIN_BELIEVABLE_FWHM 2.0		/* Minimum FWHM we'll accept */

/****************************************************************/
/* psf_write_stats() writes average PSF stats for all the cells */
STATIC int psf_write_stats(int nx, int ny, CELL ota[], int otanum, 
			   const char *statfile, int psfavg)
{
   int i0, j0, i, j;
   int k, l, cellx, celly, ota_xid, ota_yid, nfwave, nqavg;
   double xota, yota, xfp, yfp, phi;
   double fwhm[3], q[7], fw[MAXPSFMEDIAN];
   double m2[MAXPSFMEDIAN], qp[MAXPSFMEDIAN], qc[MAXPSFMEDIAN];
   double qt[MAXPSFMEDIAN], fwavg[MAXPSFMEDIAN];
   double qpavg[MAXPSFMEDIAN], qcavg[MAXPSFMEDIAN], qtavg[MAXPSFMEDIAN];
   double q3c[MAXPSFMEDIAN], q3s[MAXPSFMEDIAN];
   double q3cavg[MAXPSFMEDIAN], q3savg[MAXPSFMEDIAN];
   double q1c[MAXPSFMEDIAN], q1s[MAXPSFMEDIAN];
   double q1cavg[MAXPSFMEDIAN], q1savg[MAXPSFMEDIAN];
   int nstar[MAXCELL], nfw[MAXCELL];
   double fwmed[MAXCELL], m2med[MAXCELL];
   double qpmed[MAXCELL], qcmed[MAXCELL], qtmed[MAXCELL];
   double q3cmed[MAXCELL], q3smed[MAXCELL];
   double q1cmed[MAXCELL], q1smed[MAXCELL];
   double qpmacro, qcmacro, qtmacro, fwmacro, q3cmacro, q3smacro, q1cmacro, q1smacro;
   FILE *fp;
   CELL *cell;
   OBJBOX *box;

/* Convert psfavg from log2(N) */
   if(psfavg >= 3) psfavg = 8;
   else if(psfavg == 2) psfavg = 4;
   else if(psfavg == 1) psfavg = 2;
   else psfavg = 1;

   ota_xid = otanum % PSC_NX;
   ota_yid = otanum / PSC_NX;

/* Open the output file */
   if( (fp=fopen(statfile, "w")) == NULL) {
      fprintf(stderr, "error: psf_write_stats cannot open '%s' for writing\n",
	      statfile);
      return(-1);
   }

/* Loop over all the macro-cells */
   for(j0=0; j0<PSC_NY; j0+=psfavg) {
      for(i0=0; i0<PSC_NX; i0+=psfavg) {
	 nfwave = nqavg = 0;
/* Loop over the constituent cells */
	 for(j=0; j<psfavg; j++) {
	    for(i=0; i<psfavg; i++) {
	       cellx = i0 + i;
	       celly = j0 + j;
	       k = PSC_NX*celly + cellx;
	       cell = ota + k;
	       nstar[k] = nfw[k] = 0;
/* Generate average statistics for all the PSF stars */
	       for(l=0; l<cell->nstar; l++) {
		  box = cell->star+l;
		  if(box->func != PSF_STAR) continue;
		  psf_stats(nx, ny, box->stamp, USHORT_BIAS-BZERO, fwhm, q);
		  if(nstar[k] < MAXPSFMEDIAN) {
		     m2[nstar[k]] = q[0];
		     qp[nstar[k]] = q[1];
		     qc[nstar[k]] = q[2];
		     q3c[nstar[k]] = q[3];
		     q3s[nstar[k]] = q[4];
		     q1c[nstar[k]] = q[5];
		     q1s[nstar[k]] = q[6];
/* Get the position in the focal plane and therefore the qt statistic */
		     psc_cell_to_pixel(cellx, celly, 0.5*PSC_HCELL/PSC_PIXEL, 
				       0.5*PSC_VCELL/PSC_PIXEL, &xota, &yota);
		     psc_pixel_to_fp(ota_xid, ota_yid,	xota, yota, &xfp, &yfp);
		     phi = atan2(yfp, xfp);
/* Change so that + means tangential, - means radial */
		     qt[nstar[k]] = -q[1] * cos(2*phi) - q[2] * sin(2*phi);
		     nstar[k] += 1;
		  }
		  if(fwhm[0] > MIN_BELIEVABLE_FWHM && 
		     fwhm[1] > MIN_BELIEVABLE_FWHM && nfw[k] < MAXPSFMEDIAN) {
		     fw[nfw[k]] = sqrt(fwhm[0]*fwhm[1]);
		     nfw[k] += 1;
		  }
	       }
	       fwmed[k] = double_median(nfw[k], fw);
	       if(nstar[k] > 0) {
		  m2med[k] = double_median(nstar[k], m2);
		  qpmed[k] = double_median(nstar[k], qp);
		  qcmed[k] = double_median(nstar[k], qc);
		  qtmed[k] = double_median(nstar[k], qt);
		  q3cmed[k] = double_median(nstar[k], q3c);
		  q3smed[k] = double_median(nstar[k], q3s);
		  q1cmed[k] = double_median(nstar[k], q1c);
		  q1smed[k] = double_median(nstar[k], q1s);
	       } else {
		  m2med[k] = qpmed[k] = qcmed[k] = qtmed[k] = -99.99;
		  q3cmed[k] = q3smed[k] = -99.99;
		  q1cmed[k] = q1smed[k] = -99.99;
	       }
/* Toss these results into the macrocell median hopper */
	       for(l=0; l<nfw[k]; l++) {
		  if(nfwave < MAXPSFMEDIAN) fwavg[nfwave++] = fw[l];
	       }
	       for(l=0; l<nstar[k]; l++) {
		  if(nqavg < MAXPSFMEDIAN) {
		     qpavg[nqavg] = qp[l];
		     qcavg[nqavg] = qc[l];
		     qtavg[nqavg] = qt[l];
		     q3cavg[nqavg] = q3c[l];
		     q3savg[nqavg] = q3s[l];
		     q1cavg[nqavg] = q1c[l];
		     q1savg[nqavg] = q1s[l];
		     nqavg++;
		  }
	       }
	    }
	 }
/* Get the median of FWHM and qt over this macrocell */
	 qpmacro = double_median(nqavg, qpavg);
	 qcmacro = double_median(nqavg, qcavg);
	 qtmacro = double_median(nqavg, qtavg);
	 q3cmacro = double_median(nqavg, q3cavg);
	 q3smacro = double_median(nqavg, q3savg);
	 q1cmacro = double_median(nqavg, q1cavg);
	 q1smacro = double_median(nqavg, q1savg);
	 if(nqavg == 0) qpmacro = qcmacro = qtmacro = q3cmacro = q3smacro = q1cmacro = q1smacro = -99.99;
	 fwmacro = double_median(nfwave, fwavg);

/* Print out the results from this macrocell */
	 for(j=0; j<psfavg; j++) {
	    for(i=0; i<psfavg; i++) {
	       cellx = i0 + i;
	       celly = j0 + j;
	       k = PSC_NX*celly + cellx;
	       cell = ota + k;

	       fprintf(fp, "ext=xy%1d%1d bias=%d sky=%d rmssky=%d npsf=%d fwhm=%.2f fwmed=%.2f m2=%.2f qp=%.3f qc=%.3f qt=%.3f q3c=%.3f q3s=%.3f q1c=%.3f q1s=%.3f qpm=%.3f qcm=%.3f qtm=%.3f q3cm=%.3f q3sm=%.3f q1cm=%.3f q1sm=%.3f\n", 
		       cellx, celly, cell->bias, cell->sky, cell->rms, 
		       nstar[k], fwmed[k], fwmacro, m2med[k], 
		       qpmed[k], qcmed[k], qtmed[k], q3cmed[k], q3smed[k], 
		       q1cmed[k], q1smed[k],
		       qpmacro, qcmacro, qtmacro, q3cmacro, q3smacro, 
		       q1cmacro, q1smacro);
	    }
	 }



      } /* i0 */
   } /* j0 */

   fclose(fp);
   return(0);
}
