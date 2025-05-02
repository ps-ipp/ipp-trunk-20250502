/*
 * There are four different algorithms here with different strengths.
 *
 * PSF_MARGIN finds the brightest spot, estimates a crude FWHM,
 *   sums up a marginal profile in x and y, fits a 1D curve to it
 *   (default Gaussian or parabola), and then sums up the total
 *   flux with varying sophistication in estimating sky
 *
 * PSF_MOMENT estimates sky from the median and then simply computes
 *   moments of the brightest pixels to ascertain center and FWHM.  It
 *   iterates once using all pixels within an aperture from the first pass.
 *   PSF_MOMENT's FWHM is actually 2.35 * the RMS, which is neither the 
 *   FWHM of a condensed PSF nor the outer diameter of a donut.  It is 
 *   monotonic in PSF size, so is very appropriate for focussing, especially
 *   grossly out of focus donuts.
 *
 * PSF_2DIM is a rather involved (fortran) routine to fit a full 2-D
 *   profile at an initial position.  The center, FWHM, position angle,
 *   and flux it returns are very refined.  The psf() wrapper limits
 *   some of the underlying features (such as floating data and some
 *   of the fit quality diagnostics).
 *
 * PSF_BIN tries to cope with grossly out of focus data by sucessively
 *   binning by factors of 2, fitting the profile by PSF_MOMENT.  The
 *   results are scored and a best-guess combination is returned.
 *
 *       Ratings: E-G-F-P for excellent - good - fair - poor
 *
 *                        PSF_MARGIN  PSF_MOMENT  PSF_2DIM  PSF_BIN
 *  Speed                     E           E           F        G
 *  Overall robustness        E           G           F        G
 *  Centroid accuracy         E           G           E        G
 *  FWHM accuracy             G           F*          E        F
 *  FWHM of donut             P           E           P        G
 *  Flux/bkgnd accuracy       F           F           E        F
 *  Extra FWHM info           F           G           E        F
 *
 *  PSF_MERGE runs PSF_MOMENT and then if the FWHM is small enough
 *  it also runs PSF_MARGIN.  It merges the x,y,FWHM from both routines
 *  in a smooth way as a function of FWHM; preferring MARGIN for small
 *  FWHM and MOMENT for large.  Obviously it usually costs the CPU time
 *  for running both routines.
 */

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define ABS(a) (((a) > 0) ? (a) : -(a))

/* Enumeration of PSF algorithm choices */
typedef enum {
   PSF_MERGE,			/* Merge results from moment and margin */
   PSF_MARGIN,			/* 1D fit to x,y marginalized profiles */
   PSF_MOMENT,			/* Moment estimates of center, FWHM */
   PSF_BIN,			/* Iterative binning, the PSF_MARGIN */
   PSF_2DIM			/* Full 2D fit of profile */
} PSF_ALGORITHM;

/* Structure describing extra PSF image parameters */
typedef struct {
         int sx; 		/* x offset:  x_real = sx + binx * x_array */
	 int sy;  		/* y offset:  y_real = sy + biny * y_array */
	 int binx;  		/* x bin factor */
	 int biny;   		/* y bin factor */
	 int nodata;   		/* data value meaning *NO DATA* */
	 int NX;		/* stride of image storage: addr = [x+NX*y] */
	 int bordx;  		/* avoidance border width in x */
	 int bordy;   		/* avoidance border width in y */
} PSF_IMPARAM;

/* Structure describing output from psf() */
typedef struct {
         int ix;		/* Highest pixel x posn in array */
	 int iy;		/* Highest pixel y posn in array */
	 double x0;		/* Fitted location of centroid, x */
	 double y0;		/* Fitted location of centroid, y */
	 double fwhm;		/* Star fwhm */
	 double peak;		/* Highest pixel (minus background) */
	 double bkgnd;		/* Background level */
	 double flux;		/* Total flux in star */
	 double sn;		/* Flux signal to noise */
	 double weight;		/* Overall weight of star: */
	    			/*    1-10 so-so, 10-30 OK, >30 fine */
} PSF_PARAM;

/* Structure describing possible extra output from psf() */
typedef struct {
	 int binfactor;		/* Best bin factor used for PSF_BIN */
         double xfw;		/* Star fwhm in x dir */
	 double yfw;		/* Star fwhm in y dir */
/* Results available only from PSF_2DIM */
         double majfw;		/* Major axis fwhm */
	 double minfw;		/* Minor axis fwhm */
	 double thfw;		/* Angle of major axis (CCW from x) [rad] */
	 double wpeak;		/* Peak of Waussian fit */
	 double wbkgnd;		/* Background of Waussian fit */
	 double dflux;		/* Uncertainty in total flux estimate */
	 double dbkgnd;		/* Uncertainty in total background estimate */
	 double rmsbkgnd;	/* RMS in background */
} PSF_EXTRA;

int psf(
   int nx,			/* x size of image */
   int ny,			/* y size of image */
   unsigned short *im,		/* Data array */
   PSF_ALGORITHM alg,		/* Choice of PSF algorithm */
   PSF_IMPARAM *imparam,	/* Extra image parameters */
   PSF_PARAM *psf,		/* Results of fit */
   PSF_EXTRA *extra);		/* Extra results from fit */

/* Prototypes */

