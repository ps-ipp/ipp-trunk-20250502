/* persistio.c - read and write persistence info */

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
/* persist_read(): Read all the persistence trails from a file */
STATIC int persist_read(CELL *cell, const char *infile, int apply)
{
   int i, k, nbox=0;
   char line[1024];
   FILE *fp;

/* Initialize the counts */
   for(k=0; k<MAXCELL; k++) {
      cell[k].npersist = 0;
      cell[k].persist = NULL;
   }

   if(infile == NULL) return(0);

   if( (fp=fopen(infile, "r")) == NULL) {
      fprintf(stderr, "\rerror: cannot open '%s' for reading\n", infile);
      return(-1);
   }
/* Read in all the burns */
   nbox = 0;
   while(fgets(line, 1024, fp) != NULL) {
      if(line[0] == '#') continue;
      sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %lf %d %d %d %d %d",
	     &boxbuf[nbox].cell, &boxbuf[nbox].time,
	     &boxbuf[nbox].cx, &boxbuf[nbox].cy,
	     &boxbuf[nbox].max, &boxbuf[nbox].y0,
	     &boxbuf[nbox].sx, &boxbuf[nbox].sy, 
	     &boxbuf[nbox].ex, &boxbuf[nbox].ey,
	     &boxbuf[nbox].y0m, &boxbuf[nbox].y0p,
	     &boxbuf[nbox].y1m, &boxbuf[nbox].y1p,
	     &boxbuf[nbox].x0m, &boxbuf[nbox].x0p,
	     &boxbuf[nbox].x1m, &boxbuf[nbox].x1p,
	     &boxbuf[nbox].func, &boxbuf[nbox].up,
	     &boxbuf[nbox].slope, &boxbuf[nbox].nfit,
	     &boxbuf[nbox].sxfit, &boxbuf[nbox].exfit,
	     &boxbuf[nbox].fiterr, &boxbuf[nbox].eyfit);
      if(boxbuf[nbox].nfit > 0) {
	     boxbuf[nbox].zero = (double *)calloc(boxbuf[nbox].nfit, sizeof(double));
	     boxbuf[nbox].xfit = (int *)calloc(boxbuf[nbox].nfit, sizeof(int));
	     boxbuf[nbox].yfit = (int *)calloc(boxbuf[nbox].nfit, sizeof(int));
	     if(boxbuf[nbox].zero == NULL ||
	      boxbuf[nbox].xfit == NULL ||
	      boxbuf[nbox].yfit == NULL) {
	     fprintf(stderr, "\rerror: failed to alloc boxbuf\n");
	     exit(-673);
	   }

	 for(i=0; i<boxbuf[nbox].nfit; i++) {
	    if(fgets(line, 1024, fp) == NULL) {
	       fprintf(stderr, "\rerror: short read of burn lines\n");
	       return(-1);
	    }
	    sscanf(line, "%d %d %lf\n", &boxbuf[nbox].xfit[i], 
		   &boxbuf[nbox].yfit[i], &boxbuf[nbox].zero[i]);
	 }
      }
// 100203 JT: fiterr now saved and read, refit if not just an "apply"
      if(!apply) boxbuf[nbox].fiterr = 0;
/* Augment counts */
      k = boxbuf[nbox].cell;
      if(k < 0 || k >= MAXCELL) {
	 fprintf(stderr, "\rerror: illegal cell %d at %d from '%s'\n", 
		 k, nbox, line);
	 fclose(fp);
	 return(-1);
      }
      cell[k].npersist += 1;

      if(++nbox >= MAXBURN) {
	 fprintf(stderr, "\rerror: persist_read overflowed MAXBURN\n");
	 fclose(fp);
	 return(-1);
      }
   }

/* Allocate some space for them */
   for(k=0; k<MAXCELL; k++) {
      if( (i=cell[k].npersist) > 0) {
	 if( (cell[k].persist = (OBJBOX *)calloc(i, sizeof(OBJBOX))) == NULL) {
	    fprintf(stderr, "\rerror: failed to alloc cell persist buffer\n");
	    exit(-674);
	 }
	 cell[k].npersist = 0;
      }
   }
