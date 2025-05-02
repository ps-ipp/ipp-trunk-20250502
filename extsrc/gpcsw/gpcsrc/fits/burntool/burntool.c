/*
 * burntool.c - identify and remove persistance streaks from a MEF.
 * Syntax: burntool mef_file [options]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "math.h"
#include "fh/fh.h"
#include "fhreg/general.h"
#include "fhreg/gpc_detector.h"

#include "burntool.h"
#define EXTERN  /* Define EXTERN to declare variables in params.h */
#include "burnparams.h"
#include "persist_fits.h"

int
main(int argc, const char* argv[])
{
   int i, j, k, nx, ny, err;
   HeaderUnit ihu = fh_create();
   HeaderUnit ehu;
   const char* ifilename = "-";
   char extname[FH_MAX_STRLEN+1], otaposn[FH_MAX_STRLEN+1], *camera="gpc1";
   int cellmask[MAXCELL];
   int otanum;
   int nextend, cellxy, cell, cellcode;
   int ext, update, restore, apply, tableonly, psfsize, psfavg;
   IMTYPE *buf;
   const char *burnfile=NULL,  *persistfile=NULL, *persistfitsfile=NULL;
   const char *deltablefitsfile=NULL;
   const char *psffile=NULL, *psfstatfile=NULL;
   CELL OTA[MAXCELL];	/* Cell structure for entire OTA */

   if(argc > 1 && strncmp(argv[1], "help", 4) == 0) {
      syntax(argv[0]);
      exit(EXIT_SUCCESS);
   }

   if(argv[1]) ifilename = argv[1];

   if(fh_file(ihu, ifilename, FH_FILE_RDWR) != FH_SUCCESS) {
      fprintf(stderr, "\rerror: burntool could not open file `%s'\n",
	      ifilename);
      exit(-314);
   }
   nextend = fh_extensions(ihu);
   if (nextend < 1) {
      fprintf(stderr, "\rerror: `%s' is not a multi-extension FITS\n",
	      ifilename);
#ifndef JT2DHACK
      exit(-315);
#endif
   }

/* Initialize the globals */
   VERBOSE = 0;			/* Verbosity level */

   median_buf=NULL;		/* Buffer for medians */
   nmedian_buf=0;

   mbuf=NULL;			/* Mask buffer */
   nmbuf=0;

   imbuf=NULL;			/* Image copy buffer */
   nimbuf=0;

   BZERO = 32768;		/* BZERO for input data */
   USHORT_BIAS = 1000;		/* Bias level restored to ushort stamps */

   MAX_READ_NOISE = 20;		/* Maximum believable read noise (ADU) */
   MIN_EADU = 0.3;		/* Minimum believable e/ADU */
   SAT4SURE  = 60000;		/* Ignore pixels above for noise estimate */
   MIN_BLAST_PASS = 0.1;	/* Allow blasted cells if they have BIG satfrac */
   MAX_BLAST_PASS = 0.9;	/* But not if it's all wiped out! */

   BURN_THRESH  = 30000;	/* Threshold for onset of burning */
   TRAIL_THRESH = 10000;	/* Trailing might go this low */
   MAX_THRESH   = 20000;	/* Possibly trailing stars? */
   STAR_THRESH  =  1000;	/* Threshold for star above sky */
   STAR_FRAC =  0.3;		/* Fraction to follow star profile */
   PSF_THRESH   =  5000;	/* Threshold for a star to be a PSF */

   BMASK_GROW =  3.0;		/* Growth of burned boxes in pixels */
   RMASK_GROW =  2.0;		/* Growth of burn/star in diameter */

   MIN_PSF_SIZE  =    3;	/* Min box size for stamp selection */
   PSF_CTR_TOL   =    8;	/* Choose max or box ctr for stamp */
   CONCAT_FITS = 1;		/* Write concat FITS for PSF? (else 3D) */
   MAX_PSF_PER_CELL = 20; 	/* Max number of PSF stars accepted per cell */

   NEGLIGIBLE_TRAIL = 0.4;	/* Don't sweat less than this * sigma */
   EXPIRE_TRAIL_TIME = 2000;	/* Expire a persist after this [sec] */

   PERSIST_RETAIN = 0;		/* Retain persists with bad slopes? */

/* Parse the args */
   cellxy = -1;
   update = 1;		/* Calc fits, apply fits, write img and table */
   restore = 0;		/* Restore previous fit only, write img */
   apply = 0;		/* Apply previous fit only, write img */
   tableonly = 0;	/* Calc fits (apply fits), write table only */
   psfsize = 32;
   psfavg = 0;
   for(i=0; i<MAXCELL; i++) cellmask[i] = 1;
   for(i=2; i<argc; i++) {

/* Work on just one cell?  xy ID mode. */
      if(strncmp(argv[i], "xy=", 3) == 0) {		/* xy=CELL_ID */
	 if(sscanf(argv[i]+3, "%d", &cellxy) != 1) {
	    fprintf(stderr, "\rerror: cannot get cell ID from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

/* Work on just one cell? Cell count [0:63] mode. */
      } else if(strncmp(argv[i], "cell=", 5) == 0) {	/* cell=0:63 */
	 if(sscanf(argv[i]+5, "%d", &cellxy) != 1) {
	    fprintf(stderr, "\rerror: cannot get cell ID from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }
	 cellxy = cellxy/8 + 10*(cellxy%8);

/* Veto cells by a 64 digit mask (0/1 for cells 0-63) */
      } else if(strncmp(argv[i], "mask=", 5) == 0) {	/* mask=64_digits */
	 if(strlen(argv[i]) < 5+MAXCELL) {
	    fprintf(stderr, "\rerror: cannot get mask from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 } else {
	    for(j=0; j<MAXCELL; j++) cellmask[j] = argv[i][5+j] != '0';
	 }

/* Modify the input MEF by subtracting fits? */
      } else if(strncmp(argv[i], "update=", 7) == 0) {	/* update={t|f} */
	 update = argv[i][7] == 'y' || argv[i][7] == '1' || argv[i][7] == 't';

/* Modify the input MEF by adding back fits? */
      } else if(strncmp(argv[i], "restore=", 8) == 0) {	/* restore={t|f} */
	 restore = argv[i][8] == 'y' || argv[i][8] == '1' || argv[i][8] == 't';

/* Modify the input MEF by subtracting previously calculated fits? */
      } else if(strncmp(argv[i], "apply=", 6) == 0) {	/* apply={t|f} */
	 apply = argv[i][6] == 'y' || argv[i][6] == '1' || argv[i][6] == 't';

/* Calculate and write tables only? */
      } else if(strncmp(argv[i], "tableonly=", 10) == 0) {/* tableonly={t|f} */
	 tableonly = argv[i][10] == 'y' || argv[i][10] == '1' || argv[i][10] == 't';

/* Output file for burn streaks */
      } else if(strncmp(argv[i], "trailout=", 9) == 0) {/* trailout=fname */
	 burnfile = argv[i] + 9;

/* Output text file for burn streaks */
      } else if(strncmp(argv[i], "out=", 4) == 0) {	/* out=fname */
	 burnfile = argv[i] + 4;

/* Input text file for previous burn persistence streaks */
      } else if(strncmp(argv[i], "trailin=", 8) == 0) {/* trailin=fname */
	 persistfile = argv[i] + 8;

/* Same thing, but information is stored in tables in a FITS file. */
      } else if(strncmp(argv[i], "trailinfits=", 12) == 0) { /* trailinfits=fname */
	 persistfitsfile = argv[i] + 12;

/* Input file for previous burn persistence streaks */
      } else if(strncmp(argv[i], "in=", 3) == 0) {	/* in=fname */
	 persistfile = argv[i] + 3;

/* Same thing, but information is stored in tables in a FITS file. */
      } else if(strncmp(argv[i], "infits=", 7) == 0) { /* infits=fname */
	 persistfitsfile = argv[i] + 7;
	 
/* Input camera keyword (optional) */
      } else if(strncmp(argv[i], "camera=", 7) == 0) {	/* camera=fname */
	 camera = argv[i] + 7;

/* Keep persistence streaks which had a bad slope? */
      } else if(strncmp(argv[i], "persist=", 8) == 0) {/* persist={t|f} */
	 PERSIST_RETAIN = argv[i][8] == 'y' || argv[i][8] == '1' || argv[i][8] == 't';

/* Output file for PSF gallery */
      } else if(strncmp(argv[i], "psf=", 4) == 0) {	/* psf=fname */
	 psffile = argv[i] + 4;

/* Output file for PSF stats on each cell */
      } else if(strncmp(argv[i], "psfstat=", 8) == 0) {	/* psfstat=fname */
	 psfstatfile = argv[i] + 8;

/* How many cells to average PSF stats over? (2^N) */
      } else if(strncmp(argv[i], "psfavg=", 7) == 0) {	/* psfavg=N */
	 if(sscanf(argv[i]+7, "%d", &psfavg) != 1) {
	    fprintf(stderr, "\rerror: cannot get psf avg from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

/* How big a box to use for PSF extraction? */
      } else if(strncmp(argv[i], "psfsize=", 8) == 0) {	/* psfsize=size */
	 if(sscanf(argv[i]+8, "%d", &psfsize) != 1) {
	    fprintf(stderr, "\rerror: cannot get psf size from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

/* How big a box to use for PSF extraction? */
      } else if(strncmp(argv[i], "psfctr=", 7) == 0) {	/* psfsize=size */
	 if(sscanf(argv[i]+7, "%d", &PSF_CTR_TOL) != 1) {
	    fprintf(stderr, "\rerror: cannot get psf decenter from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

/* How big must a box be to qualify for PSF extraction? */
      } else if(strncmp(argv[i], "psfmin=", 7) == 0) {	/* psfmin=size */
	 if(sscanf(argv[i]+7, "%d", &MIN_PSF_SIZE) != 1) {
	    fprintf(stderr, "\rerror: cannot get psf size from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

/* How big must a box be to qualify for PSF extraction? */
      } else if(strncmp(argv[i], "psfmaxn=", 8) == 0) {	/* psfmaxn=N */
	 if(sscanf(argv[i]+8, "%d", &MAX_PSF_PER_CELL) != 1) {
	    fprintf(stderr, "\rerror: cannot get psf limit from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

/* Thresholds */
      } else if(strncmp(argv[i], "thrburn=", 8) == 0) {	/* thrburn=thresh */
	 if(sscanf(argv[i]+8, "%d", &BURN_THRESH) != 1) {
	    fprintf(stderr, "\rerror: cannot get value from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

      } else if(strncmp(argv[i], "thrtrail=", 9) == 0) {/* thrtrail=thresh */
	 if(sscanf(argv[i]+9, "%d", &TRAIL_THRESH) != 1) {
	    fprintf(stderr, "\rerror: cannot get value from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

      } else if(strncmp(argv[i], "thrmax=", 7) == 0) {	/* thrmax=thresh */
	 if(sscanf(argv[i]+7, "%d", &MAX_THRESH) != 1) {
	    fprintf(stderr, "\rerror: cannot get value from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

      } else if(strncmp(argv[i], "thrstar=", 8) == 0) {	/* thrstar=thresh */
	 if(sscanf(argv[i]+8, "%d", &STAR_THRESH) != 1) {
	    fprintf(stderr, "\rerror: cannot get value from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

      } else if(strncmp(argv[i], "thrpsf=", 7) == 0) {	/* thrpsf=thresh */
	 if(sscanf(argv[i]+7, "%d", &PSF_THRESH) != 1) {
	    fprintf(stderr, "\rerror: cannot get value from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

      } else if(strncmp(argv[i], "expire=", 7) == 0) {	/* expire=nsec */
	 if(sscanf(argv[i]+7, "%d", &EXPIRE_TRAIL_TIME) != 1) {
	    fprintf(stderr, "\rerror: cannot get value from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

/* Star skirt fraction */
      } else if(strncmp(argv[i], "fracstar=", 9) == 0) {/* fracstar=skirt */
	 if(sscanf(argv[i]+9, "%lf", &STAR_FRAC) != 1) {
	    fprintf(stderr, "\rerror: cannot get value from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

/* Mask growth fraction in diameter */
///* Mask growth fraction in x direction */
//      } else if(strncmp(argv[i], "xmask=", 6) == 0) {	/* xmask=factor */
//	 if(sscanf(argv[i]+6, "%lf", &XMASK_GROW) != 1) {
      } else if(strncmp(argv[i], "rmask=", 6) == 0) {	/* rmask=factor */
	 if(sscanf(argv[i]+6, "%lf", &RMASK_GROW) != 1) {
	    fprintf(stderr, "\rerror: cannot get value from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

///* Mask growth fraction in y direction */
//      } else if(strncmp(argv[i], "ymask=", 6) == 0) {	/* ymask=factor */
//	 if(sscanf(argv[i]+6, "%lf", &YMASK_GROW) != 1) {
/* Mask growth fraction in box size */
      } else if(strncmp(argv[i], "bmask=", 6) == 0) {	/* bmask=factor */
	 if(sscanf(argv[i]+6, "%lf", &BMASK_GROW) != 1) {
	    fprintf(stderr, "\rerror: cannot get value from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

/* Concatenated FITS? */
      } else if(strncmp(argv[i], "psf3dfits=", 10) == 0) {/* concat FITS? */
	 CONCAT_FITS = argv[i][10] == 'n' || argv[i][10] == '0' || 
	    argv[i][10] == 'f';

/* Set the e/ADU for the noise gate that cells must pass */
      } else if(strncmp(argv[i], "EADU=", 5) == 0) {	/* EADU=e */
	 if(sscanf(argv[i]+5, "%lf", &MIN_EADU) != 1) {
	    fprintf(stderr, "\rerror: cannot get e/ADU size from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

/* Set the read noise for the noise gate that cells must pass */
      } else if(strncmp(argv[i], "RN=", 3) == 0) {	/* EADU=e */
	 if(sscanf(argv[i]+3, "%d", &MAX_READ_NOISE) != 1) {
	    fprintf(stderr, "\rerror: cannot get read noise size from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

/* Quiet? */
      } else if(strncmp(argv[i], "quiet=", 6) == 0) {	/* quiet={t|f} */
	 if(argv[i][6] == 'y' || argv[i][6] == '1' || argv[i][6] == 't') {
	    VERBOSE = 0;
	 } else {
	    VERBOSE = 1;
	 }

/* Verbosity level? */
      } else if(strncmp(argv[i], "verbose=", 8) == 0) {	/* verbose=N */
	 if(sscanf(argv[i]+8, "%i", &VERBOSE) != 1) {
	    fprintf(stderr, "\rerror: cannot get value from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 }

      } else if(strncmp(argv[i], "deltables=", 8) == 0) { /* trailin=fname */
	 deltablefitsfile = argv[i] + 10;        

      } else if(strncmp(argv[i], "help", 4) == 0) {	/* help output */
	 syntax(argv[0]);
	 exit(EXIT_SUCCESS);

      } else {
	 fprintf(stderr, "\rerror: unrecognized option `%s'\n", argv[i]);
	 syntax(argv[0]);
	 exit(EXIT_FAILURE);
      }
   }

   /* If we're told to remove the table from a FITS file, then
    * do nothing else. */
   if(deltablefitsfile) {
     fprintf(stderr, 
             "\rRemoving burn tables from %s, all other options ignored.\n",
             ifilename);
     if(persist_fits_remove_tables(ihu, deltablefitsfile) != FH_SUCCESS) {
       exit(-316);
     } else {
       exit(EXIT_SUCCESS);
     }
   }
   
   /* If there is no other persistence info supplied, try getting
    * it from the input FITS file. */
   if((restore || apply) && persistfile == NULL) {
      if(persistfitsfile == NULL) persistfitsfile = ifilename;
   }

/* Inititialize cell structure */
   for(cell=0; cell<MAXCELL; cell++) {
      OTA[cell].cell = cell;
      OTA[cell].nburn = OTA[cell].nstar = OTA[cell].npersist = 0;
      OTA[cell].bias = OTA[cell].sky = OTA[cell].rms = OTA[cell].time = 0;
   }

/* Read the persistence data for this OTA */
   if(persistfile != NULL) {			/* Text data file */
      if(persist_read(OTA, persistfile, apply)) exit(-317);
   } else if(persistfitsfile != NULL) {		/* FITS table */
      if(persist_fits_read(OTA, persistfitsfile, apply) != FH_SUCCESS)
	 exit(-318);
   }

/* Which OTA is this??? */
   if(fh_get_str(ihu, "FPPOS", otaposn, sizeof(otaposn)) != FH_SUCCESS || 
      strlen(otaposn) != 4) {
      fprintf(stderr, "logonly: unrecognized fppos\n");
      otanum = -1;

   } else {
      otanum = (otaposn[2] - '0') + 8*(otaposn[3] - '0');
   }

   /* Warn that what you're doing might not be a good idea. */
   {
     fh_bool burn_applied;
     
     if(fh_get_bool(ihu, PHU_NAME_BURN_APPLIED, 
                    &burn_applied) != FH_SUCCESS) {
	if(VERBOSE > 0) {
	   fprintf(stderr, 
		   "warning: Unable to determine whether burn correction "
		   "already applied - unable to find %s in primary header\n",
		   PHU_NAME_BURN_APPLIED);
	}
	burn_applied = FH_FALSE;
     }
     if(restore && (burn_applied == FH_FALSE)) {
       fprintf(stderr, 
               "warning: Restoring old burns, but header indicates no burns previously corrected.\n");             
     }
     else if ((update||apply) && (burn_applied == FH_TRUE)) {
       fprintf(stderr, 
               "warning: Applying burn correction, but header indicates burns previously corrected.\n");             
       }
   }

/* Look at all the MEF's extensions */
#ifndef JT2DHACK
   for (ext = 1; ext <= nextend; ext++) {
#else
   for (ext = 1; ext <= MAX(1,nextend); ext++) {
#endif
      int naxis, naxis1, naxis2, naxis3;
      int prescan1, ovrscan1, ovrscan2, pontime;
      double bzero_d;
      char xtension[FH_MAX_STRLEN + 1];

#ifdef JT2DHACK
      if(nextend == 0) {
	 ehu = ihu;
	 sprintf(extname, "%s", "xy00");
      } else {
#endif
      if (!(ehu = fh_ehu(ihu, ext)) || 
	  fh_get_str(ehu, "EXTNAME", extname, sizeof(extname)) != FH_SUCCESS) {
	 fprintf(stderr,
                 "\rerror: Cannot read EXTNAME from `%s' for extension #%d\n",
                 ifilename, ext);
         exit(-319);
      }
      if(fh_get_str(ehu, "XTENSION", xtension, sizeof(xtension)) != FH_SUCCESS) {
	 fprintf(stderr,
		 "\rerror: Cannot read XTENSION from `%s' for extension #%d\n",
		 ifilename, ext);
	 exit(-320);
      }
      if(!strcmp("TABLE", xtension)) {
	 if(VERBOSE > 0) {
	    fprintf(stderr,
		    "logonly: burntool skipping table extension %s\n", extname); 
	 }
	 continue;
      }
      if ((extname[0] != 'x' || extname[1] != 'y') &&
          (extname[0] != 'x' || extname[2] != 'y')) {
         fprintf(stderr,
                 "warning: Skipping non-image extension %s\n", extname);
#ifdef JT2DHACK
      }
#endif
/*
 * %%% When reading from a pipe, does the data still need
 * to get read in?
 */
         continue;
      }
      cellcode = -1;
      sscanf(extname+2, "%d", &cellcode);
      cell = (cellcode / 10) + 8*(cellcode % 10);
      if(cell < 0 || cell > MAXCELL-1) {
         fprintf(stderr,
                 "\rerror: Illegal cell number %d from '%s'\n", cell, extname);
	 exit(-321);
      }

      if((cellxy >= 0 && cellcode != cellxy) || !cellmask[cell]) {
	 if(VERBOSE > 0) {
	    fprintf(stderr,
		    "logonly: Skipping non-requested extension %s\n", extname);
	 }
	 continue;
      }

/* Get PONTIME as a counter for any burn's persistence */
      if(fh_get_int(ehu, "PONTIME", &pontime) != FH_SUCCESS) {
	 fprintf(stderr, "logonly: cannot read PONTIME\n");
      }
      OTA[cell].time = pontime;

      naxis3 = 1;
      if (fh_get_PRESCAN1(ehu, &prescan1) != FH_SUCCESS ||
	  fh_get_OVRSCAN1(ehu, &ovrscan1) != FH_SUCCESS ||
	  fh_get_OVRSCAN2(ehu, &ovrscan2) != FH_SUCCESS ||
	  fh_get_NAXIS(ehu, &naxis) != FH_SUCCESS ||
          fh_get_NAXIS1(ehu, &naxis1) != FH_SUCCESS ||
          fh_get_NAXIS2(ehu, &naxis2) != FH_SUCCESS ||
          (naxis >= 3 && fh_get_NAXIS3(ehu, &naxis3) != FH_SUCCESS)) {
         fprintf(stderr, "\rerror: Cannot get NAXIS*'\n");
         exit(-322);
      }
      if (naxis != 2) {

/* If this extension looks like video, just skip over it
 * with a little mention that it was ignored and allow all 
 * of the other extensions to be processed. This will get 
 * faked out if we ever provide a MEF with a 2-frame video
 * extension in it, but that should be pretty unlikely. */
	 if((naxis == 3) && (naxis3 > 2)) {
	    if(VERBOSE > 0) {
	       fprintf(stderr, "logonly: burntool skipping 3D, %d frame (video?) extension %s.\n", naxis3, extname);
	    }
	    continue;
	 }
	 fprintf(stderr, "\rerror: 32bpp support not yet implemented in burntool.\n");
	 exit(-324);
      }
/* Check BSCALE and warn if it is anything other than 1.0.
 * This program does not handle applying a BSCALE, nor
 * does it handle anything other than 16-bit input at the
 * moment.  (It is intended only to manipulate raw OTA data. */
      {
	 double bscale_d;
	 if (fh_get_BSCALE(ehu, &bscale_d) == FH_SUCCESS &&
	     bscale_d != 1.0)
	 {
	    fprintf(stderr, "warning: Ignoring BSCALE value of %f!\n", bscale_d);
	 }
      }
/* Trust whatever is in the FITS header for BZERO. */
      if (fh_get_BZERO(ehu, &bzero_d) == FH_SUCCESS) BZERO = bzero_d;

      if(VERBOSE & VERB_NORM) {
	 printf("nx=%d ny=%d prex=%d postx=%d posty=%d BZERO=%d\n", 
		naxis1, naxis2, prescan1, ovrscan1, ovrscan2, BZERO);
      }

      if( (buf = (IMTYPE*)malloc(naxis1*naxis2*naxis3*sizeof(short))) == NULL) {
	 fprintf(stderr, "\rerror: failed to alloc FITS buffer\n");
	 exit(-325);
      }
      if (fh_read_padded_image(ehu, fh_file_desc(ehu), buf,
			       naxis1*naxis2*naxis3*sizeof(short),
			       FH_TYPESIZE_16) != FH_SUCCESS) {
	 fprintf(stderr, "\rerror: failed to read image data for extension `%s'.\n",
		 extname);
//	 free(buf);
	 exit(-326);
      }

      {

	 nx = naxis1 - ovrscan1;
	 ny = naxis2 - ovrscan2;

/* Check for space... */
	 mem_init(nx, ny, naxis1, naxis2);

/* Copy this cell's data */
	 for(k=0; k<naxis1*naxis2; k++) imbuf[k] = buf[k] + BZERO;

/* Get bias and sky levels */
	 err = cell_stats(nx, ny, naxis1, naxis2, imbuf, OTA+cell);
	 if(err) {
	    if(VERBOSE > 0) {
	       fprintf(stderr, "logonly: error getting bias/sky/rms for cell %d, skipping...\n", cell);
	    }
	    continue;
	 }

/* Does this cell get a pass because it's heavily blasted? */
/* Turn it in to a relaxation factor for the "read noise" */
	 i = OTA[cell].satfrac > MIN_BLAST_PASS &&
	     OTA[cell].satfrac < MAX_BLAST_PASS &&
	     OTA[cell].sky < TRAIL_THRESH ? 30 : 1;

/* Does this cell look kosher? */
//	 if(OTA[cell].rms*OTA[cell].rms > 
//	    OTA[cell].sky/MIN_EADU + i*i*MAX_READ_NOISE*MAX_READ_NOISE) {
//	    if(VERBOSE > 0) {
//	       fprintf(stderr, "logonly: cell %d is unreasonably noisy, skipping...\n", cell);
//	       fprintf(stderr, "logonly: bias = %d sky = %d rms = %d satfrac = %.2f\n", 
//		       OTA[cell].bias, OTA[cell].sky, OTA[cell].rms, OTA[cell].satfrac);
//	    }
//	    continue;
//	 }

	 if(apply) {
/* Use the table-driven fits instead of calculating new ones */
	    burn_apply(naxis1-ovrscan1, naxis2-ovrscan2, naxis1, 
		       buf, OTA+cell,camera);
/* Tell us about it? */
	    if(VERBOSE & VERB_NORM) burn_blab(OTA+cell);

	 } else if(restore) {
/* Restore the old burns */
	    burn_restore(naxis1-ovrscan1, naxis2-ovrscan2, naxis1, 
			 buf, OTA+cell,camera);

	 } else {
/* Fix up the burns */
	    burn_fix(naxis1-ovrscan1, naxis2-ovrscan2, naxis1, naxis2, buf, 
		     OTA+cell, cell,camera);

/* Collect up a good star list */
	    if(psffile != NULL || psfstatfile != NULL) {
	       psf_select(naxis1-ovrscan1, naxis2-ovrscan2, naxis1, 
			  mbuf, imbuf, OTA[cell].nstar, OTA[cell].star, 
			  psfsize, OTA[cell].sky+OTA[cell].bias);
	    }

/* Tell us about it? */
	    if(VERBOSE & VERB_NORM) burn_blab(OTA+cell);

	    persist_fix(naxis1-ovrscan1, naxis2-ovrscan2, naxis1, buf, 
			   OTA+cell,camera);
/* Tell us about it? */
	    if(VERBOSE & VERB_NORM) persist_blab(OTA+cell);

	 }

/* Write the corrected data back to the FITS. */
	 if(!tableonly && update) {
	    fh_ehu(ehu, 0);	/* Seek back to the start of data */
	    if (fh_write_padded_image(ehu, fh_file_desc(ehu), buf,
				      naxis1*naxis2*naxis3*sizeof(short),
				      FH_TYPESIZE_16) != FH_SUCCESS) {
	       fprintf(stderr, "\rerror: failed to re-write image data for extension `%s'.\n",
		       extname);
//	       free(buf);
	       exit(-327);
	    }
	 }
      }

      free(buf);
   }

/* Dump out the postage stamp file */
   if(psffile != NULL && !restore && !apply) {
      psf_write(psfsize, psfsize, OTA, otanum, psffile);
   }

/* Dump out the PSF stats */
   if(psfstatfile != NULL && !restore && !apply) {
      psf_write_stats(psfsize, psfsize, OTA, otanum, psfstatfile, psfavg);
   }

/* Write burn info to FITS file. */
   if(update) persist_fits_write(OTA, ihu);
   
   if(restore || tableonly) {    
     /* Indicate in the header that the burns are not applied. */
      fh_set_bool(ihu, FH_AUTO, PHU_NAME_BURN_APPLIED, 
		  FH_FALSE, PHU_COMMENT_BURN_APPLIED);
      fh_rewrite(ihu);
   } else if(apply) {
     /* Indicate in the header that the burns have been applied. */
      fh_set_bool(ihu, FH_AUTO, PHU_NAME_BURN_APPLIED, 
		  FH_TRUE, PHU_COMMENT_BURN_APPLIED);
      fh_rewrite(ihu);
   }

   fh_destroy(ihu);

/* Write the persistence data for the next image */
   if(burnfile != NULL) persist_write(OTA, burnfile);


   exit(EXIT_SUCCESS);
}

/****************************************************************/
/* mem_init(): Allocate space for various functions */
STATIC int mem_init(int nx, int ny, int NX, int NY)
{
/* Make some space for medians */
/*   if(2*SKY_MARG*ny > nmedian_buf) {
      if(median_buf != NULL) free(median_buf);
      if( (median_buf = (int *)calloc(2*SKY_MARG*ny, sizeof(int))) == NULL) {
	 fprintf(stderr, "\rerror: failed to alloc median buffer\n");
	 exit(-667);
      }

      nmedian_buf = 2*SKY_MARG*ny;
   }
*/

/* Make some space for cell copy */
   if(nx*ny > nmedian_buf) {
      if(median_buf != NULL) free(median_buf);
      if( (median_buf = (int *)calloc(nx*ny, sizeof(DTYPE))) == NULL) {
       fprintf(stderr, "\rerror: failed to alloc cell copy\n");
       exit(-668);
      }
      nmedian_buf = nx*ny;
   }

/* Make some space for cell copy */
   if(NX*NY > nimbuf) {
      if(imbuf != NULL) free(imbuf);
      if( (imbuf = (int *)calloc(NX*NY, sizeof(DTYPE))) == NULL) {
	 fprintf(stderr, "\rerror: failed to alloc cell copy\n");
	 exit(-668);
      }
      nimbuf = NX*NY;
   }

/* Make some space for masks */
   if(NX*NY > nmbuf) {
      if(mbuf != NULL) free(mbuf);
      if( (mbuf = (int *)calloc(NX*NY, sizeof(MTYPE))) == NULL) {
	 fprintf(stderr, "\rerror: failed to alloc mask buffer\n");
	 exit(-669);
      }
      nmbuf = NX*NY;
   }

/* Make some space for star veto masks */
   if(NX*NY > nmsbuf) {
      if(msbuf != NULL) free(msbuf);
      if( (msbuf = (int *)calloc(NX*NY, sizeof(MTYPE))) == NULL) {
	 fprintf(stderr, "\rerror: failed to alloc veto buffer\n");
	 exit(-670);
      }
      nmsbuf = NX*NY;
   }
   return(0);
}


#define Q_CLIP 3.0
/****************************************************************/
/* cell_stats(): Get bias, sky and noise levels */
STATIC int cell_stats(int nx, int ny, int NX, int NY, DTYPE *data,
		      CELL *cell)
{
   int i, j, k, n, nsat;

/* Get bias stats */
   if(nx == NX) {
      cell->bias = 0;
   } else {
      for(j=1; j<ny-1; j++) median_buf[j-1] = data[nx+(NX-nx)/2+j*NX];
      cell->bias = int_median(ny-2, median_buf);
   }

/* Get sky stats */
   for(k=n=nsat=0; k<nx*ny; k+=((617*nx)/1000)) {
//   for(k=n=nsat=0; k<nx*ny; k+=((100*nx)/1000)) {
//   for(k=n=nsat=0; k<nx*ny; k++) {
      i = k % nx;
      j = k / nx;
      if(data[i+NX*j] > SAT4SURE) {
	 nsat++;
      } else if(data[i+NX*j] != NODATA) {
	 median_buf[n++] = data[i+NX*j];
      }
   }

   if(n < 20) {		/* Better have hit at least 20! */
      cell->sky = cell->rms = 0.0;
      return(-1);
   }
   cell->satfrac = ((double)nsat) / n;

/* First pass at sky and quartile */
   cell->sky = int_median(n, median_buf);
   cell->rms = 1.33*(cell->sky - median_buf[n/4]);

/* Clip at 3 sigma */
   for(j=0; median_buf[j] < cell->sky - Q_CLIP*cell->rms && j<n/2; j++);
   for(k=n-1; median_buf[k] > cell->sky + Q_CLIP*cell->rms && k>n/2; k--);
//   printf("%5d %5d %5d %5d %5d", cell->sky, cell->rms, n, j, k);
   cell->sky = median_buf[(j+k)/2];
   cell->rms = 1.33 * (cell->sky - median_buf[(3*j+k)/4]);
//   printf("%5d %5d\n", cell->sky, cell->rms);
   cell->sky -= cell->bias;

   return(0);
}


STATIC void syntax(const char *prog)
{
   printf("Syntax: %s mef_file [options]\n", prog);
   printf("   where options include:\n");
   printf(" xy=xy          Work on just one cell?  xy ID mode.\n");
   printf(" cell=N         Work on just one cell? Cell count [0:63] mode.\n");
   printf(" mask=0101...   64 digits to work on cells 0:63.\n");
   printf(" update={t|f}   Modify the input MEF writing table and subtracting fits?\n");
   printf(" restore={t|f}  Restore the input MEF by adding input fits?\n");
   printf(" apply={t|f}    Modify the input MEF by subtracting previously calculated fits?\n");
   printf(" tableonly={t|f} Calculate fits but do *not* modify the input MEF pixels, only write tables\n");
   printf(" in=fname       Input file for previous burn persistence streaks\n");
   printf(" infits=fname   Input FITS file for previous burn persistence streaks (stored\n");
   printf("                   in table extensions).  If both this and the 'in' input\n");
   printf("                   file option are specified, then 'in' takes precedence.\n");
   printf(" out=fname      Output file for burn streaks\n");

//   printf(" trailin=fname  Input file for previous burn persistence streaks\n");
//   printf(" trailinfits=fname  Input FITS file for previous burn persistence streaks\n");
//   printf(" trailout=fname Output file for burn streaks\n");
   printf(" deltables=fname Copy mef_file to new FITS file 'fname' with burn streak\n");
   printf("                   tables removed. NOTE: if specified, all other options\n");
   printf("                   will be ignored!\n");
   printf(" psf=fname      Output file for PSF FITS stamp gallery\n");
   printf(" psfstat=fname  Output file for PSF statistics listing\n");
   printf(" psfavg=N       List PSF statistics averaged over 2^N cells (N=0:3)\n");
   printf(" psfsize=size   How big a box to use for PSF extraction?\n");
   printf(" psfmin=size    How big must a box be to use for PSF extraction?\n");
   printf(" psfmaxn=N      Max number of PSF stars per cell\n");
   printf(" psfctr=N       Max distance for max centering\n");
   printf(" psf3dfits={t|f} Write 3D FITS for PSF instead of Concat (default)?\n");
   printf(" thrburn=X      Threshold for onset of burning\n");
   printf(" thrtrail=X     Trailing might go this low\n");
   printf(" thrmax=X       Possibly trailing stars?\n");
   printf(" thrstar=X      Threshold for star above sky\n");
   printf(" fracstar=X     Fraction of peak to follow star profile\n");
   printf(" thrpsf=X       Threshold for a star to be a PSF\n");
   printf(" rmask=X        Diameter growth factor of burned spots\n");
   printf(" bmask=X        Box size growth of burn/star boxes\n");
   printf(" expire=N       Retire a blasted burn after N seconds\n");
   printf(" quiet={t|f}    Quiet?\n");
   printf(" verbose=N      Set verbosity bits:\n");
   printf("    0x0001         Normal, verbose output\n");
   printf("    0x0002         Dump detection process\n");
   printf("    0x0004         Dump out PSF selection process\n");
   printf("    0x0008         Dump fit progress\n");
   printf("    0x0010         Dump fit profiles\n");
   printf("    0x0020         Write Vista marker procedure to /tmp/markem.pro\n");
   printf("    0x0040         Dump box growth diagnostics\n");
   printf("    0x0080         Write mask in place of corrected image\n");
}