/* Basic PSF calculation routine */
int psfmargin_guts(int algo,	/* What algorithm to use? */
    int *ix,		/* Highest pixel x posn*/
    int *iy,		/* Highest pixel y posn */
    int *big,		/* Highest pixel */
    double *x0,		/* Fitted location of centroid, x */
    double *y0,		/* Fitted location of centroid, y */
    double *fwhm,	/* Star fwhm */
    double *xfwhm,	/* Star fwhm in x dir */
    double *yfwhm,	/* Star fwhm in y dir */
    double *sn,		/* Star signal to noise */
    double *sky,	/* Sky level */
    double *flux,	/* Total flux in star */
    int nx,		/* x size of image */
    int ny,		/* y size of image */
    int mx,		/* x size of image storage */
    unsigned short *im);	/* Data array */

/* Find and report Gaussian fit to position of brightest star */
int psfmargin(int sx, 			/* x offset of pixel 0,0 */
	     int sy,  			/* y offset of pixel 0,0 */
	     int binx,  		/* x bin factor */
	     int biny,   		/* y bin factor */
	     int bordx,  		/* x border */
	     int bordy,   		/* y border */
	     int nx,   			/* x size of image */
	     int ny,    		/* y size of image */
	     int NX,   			/* x stride of image */
	     unsigned short *data,	/* ushort image data: 0 = *NO_DATA* */
	     double *xu,		/* Fitted x position */
	     double *yu,		/* Fitted y position */
	     int *fmax,			/* Highest pixel in best bin */
	     double *fwhm,		/* FWHM of psf (binx,y corrected) */
	     double *xfwhm,		/* FWHM in x dir (binx corrected) */
	     double *yfwhm,		/* FWHM in y dir (biny corrected) */
	     double *bkgnd,		/* Sky level */
	     double *ftot,		/* Total flux */
	      double *weight,		/* Composite quality of star: <1 bad,*/
	     				/*    1-10 so-so, 10-30 OK, >30 fine */
	     double *snr);		/* S/N of (big-sky)/noise */

/* Find and report moments of brightest star */
int psfmoment(int sx, 			/* x offset of pixel 0,0 */
	     int sy,  			/* y offset of pixel 0,0 */
	     int binx,  		/* x bin factor */
	     int biny,   		/* y bin factor */
	     int bordx,  		/* x border */
	     int bordy,   		/* y border */
	     int nx,   			/* x size of image */
	     int ny,    		/* y size of image */
	     int NX,   			/* x stride of image */
	     unsigned short *data,	/* ushort image data: 0 = *NO_DATA* */
	     double *xu,		/* Fitted x position */
	     double *yu,		/* Fitted y position */
	     int *fmax,			/* Highest pixel in best bin */
	     double *fwhm,		/* FWHM of psf (binx,y corrected) */
	     double *xfwhm,		/* FWHM in x dir (binx corrected) */
	     double *yfwhm,		/* FWHM in y dir (biny corrected) */
	     double *bkgnd,		/* Sky level */
	     double *ftot,		/* Total flux */
	     double *weight,		/* Composite quality of star: <1 bad,*/
	     				/*    1-10 so-so, 10-30 OK, >30 fine */
	     double *snr,		/* S/N of (big-sky)/noise */
	     int *extra);		/* More return stuff? */

/* Find and report on brightest star via hierarchical binning */
int psfdonut(int sx, 			/* x offset of pixel 0,0 */
	     int sy,  			/* y offset of pixel 0,0 */
	     int binx,  		/* x anamorphic compression factor */
	     int biny,   		/* y anamorphic compression factor */
	     int bordx,  		/* x border */
	     int bordy,   		/* y border */
	     int nx,   			/* x size of image */
	     int ny,    		/* y size of image */
	     int NX,   			/* x stride of image */
	     unsigned short *data,	/* ushort image data: 0 = *NO_DATA* */
	     double *xu,		/* Fitted x position */
	     double *yu,		/* Fitted y position */
	     int *fmax,			/* Highest pixel in best bin */
	     double *fwhm,		/* FWHM of psf (binx,y corrected) */
	     double *xfwhm,		/* FWHM in x dir (binx corrected) */
	     double *yfwhm,		/* FWHM in y dir (biny corrected) */
	     double *bkgnd,		/* Sky level */
	     double *ftot,		/* Total flux */
	     double *weight,		/* Composite quality of star: <1 bad,*/
	     				/*    1-10 so-so, 10-30 OK, >30 fine */
	     double *snr,		/* S/N of (big-sky)/noise */
	     int *binfactor);		/* Best bin factor used */

/* Find and report about brightest star via 2-D fit */
int psf2dim(
   int nx,   			/* x size of image */
   int ny,    			/* y size of image */
   unsigned short *data,	/* ushort image data: 0 = *NO_DATA* */
   PSF_IMPARAM *imparam,	/* Extra image parameters */
   PSF_PARAM *psfout,		/* Results of fit */
   PSF_EXTRA *psfextra);	/* Extra results from fit */

/* The fortran guts, bleah! */
int jtpsf_(
   int *nx,		/* x size */
   int *ny,		/* y size */
   float *data,		/* image */
   float *eadu,		/* e/ADU for noise calc (use 1.0) */
   int *aprad,		/* Aperture radius */
   int *skyrad,		/* Sky radius */
   int *nwpar,		/* Number of fit params (use 7) */
   float *wpar,		/* Fit results (16) */
   float *flux,		/* Flux results (5) */
   int *err);		/* Error */
