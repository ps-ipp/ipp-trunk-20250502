/* -*- c-file-style: "Ellemtel" -*- */
/* tiltystreak.c - fit and subtract tilted streaks from a row of cells */
/* Assumes that biastool has already been run -- zero streakiness near bias */
/* Syntax: tiltystreak Uncompressed_GPC1_MEF.fits */
/* Timing is currently 1.7 sec on my laptop for an OTA */

/* 090611 v1.0 John Tonry */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "math.h"
#include "fh/fh.h"
#include "fhreg/general.h"
#include "fhreg/gpc_detector.h"

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define NINT(x) (x<0?(int)((x)-0.5):(int)((x)+0.5))
#define ABS(a) (((a) > 0) ? (a) : -(a))

#define NODATA 0		/* Marker for *No Data* */
#define NCELL 8			/* Number of cells in a row */
#define SAT4SURE 45000		/* Blasted, we don't care about pixels above */
#define NPOLY 3			/* N terms in of flattening polynomial */

//#define DEBUG			/* Turn on debug output? */

/* Info for a cell */
typedef struct cell_info {
      HeaderUnit hu;		/* Header unit for this cell */
      int cell;			/* Cell number */
      int mefxt;		/* Where is it in the MEF? */
      int bias;			/* Bias level */
      int sky;			/* Sky level */
      int rms;			/* RMS in the sky */
      int nsat;			/* Saturated pixel count */
      double ampl;		/* Streak amplitude */
      short *data;		/* Image data */
      int *mask;		/* Mask bits */
} CELL;

/* Prototypes */
static int cell_stats(int sx, int nx, int ny, int NX, int NY, int bzero, 
		      CELL *cell);
static int int_median(int n, int *key);
int qsort_dbl(int n, double *x);
int qsort_int(int n, int *x);
static int streak_ratio(int sx, int nx, int ny, int NX, CELL otarow[]);
static int linear_streak_fit(int sx, int nx, int ny, int NX, CELL otarow[]);
static int bilinear_streak_fit(int sx, int nx, int ny, int NX, CELL otarow[]);
static int parabola_streak_fit(int sx, int nx, int ny, int NX, CELL otarow[]);
int linearfit(int npt, double *y, int nx, double *x, double *param);
static void syntax(const char *prog);

static int nmedian_buf=0;
static int *median_buf=NULL;
static double *Abuf=NULL, *Bbuf=NULL, *Xbuf=NULL;

