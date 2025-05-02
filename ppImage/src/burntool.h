#ifndef _INCLUDED_burntool_
#define _INCLUDED_burntool_

/* Burn correction routines:
   -------------------------  
 *  1. Identify burned spots from saturation threshold
 ?  2. Identify burns and trails morphologically?
 ?    2.1 Look for flat-topped stars
 ?    2.2 Look for asymmetric, smooth, declining trails?
 ?    2.3 Use y overscan?
 ?    2.4 Bin in x?
 *  3. Mask stars and other cruft?
 *  4. Fit up and down row functions
 *  5. Read burn time list
 *  6. Write updated burn time list
 *  8. Undo subtraction
    7. Create and save burn correction FITS table
*/
/* Algorithm:
   ----------
    1. Find new burned spots
    2. Create new burn entries: x1,x2,y,time=now
    3. For each burn:
        if t = now  detrail up
              else  deburn down
        if fit negligible  delete burn 
                     else  save burn fits
 */
/* ToDo:
   -----
 *  clean up fit start points, esp for persistence
 *  change burn= and persist= to trailin= and trailout=
 *  trim edges of fits
 *  implement MASK_SAT
 >  test for garbage fits (positive slope, etc)
 *  test nearly saturated stars for burn.
 *  undo subtraction (almost perfect -- occasional pixels and baddies)
 *  star gather into 3D FITS
 *  test on donuts
 *  stamp extraction center on box, not max
 *  Add back nominal bias; output as ushort   
 *  man page... ongoing...
 *  Scale uses peak/sum?
 *  Change psf coords to center; save in header not pixels
 x  better trail function?  A tuned ln(x+A) is better, but too hard.
 *  why various restorations imperfect?  edge of cell?
 *  cell mask argument
... why the honker star not ID'ed as a burn?  various other gotchas...
 *  age away old persistence streaks; merge persistence streaks
 *  PSF info on median psf
 *  clean up sky/rms calculation with 5*quartile clipping
 *  change VERBOSE to a bitmask for access to various portions of the code
 *  add access to a lot of the tuning variables...
 *  update man page
    tuning, tuning, tuning...
    add correction FITS table to MEF
*/

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define NINT(x) (x<0?(int)((x)-0.5):(int)((x)+0.5))
#define ABS(a) (((a) > 0) ? (a) : -(a))

// #define STATIC static	/* To make function declarations static */
#define STATIC			/* To make function declarations global */

#define MAXCELL	64		/* Max cells in an OTA */
#define MAXSIZE 2048		/* Maximum vertical cell size */

#define STAR_RADIUS  4		/* Radius over which a star ctr must be max */
#define SKY_MARG 3		/* Horiz offset from burn area for sky */
#define FIT_EDGE 5		/* How far beyond saturation to start fit? */
#define Y_SCALE 0.01		/* Scale factor for y in fits */

typedef signed short int IMTYPE;/* Data type of image */

typedef int DTYPE;		/* Data type of data copy */
#define NODATA 0		/* Marker for *No Data */

/* Mask codes: keep in order of severity for psf_select()! */
typedef int MTYPE;		/* Data type of mask */
#define MASK_NONE      0	/* Unmasked pixel (MUST be 0) */
#define MASK_STAR_HALO 1	/* Extended halo box from star */
#define MASK_SAT_HALO  2	/* Extended halo box from grow_mask() */
#define MASK_CTR       3	/* Center mask from star_detect() */
#define MASK_SAT       4	/* Center mask of saturated pixels */

/* Enumeration of function choices */
#define FUNC_NONE 0		/* No associated function */
#define BURN_PWR  1		/* Power law */
#define BURN_EXP  2		/* Exponential */
#define BURN_BLASTED 3		/* Blasted top to bottom: flag only for IPP */
#define BURN_POSSLOPE 4		/* Positive slope fit (bad) but significant */
#define PSF_STAR  9		/* Unfitted: good psf star */

/* Fit error codes */
#define FIT_ERROR  1		/* linearfit failed */
#define FIT_TOP_ERROR  2	/* Saturation extends to top: no points */
#define FIT_SLOPE_ERROR  3	/* Unreasonable fit */
#define FIT_ALL_GONE  4		/* No column survived as significant */
#define FIT_EXPIRED 9		/* Don't carry any more as persistent */

/* Fit parameters */
#define FIT_MIN_SLOPE -10.0	/* minimum slope which is a credible fit */
#define FIT_MAX_SLOPE   0.0	/* maximum slope which is a credible fit */

/* Verbosity bits */
#define VISTAMARKER "/tmp/markem.pro"	/* Write Vista pro to mark stars?*/
#define VERB_NORM     0x0001	/* Normal, verbose output */
#define VERB_DETECT   0x0002	/* Dump detection process */
#define VERB_PSFSEL   0x0004	/* Dump out PSF selection process */
#define VERB_FIT      0x0008	/* Dump fit progress */
#define VERB_FITPROF  0x0010	/* Dump fit profiles */
#define VERB_VISTA    0x0020	/* Write vista markers as /tmp/markem.pro */
#define VERB_BOXGROW  0x0040	/* Dump box growth diagnostics */
#define VERB_MASK     0x0080	/* Write mask in place of corrected image */

