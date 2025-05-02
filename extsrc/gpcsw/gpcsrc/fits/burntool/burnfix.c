/* burnfix.c - various functions to identify and fix burn trails */

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
/* burn_fix(): Find and fix all the burned patches in a cell */
STATIC int burn_fix(int nx, int ny, int NX, int NY, IMTYPE *buf, 
		    CELL *cell, int cellnum, char *camera)
{
   int k, err;

/* Find all the burned patches and bright stars */
   err = star_detect(nx, ny, NX, NY, imbuf, mbuf, msbuf, cell, cellnum, camera);

   err = burn_check(nx, ny, NX, NY, imbuf, mbuf, cell, camera);

//   fprintf(stderr, "Got through burn_check\n");

/* Expand the masks; burns asymmetrically, stars symmetrically */
   err = grow_mask(nx, ny, NX, mbuf, BMASK_GROW, RMASK_GROW, 
		   MASK_SAT_HALO, cell->nburn, cell->burn);
//   fprintf(stderr, "Got through grow mask for burns\n");
   err = grow_mask(nx, ny, NX, mbuf, BMASK_GROW, RMASK_GROW, 
         MASK_STAR_HALO, cell->nstar, cell->star);
//   fprintf(stderr, "Got through grow mask for stars\n");

/* Fix up all the burns */
   for(k=0; k<cell->nburn; k++) {
      if(!cell->burn[k].burned) continue;
/* Fit the trail */
//      fprintf(stderr, "Fitting trail %d\n", k);
      fit_trail(nx, ny, NX, imbuf, mbuf, cell->burn+k, 1, 
	     cell->sky+cell->bias, cell->rms, BURN_PWR, camera);
/* Subtract the fit from the image */
//      fprintf(stderr, "Subtracting trail %d\n", k);
      if(!cell->burn[k].fiterr) sub_fit(nx, ny, NX, buf, cell->burn+k, 1);
   }

/* For testing, blow away the data in favor of the mask */
   if(VERBOSE & VERB_MASK) {
      for(k=0; k<NX*ny; k++) buf[k] = mbuf[k] - BZERO;
   }
   
   return(err);
}

/****************************************************************/
/* burn_blab(): Tell us about all the objects found in this cell */
STATIC int burn_blab(CELL *cell)
{
   int i, k, ymid;

/* Tell us about it? */
   printf("Cell: %d  sky= %d   rms= %d   bias= %d\n", 
	  cell->cell, cell->sky, cell->rms, cell->bias);

/* The burns */
   printf("Burns:\n");
   printf("  #    cx  cy  max     sx  sy    ex  ey  midy  diff  S/B/E  slope     zero\n");
   for(k=0; k<cell->nburn; k++) {
      /*ymid = (cell->burn[k].y0m + cell->burn[k].y1m +
	      cell->burn[k].y0p + cell->burn[k].y1p + 2) / 4;*/
      ymid = (cell->burn[k].sy + cell->burn[k].ey + 1) / 2;
      i = cell->burn[k].nfit / 2;
      printf("%3d %5d %3d %5d %5d %3d %5d %3d %5d %5d %2d %1d %1d %6.3f %8d\n", 
	     k, 
	     cell->burn[k].cx, cell->burn[k].cy,  cell->burn[k].max,
	     cell->burn[k].sx, cell->burn[k].sy, 
	     cell->burn[k].ex, cell->burn[k].ey, ymid,
	     cell->burn[k].diff, cell->burn[k].sat, 
	     cell->burn[k].burned, cell->burn[k].fiterr,
	     cell->burn[k].slope, 
	     cell->burn[k].zero != NULL ? NINT((cell->burn[k]).zero[i]) : 0);
   }

/* The stars */
   printf("Stars:\n");
   printf("  #    cx  cy  max     sx  sy    ex  ey  midy  S/P\n");
   for(k=0; k<cell->nstar; k++) {
      /*ymid = (cell->star[k].y0m + cell->star[k].y1m +
	      cell->star[k].y0p + cell->star[k].y1p + 2) / 4;*/
      ymid = (cell->star[k].sy + cell->star[k].ey + 1) / 2;

      printf("%3d %5d %3d %5d %5d %3d %5d %3d %5d %2d %1d\n", 
	     k, cell->star[k].cx, cell->star[k].cy, 
	     cell->star[k].max, 
	     cell->star[k].sx, cell->star[k].sy, 
	     cell->star[k].ex, cell->star[k].ey, ymid,
	     cell->star[k].sat, cell->star[k].func);
   }
   return(0);
}