int main(int argc, const char* argv[])
{
   int i, j, row, col, ext;
   HeaderUnit ihu = fh_create();
   HeaderUnit ehu = NULL;
   const char* ifilename;
   char extname[FH_MAX_STRLEN+1], xtension[FH_MAX_STRLEN+1], cellsought[5];
   int nextend;
   CELL otarow[NCELL];
   int naxis, naxis1=0, naxis2=0;
   int n1, n2;
   int prescan1, ovrscan1, ovrscan2;
   double bzero_d;
   int bzero = 32768, firstime = 1, algo=2;
   int cellmask[NCELL*NCELL];

   if(argc < 2) {
      syntax(argv[0]);
      fprintf(stderr, "error: syntax: tiltystreak Uncompressed_GPC1_MEF.fits\n");
      exit(EXIT_FAILURE);
   }
   ifilename = argv[1];

/* Defaults */
   for(i=0; i<NCELL*NCELL; i++) cellmask[i] = 1;	/* Use all cells */
   algo = 2;			/* Default algorithm is a parabola */

/* Process arguments */
   for(i=2; i<argc; i++) {

/* Veto cells by a 64 digit mask (0/1 for cells 0-63) */
      if(strncmp(argv[i], "mask=", 5) == 0) {	/* mask=64_digits */
	 if(strlen(argv[i]) < 5+NCELL*NCELL) {
	    fprintf(stderr, "\rerror: cannot get mask from `%s'\n", argv[i]);
	    exit(EXIT_FAILURE);
	 } else {
	    for(j=0; j<NCELL*NCELL; j++) cellmask[j] = argv[i][5+j] != '0';
	 }

      } else if(strncmp(argv[2], "alg=", 4) == 0) {	/* Algorithm */
	 if(strcmp(argv[2]+4, "line") == 0) {
	    algo = 1;
	 } else if(strcmp(argv[2]+4, "parabola") == 0) {
	    algo = 2;
	 } else if(strcmp(argv[2]+4, "bent") == 0) {
	    algo = 3;
	 } else {
	    fprintf(stderr, "error: `%s' is not a valid algorithm\n", argv[2]);
	    exit(EXIT_FAILURE);
	 }

      } else if(strncmp(argv[i], "help", 4) == 0) {	/* help output */
	 syntax(argv[0]);
	 exit(EXIT_SUCCESS);

      } else {
	 fprintf(stderr, "\rerror: unrecognized option `%s'\n", argv[i]);
	 syntax(argv[0]);
	 exit(EXIT_FAILURE);
      }
   }


   if (fh_file(ihu, ifilename, FH_FILE_RDWR) != FH_SUCCESS) {
      fprintf(stderr, "error: tiltystreak could not open file `%s'\n",
	      ifilename);
      exit(EXIT_FAILURE);
   }
   nextend = fh_extensions(ihu);
   if (nextend < 1) {
      fprintf(stderr, "error: `%s' is not a multi-extension FITS\n",
	      ifilename);
      exit(EXIT_FAILURE);
   }

/* Loop over all rows of cells */
   for(row=0; row<NCELL; row++) {

/* Loop over all cells and grab the data */
      for(col=0; col<NCELL; col++) {

	 sprintf(cellsought, "xy%1d%1d", col, row);
	 otarow[col].cell = col + NCELL*row;

/* Find the extension holding this cell */
	 for (ext=1; ext<=nextend; ext++) {

	    if (!(ehu = fh_ehu(ihu, ext)) ||
		fh_get_str(ehu, "EXTNAME", extname, sizeof(extname)) != FH_SUCCESS) {
	       fprintf(stderr, "error: Cannot read EXTNAME from `%s' for extension #%d\n",
		       ifilename, ext);
	       exit(EXIT_FAILURE);
	    }
	    if(fh_get_str(ehu, "XTENSION", xtension, sizeof(xtension)) != FH_SUCCESS) {
	       fprintf(stderr,
		       "error: Cannot read XTENSION from `%s' for extension #%d\n",
		       ifilename, ext);
	       exit(EXIT_FAILURE);	 
	    }

	    if(!strcmp("TABLE", xtension)) continue;	/* Skip tables */
	    if(strncmp(extname, cellsought, 4) == 0) break; /* Got the cell */
	 }	/* Cell seek loop */

	 if(ext > nextend) {
	    fprintf(stderr,
		    "error: Could not find extension `%s'\n", cellsought);
	    exit(EXIT_FAILURE);	 
	 }

/* Positioned at the desired cell now */
	 if (fh_get_PRESCAN1(ehu, &prescan1) != FH_SUCCESS ||
	     fh_get_OVRSCAN1(ehu, &ovrscan1) != FH_SUCCESS ||
	     fh_get_OVRSCAN2(ehu, &ovrscan2) != FH_SUCCESS ||
	     fh_get_NAXIS(ehu, &naxis) != FH_SUCCESS ||
	     fh_get_NAXIS1(ehu, &n1) != FH_SUCCESS ||
	     fh_get_NAXIS2(ehu, &n2) != FH_SUCCESS) {
	    fprintf(stderr, "error: Cannot get NAXIS* in extension %d row %d col %d\n", ext, row, col);
	    exit(EXIT_FAILURE);
	 }
/* Cell isn't an image or is 32bpp */
	 if(naxis != 2) {
	    otarow[col].mefxt = -1;
	    continue;
	 }

/* Make some space now that we finally know the size */
	 if(firstime) {
	    naxis1 = n1;
	    naxis2 = n2;
	    firstime = 0;
	    for(i=0; i<NCELL; i++) {
	       otarow[i].data = (short*)calloc(naxis1*naxis2, sizeof(short));
	       otarow[i].mask = (int *)calloc(naxis1*naxis2, sizeof(int));
	    }
	    median_buf = (int *)calloc(2*naxis2, sizeof(int));
	    nmedian_buf = 2*naxis2;
	    Abuf = (double *)calloc(naxis2, sizeof(double));
	    Bbuf = (double *)calloc(naxis2, sizeof(double));
	    Xbuf = (double *)calloc(NPOLY*naxis2, sizeof(double));
	 }
	 else {
	    if(naxis1 != n1 || naxis2 != n2) {
	       fprintf(stderr, "error: All cells must be same size (%dx%d != %dx%d)\n",
		       n1, n2, naxis1, naxis2);
	       exit(EXIT_FAILURE);
	    }
	 }

/* Write down which extension this cell came from */
	 otarow[col].mefxt = ext;
	 otarow[col].hu = ehu;

/* Check BSCALE and abort if it is anything other than 1.0 */
/* This program only handles raw OTA data */
	 {
	    double bscale_d;
	    if (fh_get_BSCALE(ehu, &bscale_d) == FH_SUCCESS && bscale_d != 1.0) {
	       fprintf(stderr, "error: Ignoring BSCALE value of %f!\n", bscale_d);
	       exit(EXIT_FAILURE);
	    }
	 }
/* Trust whatever is in the FITS header for BZERO */
	 if (fh_get_BZERO(ehu, &bzero_d) == FH_SUCCESS) bzero = bzero_d;

/* Read the bits for this cell */
	 if (fh_read_padded_image(ehu, fh_file_desc(ehu), otarow[col].data,
				  naxis1*naxis2*sizeof(short),
				  FH_TYPESIZE_16) != FH_SUCCESS) {
	    fprintf(stderr, "error: failed to read image data for extension `%s'.\n",
		    extname);
	    exit(EXIT_FAILURE);
	 }
      } /* Loop collecting cells in a row */

/* Anything to do? */
      for(col=i=0; col<NCELL; col++) if(otarow[col].mefxt > 0) i++;
      if(i == 0) continue;	/* Nope, entirely wiped out */

/* Process this row of cells */

/* What's the stats in each cell, generate mask */
      for(col=0; col<NCELL; col++) {
	 if(otarow[col].mefxt < 0) continue;	/* Skip non-cell */
	 cell_stats(prescan1, naxis1-prescan1-ovrscan1, naxis2-ovrscan2, 
		    naxis1, naxis2, bzero, otarow+col);
      }

/* Calculate the streaky ratios for each cell */
      streak_ratio(prescan1, naxis1-prescan1-ovrscan1, naxis2-ovrscan2, 
		   naxis1, otarow);

#ifdef DEBUG
      for(col=0; col<NCELL; col++) {
	 printf("%1d %1d %3d %3d 0x%08x %6d %6d %6d %6d %8.3f\n",
		col, row, otarow[col].cell, otarow[col].mefxt, 
		(int)otarow[col].hu,
		otarow[col].bias, otarow[col].sky, otarow[col].rms, 
		otarow[col].nsat, otarow[col].ampl);
      }
#endif

/* 
 * Form a "median streaky" image
 * Do the mean slope fits to this mean image
 * Apply scaled slope corrections to each cell
 */
      if(algo == 1) {
/* 
 * Linear is pretty good, but the errors really tend to be bent
 * lines, so this will tend to be excellent at the bias end (where
 * biastool has already corrected, and excellent at the 1/3 mark
 * across a cell, but crummy at the left and the 2/3 mark
 */
	 linear_streak_fit(prescan1, naxis1-prescan1-ovrscan1, 
			    naxis2-ovrscan2, naxis1, otarow);
/* 
 * Parabola can match a bent line pretty well, so it's mostly really
 * good except sometimes a bit in the center.  I'm not certain whether
 * that is amplitude errors or the bent line not being a parabola.
 * The worry is that parabola might go wild...
 */
      } else if(algo == 2) {
	 parabola_streak_fit(prescan1, naxis1-prescan1-ovrscan1, 
			     naxis2-ovrscan2, naxis1, otarow);
/* 
 * Bent line (called "bilinear") in principle could be really good.  But
 * the technique of chopping out the real stuff by differencing (y - y-1)
 * is susceptible to slow drifts in y.  Linear and parabola have those
 * explicitly fitted and removed, but I've not gotten it done for this
 * three point fit, so there's lots of artifacts left as yet.
 */
      } else if(algo == 3) {
	 bilinear_streak_fit(prescan1, naxis1-prescan1-ovrscan1, 
			     naxis2-ovrscan2, naxis1, otarow);
      }

/* Return to all the cells in the MEF and rewrite the data */
      for(col=0; col<NCELL; col++) {
#ifdef DEBUG
	 printf("Now writing cell %d %d ext %d (hu 0x%08x)\n", 
		col, row, otarow[col].mefxt, (int)otarow[col].hu);
#endif
	 if(otarow[col].mefxt < 0) continue;
/* Skip if cell mask not set */
	 if(cellmask[col+NCELL*row] != 1) continue;
/* This seeks back to the start of data from this cell again... */
	 ehu = otarow[col].hu;
	 fh_ehu(ehu, 0);

#ifdef DEBUG
	 printf("Now writing cell %d %d ext %d (hu 0x%08x)\n", 
		col, row, otarow[col].mefxt, (int)otarow[col].hu);
#endif
	 if (fh_write_padded_image(ehu, fh_file_desc(ehu), otarow[col].data,
				   naxis1*naxis2*sizeof(short),
				   FH_TYPESIZE_16) != FH_SUCCESS) {
	    fprintf(stderr, "error: failed to re-write image data for extension `%s'.\n",
		    extname);
	    exit(EXIT_FAILURE);
	 }
      }

   }	/* Loop over rows */

   fh_destroy(ihu);
   exit(EXIT_SUCCESS);
}

