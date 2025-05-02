/*
 *   ppImageBurntoolApply.c
 *   Includes code ported from gpcsw/gpcsrc to read burntool table files and apply the fits
 */

#include "ppImage.h"

// Now include the files copied from gpcsw/gpcsrc

// these get redefined
#undef MAX
#undef MIN

#include "burntool.h"
#define EXTERN  /* define EXTERN to declare variables in params.h */
#include "burnparams.h"


static void * getBurntoolDataForCell(pmConfig *config, ppImageOptions *options, int burntool_cell);

bool ppImageBurntoolApply(pmConfig *config, ppImageOptions *options, pmFPAview *view, pmReadout *readout)
{
    // convert from pmFPA's cell number to burntool's notion
    int burntool_cell = (view->cell % 8) * 8 + (view->cell - (view->cell % 8)) / 8;

    psString cellName = psMetadataLookupStr(NULL, readout->parent->concepts, "CELL.NAME");
    (void) cellName;

    CELL *burntoolData = getBurntoolDataForCell(config, options, burntool_cell);
    if (!burntoolData) {
        psError(PS_ERR_UNKNOWN, false, "Unable to find burntool data for cell");
        return false;
    }

    psImage *image = readout->image;

#ifdef DUMP_CELL_IMAGES
    psString dumpPath = NULL;
    psStringAppend(&dumpPath, "cellimages/%s.before.fits", cellName);
    psphotSaveImage(NULL, image, dumpPath);
    psFree(dumpPath);
#endif

    // The following code is adapted from burntool's functions burn_apply() which is in burn_fix.c and 
    // sub_fit() which was located in trailfit.c

    int ny = image->numRows;

    psF32 satVal = psMetadataLookupF32(NULL, readout->parent->concepts, "CELL.SATURATION");

    /* The new burntool tables and method coming into play in 2022 has a slightly different format (an extra column). Need to differentiate */
    psS16 BURNTOOL_STATE_GOOD = psMetadataLookupS16(NULL, config->camera, "BURNTOOL.STATE.GOOD");

    for (int p = 0; p < burntoolData->npersist; p++) {
        OBJBOX *box = &(burntoolData->persist[p]);

        if(box->fiterr) continue;
        if(box->func != BURN_PWR && box->func != BURN_EXP) {
            continue;
        }

        int i, j, k;
        int y0, y1, ys, dy;

        y0 = box->y0;
        if (BURNTOOL_STATE_GOOD >= 15) {
          y1 = box->eyfit;
        } else {
          y1 = (box->up) ? ny-1 : 0;
        }
        dy = (box->up) ? +1 : -1;
        if (BURNTOOL_STATE_GOOD >= 15) {
          ys = (box->up) ? box->sy : box->y0;
        } else {
          ys = (box->up) ? box->y1m : box->y1p;
        }

        /* Calculate all needed y values */
        double ybuf[MAXSIZE];
        for(j=ys; dy*j<=dy*y1; j+=dy) {
            if(box->func == BURN_PWR) {
                if(dy*(j-y0) <= 0) {
                    ybuf[j] = exp(box->slope*log(Y_SCALE*1.0));
                } else {
                    ybuf[j] = exp(box->slope*log(Y_SCALE*dy*(j-y0)));
                }
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
            for(j=ys; dy*j<=dy*y1; j+=dy) {
                psF32 inputPixel = image->data.F32[j][i];
                if (false && inputPixel >= satVal) {
                    // don't modify saturated pixels
                    continue;
                }

#ifndef MATCH_BURNTOOL
                psF32 delta = box->zero[k] * ybuf[j];
                image->data.F32[j][i] -= delta;
#else
                // do math in 16 bit signed int space in order to match how the burntool program
                // behaves with regard to underflows. burntool allows underflows so that the burntool
                // application is reversible.
                // Verified that the image matches the burntool implementation with this code
                // which shows that we are reading and processing the table correctly.
                int delta = box->zero[k] * ybuf[j] + 0.5;
                psS16 pixel;
                // adjust for BZERO
                if (inputPixel >= 65535) {
                    pixel = 32767;
                } else if (inputPixel <= 0) {
                    pixel = -32768;
                } else {
                    pixel = inputPixel - 32768;
                }
                pixel -= delta;
                // replace the pixel with correction applied
                image->data.F32[j][i] = (psF32) pixel + 32768;
#endif
            }
        }
    }
#ifdef DUMP_CELL_IMAGES
    dumpPath = NULL;
    psStringAppend(&dumpPath, "cellimages/%s.after.fits", cellName);
    psphotSaveImage(NULL, image, dumpPath);
    psFree(dumpPath);
#endif

    return true;
}


bool burntoolFileRead = false;
static CELL OTA[MAXCELL]; // cell structure for entire OTA
static void * getBurntoolDataForCell(pmConfig *config, ppImageOptions *options, int burntool_cell)
{
    if (!burntoolFileRead) {
        psString burntoolTablePath = psMetadataLookupStr(NULL, config->arguments, "BURNTOOL.TABLE");
        if (!burntoolTablePath) {
            psError(PS_ERR_UNKNOWN, false, "failed to lookup BURNTOOL.TABLE in arguments");
            return NULL;
        }
        psString burntoolFileName = pmConfigConvertFilename(burntoolTablePath, config, false, false);
        if (!burntoolFileName) {
            psError(PS_ERR_UNKNOWN, false, "failed to convert filename for %s", burntoolTablePath);
            return NULL;
        }

        /* The new burntool tables and method coming into play in 2022 has a slightly different format (an extra column). Need to differentiate */
        psS16 BURNTOOL_STATE_GOOD = psMetadataLookupS16(NULL, config->camera, "BURNTOOL.STATE.GOOD");
        int oldfile = 1; 
        if (BURNTOOL_STATE_GOOD >= 15) oldfile = 0;

        if (persist_read(OTA, burntoolFileName, 1,oldfile)) {
            psError(PS_ERR_UNKNOWN, "true", "failed to read burntool file");
            return NULL;
        }
        psFree(burntoolFileName);
        burntoolFileRead = true;
    }

    return (void *) &OTA[burntool_cell];
}
#define PPIMAGE_BURNTOOL_DEBUG 0

// adapted from ppImageBurntoolMask
bool ppImageBurntoolMaskFromTable(pmConfig *config, ppImageOptions *options, pmFPAview *view, pmReadout *mask)
{
  bool status = true;
  int burntool_cell;

  /* Redirects and Memory juggling. */
  view->readout = 0;
  psImage *image = mask->mask;

  /* Set the maskValue from the recipes. */
  psImageMaskType maskValue = options->burntoolMask;

  /* The new burntool tables and method coming into play in 2022 has a slightly different format (an extra column). Need to differentiate */
  psS16 BURNTOOL_STATE_GOOD = psMetadataLookupS16(NULL, config->camera, "BURNTOOL.STATE.GOOD");

  burntool_cell = (view->cell % 8) * 8 + (view->cell - (view->cell % 8)) / 8;
  CELL *burntoolData = getBurntoolDataForCell(config, options, burntool_cell);
  if (!burntoolData) {
    psError(PS_ERR_UNKNOWN, false, "Unable to find burntool data for cell");
    return false;
  }

#if PPIMAGE_BURNTOOL_DEBUG
  psLogMsg("ppImageBurntoolMask", 4, "Status: %d %d\n",burntoolData->npersist,maskValue);
#endif

  psLogMsg("ppImageBurntoolMask", 4, "Cell mapping: %d %d %d\n",view->cell,burntool_cell,-1);
  for (int row = 0; row < burntoolData->npersist; row++) {
      OBJBOX *box = &(burntoolData->persist[row]);
      if (((options->burntoolTrails & 0x01)&& (box->func == 4))||
          (((options->burntoolTrails & 0x02)&& (box->up == 1))||
           ((options->burntoolTrails & 0x04)&& (box->up == 0)))) {
        /*       If the fit fails, burntool reports zero here.  This
                 signifies that it expected to see a trail (else why
                 fit) but did not find it when it attempted to
                 correct. */
#if PPIMAGE_BURNTOOL_DEBUG
          psLogMsg ("ppImageBurntoolMask", 4, "Masking! %d (%d %d %d) %d %d",
                  burntool_cell,
                  ((options->burntoolTrails & 0x0001)&& (box->nfit == 0)),
                  ((options->burntoolTrails & 0x02)&& (box->up == 1)),
                  ((options->burntoolTrails & 0x04)&& (box->up == 0)),
                  options->burntoolTrails,
                  maskValue
                  );
#endif


        /* do separate masking strategy for old (14 and lower) and new burntool tables*/
        if (BURNTOOL_STATE_GOOD >= 15) {
	 /*printf("DET: New burntool style %d\n",BURNTOOL_STATE_GOOD);*/
          for (int i = box->sxfit; i<= box->exfit; i++) {
              if (box->up == 0) {
                  for (int j = box->eyfit; j <= box->y0; j++) {
                    #if PPIMAGE_BURNTOOL_DEBUG
                    psLogMsg("ppImageBurntoolMask", 4, "Noisy!: %d %d %d %d\n",
                         i,j,image->data.PS_TYPE_IMAGE_MASK_DATA[j][i],maskValue);
                    #endif
                      image->data.PS_TYPE_IMAGE_MASK_DATA[j][i] |= maskValue;
                  }
              } else {
                  for (int j = box->sy; j < box->eyfit ; j++) {
                    #if PPIMAGE_BURNTOOL_DEBUG
                      psLogMsg("ppImageBurntoolMask", 4, "Noisy!: %d %d %d %d\n",
                       i,j,image->data.PS_TYPE_IMAGE_MASK_DATA[j][i],maskValue);
                    #endif
                      image->data.PS_TYPE_IMAGE_MASK_DATA[j][i] |= maskValue;
                  }
              }
          }

        } else {
	/* printf("DET: Old burntool style %d\n",BURNTOOL_STATE_GOOD);*/
          for (int i = box->sxfit; i<= box->exfit; i++) {
              if (box->up == 0) {
                  for (int j = 0; j <= box->y1p; j++) {
                    #if PPIMAGE_BURNTOOL_DEBUG
                    psLogMsg("ppImageBurntoolMask", 4, "Noisy!: %d %d %d %d\n",
                         i,j,image->data.PS_TYPE_IMAGE_MASK_DATA[j][i],maskValue);
                    #endif
                      image->data.PS_TYPE_IMAGE_MASK_DATA[j][i] |= maskValue;
                  }
              } else {
                  for (int j = box->y1m; j < image->numRows ; j++) {
                    #if PPIMAGE_BURNTOOL_DEBUG
                      psLogMsg("ppImageBurntoolMask", 4, "Noisy!: %d %d %d %d\n",
                       i,j,image->data.PS_TYPE_IMAGE_MASK_DATA[j][i],maskValue);
                    #endif
                      image->data.PS_TYPE_IMAGE_MASK_DATA[j][i] |= maskValue;
                  }
              }
          }
        }


      }
  }

  return(status);
}

/* From persistio.c - read and write persistence info */
/****************************************************************/
/* persist_read(): Read all the persistence trails from a file */
STATIC int persist_read(CELL *cell, const char *infile, int apply, int oldfile)
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

      /* read-in of new burntool tables (including the eyfit column)*/
      if(!oldfile) {
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
	     &boxbuf[nbox].fiterr,&boxbuf[nbox].eyfit);

      } else {
        sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %lf %d %d %d %d",
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
	     &boxbuf[nbox].fiterr);
      }

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