/****************************************************************/
/* burn_restore(): Restore the fitted trails back to the image */
STATIC int burn_restore(int nx, int ny, int NX, IMTYPE *buf, CELL *cell, char *camera)
{
   int k;

/* Restore all the burns */
   for(k=0; k<cell->npersist; k++) {
      if(!cell->persist[k].fiterr && 
	 (cell->persist[k].func == BURN_PWR || 
	  cell->persist[k].func == BURN_EXP) ) {
	 sub_fit(nx, ny, NX, buf, &(cell->persist[k]), -1);
      }
   }
   return(0);
}


/****************************************************************/
/* burn_apply(): Subtract the trail fits from the image */
STATIC int burn_apply(int nx, int ny, int NX, IMTYPE *buf, CELL *cell, char *camera)
{
   int k;

/* Restore all the burns */
   for(k=0; k<cell->npersist; k++) {
      if(!cell->persist[k].fiterr && 
	 (cell->persist[k].func == BURN_PWR || 
	  cell->persist[k].func == BURN_EXP) ) {
	 sub_fit(nx, ny, NX, buf, &(cell->persist[k]), +1);
      }
   }
   return(0);
}


/****************************************************************/
/* burn_check(): Find all the burned patches in a cell */
STATIC int burn_check(int nx, int ny, int NX, int NY, DTYPE *data,
		       MTYPE *mask, CELL *cell, char *camera)
{
   int err,k;

/* Given these big stars, identify which ones really have a burn */
   for(k=0; k<cell->nburn; k++) {
      if(!cell->burn[k].burned) {
	 cell->burn[k].diff = -1;
	 continue;
      }
      err = burn_test(nx, ny, NX, data, cell->rms, mask, cell->burn+k, camera);
   }

#ifdef TEST
   printf("Boxes:  sky= %d  rms= %d  bias= %d\n", 
	  cell->sky, cell->rms, cell->bias);
   for(k=0; k<cell->nburn; k++) {
      printf("%5d %5d %5d %5d %5d %5d %5d %5d %5d %5d %5d\n", 
	     k, cell->burn[k].sx, cell->burn[k].sy, 
	     cell->burn[k].ex, cell->burn[k].ey, 
	     cell->burn[k].y0m, cell->burn[k].y1m, cell->burn[k].y0p, cell->burn[k].y1p,
	     cell->burn[k].diff, cell->burn[k].burned);
   }
#endif

/* Make Vista decorations for selected boxes? */
   if(VERBOSE & VERB_VISTA) vista_marker(cell, VISTAMARKER);
   return(0);
}

/****************************************************************/
/* vista_marker(): Put out Vista commands to mark stars */
STATIC int vista_marker(CELL *cell, char *fname)
{
   int k;
   FILE *fp;

/* Make Vista decorations for selected boxes? */
   if(fname != NULL) {
      if( (fp=fopen(fname, "w")) == NULL) {
	 fprintf(stderr, "error: cannot open '%s' for writing\n", fname);
	 return(-1);
      }
/* Saturated objects are red (burned) or green (not burned) */
      for(k=0; k<cell->nburn; k++) {
	 if(cell->burn[k].burned) fprintf(fp, "itv action=v color=1\n");/*red*/
	 fprintf(fp, "itv action=b corners=(%d,%d,%d,%d)\n", 
		 cell->burn[k].sx, cell->burn[k].sy, 
		 cell->burn[k].ex, cell->burn[k].ey);
	 fprintf(fp, "itv action=l corners=(%d,%d,%d,%d)\n", 
		 cell->burn[k].sx, cell->burn[k].y0m, 
		 cell->burn[k].x0m, cell->burn[k].sy);
	 fprintf(fp, "itv action=l corners=(%d,%d,%d,%d)\n", 
		 cell->burn[k].x0p, cell->burn[k].sy, 
		 cell->burn[k].ex, cell->burn[k].y1m);
	 fprintf(fp, "itv action=l corners=(%d,%d,%d,%d)\n", 
		 cell->burn[k].ex, cell->burn[k].y1p, 
		 cell->burn[k].x1p, cell->burn[k].ey);
	 fprintf(fp, "itv action=l corners=(%d,%d,%d,%d)\n", 
		 cell->burn[k].x1m, cell->burn[k].ey, 
		 cell->burn[k].sx, cell->burn[k].y0p);
	 fprintf(fp, "itv action=v color=0\n");/*green*/
      }
/* Stars are white */
      fprintf(fp, "itv action=v color=3\n");/*white*/
      for(k=0; k<cell->nstar; k++) {
	 fprintf(fp, "itv action=b corners=(%d,%d,%d,%d)\n", 
		 cell->star[k].sx, cell->star[k].sy, 
		 cell->star[k].ex, cell->star[k].ey);
	 fprintf(fp, "itv action=l corners=(%d,%d,%d,%d)\n", 
		 cell->star[k].sx, cell->star[k].y0m, 
		 cell->star[k].x0m, cell->star[k].sy);
	 fprintf(fp, "itv action=l corners=(%d,%d,%d,%d)\n", 
		 cell->star[k].x0p, cell->star[k].sy, 
		 cell->star[k].ex, cell->star[k].y1m);
	 fprintf(fp, "itv action=l corners=(%d,%d,%d,%d)\n", 
		 cell->star[k].ex, cell->star[k].y1p, 
		 cell->star[k].x1p, cell->star[k].ey);
	 fprintf(fp, "itv action=l corners=(%d,%d,%d,%d)\n", 
		 cell->star[k].x1m, cell->star[k].ey, 
		 cell->star[k].sx, cell->star[k].y0p);
      }
      fprintf(fp, "itv action=v color=0\n");/*green*/
      fclose(fp);
   }
   return(0);
}