#define Q_CLIP 3.0
#define MASK_CLIP 8
/****************************************************************/
/* cell_stats(): Get bias, sky and noise levels, create mask */
static int cell_stats(int sx, int nx, int ny, int NX, int NY, int bzero, 
		      CELL *cell)
{
   int i, j, k, n, thresh;

/* Get bias stats */
   if(nx == NX) {
      cell->bias = 0;
   } else {
      for(j=1; j<ny-1; j++) 
	 median_buf[j-1] = cell->data[sx+nx+(NX-nx-sx)/2+j*NX] + bzero;
      cell->bias = int_median(ny-2, median_buf);
   }

/* Get sky stats */
   for(k=n=0; k<nx*ny; k+=((617*nx)/1000)) {
      i = k % nx;
      j = k / nx;
      if(cell->data[sx+i+NX*j]+bzero < SAT4SURE &&
	 cell->data[sx+i+NX*j]+bzero != NODATA) {
	 median_buf[n++] = cell->data[sx+i+NX*j] + bzero;
      }
   }

   if(n < 20) {		/* Better have hit at least 20! */
      cell->sky = cell->rms = 0.0;
      return(-1);
   }

/* First pass at sky and quartile */
   cell->sky = int_median(n, median_buf);
   cell->rms = 1.33*(cell->sky - median_buf[n/4]);

/* Clip at 3 sigma */
   for(j=0; median_buf[j] < cell->sky - Q_CLIP*cell->rms && j<n/2; j++);
   for(k=n-1; median_buf[k] > cell->sky + Q_CLIP*cell->rms && k>n/2; k--);
   cell->sky = median_buf[(j+k)/2];
   cell->rms = 1.33 * (cell->sky - median_buf[(3*j+k)/4]);
   cell->sky -= cell->bias;

/* Create mask */
   thresh = cell->bias + cell->sky + MASK_CLIP*cell->rms;
   cell->nsat = 0;
   for(j=0; j<ny; j++) {
      for(i=sx; i<sx+nx; i++) {
	 cell->mask[i+NX*j] = 1;
	 if(cell->data[i+NX*j]+bzero > thresh) cell->mask[i+NX*j] = 0;
	 if(cell->data[i+NX*j]+bzero > SAT4SURE) (cell->nsat)++;
      }
   }

   return(0);
}