/* Description of a potentially burned trail */
typedef struct obj_box {
      int cell;		/* what cell is this one in? */
      int time;		/* PON time when it was created */
      int sx;		/* left corner */
      int sy;		/* bottom corner */
      int ex;		/* right corner */
      int ey;		/* top corner */
      int cx;		/* center x (position of max) */
      int cy;		/* center y (position of max) */
      int max;		/* max data value (above sky) */
      int y0m;		/* min y value at sx */
      int y0p;		/* max y value at sx */
      int y1m;		/* min y value at ex */
      int y1p;		/* max y value at ex */
      int x0m;		/* min x value at sy */
      int x0p;		/* max x value at sy */
      int x1m;		/* min x value at ey */
      int x1p;		/* max x value at ey */
      int sat;		/* saturated (i.e. bigger than BURNTHRESH)? */
      int func;		/* what are we going to do about it? */
      int diff;		/* median diff of up minus down (sum for stars) */
      int up;		/* does it trail up or down? (stamp sx for stars) */
      int y0;		/* y origin for the fit (stamp sy for stars) */
      int burned;	/* do we think it's left a burn trail? */
      int fiterr;	/* error fitting the trail? */
      int sxfit;	/* starting column for fits */
      int exfit;	/* ending column for fits */
      int eyfit;        /* y-coord ending column for fits */
      int nfit;		/* how many columns were corrected? */
      IMTYPE *stamp;	/* postage stamp of this object */
      int *xfit;	/* x of each value of the start of correction */
      int *yfit;	/* y value of the start of correction */
      double slope;	/* slope of fit (stamp sum/max) */
      double *zero;	/* zero of fit for each of the columns */
} OBJBOX;

/* Info for an entire cell */
typedef struct cell_info {
      int cell;			/* Cell number */
      int bias;			/* Bias level */
      int sky;			/* Sky level */
      int rms;			/* RMS in the sky */
      int time;			/* PON time of this cell */
      double satfrac;		/* Fraction of SAT4SURE saturated pixels */
      int nburn;		/* Number of trails left by sat stars */
      OBJBOX *burn;		/* Stars which we think left a trail */
      int npersist;		/* Number of old persistence streaks */
      OBJBOX *persist;		/* Persistent streaks */
      int nstar;		/* Number of stars */
      OBJBOX *star;		/* Stars, harmless we think */
} CELL;

/* Prototypes */
STATIC int mem_init(int nx, int ny, int NX, int NY);
STATIC int burn_fix(int nx, int ny, int stride, int NY, IMTYPE *buf, 
		    CELL *cell, int cellnum);
STATIC int burn_test(int nx, int ny, int NX, DTYPE *data, int rms,
		     MTYPE *mask, OBJBOX *box);
STATIC int burn_restore(int nx, int ny, int NX, IMTYPE *buf, CELL *cell);
STATIC int burn_apply(int nx, int ny, int NX, IMTYPE *buf, CELL *cell);
STATIC int persist_read(CELL *cell, const char *infile, int apply, int oldfile);
STATIC int persist_write(CELL *cell, const char *outfile, int oldfile);
STATIC int persist_fix(int nx, int ny, int stride, IMTYPE *buf, CELL *cell);
STATIC int persist_merge(CELL *cell);

//fh_result persist_fits_read(CELL *cell, const char *filename, int apply);
//fh_result persist_fits_write(CELL *cell, HeaderUnit phu);
//fh_result persist_fits_remove_tables(HeaderUnit phu_in, const char *fileout);

STATIC int star_detect(int nx, int ny, int NX, int NY, DTYPE *data,
		       MTYPE *mask, MTYPE *veto, CELL *cell, int cellnum);
STATIC int burn_check(int nx, int ny, int stride, int NY, DTYPE *buf,
		       MTYPE *mask, CELL *cell);

STATIC int cell_stats(int nx, int ny, int NX, int NY, DTYPE *data, CELL *cell);
STATIC int burn_blab(CELL *cell);
STATIC int persist_blab(CELL *cell);
STATIC int vista_marker(CELL *cell, char *fname);

STATIC int fit_trail(int nx, int ny, int NX, DTYPE *data, MTYPE *mask, 
		    OBJBOX *box, int up, int sky, int rms, int fitfunc);
STATIC int sub_fit(int nx, int ny, int NX, IMTYPE *buf, OBJBOX *box, int sign);

STATIC int psf_select(int nx, int ny, int NX, MTYPE *mask, DTYPE *buf,
		      int nbox, OBJBOX *box, int size, int sky);
STATIC int psf_write(int nx, int ny, CELL *OTA, int otanum, const char *psffile);
STATIC int psf_write_stats(int nx, int ny, CELL *OTA, int otanum, const char *statfile, int psfavg);
STATIC int psf_stats(int nx, int ny, IMTYPE *data, int bias, 
		     double *fwhm, double *q);

STATIC int grow_box(int nx, int ny, int NX, DTYPE *data, int thr, OBJBOX *box);
STATIC int grow_mask(int nx, int ny, int NX, DTYPE *mask, double xfac, 
		     double yfac, int maskval, int nbox, OBJBOX *box);
STATIC int local_max(int i, int j, int r, int nx, int ny, int NX, DTYPE *data);

STATIC int wlinearfit(int npt, double *x, double *y, 
		      double *w, double *a, double *b);
STATIC int linearrms(int npt, double *x, double *y, double a, double b, double *rms);
STATIC int int_median(int n, int *key);
STATIC double double_median(int n, double *key);

STATIC int qsort_int(int n, int *key);
STATIC double qsort_dbl(int n, double *key);

STATIC void syntax(const char *prog);

int write_2dfits(int nx, int ny, int sx, int sy, IMTYPE *data, int fd);
int write_3dhdr(int nx, int ny, int nz, int ncmt, char *cmt[], int fd);
int write_2ddata(int nx, int ny, int *ntot, IMTYPE *data, int fd);
int write_3dend(int *ntot, int fd);

#endif /* _INCLUDED_burntool_ */
