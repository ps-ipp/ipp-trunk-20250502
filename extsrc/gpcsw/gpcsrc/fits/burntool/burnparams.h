/* A global buffer into which routines can accumulate detections */
#ifndef _INCLUDED_burnparams_
#define _INCLUDED_burnparams_

/* '#define EXTERN' in just one file, burntool.c, to declare variables */
#ifndef EXTERN
#define EXTERN extern
#endif

#define MAXBURN 10000
EXTERN OBJBOX boxbuf[MAXBURN];

EXTERN int *median_buf;		/* Generic buffer for integer medians */
EXTERN int nmedian_buf;

EXTERN MTYPE *mbuf;		/* Buffer for object mask */
EXTERN int nmbuf;

EXTERN MTYPE *msbuf;		/* Buffer for vetoed star mask */
EXTERN int nmsbuf;

EXTERN DTYPE *imbuf;		/* Copy of cell data w/o BZERO */
EXTERN int nimbuf;

EXTERN int VERBOSE;		/* Verbosity level */

EXTERN int BZERO;
EXTERN int USHORT_BIAS;		/* Bias level restored to ushort stamps */

EXTERN int MAX_READ_NOISE;	/* Maximum believable read noise (ADU) */
EXTERN double MIN_EADU;		/* Minimum believable e/ADU */
EXTERN int SAT4SURE;		/* Ignore pixels above for noise estimate */
EXTERN double MIN_BLAST_PASS;	/* Allow blasted cells if they have BIG satfrac */
EXTERN double MAX_BLAST_PASS;	/* But not if it's all wiped out! */

EXTERN int BURN_THRESH  ;	/* Threshold for onset of burning */
EXTERN int TRAIL_THRESH ;	/* Trailing might go this low */
EXTERN int MAX_THRESH   ;	/* Possibly trailing stars? */
EXTERN int STAR_THRESH  ;	/* Threshold for star above sky */
EXTERN double STAR_FRAC ;	/* Fraction to follow star profile */
EXTERN int PSF_THRESH   ;	/* Threshold for a star to be a PSF */

//EXTERN double XMASK_GROW ;	/* Growth of burned boxes in x dir */
//EXTERN double YMASK_GROW ;	/* Growth of burn/star in y dir */

EXTERN double BMASK_GROW ;	/* Growth of burned boxes in size */
EXTERN double RMASK_GROW ;	/* Growth of burn/star in diameter */

EXTERN int MIN_PSF_SIZE  ;	/* Min box size for stamp selection */
EXTERN int PSF_CTR_TOL   ;	/* Choose max or box ctr for stamp */
EXTERN int CONCAT_FITS;		/* Write concat FITS for PSF? (else 3D) */
EXTERN int MAX_PSF_PER_CELL ; 	/* Max number of PSF stars accepted per cell */

EXTERN double NEGLIGIBLE_TRAIL;	/* Don't sweat less than this * sigma */
EXTERN int EXPIRE_TRAIL_TIME;	/* Expire trails after this interval */

EXTERN int PERSIST_RETAIN;	/* Retain bad-slope persistence fits */

#endif /* _INCLUDED_burnparams_ */