static int int_median(int n, int *key)
{
   if(n == 0) return(0);
   qsort_int(n, key);
   return((key[n/2]+key[(n-1)/2]) / 2);
}

/* Determine the relative streakiness of all cells in this row */
/* 
 * Compare each cell's pixels in the difference which are greater
 * than MINSTREAK with the average using pixels surviving the 
 * intersection of all the masks
 */
#define MINSTREAK 50
#define EDGE 3
static int streak_ratio(int sx, int nx, int ny, int NX, CELL otarow[])
{
   int i, j, k, nalive=0, ncontrib;
   double diff;

   for(k=0; k<NCELL; k++) {
      otarow[k].ampl = 0.0;
      if(otarow[k].mefxt > 0) {
	 nalive++;
      } else {
	 otarow[k].ampl = 0.0;
      }
   }

/* Loop over all the pixels */
   ncontrib = 0;
   for(j=1+EDGE; j<ny-EDGE; j++) {
      for(i=sx+EDGE; i<sx+nx-EDGE; i++) {
/* Get the mean of the (y - y-1) difference */
	 diff = 0;
	 for(k=0; k<NCELL; k++) {
	    if(otarow[k].mefxt < 0) continue;
	    if(otarow[k].mask[i+j*NX] == 0 || 
	       otarow[k].mask[i+(j-1)*NX] == 0) {
	       diff = 0;
	       break;
	    }
	    diff += (int)otarow[k].data[i+j*NX] - otarow[k].data[i+(j-1)*NX];
	 }
/* Compare it with each cell */
	 if(ABS(diff > MINSTREAK)) {
	    for(k=0; k<NCELL; k++) {
	       otarow[k].ampl += ((int)otarow[k].data[i+j*NX] - 
		   (int)otarow[k].data[i+(j-1)*NX]) / diff;
	    }
	    ncontrib++;
	 }
      }
   }
   for(k=0; k<NCELL; k++) otarow[k].ampl *= ((double)nalive)/(ncontrib);
   
   return(0);
}