/* Copy the results to the cells */
   for(i=0; i<nbox; i++) {
      k = boxbuf[i].cell;
      memcpy(cell[k].persist+cell[k].npersist, boxbuf+i, sizeof(OBJBOX));
      cell[k].npersist += 1;
   }

   fclose(fp);
   return(0);
}


#define DIFFERENT_STREAK 20	/* Proximity for union versus intersection */

# define myMAXSIZE 20000


/****************************************************************/
/* persist_merge(): Disentangle overlapping persistence streaks */
STATIC int persist_merge(CELL *cell)
{
   int i, j, k, n, nbox, xs, xe;
   int jp, kp,xdiff;
   int lapmax;
   OBJBOX *box;
   int xusage[myMAXSIZE], yctr[myMAXSIZE], boxid[myMAXSIZE];
   double zk, zj;

/* Look at all the boxes -- if not isolated then merge or sever */
   nbox = cell->npersist;
   box = cell->persist;

/* Assess the clustering of all the streaks */
   for(i=0; i<myMAXSIZE; i++) xusage[i] = 0;
   for(i=0; i<myMAXSIZE; i++) boxid[i] = -1;
   for(k=0; k<nbox; k++) {
      if(box[k].ex >= myMAXSIZE-1) continue;
      for(i=box[k].sx; i<=box[k].ex; i++) xusage[i] += 1;
   }

/* Identify clusters */
   for(xs=0; xs<myMAXSIZE-1; xs++) {
    if(xusage[xs] == 0) continue;
      for(xe=xs, lapmax=0; xe<myMAXSIZE-1; xe++) {
	     if(xusage[xe+1] == 0) break;
	     lapmax = MAX(lapmax, xusage[xe]);
      }
      if(lapmax == 1) {	/* No overlap?  No problem. */
	    xs = xe + 1;	/* Hop to next gap */
	    continue;
      }

      /* Which boxes overlap this cluster? */
      for(k=n=0; k<nbox; k++) {
        if(box[k].sx > xe || box[k].ex < xs) continue;
	     boxid[n] = k;
	     //yctr[n] = (box[k].y0m+box[k].y1m+box[k].y0p+box[k].y1p+2) / 4;
	     yctr[n] = (box[k].sy+box[k].ey+1) / 2;
	     n++;
      }

      for(kp=0; kp<n; kp++) {
	    k = boxid[kp];
	    zk = 0.0;
	    if(box[k].nfit > 0) zk = box[k].zero[box[k].nfit/2];
	    for(jp=kp+1; jp<n; jp++) {
	     j = boxid[jp];
	     zj = 0.0;
	     if(box[j].nfit > 0) zj = box[j].zero[box[j].nfit/2];
	     if(box[j].sx > box[k].sx) xdiff = box[j].sx - box[k].ex;
	     if(box[j].sx <= box[k].sx) xdiff = box[k].sx - box[j].ex;

	     /* CZW: Since I added the initialization statement above, */
	     /*      any box with boxid == -1 hasn't been set, and */
	     /*      therefore needs to be dropped, I think. */
	     if ((k < 0)||(j < 0)) { continue; }

        /*Tdb20220222: In the case of overlapping x-coords but not overlapping y-coords*/
        /*I fail to see the need to change the x-range of any streaks. That just spells disaster*/
        // Case 1: different y start => sever 
	     if(ABS(yctr[jp]-yctr[kp]) > DIFFERENT_STREAK) {
         // Trim back the feebler streak 
	      if(zk > zj) {
		    /*
		    if(box[j].sxfit >= box[k].sxfit) 
	       box[j].sxfit = MAX(box[j].sxfit, box[k].exfit+1);
		    if(box[j].exfit <= box[k].exfit) 
		    box[j].exfit = MIN(box[j].exfit, box[k].sxfit-1);
		    */
	      } else {
	       /*	
          if(box[k].sxfit >= box[j].sxfit) 
		    box[k].sxfit = MAX(box[k].sxfit, box[j].exfit+1);
		    if(box[k].exfit <= box[j].exfit) 
		    box[k].exfit = MIN(box[k].exfit, box[j].sxfit-1);
		    */
	      }
	     }

	     /* Case 2: pretty much the same y start => union */
	     if((ABS(yctr[jp]-yctr[kp]) <= DIFFERENT_STREAK) && (xdiff < 0) ) {
          /* Merge k into j, trash k from further consideration */
	       box[j].time = MAX(box[j].time, box[k].time);
	       box[j].sx = MIN(box[j].sx, box[k].sx);
	       box[j].ex = MAX(box[j].ex, box[k].ex);
	       box[j].sy = MIN(box[j].sy, box[k].sy);
	       box[j].ey = MAX(box[j].ey, box[k].ey);
	       box[j].y0m = MIN(box[j].y0m, box[k].y0m);
	       box[j].y0p = MAX(box[j].y0p, box[k].y0p);
	       box[j].x0m = MIN(box[j].x0m, box[k].x0m);
	       box[j].x0p = MAX(box[j].x0p, box[k].x0p);
	       box[j].y1m = MIN(box[j].y1m, box[k].y1m);
	       box[j].y1p = MAX(box[j].y1p, box[k].y1p);
	       box[j].x1m = MIN(box[j].x1m, box[k].x1m);
	       box[j].x1p = MAX(box[j].x1p, box[k].x1p);
	       box[j].sxfit = MIN(box[j].sxfit, box[k].sxfit);
	       box[j].exfit = MAX(box[j].exfit, box[k].exfit);
	       box[j].eyfit = MIN(box[j].eyfit, box[k].eyfit);

	       box[k].exfit = -999;
	       yctr[kp] = -2 * DIFFERENT_STREAK;
	    }
	 }
      }

      xs = xe + 1;	/* Hop to next gap */

   } /* Cluster loop */

/* Excise the boxes which have been eradicated (sxfit > exfit) */
   for(k=0; k<nbox; k++) {
      if(box[k].exfit < 0) {
	    for(j=k; j<nbox-1; j++) memcpy(box+j, box+j+1, sizeof(OBJBOX));
	    nbox--;
	    k--;
      }
   }
   cell->npersist = nbox;


   return(0);
}


