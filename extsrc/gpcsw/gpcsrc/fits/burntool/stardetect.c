/* stardetect.c - identify burns and stars */

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
/* star_detect(): Find all the stars and burned patches in a cell */
STATIC int star_detect(int nx, int ny, int NX, int NY, DTYPE *data,
		       MTYPE *mask, MTYPE *veto, CELL *cell, int cellnum, char *camera)
{
   int i, j, k, l;
   int burnthresh, trailthresh, starthresh, starcut, thresh_hi, thresh_lo;
   int nbox, xon, xoff;

/* Reset the masks (DEPENDS on MASK_NONE = 0, tough!) */
   bzero(mask, NX*ny*sizeof(MTYPE));
   bzero(veto, NX*ny*sizeof(MTYPE));

   nbox = 0;

   burnthresh = thresh_hi = BURN_THRESH + cell->bias;
   trailthresh = thresh_lo = TRAIL_THRESH + cell->bias;
   starthresh = STAR_THRESH + cell->sky + cell->bias;
   starcut = STAR_THRESH/2 + cell->sky + cell->bias;

   /*try a new value for thresh_lo which is 10 sigmas above the background, but with a safety limit of 1000 counts*/
	trailthresh = cell->sky + (10*cell->rms);
	if(trailthresh <= 1000) trailthresh = 1000;
	trailthresh = trailthresh + cell->bias;
	thresh_lo = trailthresh;

   /*try a new value for burnthresh which is twice the bias above the sky background, but with a safety limit of 30000 counts*/
	burnthresh = cell->sky + (2*cell->bias);
	if(burnthresh <= BURN_THRESH) burnthresh = BURN_THRESH;
	thresh_hi = burnthresh;

/* Look at all the pixels which pass the burn threshold */
   for(j=0; j<ny; j++) {
      xon = -1;
      for(i=0; i<nx; i++) {
         /* Big enough?  Initialize a box and push it outto trail level. */
	      if(mask[i+j*NX] > MASK_NONE) continue;
	      if(xon < 0) {
	        if(data[i+j*NX] > burnthresh) {
	          xon = i;
	          thresh_hi = burnthresh;
	          thresh_lo = trailthresh;
	          if(VERBOSE & VERB_DETECT) {
		         printf("Starting a burn at %d %d %d %d\n", i, j, data[i+j*NX],thresh_lo);
	          }
	        } else if(data[i+j*NX] > starthresh) {
             /* Local maximum? */
	          if(!local_max(i, j, STAR_RADIUS, nx, ny, NX, data)) continue;
	          if(data[i+j*NX] < veto[i+j*NX]) continue;
	          xon = i;
//	          thresh_hi = starthresh;
//	          thresh_lo = starcut;
	          thresh_hi = data[i+j*NX];
	          thresh_lo = STAR_FRAC*(data[i+j*NX]-cell->sky-cell->bias) + 
		       cell->sky + cell->bias;
	          if(VERBOSE & VERB_DETECT) {
		         printf("Starting a star at %d %d %d %d\n", i, j, data[i+j*NX],thresh_lo);
	          }
	        }
	      }

	      if(xon >= 0 && (data[i+j*NX] < thresh_hi || i == nx-1)) {
	        xoff = i<nx-1 ? i-1 : nx-1;
	        if(nbox >= MAXBURN) {
	           fprintf(stderr, "error: too many burn boxes\n");
	           return(-1);
	        }
	        boxbuf[nbox].sx = xon;
	        boxbuf[nbox].ex = xoff;
	        boxbuf[nbox].sy = j;
	        for(k=j; k<ny && data[(xon+xoff)/2+k*NX] > thresh_hi; k++);
	        boxbuf[nbox].ey = k-1;
	        grow_box(nx, ny, NX, data, thresh_lo, boxbuf+nbox);
		
           /* Fill in max and center info */
	        boxbuf[nbox].max = 0;
	        for(l=boxbuf[nbox].sy; l<=boxbuf[nbox].ey; l++) {
	         for(k=boxbuf[nbox].sx; k<=boxbuf[nbox].ex; k++) {
		       if(data[k+l*NX] > boxbuf[nbox].max) {
		        boxbuf[nbox].cx = k;
		        boxbuf[nbox].cy = l;
		        boxbuf[nbox].max = data[k+l*NX];
		       }
	         }
	        }

/* A box which triggered on starthresh cannot envelop burnthresh */
	        if(thresh_hi < burnthresh && boxbuf[nbox].max > burnthresh) {
	          if(VERBOSE & VERB_DETECT) {
		        printf("Ditching box %d %d %d %d  max = %d > burnthresh\n",
			     boxbuf[nbox].sx, boxbuf[nbox].sy,
			     boxbuf[nbox].ex, boxbuf[nbox].ey,
			     boxbuf[nbox].max);
		        fflush(stdout);
	          }
/* Mark the veto mask so as not to trigger on this one again */
	          for(l=boxbuf[nbox].sy; l<=boxbuf[nbox].ey; l++) {
		         for(k=boxbuf[nbox].sx; k<=boxbuf[nbox].ex; k++) {
		          veto[k+l*NX] = boxbuf[nbox].max;
		         }
	          }
	          xon = -1;
	          continue;
	        }

/* A box that is composed of a single pixel should not be considered a burn */
	        if((boxbuf[nbox].ex - boxbuf[nbox].sx) <= 2 && (boxbuf[nbox].ey - boxbuf[nbox].sy) <= 2) {
	          if(VERBOSE & VERB_DETECT) {
		        printf("Ditching box %d %d %d %d  for being too small\n",
			     boxbuf[nbox].sx, boxbuf[nbox].sy,
			     boxbuf[nbox].ex, boxbuf[nbox].ey);
		        fflush(stdout);
	          }
/* Mark the veto mask so as not to trigger on this one again */
	          for(l=boxbuf[nbox].sy; l<=boxbuf[nbox].ey; l++) {
		         for(k=boxbuf[nbox].sx; k<=boxbuf[nbox].ex; k++) {
		          veto[k+l*NX] = boxbuf[nbox].max;
		         }
	          }
	          xon = -1;
	          continue;
	        }

/* Mask the pixels so as not to catch them again! */
	        for(l=boxbuf[nbox].sy; l<=boxbuf[nbox].ey; l++) {
	         for(k=boxbuf[nbox].sx; k<=boxbuf[nbox].ex; k++) {
		       mask[k+l*NX] = MASK_CTR;
	         }
	        }
	        boxbuf[nbox].sat = boxbuf[nbox].max >= burnthresh;
	        boxbuf[nbox].max -= cell->sky + cell->bias;
/* Burned only if it triggered on burnthresh, not if it enveloped it */
	        boxbuf[nbox].burned = thresh_hi >= burnthresh;
/* But take a hard look at stars with really bright centers... */
	        if(!boxbuf[nbox].burned && boxbuf[nbox].max > MAX_THRESH) {
	         burn_test(nx, ny, NX, data, cell->rms, mask, boxbuf+nbox, camera);
	        }
	        boxbuf[nbox].time = cell->time;
	        boxbuf[nbox].cell = cellnum;
	        boxbuf[nbox].stamp = NULL;
	        boxbuf[nbox].func = FUNC_NONE;
 
	        boxbuf[nbox].sxfit = boxbuf[nbox].sx;
	        boxbuf[nbox].exfit = boxbuf[nbox].ex;
	        boxbuf[nbox].nfit = 0;
	        boxbuf[nbox].zero = NULL;
	        boxbuf[nbox].xfit = boxbuf[nbox].yfit = NULL;
	        if(VERBOSE & VERB_DETECT) {
	         printf("New box at %d %d %d %d", 
		      boxbuf[nbox].sx, boxbuf[nbox].sy,boxbuf[nbox].ex, boxbuf[nbox].ey);
	         printf("   %d %d %d %d", 
		      boxbuf[nbox].x0m, boxbuf[nbox].x0p,boxbuf[nbox].x1m, boxbuf[nbox].x1p);
	         printf("   %d %d %d %d\n", 
		      boxbuf[nbox].y0m, boxbuf[nbox].y0p,boxbuf[nbox].y1m, boxbuf[nbox].y1p);
	         fflush(stdout);
	        }
	        xon = -1;
	        nbox++;
	      }
      }
   }

/* Reject any stars which have enveloped a burned core */
   for(k=0; k<nbox; k++) {
//      printf("Examining box %d\n", k);
      if(!(boxbuf[k].burned)) continue;
//      printf("  which is a burn\n");
      for(l=0; l<nbox; l++) {
	 if(k == l) continue;
//	 printf("Examining star %d\n", l);
/* Has star l enveloped burn k? */
/* Don't care whether this is burned or unburned: enveloping is bad */
//	 if(!(boxbuf[l].burned)) continue;
//	 printf("  which has a burned core\n");
//	 printf("%3d %3d %3d %3d %3d %3d %3d %3d\n", 
//		boxbuf[k].sx, boxbuf[l].sx, boxbuf[k].ex, boxbuf[l].ex,
//		boxbuf[k].sy, boxbuf[l].sy, boxbuf[k].ey, boxbuf[l].ey);
	 if(boxbuf[k].sx >= boxbuf[l].sx &&
	    boxbuf[k].ex <= boxbuf[l].ex &&
	    boxbuf[k].sy >= boxbuf[l].sy &&
	    boxbuf[k].ey <= boxbuf[l].ey) {
/* Wipe it out */
	    for(i=l; i<nbox-1; i++) memcpy(boxbuf+i, boxbuf+i+1, sizeof(OBJBOX));
//	    printf("Wiping out star %d enveloping burn %d\n", l, k);
	    nbox--;
	    l--;
	    if(k > l) k--;
	 }
      }
   }

/* Remark mask if saturated */
   for(i=0; i<nbox; i++) {
      if(boxbuf[i].sat) {
	 for(l=boxbuf[i].sy; l<=boxbuf[i].ey; l++) {
	    for(k=boxbuf[i].sx; k<=boxbuf[i].ex; k++) {
	       mask[k+l*NX] = MASK_SAT;
	    }
	 }
      }
   }

/* How many real burns? */
   for(k=cell->nburn=0; k<nbox; k++) if(boxbuf[k].burned) cell->nburn++;
   cell->nstar = nbox - cell->nburn;
   cell->burn = (OBJBOX *)calloc(cell->nburn, sizeof(OBJBOX));
   cell->star = (OBJBOX *)calloc(cell->nstar, sizeof(OBJBOX));
   if( cell->burn == NULL || cell->star == NULL) {
      fprintf(stderr, "\rerror: failed to alloc burn box\n");
      exit(-678);
   }

/* Copy the boxes to the cell info structure */
   for(k=i=j=0; k<nbox; k++) {
      if(boxbuf[k].burned) memcpy(cell->burn+j++, boxbuf+k, sizeof(OBJBOX));
      else                 memcpy(cell->star+i++, boxbuf+k, sizeof(OBJBOX));
   }
   return(0);
}