#define YSTART 2

/* Determine the relative streakiness of all cells in this row */
/* Fit just a line to each streak */
static int linear_streak_fit(int sx, int nx, int ny, int NX, CELL otarow[])
{
   int i, j, k, n, medimg[NCELL];
   double s1, sy, A, par[NPOLY];

/* Loop over all rows */
/* Note that we take row 2 (j=1) as our fiducial which needs no correction */
   Abuf[YSTART-1] = 0.0;
   for(j=YSTART; j<ny; j++) {
      s1 = sy = 0.0;
/* Fit a function z = A*(i-nx-sx), assuming biastool already applied */
      for(i=sx+EDGE; i<sx+nx-EDGE; i++) {
	 n = 0;
/* Get the median, weighted streaky at this point */
	 for(k=0; k<NCELL; k++) {
	    if(otarow[k].mefxt < 0) continue;
	    if(otarow[k].mask[i+j*NX] == 0 ||
	       otarow[k].mask[i+(j-1)*NX] == 0) continue;
	    medimg[n] = ((int)otarow[k].data[i+j*NX] - 
			 (int)otarow[k].data[i+(j-1)*NX]) / otarow[k].ampl;
	    n++;
	 }
	 if(n > 0) {
	    k = int_median(n, medimg) + Abuf[j-1] * (i-nx*sx);
	    sy  += k;
	    s1  += i - (sx+nx);
	 }
      }
      Abuf[j] = 0.0;
      if(s1 != 0) Abuf[j] = sy / s1;
      for(k=0, n=1; k<NPOLY; k++, n*=j) Xbuf[k+j*NPOLY] = n;
   }
   
/* Also correct the correction wander by removing a parabola */
   linearfit(ny-YSTART, Abuf, NPOLY, Xbuf, par);

/* Apply the corrections */
   for(j=YSTART; j<ny; j++) {
      A = Abuf[j] - (par[0] + j*par[1] + j*j*par[2]);
#ifdef DEBUG
//      if(otarow[3].mefxt == 4) printf("%4d %9.3f %9.3f\n", j, Abuf[j], A);
#endif
      for(k=0; k<NCELL; k++) {
/* Fix up the lines */
	 for(i=sx; i<sx+nx; i++) {
	    otarow[k].data[i+j*NX] -= otarow[k].ampl * A * (i-nx-sx);
	 }
      }
   }
   return(0);
}