/****************************************************************/
/* persist_write(): Write all the persistence data for the next image */
STATIC int persist_write(CELL *cell, const char *outfile)
{
   int i, k, j, err=0;
   FILE *fp;

   if(outfile == NULL) return(0);
   if( (fp=fopen(outfile, "w")) == NULL) {
      fprintf(stderr, "\rerror: cannot open '%s' for writing\n", outfile);
      return(-1);
   }

/* Dump out all the burns for the next image... */
/* FIXME: what to do about generic cell info? */
   fprintf(fp,  "# Cell: %d  sky= %d   rms= %d   bias= %d\n", 
	   cell[0].cell, cell[0].sky, cell[0].rms, cell[0].bias);
   fprintf(fp, "#Cell time    cx  cy  max   y0   sx  sy   ex  ey  y0m y0p y1m y1p x0m x0p x1m x1p F up    slope nfit sxf exf fiterr eyf\n");

   for(j=0; j<MAXCELL; j++) {
/* First: patched up persists */
      for(k=0; k<cell[j].npersist; k++) {

/* Retire old burns */
	 if(cell[j].time - cell[j].persist[k].time > EXPIRE_TRAIL_TIME)
	    continue;

	 if(PERSIST_RETAIN) {
/* Keep fits which have a dubious slope */
	    if(cell[j].persist[k].fiterr != FIT_SLOPE_ERROR) {
	       if(cell[j].persist[k].fiterr) continue;
	       if(cell[j].persist[k].nfit <= 0) continue;
	    }
	 } else {
	    if(cell[j].persist[k].fiterr) continue;
	    if(cell[j].persist[k].nfit <= 0) continue;
	 }

	 fprintf(fp, "%3d %7d  %3d %3d %5d %3d  %3d %3d  %3d %3d  %3d %3d %3d %3d %3d %3d %3d %3d  %1d %1d %9.6f %3d %3d %3d %d %3d\n",
		 j, cell[j].persist[k].time, 
		 cell[j].persist[k].cx, cell[j].persist[k].cy, 
		 cell[j].persist[k].max, cell[j].persist[k].y0,
		 cell[j].persist[k].sx, cell[j].persist[k].sy, 
		 cell[j].persist[k].ex, cell[j].persist[k].ey,
		 cell[j].persist[k].y0m, cell[j].persist[k].y0p,
		 cell[j].persist[k].y1m, cell[j].persist[k].y1p,
		 cell[j].persist[k].x0m, cell[j].persist[k].x0p,
		 cell[j].persist[k].x1m, cell[j].persist[k].x1p,
		 cell[j].persist[k].func, cell[j].persist[k].up, 
		 cell[j].persist[k].slope, cell[j].persist[k].nfit,
		 cell[j].persist[k].sxfit, cell[j].persist[k].exfit, 
		 cell[j].persist[k].fiterr, cell[j].persist[k].eyfit);
	 for(i=0; i<cell[j].persist[k].nfit; i++) {
	    fprintf(fp, "%3d %3d %8.4f\n", cell[j].persist[k].xfit[i], 
		    cell[j].persist[k].yfit[i], cell[j].persist[k].zero[i]);
	 }
      }

/* Second: new burns */
      for(k=0; k<cell[j].nburn; k++) {
	    if(!cell[j].burn[k].burned) continue;
	    if(PERSIST_RETAIN) {
        /* Keep fits which have a dubious slope */
	     if(cell[j].burn[k].fiterr != FIT_SLOPE_ERROR) {
	       if(cell[j].burn[k].fiterr && 
		    cell[j].burn[k].fiterr != FIT_TOP_ERROR) continue;
	       if(cell[j].burn[k].nfit <= 0) continue;
	       /*kick out burns which are single pixels*/
	     }
	    } else {
	     if(cell[j].burn[k].fiterr && 
	       cell[j].burn[k].fiterr != FIT_TOP_ERROR) continue;
	     if(cell[j].burn[k].nfit <= 0) continue;
	     /*kick out burns which are single pixels*/

	    }

	 i = (cell[j].burn[k].ex - cell[j].burn[k].sx + 1) / 2;
	 fprintf(fp, "%3d %7d  %3d %3d %5d %3d  %3d %3d  %3d %3d  %3d %3d %3d %3d %3d %3d %3d %3d  %1d %1d %9.6f %3d %3d %3d %d %3d\n", 
		 j, cell[j].burn[k].time, 
		 cell[j].burn[k].cx, cell[j].burn[k].cy,
		 cell[j].burn[k].max, cell[j].burn[k].y0,
		 cell[j].burn[k].sx, cell[j].burn[k].sy, 
		 cell[j].burn[k].ex, cell[j].burn[k].ey,
		 cell[j].burn[k].y0m, cell[j].burn[k].y0p,
		 cell[j].burn[k].y1m, cell[j].burn[k].y1p,
		 cell[j].burn[k].x0m, cell[j].burn[k].x0p,
		 cell[j].burn[k].x1m, cell[j].burn[k].x1p,
		 cell[j].burn[k].func, cell[j].burn[k].up, 
		 cell[j].burn[k].slope, cell[j].burn[k].nfit,
		 cell[j].burn[k].sxfit, cell[j].burn[k].exfit, 
		 cell[j].burn[k].fiterr, cell[j].burn[k].eyfit);
	 for(i=0; i<cell[j].burn[k].nfit; i++) {
	    fprintf(fp, "%3d %3d %8.4f\n", cell[j].burn[k].xfit[i], 
		    cell[j].burn[k].yfit[i], cell[j].burn[k].zero[i]);
	 }
      }
   }
   fclose(fp);

   return(err);
}