/****************************************************************/
/* burn_test() tests whether a box is burned or not */
/* It's trying to balance the flux of above-below to see whether 
 * there's a net burn above.  It has to cope with the case that the box is
 * so low there is no above in which case it compares center-side.
 * It's somewhat rudimentary as indicated by the FIXME's, and the original
 * version had some real logical problems.
 */
STATIC int burn_test(int nx, int ny, int NX, DTYPE *data, int rms,
		       MTYPE *mask, OBJBOX *box, char *camera)
{
   int i, j, nburn, nref, nmed;
   int y0, y1, x0, x1, xwidth, ymid, xburn, xref, dx=0, dy=0;

   /*introduce a maximum to the burn contrast check, to avoid a very large number in crowded regions*/
   int burn_contrast = MIN(0.3*rms,5); 
   /*introduce a maximum size for the burn contrast check box, instead of a random large 100 pixels */
   /*for small stars you will lose signal after 10 pixels, so use the fitted box size as a good guide*/
   int test_box = MAX(box->ey-box->sy,10); 

   if(VERBOSE & VERB_BOXGROW) {
	printf("burn contrast test using box of max size: %8d\n", test_box);
   }

/* FIXME: needs a test for flat-toppedness as well as trail... */
/* FIXME: needs a test for pure, massive saturation */

/* Center line */
   //ymid = (box->y0m + box->y0p + box->y1m + box->y1p + 2) / 4;
   ymid = (box->sy+box->ey)/2;

/* Irrelevant, too near the top to detect a burn anyway */
   if(box->ey > ny-FIT_EDGE-3) {
      box->diff = -1;
      box->burned = -1;
/* But looks like a bad one which will leave behind persistence */
      if(box->ey-box->sy+1 > 4) box->burned = 1;
      return(0);

/* Too near the bottom, do a side-side test */
   } else if(box->sy < FIT_EDGE+3) {
      dy = 0;
      y0 = ymid + MAX(box->ey-ymid, FIT_EDGE);
      y1 = MIN(y0+100, ny-1);
      x0 = box->sx;
      x1 = box->ex;

      if(2*box->ex - box->sx + 1 < nx-1) {	/* Work right? */
	 dx = box->ex - box->sx + 1;

      } else if(2*box->sx - box->ex - 1 > 0) {	/* Work left? */
	 dx = -(box->ex - box->sx + 1);

      } else {	/* No room!  Oh no! */
	 box->diff = -1;
	 box->burned = -1;
/* But looks like a bad one which will leave behind persistence */
	 if(box->ey-box->sy+1 > 4) box->burned = 1;
	 return(0);
      }

/* Do a top-bottom test for excess flux */
   } else {
      dx = 0;
      dy = MAX(ymid-box->sy, box->ey-ymid);
      
      if(!strcmp(camera,"gpc2")) {
        /*start right at the edge of the star box might not be the best*/
        /*add 20% of box size on top, with a maximum of ten pixels*/
        dy = dy + MIN((box->ey-box->sy)/5,10);

        /*Use the inner third of the star width for this test. Should be brightest there*/
        xwidth = MAX((box->ex-box->sx)/3,10);      
        x0 = box->sx + (box->ex-box->sx)/2 - xwidth/2;
        x1 = box->sx + (box->ex-box->sx)/2 + xwidth/2;
      } else {
        //regular gpc1 setup
        x0 = box->sx;
        x1 = box->ex;
      }
      
      if(dy < FIT_EDGE) dy = FIT_EDGE;
      y0 = ymid + dy;
      y1 = y0 + MIN(ny-1-y0, 2*ymid-y0-1);
      if(y1-y0 > test_box) y1 = y0 + test_box;
      
   }
   if(VERBOSE & VERB_BOXGROW) {
     printf("test input dx,dy,x0,x1, y0, y1, test_box, ymid: %8d %8d %8d %8d %8d %8d %8d %8d\n", dx,dy,x0,x1, y0, y1, test_box, ymid);
   }

/* Do the test; get the median of the averages across x */
   nmed = 0;
   for(j=y0; j<y1; j++) {
      xburn = xref = 0;
      nburn = nref = 0;
      for(i=x0; i<=x1; i++) {
	     if(mask[i+j*NX] == MASK_NONE) {
	       xburn += data[i+j*NX];
	       nburn++;
	     }
	     if(dx != 0) {	/* Side to side */
	       if(mask[i+dx+j*NX] == MASK_NONE) {
	        xref += data[i+dx+j*NX];
	        nref++;
	       }
	     } else {	/* top-bottom */
	       if(mask[i+(2*ymid-j)*NX] == MASK_NONE) {
	        xref += data[i+(2*ymid-j)*NX];
	        nref++;
	       }
	     }
      }
      if(nburn > 0) xburn /= nburn;
      if(nref > 0) xref /= nref;
      if(nref > 0 && nburn > 0) median_buf[nmed++] = xburn - xref;
      if(VERBOSE & VERB_BOXGROW) {
        printf("contrast: %8d %8d %8d %8d %8d %8d %8d\n", j, xburn,nburn, (2*ymid-j), xref,nref, xburn - xref);
      }

   }
   box->diff = int_median(nmed, median_buf);
   box->burned = (box->diff > burn_contrast);
   if(VERBOSE & VERB_BOXGROW) {
     printf("test output: %8d %8d %8d\n", box->burned, burn_contrast, box->diff);
   }

   return(0);
}