/* Determine the relative streakiness of all cells in this row */
/* Fit just a line to each streak */
static int parabola_streak_fit(int sx, int nx, int ny, int NX, CELL otarow[])
{
   int i, j, k, n, medimg[NCELL], n1, n2, ia, ib;
   double s1, s2, a, b, A, B, Apar[NPOLY], Bpar[NPOLY];

/* Loop over all rows */
/* Note that we take row 2 (j=1) as our fiducial which needs no correction */
   Abuf[YSTART-1] = Bbuf[YSTART-1] = 0.0;
   for(j=YSTART; j<ny; j++) {
      s1 = s2 = 0.0;
      n1 = n2 = 0;
/* Fit a function z = A*(i-nx-sx), assuming biastool already applied */
      for(i=sx+EDGE; i<sx+(2*nx)/3; i++) {
	 n = 0;
/* Get the median, weighted streaky at this point */
	 for(k=0; k<NCELL; k++) {
	    if(otarow[k].mefxt < 0) continue;
	    if(otarow[k].mask[i+j*NX] == 0 ||
	       otarow[k].mask[i+(j-1)*NX] == 0) continue;
	    medimg[n] = ((int)otarow[k].data[i+j*NX] - 
			 (int)otarow[k].data[i+(j-1)*NX]) / otarow[k].ampl;
	    n++;
	 }
	 if(n > 0) {
	    k = int_median(n, medimg);
	    if(i < sx+nx/3) {
	       s1 += k + Abuf[j-1];
	       n1++;
	    } else {
	       s2 += k + Bbuf[j-1];
	       n2++;
	    }
	 }
      }
      Abuf[j] = Bbuf[j] = 0.0;
      if(n1 > 0) Abuf[j] = s1/n1;
      if(n2 > 0) Bbuf[j] = s2/n2;
      for(k=0, n=1; k<NPOLY; k++, n*=j) Xbuf[k+j*NPOLY] = n;
   }
   
/* Correct the correction wander by removing a parabola */
   linearfit(ny-YSTART, Abuf, NPOLY, Xbuf, Apar);
   linearfit(ny-YSTART, Bbuf, NPOLY, Xbuf, Bpar);
   ia = sx + nx/6 - (sx+nx);
   ib = sx + nx/2 - (sx+nx);

/* Apply the corrections */
   for(j=YSTART; j<ny; j++) {
      A = Abuf[j] - (Apar[0] + j*Apar[1] + j*j*Apar[2]);
      B = Bbuf[j] - (Bpar[0] + j*Bpar[1] + j*j*Bpar[2]);
#ifdef DEBUG
//      if(otarow[3].mefxt == 4) printf("%4d %9.3f %9.3f %9.3f %9.3f\n", 
//				      j, Abuf[j], A, Bbuf[j], B);
#endif
/* Parabola passes through 0 at i=sx+nx, A at sx+nx/6, B at sx+nx/2 */
      a = (A/ia-B/ib) / (ia-ib);
      b = A/ia - a*ia;
      
      for(k=0; k<NCELL; k++) {
/* Fix up the lines */
	 for(i=sx; i<sx+nx; i++) {
	    otarow[k].data[i+j*NX] -= otarow[k].ampl * 
	       (a*(i-nx-sx)*(i-nx-sx) + b*(i-nx-sx));
	 }
      }
   }
   return(0);
}