#ifdef NOT_NEEDED

#define DIFFERENT_STREAK 20	/* Proximity for union versus intersection */

/****************************************************************/
/* persist_merge(): Disentangle overlapping persistence streaks */
STATIC int persist_merge(CELL *cell)
{
   int i, j, k, n, nbox, xs, xe;
   int jp, kp;
   int lapmax;
   OBJBOX *box;
   int xusage[MAXSIZE], yctr[MAXSIZE], boxid[MAXSIZE];
   double zk, zj;

/* Look at all the boxes -- if not isolated then merge or sever */
   nbox = cell->npersist;
   box = cell->persist;

/* Assess the clustering of all the streaks */
   for(i=0; i<MAXSIZE; i++) xusage[i] = 0;
   for(i=0; i<MAXSIZE; i++) boxid[i] = -1;
   for(k=0; k<nbox; k++) {
      if(box[k].exfit >= MAXSIZE-1) continue;
      for(i=box[k].sxfit; i<=box[k].exfit; i++) xusage[i] += 1;
   }

/* Identify clusters */
   for(xs=0; xs<MAXSIZE-1; xs++) {
      if(xusage[xs] == 0) continue;
      for(xe=xs, lapmax=0; xe<MAXSIZE-1; xe++) {
	 if(xusage[xe+1] == 0) break;
	 lapmax = MAX(lapmax, xusage[xe]);
      }
      if(lapmax == 1) {	/* No overlap?  No problem. */
	 xs = xe + 1;	/* Hop to next gap */
	 continue;
      }

/* Which boxes overlap this cluster? */
      for(k=n=0; k<nbox; k++) {
	 if(box[k].sxfit > xe || box[k].exfit < xs) continue;
	 boxid[n] = k;
	 yctr[n] = (box[k].y0m+box[k].y1m+box[k].y0p+box[k].y0p+2) / 4;
	 n++;
      }

/* Case 1: different y start => sever */
      for(kp=0; kp<n; kp++) {
	 k = boxid[kp];
	 zk = 0.0;
	 if(box[k].nfit > 0) zk = box[k].zero[box[k].nfit/2];
	 for(jp=kp+1; jp<n; jp++) {
	    j = boxid[jp];
	    zj = 0.0;
	    if(box[j].nfit > 0) zj = box[j].zero[box[j].nfit/2];
	    if(ABS(yctr[jp]-yctr[kp]) > DIFFERENT_STREAK) {
/* Trim back the feebler streak */
	       if(zk > zj) {
		  if(box[j].sxfit >= box[k].sxfit) 
		     box[j].sxfit = MAX(box[j].sxfit, box[k].exfit+1);
		  if(box[j].exfit <= box[k].exfit) 
		     box[j].exfit = MIN(box[j].exfit, box[k].sxfit-1);
	       } else {
		  if(box[k].sxfit >= box[j].sxfit) 
		     box[k].sxfit = MAX(box[k].sxfit, box[j].exfit+1);
		  if(box[k].exfit <= box[j].exfit) 
		     box[k].exfit = MIN(box[k].exfit, box[j].sxfit-1);
	       }
	    }
	 }
      }

/* Case 2: pretty much the same y start => union */
      for(kp=0; kp<n; kp++) {
	 k = boxid[kp];
	 for(jp=kp+1; jp<n; jp++) {
	    j = boxid[jp];
	    /* CZW: Since I added the initialization statement above, */
	    /*      any box with boxid == -1 hasn't been set, and */
	    /*      therefore needs to be dropped, I think. */
	    if ((k < 0)||(j < 0)) { continue; }
	    if(ABS(yctr[jp]-yctr[kp]) <= DIFFERENT_STREAK) {
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

	       box[k].exfit = box[k].sxfit - 1;
	       yctr[kp] = -2 * DIFFERENT_STREAK;
	    }
	 }
      }

      xs = xe + 1;	/* Hop to next gap */

   } /* Cluster loop */

/* Excise the boxes which have been eradicated (sxfit > exfit) */
   for(k=0; k<nbox; k++) {
      if(box[k].sxfit > box[k].exfit) {
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
STATIC int persist_write(CELL *cell, const char *outfile, int oldfile)
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

   /*write new burntool tables (including the eyfit column)*/
   if(!oldfile) {
     fprintf(fp, "#Cell time    cx  cy  max   y0   sx  sy   ex  ey  y0m y0p y1m y1p x0m x0p x1m x1p F up    slope nfit sxf exf fiterr eyf\n");
   } else {
     fprintf(fp, "#Cell time    cx  cy  max   y0   sx  sy   ex  ey  y0m y0p y1m y1p x0m x0p x1m x1p F up    slope nfit sxf exf fiterr\n");
   }

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
 
         /*write new burntool tables (including the eyfit column)*/
         if(!oldfile) {
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
         } else {
	   fprintf(fp, "%3d %7d  %3d %3d %5d %3d  %3d %3d  %3d %3d  %3d %3d %3d %3d %3d %3d %3d %3d  %1d %1d %9.6f %3d %3d %3d %d\n",
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
		 cell[j].persist[k].fiterr);
         } 

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
	    }
	 } else {
	    if(cell[j].burn[k].fiterr && 
	       cell[j].burn[k].fiterr != FIT_TOP_ERROR) continue;
	    if(cell[j].burn[k].nfit <= 0) continue;
	 }

	 i = (cell[j].burn[k].ex - cell[j].burn[k].sx + 1) / 2;

         if(!oldfile) {
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
         } else {
	   fprintf(fp, "%3d %7d  %3d %3d %5d %3d  %3d %3d  %3d %3d  %3d %3d %3d %3d %3d %3d %3d %3d  %1d %1d %9.6f %3d %3d %3d %d\n", 
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
		 cell[j].burn[k].fiterr);
         } 

	 for(i=0; i<cell[j].burn[k].nfit; i++) {
	    fprintf(fp, "%3d %3d %8.4f\n", cell[j].burn[k].xfit[i], 
		    cell[j].burn[k].yfit[i], cell[j].burn[k].zero[i]);
	 }
      }
   }
   fclose(fp);

   return(err);
}
#endif /* NOT_NEEDED */