#ifdef ORIG_BURN_TEST
// This has a lot of problems -- chief among which is what happens with
// a huge kite-shaped bad region.  It's fundamentally trying to balance
// the flux of above-below to see whether there's a net burn above.  But
// it has to cope with the case that the box is so low there is no above
// in which case it compares center-side.
/****************************************************************/
/* burn_test() tests whether a box is burned or not */
STATIC int burn_test(int nx, int ny, int NX, DTYPE *data, int rms,
		       MTYPE *mask, OBJBOX *box, char *camera)
{
   int i, j, n, nrow, nmed;
   int y0, y1, ymid, xsum, dx=0;

/* FIXME: needs a test for flat-toppedness as well as trail... */
/* FIXME: needs a test for pure, massive saturation */

/* Center line */
   //ymid = (box->y0m + box->y0p + box->y1m + box->y1p + 2) / 4;
   ymid = (box->sy + box->ey + 1) / 2;

/* Starting point offset */
   y0 = MAX(ymid-box->sy, box->ey-ymid);
   if(y0 < FIT_EDGE) y0 = FIT_EDGE;

/* Irrelevant, too near the top */
   if(box->ey > ny-FIT_EDGE-3) {
      box->diff = -1;
      box->burned = -1;
/* But looks like a bad one which will leave behind persistence */
      if(box->ey-box->sy+1 > 4) box->burned = 1;
      return(0);

/* Need a one-sided test */
   } else if(box->sy < FIT_EDGE+3) {
/* Ending point offset */
      y1 = ny;
      dx = box->ex - box->sx;
      if(box->sx > nx-1-box->ex) dx *= -1;

   } else {
/* Ending point offset */
      y1 = MIN(ny-2-ymid, ymid-1);
   }
   n = MIN(100, y1-y0);

   nmed = 0;
   for(j=0; j<n; j++) {
      xsum = 0;
      nrow = 0;
      for(i=box->sx; i<=box->ex; i++) {
// IS THIS THE PROBLEM?
	 xsum += data[i+(ymid+y0+j)*NX]*(mask[i+(ymid+y0+j)*NX] == MASK_NONE);
	 if(y1 < ny) {
	    xsum -= data[i+(ymid-y0-j)*NX] * 
	       (mask[i+(ymid-y0-j)*NX] == MASK_NONE);
	    nrow += (mask[i+(ymid-y0-j)*NX] == MASK_NONE) * 
	       (mask[i+(ymid+y0+j)*NX] == MASK_NONE);
	 } else {
	    xsum -= data[i+dx+(ymid+y0+j)*NX] * 
	       (mask[i+dx+(ymid+y0+j)*NX] == MASK_NONE);
	    nrow += (mask[i+dx+(ymid+y0+j)*NX] == MASK_NONE) * 
	       (mask[i+(ymid+y0+j)*NX] == MASK_NONE);
	 }
      }
      if(nrow > 0) median_buf[nmed++] = xsum / nrow;
   }
   box->diff = int_median(nmed, median_buf);
   box->burned = 0;
   if(box->diff > 0.3*rms) box->burned = 1;
   return(0);
}
#endif