/* NOTE: This no worky very well. No removal of wander makes Jack a dull boy */
/* Determine the relative streakiness of all cells in this row */
/* Fit just a broken line to each streak */
static int bilinear_streak_fit(int sx, int nx, int ny, int NX, CELL otarow[])
{
   int i, j, k, n, medimg[NCELL], npt, i0;
   double s0, s1, sy, s2, sm, A, B, C;

/* Loop over all rows */
/* Note that we take row 2 (j=1) as our fiducial which needs no correction */
   for(j=2; j<ny; j++) {

/* Fit a function z = A*(i-nx-sx) to the right third */
      s1 = sy = A = 0.0;
      for(i=sx+(2*nx)/3; i<sx+nx-EDGE; i++) {
	 n = 0;
/* Get the median, weighted streaky at this point */
	 for(k=0; k<NCELL; k++) {
	    if(otarow[k].mefxt < 0) continue;
	    if(otarow[k].mask[i+j*NX] == 0 ||
	       otarow[k].mask[i+(j-1)*NX] == 0) continue;
	    medimg[n] = ((int)otarow[k].data[i+j*NX] - 
			 (int)otarow[k].data[i+(j-1)*NX]) / otarow[k].ampl;
	    n++;
	 }
	 if(n > 0) {
	    k = int_median(n, medimg);
	    sy  += k;
	    s1  += i - (sx+nx);
	 }
      }
      if(s1 != 0) A = sy / s1;

/* Fit a full line z = B*(i-nx-sx)+C to the left third */
      s0 = s1 = s2 = sm = B = C = 0.0;
      npt = 0;
      for(i=sx+EDGE; i<sx+nx/3; i++) {
	 n = 0;
/* Get the median, weighted streaky at this point */
	 for(k=0; k<NCELL; k++) {
	    if(otarow[k].mefxt < 0) continue;
	    if(otarow[k].mask[i+j*NX] == 0 ||
	       otarow[k].mask[i+(j-1)*NX] == 0) continue;
	    medimg[n] = ((int)otarow[k].data[i+j*NX] - 
			 (int)otarow[k].data[i+(j-1)*NX]) / otarow[k].ampl;
	    n++;
	 }
	 if(n > 0) {
	    k = int_median(n, medimg);
	    s1  += k;
	    s0  += (i-sx-nx);
	    s2 += (i-sx-nx)*(i-sx-nx);
	    sm += (i-sx-nx)*k;
	    npt++;
	 }
      }
      if(npt > 0) {
	 B = (npt*sm-s0*s1) / (npt*s2-s0*s0);
	 C = (s2*s1 - s0*sm) / (npt*s2-s0*s0);
      }

      if(A != B) {
	 i0 = C / (A-B) + sx+nx;
      } else {
	 i0 = 0;
      }

#ifdef DEBUG
//      if(otarow[3].mefxt == 4) printf("%4d %4d %9.3f %9.3f %9.3f\n", 
//				      j, i0, A, B, C);
#endif

/* Correct all the cells */
      for(k=0; k<NCELL; k++) {
	 if(otarow[k].mefxt < 0) continue;
	 for(i=sx; i<sx+nx; i++) {
	    if(i < i0) {
	       otarow[k].data[i+j*NX] -= otarow[k].ampl * B * (i-nx-sx) + C;
	    } else {
	       otarow[k].data[i+j*NX] -= otarow[k].ampl * A * (i-nx-sx);
	    }
	 }
      }
   }
   
   return(0);
}

static void syntax(const char *prog)
{
   printf("Syntax: %s mef_file [options]\n", prog);
   printf("   where options include:\n\n");
   printf(" mask=0101...   64 digits to work on cells 0:63.\n");
   printf(" alg=N          Use fit algorithm 1=linear, 2=parabola (default)\n");   printf("                   3=bent linear (not yet fully implemented)\n");
}
