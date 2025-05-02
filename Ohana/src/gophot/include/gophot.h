/* includes */

# include <ohana.h>
# include <gfitsio.h>

typedef char bool;

# define TRUE (1)
# define FALSE (0)
# define SIGN(X)  (((X) == 0) ? 0 : ((fabs((double)(X))) / (X)))
# define ROUND(X) ((int) ((X) + 0.5*SIGN(X)))
# define SQ(X)    (double) (((double)(X))*((double)(X)))
# define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
# define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
# define SWAP(X,Y) {double tmp=(X); (X) = (Y); (Y) = tmp;}

/* constants */

# define NRMAX 1024 /* max Ny, deprecated */
# define NCMAX 1024 /* max Nx, deprecated */

# define CHIPAR 0.9 /* used by chisq.f */
# define NSMAX 100000 /* max number of stars */
# define NPMAX 8    /* max number of parameters */
# define NPAR  8    /* max number of parameters used */
# define NPSKY 8    /* max sky fit parameters */
# define NSKYFIT 3  /* max sky fit parameters used */
# define NFF   20   /* max number of files / flags */
# define NAPPLE 5   /* number of aper data somethings? */
# define NAPMAX 30  /* number of correction apertures in file? */
# define NFIT0  2   /* N par in fit 0 */
# define NFIT1  4   /* N par in fit 1 */
# define NFIT2  7   /* N par in fit 2 */
# define NFIT3  8   /* N par in fit 3 */

# define MAXFIL 5000 /* max size of subraster vector */
# define NMASK 17    /* max mask size */
# define MAGIC HUGE_VAL  /* sentinel for bad pixels */
/* # define MAGICSET 2e30 old value for sentinel */

# define ADD +1
# define SUB -1

/* global variables */

/* int   lverb;     * verbosity - no longer global */
float chipar;       /* unknown chisq scale factor */
float ufactor;      /* star scaling factor */

/* float b[2*NPMAX];   * two-star fit array */
/* float fb[2*NPMAX];  * two-star fit error array */

bool test7; 
bool needit;        /* deprecated? */

int xs[MAXFIL], ys[MAXFIL], nrect[3];        /* subraster vectors */
float zs[MAXFIL], dzs[MAXFIL];   /* subraster vectors */
float ts[MAXFIL];

float starmask[NMASK][NMASK];
float parms[NPMAX];

/* float a[NPMAX], fa[NPMAX], c[NPMAX][NPMAX];  fit param arrays */
float chiimp, apertime, filltime, addtime;   /* deprecated? */

float sum0, sum1, sum2, maxval, xmax, ymax, xmax2, ymax2;  /* crude star statistics */
int   npt;

float chi[5]; /* almost deprecated, but still in galaxy & shape */

/* tuneup parameters */

enum {NONE1, PGAUSS};
enum {NONE2, PLANE, HUBBLE, MEDIAN};
enum {NONE3, COMPLETE, INCOMPLETE, INTERNAL, OLDSTYLE};

char flags[NFF];
char files[64][NFF];
bool fixpos;

float  skyguess, tmin, tmax, tfac;
float  fac, xpnd, ctpersat, widobl, cmax;
float  stograt, discrim, sig[4], arect[3];
float  chicrit, xtra, crit7, snlim, bumpcrit, sn2cos;
float  enuff4, enuff7;
float  eperdn, rnoise;
float  acc[NPMAX], parlim[NPMAX], ava[NPMAX];
float  beta4, beta64;
float  pixthresh;
float  apmagmaxerr;
float  nphsub, nphob, apmax, apskymin, apskymax, aperrmax;

int irect[3], krect[3], ibot, itop, nit, grect[3];
int icrit, ixby2, iyby2;
int n0left, n0right, nthpix, nbadleft, nbadright, nbadtop, nbadbot;
int jhxwid, jhywid, mprec, napertures;
      
/* image data */
float *big, *noise;
int nfast, nslow;   /* NAXIS1, NAXIS2 of image */

/* star data */
float starpar[NSMAX][NPMAX];
float galpar[NSMAX][NPMAX];
float shadow[NSMAX][NPMAX];
float shaderr[NSMAX][NPMAX];
float apple[NSMAX][NAPPLE];
int   imtype[NSMAX];
int   nstot;
float thresh;
float probgal[NSMAX]; /* deprecated */
float rchisq[NSMAX]; /* deprecated */
bool  fixxy;

int nregion;
float region[100][8];

/* sky data */
float skypar[NPSKY];

/* image data */
Header header;
Matrix matrix;

float (*onestar)(int, int, float *, float *);
float (*twostar)(int, int, float *, float *);
float (*skyfun)(int, int, float *, float *);

# include "prototypes.h"
