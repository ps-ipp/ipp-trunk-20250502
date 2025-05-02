/* persistfix.c - fix up persistence streaks */

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
/* persist_fix(): Repair the persistence trails in a cell */
/* burn_fix must have been run first! */
STATIC int persist_fix(int nx, int ny, int NX, IMTYPE *buf, 
		    CELL *cell, char *camera)
{
   int k, err=0;

/* Merge all overlapping streaks */
  persist_merge(cell);


/* Fix up all the persistence streaks */
   for(k=0; k<cell->npersist; k++) {
     /* Is this just a blasted area being carried for IPP? */
     if((cell->persist)[k].func == BURN_BLASTED) {
	   if(cell->time - (cell->persist)[k].time > EXPIRE_TRAIL_TIME) {
	    (cell->persist)[k].fiterr = FIT_EXPIRED;
	   }
	   continue;
     }

/* This had a significant positive slope, what do we do now? */
     if( (cell->persist)[k].func == BURN_POSSLOPE) {
/* try again, so let it slide through... */
     }

/* Fit the trail */
     err = fit_trail(nx, ny, NX, imbuf, mbuf, cell->persist+k, 0, 
	     cell->sky+cell->bias, cell->rms, BURN_EXP, camera);

/* Subtract out the fit */
     if(!cell->persist[k].fiterr) 
	   err = sub_fit(nx, ny, NX, buf, cell->persist+k, 1);
   }

   return(err);
}

/****************************************************************/
/* persist_blab(): Tell us about the persistence trails in a cell */
STATIC int persist_blab(CELL *cell)
{
   int i, k, ymid;

/* The persistent streaks */
   printf("Persistence streaks:\n");
   printf("  #    cx  cy  max     sx  sy    ex  ey  midy  F   slope  zero\n");

   for(k=0; k<cell->npersist; k++) {
      i = (cell->persist[k].ex - cell->persist[k].sx + 1) / 2;
      /*ymid = (cell->persist[k].y0m + cell->persist[k].y1m +
	      cell->persist[k].y0p + cell->persist[k].y1p + 2) / 4;*/
        ymid = (cell->persist[k].sy + cell->persist[k].ey + 1) / 2;
      printf("%3d %5d %3d %5d %5d %3d %5d %3d %5d %2d %8.5f %8.5f\n", 
	     k, cell->persist[k].cx, cell->persist[k].cy,
	     cell->persist[k].max, 
	     cell->persist[k].sx, cell->persist[k].sy, 
	     cell->persist[k].ex, cell->persist[k].ey, ymid,
	     cell->persist[k].func, 
	     cell->persist[k].slope, (cell->persist[k]).zero[i]);
   }
   return(0);
}
