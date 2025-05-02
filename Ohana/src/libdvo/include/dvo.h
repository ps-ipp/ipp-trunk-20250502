# ifndef DVO_H
# define DVO_H

# include <ohana.h>
# include <gfitsio.h>
# include <autocode.h>

# include <libdvo_astro.h>
# include <dvodb.h>

/* DVO table modes */
typedef enum {DVO_MODE_UNDEF = 0, DVO_MODE_RAW, DVO_MODE_MEF, DVO_MODE_SPLIT, DVO_MODE_MYSQL} DVOCatMode;

/* DVO table modes */
typedef enum {DVO_COMPRESS_NONE = 0, DVO_COMPRESS_NONE_1, DVO_COMPRESS_NONE_2, DVO_COMPRESS_AUTO, DVO_COMPRESS_AUTO_1, DVO_COMPRESS_AUTO_2, DVO_COMPRESS_GZIP_1, DVO_COMPRESS_GZIP_2, DVO_COMPRESS_RICE_1} DVOCatCompress; // 

/* DVO table formats */
typedef enum {
  DVO_FORMAT_UNDEF = 0, 
  DVO_FORMAT_INTERNAL, 
  DVO_FORMAT_ELIXIR, 
  DVO_FORMAT_LONEOS, 
  DVO_FORMAT_PANSTARRS_DEV_0,
  DVO_FORMAT_PANSTARRS_DEV_1,
  DVO_FORMAT_PS1_DEV_1,
  DVO_FORMAT_PS1_DEV_2,
  DVO_FORMAT_PS1_DEV_3,
  DVO_FORMAT_PS1_REF,
  DVO_FORMAT_PS1_REF_V2,
  DVO_FORMAT_PS1_REF_V3,
  DVO_FORMAT_PS1_SIM,
  DVO_FORMAT_PS1_V1,
  DVO_FORMAT_PS1_V2,
  DVO_FORMAT_PS1_V3,
  DVO_FORMAT_PS1_V4,
  DVO_FORMAT_PS1_V5,
  DVO_FORMAT_PS1_V6,
  DVO_FORMAT_PS1_V5_LOAD,
} DVOCatFormat;

typedef enum {DVO_CAT_OPEN_FAIL, DVO_CAT_OPEN_OK, DVO_CAT_OPEN_EMPTY} DVOCatalogOpenModes;

/* catalog values to be loaded */
typedef enum {
  DVO_LOAD_NONE     = 0x0000,
  DVO_LOAD_AVERAGE  = 0x0001,
  DVO_LOAD_MEASURE  = 0x0002,
  DVO_LOAD_MISSING  = 0x0004,
  DVO_LOAD_SECFILT  = 0x0008, 
  DVO_SKIP_AVERAGE  = 0x0010,
  DVO_SKIP_MEASURE  = 0x0020,
  DVO_SKIP_MISSING  = 0x0040,
  DVO_SKIP_SECFILT  = 0x0080,
  DVO_LOAD_LENSING  = 0x0100,
  DVO_LOAD_LENSOBJ  = 0x0200,
  DVO_SKIP_LENSING  = 0x0400,
  DVO_SKIP_LENSOBJ  = 0x0800,
  DVO_LOAD_STARPAR  = 0x1000,
  DVO_SKIP_STARPAR  = 0x2000,
  DVO_LOAD_GALPHOT = 0x4000,
  DVO_SKIP_GALPHOT = 0x8000,
} DVOCatFlags;

/* image data modes in RegImage */
typedef enum {T_UNDEF = -1, T_NONE, T_OBJECT, T_DARK, T_BIAS, T_FLAT, T_MASK, T_FRINGE, T_SCATTER, T_MODES, T_FRINGEPTS, T_ANY, N_TYPE} ElixirDetrendTypes;
typedef enum {M_UNDEF = -1, M_NONE, M_MEF, M_SPLIT, M_SINGLE, M_CUBE, M_SLICE, M_MODES, N_MODE} ElixirDetrendModes;

// these are used as NAN for types of int values
typedef enum {
  NAN_S_CHAR  = 0x7f,
  NAN_U_CHAR  = 0xff,   // was NO_ERR
  NAN_S_SHORT = 0x7fff, // was NO_MAG
  NAN_U_SHORT = 0xffff, 
  NAN_S_INT   = 0x7fffffff,
  NAN_U_INT   = (signed int) 0xffffffff,
} DVO_INT_NAN;

// init all data, or just catalog data
typedef enum {
  SECFILT_RESET_CHIP  = 0x01,
  SECFILT_RESET_WARP  = 0x02,
  SECFILT_RESET_STACK = 0x04,
  SECFILT_RESET_ALL   = 0x07,
} SecFiltInitMode;
  
// max path length
# define DVO_MAX_PATH 1024

/* RegImage.flag values */
# define IMREG_DIST  0x01 /* image distributed, only imregister-3.0 */

/* photometry code types */
// # define PHOT_PRI 0x01
typedef enum {
  PHOT_SEC  = 0x02,
  PHOT_DEP  = 0x03,
  PHOT_REF  = 0x04,
  PHOT_ALT  = 0x05,  /* never stored, only for look-ups */
  PHOT_MAG  = 0x06,  /* generic magnitude; never stored */
} DVOPhotCodeTypes;

/* Image.code values -- these values are 32 bit (as of PS1_V1) */
typedef enum {
  ID_IMAGE_NEW            = 0x00000000,  /* no calibrations yet attempted */
  ID_IMAGE_PHOTOM_NOCAL   = 0x00000001,  /* user-set value used within relphot: ignore */
  ID_IMAGE_PHOTOM_POOR    = 0x00000002,  /* relphot says image is bad (dMcal > limit) */
  ID_IMAGE_PHOTOM_SKIP    = 0x00000004,  /* user-set value: assert that this image has bad photometry */
  ID_IMAGE_PHOTOM_FEW     = 0x00000008,  /* currently too few measurements for photometry */
  ID_IMAGE_ASTROM_NOCAL   = 0x00000010,  /* user-set value used within relastro: ignore */
  ID_IMAGE_ASTROM_POOR    = 0x00000020,  /* relastro says image is bad (dR,dD > limit) */
  ID_IMAGE_ASTROM_FAIL    = 0x00000040,  /* relastro fit diverged, fit not applied */
  ID_IMAGE_ASTROM_SKIP    = 0x00000080,  /* user-set value: assert that this image has bad astrometry */
  ID_IMAGE_ASTROM_FEW     = 0x00000100,  /* currently too few measurements for astrometry */
  ID_IMAGE_PHOTOM_UBERCAL = 0x00000200,  /* externally-supplied photometry zero point from ubercal analysis */
  ID_IMAGE_ASTROM_GMM     = 0x00000400,  /* image was fitted to positions corrected by the galaxy motion model */
  ID_IMAGE_MOSAIC_POOR    = 0x00000800,  /* relphot says night is bad (Mcal or dMcal > limit) */
  ID_IMAGE_NIGHT_POOR     = 0x00001000,  /* relphot says mosaic is bad (Mcal or dMcal > limit) */
  ID_IMAGE_TGROUP_PHOTCAL = 0x00002000,  /* zero point fitted for tgroup */
  ID_IMAGE_MOSAIC_PHOTCAL = 0x00004000,  /* zero point fitted for mosaic */
  ID_IMAGE_IMAGE_PHOTCAL  = 0x00008000,  /* zero point fitted for image */
} DVOImageFlags;

/* Measure.flags values -- these values are 32 bit (as of PS1_V1) */
typedef enum {
  ID_MEAS_NOCAL            = 0x00000001,  // detection ignored for this analysis (photcode, time range) -- internal only 
  ID_MEAS_POOR_PHOTOM      = 0x00000002,  // detection is photometry outlier (DEPRECATED)
  ID_MEAS_SKIP_PHOTOM      = 0x00000004,  // detection was ignored for photometry measurement (DEPRECATED)
  ID_MEAS_AREA             = 0x00000008,  // detection near image edge -- internal only 
  ID_MEAS_POOR_ASTROM      = 0x00000010,  // detection is astrometry outlier					     	  
  ID_MEAS_SKIP_ASTROM      = 0x00000020,  // detection was ignored for astrometry measurement			     	  
  ID_MEAS_USED_OBJ         = 0x00000040,  // detection was used during update objects  
  ID_MEAS_USED_CHIP        = 0x00000080,  // detection was used during update chips (XXX this probably does not make it into the db)
  ID_MEAS_BLEND_MEAS       = 0x00000100,  // detection is within radius of multiple objects 
  ID_MEAS_BLEND_OBJ        = 0x00000200,  // multiple detections within radius of object 
  ID_MEAS_WARP_USED        = 0x00000400,  // measurement used to find mean warp photometry
  ID_MEAS_UNMASKED_ASTRO   = 0x00000800,  // measurement was not masked in final astrometry fit
  ID_MEAS_BLEND_MEAS_X     = 0x00001000,  // detection is within radius of multiple objects across catalogs		     
  ID_MEAS_ARTIFACT         = 0x00002000,  // detection is thought to be non-astronomical				     
  ID_MEAS_SYNTH_MAG        = 0x00004000,  // magnitude is synthetic
  ID_MEAS_PHOTOM_UBERCAL   = 0x00008000,  // externally-supplied zero point from ubercal analysis
  ID_MEAS_STACK_PRIMARY    = 0x00010000,  // this stack measurement is in the primary skycell
  ID_MEAS_STACK_PHOT_SRC   = 0x00020000,  // this measurement supplied the stack photometry
  ID_MEAS_ICRF_QSO         = 0x00040000,  // this measurement is an ICRF reference position
  ID_MEAS_IMAGE_EPOCH      = 0x00080000,  // this measurement is registered to the image epoch (not tied to ref catalog epoch)
  ID_MEAS_PHOTOM_PSF       = 0x00100000,  // this measurement is used for the mean psf mag
  ID_MEAS_PHOTOM_APER      = 0x00200000,  // this measurement is used for the mean ap mag
  ID_MEAS_PHOTOM_KRON      = 0x00400000,  // this measurement is used for the mean kron mag
  ID_MEAS_MASKED_PSF       = 0x01000000,  // this measurement is masked based on IRLS weights for mean psf mag
  ID_MEAS_MASKED_APER      = 0x02000000,  // this measurement is masked based on IRLS weights for mean ap mag
  ID_MEAS_MASKED_KRON      = 0x04000000,  // this measurement is masked based on IRLS weights for mean kron mag
  ID_MEAS_OBJECT_HAS_2MASS = 0x10000000,  // measurement comes from an object with 2mass data
  ID_MEAS_OBJECT_HAS_GAIA  = 0x20000000,  // measurement comes from an object with gaia data
  ID_MEAS_OBJECT_HAS_TYCHO = 0x40000000,  // measurement comes from an object with tycho data
} DVOMeasureFlags;

typedef enum {
  ID_GALPHOT_FAIL_FIT       = 0x00000001, // fit failed to converge or was degenerate
  ID_GALPHOT_TOO_FEW        = 0x00000002, // not enough points to fit the model
  ID_GALPHOT_OUT_OF_RANGE   = 0x00000004, // fit minimum too far outside data range
  ID_GALPHOT_BAD_ERROR      = 0x00000008, // invalid error (nan or inf)
} DVOGalphotFlags;

// XXX we used these names previously in markstar: replace with ID_MEAS_ARTIFACT
// # define ID_MEAS_TRAIL        0x2000
// # define ID_MEAS_GHOST        0x4000

/* some subtle distinctions between the blend flags:
   BLEND_IMAGE: the star on an image is matched with more 
   than one star in the catalog (image has worse seeing than catalog)
   BLEND_CATALOG: the star in the catalog is matched with more 
   than one star on the image (image has better seeing than catalog)
   CALIBRATED: relative photometry has been performed on this measurement
   BLEND_IMAGE_NEIGHBOR: the star on an image is matched with more 
   than one star in the catalog, but not in the same catalog file.
*/

/** these names were previously used for Average flags in old (LONEOS-era) dvo versions
  ID_BAD_OBJECT        = 0x00004000, // if all measurements are bad, set this bit
  ID_MOVING            = 0x00008000, // is a moving object
  ID_ROCK              = 0x0000a000, // 0x8000 + 0x2000
  ID_GHOST             = 0x0000c001, // 0x8000 + 0x4000 + 0x0001
  ID_TRAIL             = 0x0000c002, // 0x8000 + 0x4000 + 0x0002
  ID_BLEED             = 0x0000c003, // 0x8000 + 0x4000 + 0x0003
  ID_COSMIC            = 0x0000c004, // 0x8000 + 0x4000 + 0x0004
**/

/* Average.flags values -- these values are 32 bit (as of PS1_V1) */
typedef enum {
  ID_OBJ_FEW             = 0x00000001, // deprecated (was internal to relphot)
  ID_OBJ_POOR            = 0x00000002, // deprecated (was internal to relphot)

  // NOTE: bits used for object classification:
  ID_OBJ_ICRF_QSO        = 0x00000004, // object IDed with known ICRF quasar (may have ICRF position measurement)
  ID_OBJ_HERN_QSO_P60    = 0x00000008, // identified as likely QSO (Hernitschek et al 2015), P_QSO = 0.60
  ID_OBJ_HERN_QSO_P05    = 0x00000010, // identified as possible QSO (Hernitschek et al 2015), P_QSO = 0.05
  ID_OBJ_HERN_RRL_P60    = 0x00000020, // identified as likely  RR Lyra (Hernitschek et al 2015), P_RRLyra = 0.60
  ID_OBJ_HERN_RRL_P05    = 0x00000040, // identified as possible RR Lyra (Hernitschek et al 2015), P_RRLyra = 0.05
  ID_OBJ_HERN_VARIABLE   = 0x00000080, // identified as a variable based on ChiSq (Hernitschek et al 2015)
  ID_OBJ_TRANSIENT       = 0x00000100, // identified as a non-periodic (stationary) transient
  ID_OBJ_HAS_SOLSYS_DET  = 0x00000200, // identified with a known solar-system object (asteroid or other)
  ID_OBJ_MOST_SOLSYS_DET = 0x00000400, // most detections from a known solar-system object

  // NOTE: bits used for astrometry analysis
  ID_OBJ_LARGE_PM        = 0x00000800, // star with large proper motion
  ID_OBJ_RAW_AVE      	 = 0x00001000, // simple weighted average position was used (no IRLS fitting)
  ID_OBJ_FIT_AVE      	 = 0x00002000, // average position was fitted
  ID_OBJ_FIT_PM       	 = 0x00004000, // proper motion model was fitted
  ID_OBJ_FIT_PAR      	 = 0x00008000, // parallax model was fitted
  ID_OBJ_USE_AVE      	 = 0x00010000, // average position used (not PM or PAR)
  ID_OBJ_USE_PM       	 = 0x00020000, // proper motion used (not AVE or PAR)
  ID_OBJ_USE_PAR      	 = 0x00040000, // parallax used (not AVE or PM)
  ID_OBJ_NO_MEAN_ASTROM  = 0x00080000, // mean astrometry could not be measured
  ID_OBJ_STACK_FOR_MEAN  = 0x00100000, // stack position used for mean astrometry
  ID_OBJ_MEAN_FOR_STACK  = 0x00200000, // mean astrometry could not be measured
  ID_OBJ_BAD_PM          = 0x00400000, // failure to measure proper-motion model

  // NOTE: bits used for photometry analysis
  ID_OBJ_EXT             = 0x00800000, // extended in our data (eg, PS)
  ID_OBJ_EXT_ALT         = 0x01000000, // extended in external data (eg, 2MASS)
  ID_OBJ_GOOD            = 0x02000000, // good-quality measurement in our data (eg,PS)
  ID_OBJ_GOOD_ALT        = 0x04000000, // good-quality measurement in  external data (eg, 2MASS)
  ID_OBJ_GOOD_STACK      = 0x08000000, // good-quality object in the stack (> 1 good stack)
  ID_OBJ_BEST_STACK      = 0x10000000, // the primary stack measurement are the best measurements
  ID_OBJ_SUSPECT_STACK   = 0x20000000, // suspect object in the stack (> 1 good or suspect stack, < 2 good)
  ID_OBJ_BAD_STACK       = 0x40000000, // good-quality object in the stack (> 1 good stack)
} DVOAverageFlags;

/* Secfilt.flags values -- these values are 32 bit (as of PS1_V1) */
typedef enum {
  ID_SECF_STAR_FEW    		 = 0x00000001, // used within relphot: skip star
  ID_SECF_STAR_POOR   		 = 0x00000002, // used within relphot: skip star
  ID_SECF_USE_SYNTH   		 = 0x00000004, // synthetic photometry used in average measurement
  ID_SECF_USE_UBERCAL 		 = 0x00000008, // ubercal photometry used in average measurement
  ID_SECF_HAS_PS1     		 = 0x00000010, // PS1 photometry used in average measurement
  ID_SECF_HAS_PS1_STACK 	 = 0x00000020, // PS1 stack photometry exists
  ID_SECF_HAS_TYCHO   		 = 0x00000040, // Tycho photometry used for synth mags
  ID_SECF_FIX_SYNTH   		 = 0x00000080, // synth mags repaired with zpt map
  ID_SECF_RANK_0    		 = 0x00000100, // average magnitude uses rank 0 values
  ID_SECF_RANK_1    		 = 0x00000200, // average magnitude uses rank 1 values
  ID_SECF_RANK_2    		 = 0x00000400, // average magnitude uses rank 2 values
  ID_SECF_RANK_3    		 = 0x00000800, // average magnitude uses rank 3 values
  ID_SECF_RANK_4    		 = 0x00001000, // average magnitude uses rank 4 values
  ID_SECF_OBJ_EXT_PSPS  	 = 0x00002000, // In PSPS ID_SECF_OBJ_EXT is moved here so it fits within 16 bits 
  ID_SECF_STACK_PRIMARY 	 = 0x00004000, // PS1 stack photometry includes a primary skycell
  ID_SECF_STACK_BESTDET 	 = 0x00008000, // PS1 stack best measurement is a detection (not forced)
  ID_SECF_STACK_PRIMDET 	 = 0x00010000, // PS1 stack primary measurement is a detection (not forced)
  ID_SECF_STACK_PRIMARY_MULTIPLE = 0x00020000, // PS1 stack object has multiple primary measurements

  ID_SECF_HAS_SDSS      	 = 0x00100000, // this photcode has SDSS photometry
  ID_SECF_HAS_HSC       	 = 0x00200000, // this photcode has HSC  photometry
  ID_SECF_HAS_CFH       	 = 0x00400000, // this photcode has CFH  photometry (mostly Megacam)
  ID_SECF_HAS_DES       	 = 0x00800000, // this photcode has DES  photometry

  ID_SECF_OBJ_EXT       	 = 0x01000000, // extended in this band

  //ID_SECF_PHO_SYNTH            = 0x00000001, // this photcode has SYNTH photometry, turn on only for reference catalog
  //ID_SECF_PHO_ATLAS            = 0x00000002, // this photcode has ATLAS photometry, turn on only for reference catalog
  //ID_SECF_PHO_PV3              = 0x00000004, // this photcode has PV3   photometry, turn on only for reference catalog
  //ID_SECF_PHO_SOUTH            = 0x00000008, // this photcode has SOUTH photometry, turn on only for reference catalog

  ID_SECF_CHIP_FLAGS    	 = 0x01003f1f, // all chip-related bits (used to reset the correct bits only)
  ID_SECF_STACK_FLAGS   	 = 0x0003c020, // all stack-related bits (
} DVOSecfiltFlags;

/* definitions for parallel dvo host information 
   XXX : need better names (safer namespace)
*/

typedef enum {
  DATA_ON_TGT  = 0x01,
  DATA_ON_BCK  = 0x02,
  DATA_USE_BCK = 0x04,

  DATA_COPY_FAILURE = 0x80,
} SkyTableDataFlags;

typedef enum {
  HOST_STDIN = 0,
  HOST_STDOUT = 1,
  HOST_STDERR = 2,
} HostInfoIOfd;

/*** general dvo structures (internal use only / not IO) ***/

/* FITS DB structure */
typedef struct {
  FILE  *f;
  char  *filename;
  double timeout;
  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;
  VTable vtable;
  int    dbstate;
  char   lockstate;
  char   mode;          /* what data storage mode is used for disk file? */
  DVOCatFormat format;  /* what data format is used for disk file? */
  char   virtual;       /* is table in ftable or vtable? */
  char   nativeOrder;   /* is table in internal byte-order? */
  char   scaledValue;   /* is table in internal byte-order? */
} FITS_DB;

/* the basic HST GSC layout corresponds to a depth of 3 */
# define SKY_DEPTH_HST 3

/* SkyRegion : better implementation than GSCRegion */
typedef struct {
  off_t Nregions;
  off_t Nalloc;
  char **filename;
  SkyRegion *regions;
  char hosts[80];
} SkyTable;

typedef struct {
  off_t Nregions;
  int ownElements; 				  /* does this list own filename, regions? */
  char **filename;
  SkyRegion **regions;
  char hosts[80];
} SkyList;

typedef struct {
  off_t Nimage;
  FlatCorrectionImage *image;
  off_t Ncorr;
  int *IDtoSeq;
  FlatCorrection *corr;
  float ***offset; // the correction images represented as a set of arrays (same sequence as *image)
  int Nseason;
  e_time *tstart;
  e_time *tstop;
} FlatCorrectionTable;

typedef struct {
  char *hostname;	      // name of remote machine
  char *pathname;	      // name of directory for this machine's data
  char *results;	      // name of file machine's result data
  int hostID;		      // remove machine ID in SkyTable
  int stdio[3]; 	      // fd's for communication with the remote host
  int pid;		      // remote process ID
  int status;		      // job exit status
  IOBuffer stdout;
  IOBuffer stderr;
} HostInfo;

typedef struct {
  int Nhosts;
  HostInfo *hosts;
  short *index;
} HostTable;

typedef struct {
  int Nhosts;
  HostInfo **hosts;
} HostTableGroup;

// A RegionHost processes data for some region in parallel with other regions
typedef struct RegionHostInfo {
  double Rmin;	      // (Rmin,Rmax),(Dmin,Dmax) arehard RA,DEC boundaries of the 
  double Rmax;	      // region for which each host is responsible.  A given host
  double Dmin;	      // calibrates the images for which the fiducial point (center) 
  double Dmax;	      // lands in the region, and all objects in the region

  double RminCat;      // (RminCat,RmaxCat),(DminCat,DmaxCat) are the region for which 
  double RmaxCat;      // the catalogs need to be loaded : this is the outer bounds
  double DminCat;      // of the region containing all images completely
  double DmaxCat;

  char *hostname;

  int hostID;		      // remove machine ID in SkyTable
  int stdio[3]; 	      // fd's for communication with the remote host
  int pid;		      // remote process ID
  int status;
  IOBuffer stdout;
  IOBuffer stderr;

  off_t Nimage;
  off_t NIMAGE;
  Image *image;
  off_t *imseq;

  AstromOffsetTable *astromTable;

  int *neighbors;	      // list of neighbor index values
  int Nneighbors;	      // number of neighbors
  char isNeighbor;	      // TRUE if I am a neighbor to the current region host
} RegionHostInfo;

typedef struct {
  double Rmin;
  double Rmax;
  double Dmin;
  double Dmax;

  int Nhosts;
  RegionHostInfo *hosts;
  short *index;
} RegionHostTable;

// special-case function:
CMF_PS1_V2 *gfits_table_get_CMF_PS1_V1_Alt (FTable *ftable, off_t *Ndata, char *swapped);
CMF_PS1_SV1 *gfits_table_get_CMF_PS1_SV1_Alt (FTable *ftable, off_t *Ndata, char *swapped);

// another special case : does not match byte-boundaries
# include "cmf-ps1-dv3.h"
# include "cmf-ps1-sv3.h"
# include "cmf-ps1-sv4.h"
# include "cmf-ps1-v5.h"
# include "cmf-ps1-v5-r0.h"
# include "cmf-ps1-v5-r0-lensing.h"
# include "cmf-ps1-v5-r1-lensing.h"
# include "cmf-ps1-v5-r2-lensing.h"

typedef struct {
  int Ncode;					  // number of photcodes
  int Nsecfilt;					  // number of average magnitudes
  int hashcode[0x10000];		  // index from photcode value to sequence
  int hashNsec[0x10000];		  // index from photcode value to Nsec seq
  int codeNsec[0x10000];		  // index from Nsec seq to photcode value
  PhotCode *code;
} PhotCodeData;

typedef enum {
  DVO_TV_MEASURE = 0x01,
  DVO_TV_AVERAGE = 0x02,
} DVOTinyValueMode;

# define BOUNDARY_TREE_NAME_LENGTH 128

// BoundaryTree is a structure to describe the 3pi RINGS skycell boundaries in terms of lines of constant (RA,DEC)
// the structure is flexible for a variety of RINGS-like tessellations, but is not appropriate for the LOCAL style tess
typedef struct {
  int FixedGridDEC;	      // is the DEC sequence linear?
  int FixedGridRA;	      // in the RA sequence in a zone linear?

  double DEC_origin;
  double DEC_offset;

  int Nzone;
  double *RA_origin;
  double *RA_offset;
  double *DEC_min;
  double *DEC_max;
  double *DEC_min_raw;
  double *DEC_max_raw;

  int *Nband;
  int *NBAND;

  double   **ra; // RA of projection cell center
  double  **dec; // DEC of projection cell center
  int    **cell; // zone,band -> proj cell sequence
  int    **projID; // zone,band -> proj cell ID
  int    **skycellID; // zone,band -> starting skycell ID
  char  ***name; // projection cell name
  
  float NX_SUB;
  float NY_SUB;
  double dPix;

  double **Xo;
  double **Yo;
  float **dX;
  float **dY;
} BoundaryTree;

typedef enum { TESS_NONE, TESS_LOCAL, TESS_RINGS } TessType;

// TessellationTable is a structure to describe the parameters of a set of "tessellations"
// (these are not strictly tessellations but projection sets as only the non-local
// versions can cover the full sky).  For LOCAL projection cells, the structure describes
// the boundaries of a SINGLE projection cell with Nx * Ny skycells and includes some
// basic parameters (not used by the fullsky, eg RINGS, tessellations)
typedef struct {
  double Rmin; // this tessellation is valid only for RA >= Rmin
  double Rmax; // this tessellation is valid only for RA <  Rmax
  double Dmin; // this tessellation is valid only for DEC >= Dmin
  double Dmax; // this tessellation is valid only for DEC <  Dmax

  double Xo;
  double Yo;
  double Ro;
  double Do;
  double dPix;
  float dX;
  float dY;

  float NX_SUB;
  float NY_SUB;

  char *basename;
  int Nbasename;
  int projectIDoff;
  int skycellIDoff;

  TessType type; // 
  BoundaryTree *tree;
} TessellationTable;

// a reduced-subset structure for relphot
typedef struct {
  double         R;
  double         D;
  unsigned short Nmeasure;
  int            measureOffset;
  uint32_t       flags;
  int            catID;
  int            objID;
  int            nOwn;
} AverageTiny;

// a reduced-subset structure for relphot & relastro
typedef struct {
  double         R;
  double         D;
  float          M; // change to Mpsf eventually to disambiguate
  float          Mkron;
  float          McalPSF;
  float          McalAPER;
  float          Mflat;
  float          dM;
  float          airmass;
  float          Xccd;
  float          Yccd;
  float          Xfix;
  float          Yfix;
  float          dt;
  float          psfQF;
  int   	 t;
  unsigned int   averef;
  unsigned int   imageID;
  unsigned int   dbFlags;
  unsigned int   photFlags;
  int            catID; // unsigned int?
  unsigned short photcode;
  short          dXccd;
  short          dYccd;
  short          dRsys;
  char           myDet;
} MeasureTiny;

// alternate version of PS1_V4 (old dev version)
typedef struct {
  double           R;                    // RA (decimal degrees )
  double           D;                    // DEC (decimal degrees )
  float            dR;                   // RA error (arcsec)
  float            dD;                   // DEC error (arcsec)
  float            uR;                   // RA*cos(D) proper-motion (arcsec/year)
  float            uD;                   // DEC proper-motion (arcsec/year)
  float            duR;                  // RA*cos(D) p-m error (arcsec/year)
  float            duD;                  // DEC p-m error (arcsec/year)
  float            P;                    // parallax (arcsec)
  float            dP;                   // parallax error (arcsec)
  float            ChiSqAve;             // astrometry analysis chisq
  float            ChiSqPM;              // astrometry analysis chisq
  float            ChiSqPar;             // astrometry analysis chisq
  int              Tmean;                // mean epoch (PM,PAR ref) (unix time seconds)
  int              Trange;               // mean epoch (PM,PAR ref) (unix time seconds)
  float            Xp;                   // unused
  unsigned short   Npos;                 // number of detections used for astrometry
  unsigned short   Nmeasure;             // number of psf measurements
  unsigned short   Nmissing;             // number of missings
  unsigned short   Ngalphot;            // number of extended measurements
  uint32_t         measureOffset;        // offset to first psf measurement
  uint32_t         missingOffset;        // offset to first missing obs
  float            refColor;            // offset to first extended measurement
  uint32_t         flags;                // average object flags (star; ghost; etc)
  uint32_t         photFlagsUpper;       // upper bit of 2 bit summary of per-measure photflags
  uint32_t         photFlagsLower;       // lower bit of 2 bit summary of per-measure photflags
  unsigned int     objID;                // unique ID for object in table
  unsigned int     catID;                // unique ID for table in which object was first realized
  uint64_t         extID;                // external ID for object (eg PSPS objID)
} Average_PS1_V4alt;

Average_PS1_V4alt *gfits_table_get_Average_PS1_V4alt (FTable *table, off_t *Ndata, char *swapped);
Average *Average_PS1_V4alt_ToInternal (Average_PS1_V4alt *in, off_t Nvalues);

// alternate version of PS1_V4 (old dev version)
typedef struct {
  float            dR;                   // RA offset (arcsec)
  float            dD;                   // DEC offset (arcsec)
  float            M;                    // catalog mag (mag)
  float            Mcal;                 // image cal mag (mag)
  float            Map;                  // aperture mag (mag)
  float            Mkron;                // kron magnitude (mag)
  float            dMkron;               // kron magnitude error (mag)
  float            dM;                   // mag error (mag)
  float            dMcal;                // systematic calibration error (mag)
  float            dt;                   // exposure time (2.5*log(exptime))
  float            FluxPSF;              // flux from psf fit (counts/sec?)
  float            dFluxPSF;             // error on psf flux (counts/sec?)
  float            FluxKron;             // flux from kron ap (counts/sec?)
  float            dFluxKron;            // error on kron flux (counts/sec?)
  float            airmass;              // (airmass - 1) (airmass)
  float            az;                   // telescope azimuth
  float            Xccd;                 // X coord on chip (raw value) (pixels)
  float            Yccd;                 // Y coord on chip (raw value) (pixels)
  float            Sky;                  // local estimate of sky flux (counts/sec)
  float            dSky;                 // local estimate of sky flux (counts/sec)
  int              t;                    // time in seconds (UNIX)
  unsigned int     averef;               // reference to average entry      
  unsigned int     detID;                // detection ID
  unsigned int     imageID;              // reference to DVO image ID
  unsigned int     objID;                // unique ID for object in table
  unsigned int     catID;                // unique ID for table in which object was first realized
  uint64_t         extID;                // external ID (eg PSPS detID)
  float            psfQF;                // psf coverage/quality factor
  float            psfQFperf;            // psf coverage / quality factor (all mask bits)
  float            psfChisq;             // psf fit chisq
  int              psfNdof;              // psf degrees of freedom
  int              psfNpix;              // psf number of pixels
  float            crNsigma;             // Nsigma deviation towards CR
  float            extNsigma;            // Nsigma deviation towards EXT
  short            FWx;                  // object fwhm major axis (1/100 of pixels)
  short            FWy;                  // object fwhm minor axis (1/100 of pixels )
  short            theta;                // angle wrt ccd X dir ((0xffff/360) deg)
  short            Mxx;                  // second moments in pixel coords (1/100 of pixels)
  short            Mxy;                  // second moments in pixel coords (1/100 of pixels)
  short            Myy;                  // second moments in pixel coords (1/100 of pixels)
  unsigned short   t_msec;               // time fraction of second (milliseconds)
  unsigned short   photcode;             // photcode
  short            dXccd;                // X coord error on chip (1/100 of pixels)
  short            dYccd;                // Y coord error on chip (1/100 of pixels)
  short            dRsys;                // systematic error from astrom (1/100 of pixels)
  short            posangle;             // position angle sky to chip ((0xffff/360) deg)
  float            pltscale;             // plate scale (arcsec/pixel)
  unsigned int     dbFlags;              // flags supplied by analysis in database
  unsigned int     photFlags;            // flags supplied by photometry program
} Measure_PS1_V4alt;

Measure_PS1_V4alt *gfits_table_get_Measure_PS1_V4alt (FTable *table, off_t *Ndata, char *swapped);
Measure *Measure_PS1_V4alt_ToInternal (Average *ave, Measure_PS1_V4alt *in, off_t Nvalues);

typedef struct {
  double           R;                    // RA at epoch (degrees)
  double           D;                    // DEC at epoch (degrees)
  float            M;                    // catalog mag (mag)
  float            dM;                   // mag error (mag)
  float            Map;                  // aperture mag (mag)
  float            dMap;                 // aperture mag (mag)
  float            Mkron;                // kron magnitude (mag)
  float            dMkron;               // kron magnitude error (mag)
  float            Mcal;                 // image cal mag (mag)
  float            dMcal;                // systematic calibration error (mag)
  float            dt;                   // exposure time (2.5*log(exptime))
  float            FluxPSF;              // flux from psf fit (counts/sec)
  float            dFluxPSF;             // error on psf flux (counts/sec)
  float            FluxKron;             // flux from kron ap (counts/sec)
  float            dFluxKron;            // error on kron flux (counts/sec)
  float            FluxAp;               // flux from ap ap (counts/sec)
  float            dFluxAp;              // error on ap flux (counts/sec)
  float            airmass;              // (airmass - 1) (airmass)
  float            az;                   // telescope azimuth
  float            Xccd;                 // X coord on chip (raw value) (pixels)
  float            Yccd;                 // Y coord on chip (raw value) (pixels)
  float            Xfix;                 // X coord after correction (pixels)
  float            Yfix;                 // Y coord after correction (pixels)
  float            XoffKH;               // X offset from correction (pixels)
  float            YoffKH;               // Y offset from correction (pixels)
  float            XoffDCR;              // X offset from correction (pixels)
  float            YoffDCR;              // Y offset from correction (pixels)
  float            Mflat;                // flat offset from correction (arcsec)
  int              padding2;             // dummy
  float            Sky;                  // local estimate of sky flux (counts/sec)
  float            dSky;                 // local estimate of sky flux (counts/sec)
  int              t;                    // time in seconds (UNIX)
  unsigned int     averef;               // reference to average entry      
  unsigned int     detID;                // detection ID
  unsigned int     objID;                // unique ID for object in table
  unsigned int     catID;                // unique ID for table in which object was first realized
  uint64_t         extID;                // external ID (eg PSPS detID)
  unsigned int     imageID;              // reference to DVO image ID
  float            psfQF;                // psf coverage/quality factor
  float            psfQFperf;            // psf coverage / quality factor (all mask bits)
  float            psfChisq;             // psf fit chisq
  int              psfNdof;              // psf degrees of freedom
  int              psfNpix;              // psf number of pixels
  int              photFlags2;           // flags supplied by photometry program
  float            extNsigma;            // Nsigma deviation towards EXT
  short            FWx;                  // object fwhm major axis (1/100 of pixels)
  short            FWy;                  // object fwhm minor axis (1/100 of pixels )
  short            theta;                // angle wrt ccd X dir ((0xffff/360) deg)
  short            Mxx;                  // second moments in pixel coords (1/100 of pixels)
  short            Mxy;                  // second moments in pixel coords (1/100 of pixels)
  short            Myy;                  // second moments in pixel coords (1/100 of pixels)
  unsigned short   t_msec;               // time fraction of second (milliseconds)
  unsigned short   photcode;             // photcode
  short            dXccd;                // X coord error on chip (1/100 of pixels)
  short            dYccd;                // Y coord error on chip (1/100 of pixels)
  short            dRsys;                // systematic error from astrom (1/100 of pixels)
  short            posangle;             // position angle sky to chip ((0xffff/360) deg)
  float            pltscale;             // plate scale (arcsec/pixel)
  unsigned int     dbFlags;              // flags supplied by analysis in database
  unsigned int     photFlags;            // flags supplied by photometry program
  int              padding;              // padding to ensure 8byte blocks
} Measure_PS1_V5alt;

Measure *Measure_PS1_V5alt_ToInternal (Average *ave, Measure_PS1_V5alt *in, off_t Nvalues);
int gfits_convert_Measure_PS1_V5alt (Measure_PS1_V5alt *data, off_t size, off_t nitems);
Measure_PS1_V5alt *gfits_table_get_Measure_PS1_V5alt (FTable *ftable, off_t *Ndata, char *swapped);

// alternate version of PS1_V5 (old dev version)
typedef struct {
  float            M;                    // average mag in this band (mags)
  float            dM;                   // formal error on average mag (mags)
  float            Map;                  // ave aperture mag in this band (mags)
  float            dMap;                 // ave aperture mag in this band (mags)
  float            sMap;                 // standard deviation of ap mags (mags)
  float            Mkron;                // ave kron mag in this band (mags)
  float            dMkron;               // formal error on average kron mag (mags)
  float            sMkron;               // standard deviation of kron mags (mags)
  float            Mstdev;               // standard deviation of measurements (mags)
  float            Mmin;                 // min accepted mag (mags)
  float            Mmax;                 // max accepted mag (mags)
  float            Mchisq;               // chisq on average mag (value)
  short            Ncode;                // number of detections in band
  short            Nused;                // number of detections used in average
  short            NusedKron;            // number of detections used in average
  short            NusedAp;              // number of detections used in average
  uint32_t         flags;                // photometry flags
  float            MpsfStk;              // magnitude from stack (primary if available)
  float            FpsfStk;              // flux from stack (primary if available)
  float            dFpsfStk;             // mean flux psf error
  float            MkronStk;             // magnitude from stack (primary if available)
  float            FkronStk;             // flux from stack (primary if available)
  float            dFkronStk;            // mean flux kron error
  float            MapStk;               // magnitude from stack (primary if available)
  float            FapStk;               // flux from stack (primary if available)
  float            dFapStk;              // mean flux ap error
  int              stackPrmryOff;        // measure entry which is primary stack detection
  int              stackBestOff;         // measure entry which is best stack detection
  float            MpsfWrp;              // psf magnitude from stack (primary if available)
  float            FpsfWrp;              // psf flux from stack (primary if available)
  float            dFpsfWrp;             // mean flux psf error
  float            sFpsfWrp;             // mean flux psf stdev
  float            MkronWrp;             // kron magnitude from stack (primary if available)
  float            FkronWrp;             // kron flux from stack (primary if available)
  float            dFkronWrp;            // mean flux kron error
  float            sFkronWrp;            // mean flux kron stdev
  float            MapWrp;               // aper magnitude from stack (primary if available)
  float            FapWrp;               // aper flux from stack (primary if available)
  float            dFapWrp;              // mean flux ap error
  float            sFapWrp;              // mean flux ap stdev
  short            NusedWrp;             // number of detections used in average
  short            NusedKronWrp;         // number of detections used in average
  short            NusedApWrp;           // number of detections used in average
  short            ubercalDist;          // number of images from an ubercal-image
} SecFilt_PS1_V5alt;

SecFilt_PS1_V5alt *gfits_table_get_SecFilt_PS1_V5alt (FTable *table, off_t *Ndata, char *swapped);
int      gfits_convert_SecFilt_PS1_V5alt (SecFilt_PS1_V5alt *data, off_t size, off_t nitems);
SecFilt *SecFilt_PS1_V5alt_ToInternal (SecFilt_PS1_V5alt *in, off_t Nvalues);

// alternate version of PS1_V5alt (old dev version)
typedef struct {
  double           R;                    // RA (decimal degrees )
  double           D;                    // DEC (decimal degrees )
  float            dR;                   // RA error (arcsec)
  float            dD;                   // DEC error (arcsec)
  float            uR;                   // RA*cos(D) proper-motion (arcsec/year)
  float            uD;                   // DEC proper-motion (arcsec/year)
  float            duR;                  // RA*cos(D) p-m error (arcsec/year)
  float            duD;                  // DEC p-m error (arcsec/year)
  float            P;                    // parallax (arcsec)
  float            dP;                   // parallax error (arcsec)
  double           Rstk;                 // RA on stack (decimal degrees )
  double           Dstk;                 // DEC on stack (decimal degrees )
  float            dRstk;                // RA error on stack (arcsec)
  float            dDstk;                // DEC error on stack (arcsec)
  float            ChiSqAve;             // astrometry analysis chisq
  float            ChiSqPM;              // astrometry analysis chisq
  float            ChiSqPar;             // astrometry analysis chisq
  int              Tmean;                // mean epoch (PM,PAR ref) (unix time seconds)
  int              Trange;               // mean epoch (PM,PAR ref) (unix time seconds)
  float            psfQF;                // psf coverage (bad masks)
  float            psfQFperf;            // psf coverage (all masks)
  float            stargal;              // star / galaxy separator (1/100 arcsec)
  unsigned short   Npos;                 // number of detections used for astrometry
  unsigned short   Nmeasure;             // number of psf measurements
  unsigned short   Nmissing;             // number of missings
  unsigned short   Nlensing;             // number of lensing measurements
  unsigned short   Nlensobj;             // number of lensing measurements
  unsigned short   Ngalphot;            // number of galphot measurements
  int              measureOffset;        // offset to first psf measurement
  int              missingOffset;        // offset to first missing obs
  int              lensingOffset;        // offset to first lensing obs
  int              lensobjOffset;        // offset to mean lensing data
  int              galphotOffset;       // offset to extended object entry
  int              starparOffset;        // offset to stellar parameter data
  float            refColorBlue;         // color of astrometry ref stars
  float            refColorRed;          // color of astrometry ref stars
  uint32_t         flags;                // average object flags (star; ghost; etc)
  uint32_t         photFlagsUpper;       // upper bit of 2 bit summary of per-measure photflags
  uint32_t         photFlagsLower;       // lower bit of 2 bit summary of per-measure photflags
  unsigned int     objID;                // unique ID for object in table
  unsigned int     catID;                // unique ID for table in which object was first realized
  uint64_t         extID;                // external ID for object (eg PSPS objID)
  uint64_t         extIDgc;              // external ID for object in galactic coords
} Average_PS1_V5alt;

Average_PS1_V5alt *gfits_table_get_Average_PS1_V5alt (FTable *table, off_t *Ndata, char *swapped);
int      gfits_convert_Average_PS1_V5alt (Average_PS1_V5alt *data, off_t size, off_t nitems);
Average *Average_PS1_V5alt_ToInternal (Average_PS1_V5alt *in, off_t Nvalues);

/* for some reason I have merged the set of tables and the file description,
   so I need to have an internal structure to point to the separate files */

/* a catalog contains this data */
typedef struct Catalog {
  char *filename;			/* catalog file */
  FILE *f;  				/* file descriptor */
  Header  header;

  /* data in the catalog file */
  Average *average;
  Measure *measure; 
  Missing *missing; 
  SecFilt *secfilt;

  // lensing data (optional?)
  Lensing  *lensing;
  Lensobj  *lensobj;
  StarPar  *starpar;
  GalPhot *galphot;

  int Nsecfilt;  /* number of secfilt entries for each average entry */
  off_t Naverage,      Nmeasure,      Nmissing,      Nlensing,      Nlensobj,      Nstarpar,      Ngalphot,      Nsecfilt_mem;  /* current number of each component in memory */
  off_t Naverage_disk, Nmeasure_disk, Nmissing_disk, Nlensing_disk, Nlensobj_disk, Nstarpar_disk, Ngalphot_disk, Nsecfilt_disk; /* current number of each component on disk */
  off_t Naverage_off,  Nmeasure_off,  Nmissing_off,  Nlensing_off,  Nlensobj_off,  Nstarpar_off,  Ngalphot_off,  Nsecfilt_off;  /* index of first loaded data value */

  // note that we use these for the full-sky relphot analysis
  AverageTiny *averageT;
  MeasureTiny *measureT; 

  /* the Nsecf_* values above are number of table rows (eg, Naverage*Nsecfilt) */

  /* note the different counting for Nsecfilt:
     number of secfilt rows on disk is: Nave_disk * Nsecfilt
     number of secfilt rows in mem  is: Naverage * Nsecfilt
     *** that is just silly, and bad: convert to using Nsec_mem, Nsec_disk, Nsec_off.
     *** unless we always require the secfilt and average entries to be loaded sychronously.
     */

  /* pointers to split data files */
  struct Catalog *measure_catalog;		/* measure  catalog data (split) */
  struct Catalog *missing_catalog;		/* missing  catalog data (split) */
  struct Catalog *secfilt_catalog;		/* secfilt  catalog data (split) */
  struct Catalog *lensing_catalog;		/* lensing  catalog data (split) */
  struct Catalog *lensobj_catalog;		/* lensobj  catalog data (split) */
  struct Catalog *starpar_catalog;		/* starpar  catalog data (split) */
  struct Catalog *galphot_catalog;		/* galphot catalog data (split) */

  unsigned int objID;
  unsigned int catID;

  /* extra catalog information */
  char lockmode;
  DVOCatMode     catmode;     /* storage mode (raw, mef, split, mysql) */
  DVOCatFormat   catformat;			/* storage format (elixir, panstarrs, etc) */
  DVOCatCompress catcompress;			// output compress mode
  DVOCatFlags    catflags;			/* choices to be loaded */
  
  int sorted;				/* is measure table average-sorted? (NOTE this is an int only because gfits_scan %t requires it) */

  /* pointers for data manipulation */
  off_t *found_t;
  off_t *foundWarp_t;

  char *measureRank;
  int   *nOwn_t; // relastro uses this to count owned detections per object

} Catalog;

/*** prototypes ***/

/* in gfits_db.c */
int   gfits_db_init                PROTO((FITS_DB *db));
int   gfits_db_create              PROTO((FITS_DB *db));
int   gfits_db_lock                PROTO((FITS_DB *db, char *filename));
int   gfits_db_load                PROTO((FITS_DB *db));
int   gfits_db_load_segment        PROTO((FITS_DB *db, off_t start, off_t Nrows));
int   gfits_db_save                PROTO((FITS_DB *db));
int   gfits_db_update              PROTO((FITS_DB *db));
int   gfits_db_close               PROTO((FITS_DB *db));
int   gfits_db_free                PROTO((FITS_DB *db));

int gfits_fread_uncompressed (Catalog *catalog, FTable *ftable, char *nativeOrder, char VERBOSE);

char *libdvo_version (void);

int isRegisteredMosaic (void);
off_t GetRegisteredMosaic (void);
off_t *GetChipMatch (void);
int GetMosaicCoords (Coords *coords);
int FindMosaicForImage (Image *images, off_t Nimages, off_t entry);
int FindMosaicForImage_TableSearch (Image *images, off_t Nimages, off_t entry);
int FindMosaicForImage_MatchSearch (Image *images, off_t Nimages, off_t entry);
int BuildChipMatch (Image *images, off_t Nimages);
void SetImageCorners (double *X, double *Y, Image *image);

short int putMi (double value);
double getMi (short int value);
void returnMcal (Image *image, double *c);
void assignMcal (Image *image, double *c, int order);
double applyMcal (Image *image, double x, double y);
double findscatter (double *X, double *Y, double *M, double *dM, int N, double *c, int order);

PhotCode *GetPhotcodebyName (char *name);
PhotCode *GetPhotcodeEquivbyName (char *name);
PhotCode *GetPhotcodebyCode (int code);
PhotCode *GetPhotcodebyNsec (int Nsec);
PhotCode *GetPhotcodeEquivbyCode (int code);
char     *GetPhotcodeNamebyCode (int code);

float PhotInst (Measure *measure, dvoMagClassType class);
float PhotCat (Measure *measure, dvoMagClassType class);
float PhotSys (Measure *measure, Average *average, SecFilt *secfilt, dvoMagClassType class);
float PhotRel (Measure *measure, Average *average, SecFilt *secfilt, dvoMagClassType class);
float PhotCal (Measure *thisone, Average *average, SecFilt *secfilt, Measure *measure, PhotCode *code, dvoMagClassType class);
float PhotErr (Measure *measure, dvoMagClassType class);
float PhotCalErr (Measure *measure, dvoMagClassType class);

float PhotAve (PhotCode *code, Average *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source);
float PhotRef (PhotCode *code, Average *average, SecFilt *secfilt, Measure *measure, dvoMagClassType class, dvoMagSourceType source);
float PhotAveErr (PhotCode *code, Average *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source);

float PhotInstTiny (MeasureTiny *measure, dvoMagClassType class);
float PhotCatTiny (MeasureTiny *measure, dvoMagClassType class);
float PhotSysTiny (MeasureTiny *measure, AverageTiny *average, SecFilt *secfilt, dvoMagClassType class);
float PhotRelTiny (MeasureTiny *measure, AverageTiny *average, SecFilt *secfilt, dvoMagClassType class);
float PhotCalTiny (MeasureTiny *thisone, AverageTiny *average, SecFilt *secfilt, MeasureTiny *measure, PhotCode *code, dvoMagClassType class);

float PhotAveTiny (PhotCode *code, AverageTiny *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source);
float PhotRefTiny (PhotCode *code, AverageTiny *average, SecFilt *secfilt, MeasureTiny *measure, dvoMagClassType class, dvoMagSourceType source);

float PhotFluxInst (Measure *measure, dvoMagClassType class);
float PhotFluxCat (Measure *measure, dvoMagClassType class);
float PhotFluxSys (Measure *measure, Average *average, SecFilt *secfilt, dvoMagClassType class);
float PhotFluxRel (Measure *measure, Average *average, SecFilt *secfilt, dvoMagClassType class);
float PhotFluxCal (Measure *thisone, Average *average, SecFilt *secfilt, Measure *measure, PhotCode *code, dvoMagClassType class);

float PhotFluxAve (PhotCode *code, Average *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source);
float PhotFluxRef (PhotCode *code, Average *average, SecFilt *secfilt, Measure *measure, dvoMagClassType class, dvoMagSourceType source);

float PhotFluxInstErr (Measure *measure, dvoMagClassType class);
float PhotFluxCatErr (Measure *measure, dvoMagClassType class);
float PhotFluxAveErr (PhotCode *code, Average *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source);
float PhotFluxSysErr (Measure *measure, Average *average, SecFilt *secfilt, dvoMagClassType class);
float PhotFluxRelErr (Measure *measure, Average *average, SecFilt *secfilt, dvoMagClassType class);

float PhotXm (PhotCode *code, Average *average, SecFilt *secfilt);
float PhotZeroPoint (Measure *measure, Average *average, SecFilt *secfilt);

float PhotSecfiltPsfQf (PhotCode *code, Average *average, SecFilt *secfilt);
float PhotSecfiltPsfQfPerfect (PhotCode *code, Average *average, SecFilt *secfilt);

int   PhotSecfiltFlags (PhotCode *code, Average *average, SecFilt *secfilt);
int   PhotNwarp (PhotCode *code, Average *average, SecFilt *secfilt);
int   PhotNwarpGood (PhotCode *code, Average *average, SecFilt *secfilt);
int   PhotNstack (PhotCode *code, Average *average, SecFilt *secfilt);
int   PhotNstackDet (PhotCode *code, Average *average, SecFilt *secfilt);
int   PhotNcode (PhotCode *code, Average *average, SecFilt *secfilt);
int   PhotNphot (PhotCode *code, Average *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source);
float PhotMstdev (PhotCode *code, Average *average, SecFilt *secfilt, dvoMagClassType class, dvoMagSourceType source);
float PhotMmin (PhotCode *code, Average *average, SecFilt *secfilt);
float PhotMmax (PhotCode *code, Average *average, SecFilt *secfilt);
float PhotUCdist (PhotCode *code, Average *average, SecFilt *secfilt);
unsigned int PhotStackID (PhotCode *code, Average *average, SecFilt *secfilt);

float PhotColorForCode (Average *average, SecFilt *secfilt, Measure *measure, PhotCode *code);
int PhotColor (Average *average, SecFilt *secfilt, Measure *measure, int c1, int c2, double *color);

float PhotXmTiny (PhotCode *code, AverageTiny *average, SecFilt *secfilt);
float PhotdMTiny (PhotCode *code, AverageTiny *average, SecFilt *secfilt);

float PhotColorForCodeTiny (AverageTiny *average, SecFilt *secfilt, MeasureTiny *measure, PhotCode *code);
int PhotColorTiny (AverageTiny *average, SecFilt *secfilt, MeasureTiny *measure, int c1, int c2, double *color);

PhotCodeData *GetPhotcodeTable (void);
void SetPhotcodeTable (PhotCodeData *);
void FreePhotcodeData (PhotCodeData *myPhotcodes);
void FreePhotcodeTable (void);

int *GetSecFiltMap(PhotCodeData *ouput, PhotCodeData* input);
PhotCode **ParsePhotcodeList (char *rawlist, int *nphotcodes, int needAve);

int LoadPhotcodes (char *catdir_file, char *master_file, int readwrite);
int LoadPhotcodesText (char *filename);
int LoadPhotcodesFITS (char *filename);
int SavePhotcodesText (char *filename);
int SavePhotcodesFITS (char *filename);

void PrintPhotcodeNamebyCode (FILE *f, char *format, int code);

int GetPhotcodeCodebyName (char *name);
int GetPhotcodeEquivCodebyName (char *name);
int GetPhotcodeEquivCodebyCode (int code);
int GetPhotcodeNsec (int code);
int GetPhotcodeNsecfilt (void);
void SetZeroPoint (double ZP);
double GetZeroPoint (void);
int *GetPhotcodeEquivList (int code, int *nlist);
void ParseColorTerms (char *terms, float *X, int *N);

int get_image_type (char *name);
char *get_type_name (int type);
int get_image_mode (char *name);
char *get_mode_name (int mode);

/** dvo_catalog APIs */
void dvo_catalog_init (Catalog *catalog, int complete);
void dvo_catalog_create (SkyRegion *region, Catalog *catalog);
void dvo_catalog_free (Catalog *catalog);
void dvo_catalog_free_data (Catalog *catalog);
int dvo_catalog_check (Catalog *catalog, int Nsecfilt, int extend);
int dvo_catalog_lock (Catalog *catalog, int lockmode);
int dvo_catalog_unlock (Catalog *catalog);
int dvo_catalog_load (Catalog *catalog, int VERBOSE);
int dvo_catalog_open (Catalog *catalog, SkyRegion *region, int VERBOSE, char *iomode);
int dvo_catalog_save (Catalog *catalog, char VERBOSE);
int dvo_catalog_save_complete (Catalog *catalog, char VERBOSE);
int dvo_catalog_update (Catalog *catalog, char VERBOSE);
DVOCatFormat dvo_catalog_catformat (char *catformat);
DVOCatMode dvo_catalog_catmode (char *catmode);
DVOCatCompress dvo_catalog_catcompress (char *catcompress);
char *dvo_catalog_compress_string (DVOCatCompress catcompress);
void dvo_catalog_test (Catalog *catalog, int halt);

int dvo_catalog_backup (Catalog *catalog, char *suffix, int primary);
int dvo_catalog_unlink_backup (Catalog *catalog, char *suffix, int primary);

/* catmode-specific APIs */
int dvo_catalog_load_raw (Catalog *catalog, int VERBOSE);
int dvo_catalog_save_raw (Catalog *catalog, char VERBOSE);
int dvo_catalog_load_mef (Catalog *catalog, int VERBOSE);
int dvo_catalog_save_mef (Catalog *catalog, char VERBOSE);
int dvo_catalog_load_split (Catalog *catalog, int VERBOSE);
int dvo_catalog_save_split (Catalog *catalog, char VERBOSE);
int dvo_catalog_update_split (Catalog *catalog, char VERBOSE);
int dvo_catalog_save_split_complete (Catalog *catalog, char VERBOSE);

int dvo_catalog_load_segment (Catalog *catalog, int VERBOSE, off_t start, off_t Nrows);
int dvo_catalog_load_segment_split (Catalog *catalog, int VERBOSE, off_t start, off_t Nrows);

/*** conversion functions / I/O conversions ***/
Average *ReadRawAverage (FILE *f, off_t Naverage, char format, SecFilt **primary);
Measure *ReadRawMeasure (FILE *f, Average *average, off_t Nmeasure, char format);
SecFilt *ReadRawSecFilt (FILE *f, off_t Nsecfilt, char format);
int WriteRawAverage (FILE *f, Average *average, off_t Naverage, char format, SecFilt *primary);
int WriteRawMeasure (FILE *f, Average *average, Measure *measure, off_t Nmeasure, char format);
int WriteRawSecFilt (FILE *f, SecFilt *secfilt, off_t Nsecfilt, char format);

DVOCatFormat FtableGetFormat (FTable *ftable);

Average *FtableToAverage   (FTable *ftable, off_t *Naverage,  DVOCatFormat *format, SecFilt **primary, char nativeOrder);

Measure *FtableToMeasure   (FTable *ftable, Average *average, off_t *Nmeasure,  DVOCatFormat *format, char nativeOrder);
Missing *FtableToMissing   (FTable *ftable, Average *average, off_t *Nmissing,  DVOCatFormat *format, char nativeBytes);
SecFilt *FtableToSecFilt   (FTable *ftable, Average *average, off_t *Nsecfilt,  DVOCatFormat *format, char nativeOrder);
Lensing *FtableToLensing   (FTable *ftable, Average *average, off_t *Nlensing,  DVOCatFormat *format, char nativeOrder);
Lensobj *FtableToLensobj   (FTable *ftable, Average *average, off_t *Nlensobj,  DVOCatFormat *format, char nativeOrder);
StarPar *FtableToStarPar   (FTable *ftable, Average *average, off_t *Nstarpar,  DVOCatFormat *format, char nativeOrder);
GalPhot *FtableToGalPhot (FTable *ftable, Average *average, off_t *Ngalphot, DVOCatFormat *format, char nativeOrder);

int      FtableToImage   (FTable *ftable, Header *theader, DVOCatFormat *format);

int MeasureToFtable  (FTable *ftable, Average  *average,  Measure *measure, off_t Nmeasure, DVOCatFormat format, int swapFromNative);
int AverageToFtable  (FTable *ftable, Average  *average,  off_t Naverage,  DVOCatFormat format, SecFilt *primary, int swapFromNative);
int SecFiltToFtable  (FTable *ftable, SecFilt  *secfilt,  off_t Nsecfilt,  DVOCatFormat format, int swapFromNative);
int LensingToFtable  (FTable *ftable, Lensing  *lensing,  off_t Nlensing,  DVOCatFormat format, int swapFromNative);
int LensobjToFtable  (FTable *ftable, Lensobj  *lensobj,  off_t Nlensobj,  DVOCatFormat format, int swapFromNative);
int StarParToFtable  (FTable *ftable, StarPar  *starpar,  off_t Nstarpar,  DVOCatFormat format, int swapFromNative);
int GalPhotToFtable (FTable *ftable, GalPhot *galphot, off_t Ngalphot, DVOCatFormat format, int swapFromNative);

int ImageToFtable (FTable *ftable, Header *theader, DVOCatFormat format);
int ImageToVtable (VTable *vtable, Header *theader, DVOCatFormat format);

# include "loneos_defs.h"
# include "elixir_defs.h"
# include "panstarrs_dev_0_defs.h"
# include "panstarrs_dev_1_defs.h"
# include "ps1_dev_1_defs.h"
# include "ps1_dev_2_defs.h"
# include "ps1_dev_3_defs.h"
# include "ps1_v1_defs.h"
# include "ps1_v2_defs.h"
# include "ps1_v3_defs.h"
# include "ps1_v4_defs.h"
# include "ps1_v5_defs.h"
# include "ps1_v6_defs.h"
# include "ps1_v5_ld_defs.h"
# include "ps1_ref_defs.h"
# include "ps1_ref_v2_defs.h"
# include "ps1_ref_v3_defs.h"
# include "ps1_sim_defs.h"

/*** DVO image db I/O Functions ***/
int dvo_image_lock (FITS_DB *db, char *filename, double timeout, int lockstate);
int dvo_image_unlock (FITS_DB *db);
int dvo_image_load (FITS_DB *db, int VERBOSE, int FORCE_READ);
int dvo_image_save (FITS_DB *db, int VERBOSE);
int dvo_image_update (FITS_DB *db, int VERBOSE);
int dvo_image_load_raw (FITS_DB *db, int VERBOSE, int FORCE_READ);
int dvo_image_update_raw (FITS_DB *db, int VERBOSE);
int dvo_image_save_raw (FITS_DB *db, int VERBOSE);
int dvo_image_addrows (FITS_DB *db, Image *new, off_t Nnew);
int dvo_image_createID (Header *header);
void dvo_image_create (FITS_DB *db, double ZeroPoint);

int gfits_table_set_Image (FTable *ftable);
int gfits_table_mkheader_Image (Header *header);
Image *gfits_table_get_Image (FTable *ftable, off_t *Ndata, char *scaledValue, char *nativeOrder);

/* flatcorr APIs */
FlatCorrectionTable *FlatCorrectionLoad (char *filename, int VERBOSE);
int FlatCorrectionInternal(FlatCorrectionTable *flatcorrTable);
int FlatCorrectionSave (FlatCorrectionTable *flatcorrTable, char *filename);
float FlatCorrectionOffset (FlatCorrectionTable *flatcorr, int ID, int X, int Y);

/* skyregion APIs */
int        SkyTableSave        	   PROTO((SkyTable *table, char *filename));
SkyTable  *SkyTableLoad        	   PROTO((char *filename, int VERBOSE));
char      *SkyTableFilename        PROTO((char *catdir));
SkyTable  *SkyTableFromGSC     	   PROTO((char *filename, int depth, int VERBOSE));
SkyTable  *SkyTableLoadOptimal 	   PROTO((char *catdir, char *SKYFILE, char *GSCFILE, int readwrite, int depth, int VERBOSE));
int        SkyTableSetDepth    	   PROTO((SkyTable *sky, int depth));
SkyList   *SkyListMatchList        PROTO((SkyList *inlist, char **cptlist, int Ncptlist));
SkyList   *SkyRegionByIndex        PROTO((SkyTable *table, int index));
SkyList   *SkyRegionByCPT          PROTO((SkyTable *table, char *filename));
SkyList   *SkyRegionByPoint    	   PROTO((SkyTable *table, int depth, double ra, double dec));
SkyList   *SkyListByPoint      	   PROTO((SkyTable *table, double ra, double dec));
SkyList   *SkyListByRadius     	   PROTO((SkyTable *table, int depth, double RA, double DEC, double radius));
SkyList   *SkyListByPatch      	   PROTO((SkyTable *table, int depth, SkyRegion *patch));
SkyList   *SkyListByName      	   PROTO((SkyTable *table, char *name));
SkyList   *SkyListByImage      	   PROTO((SkyTable *table, int depth, Image *image));
SkyList   *SkyListByBounds     	   PROTO((SkyTable *table, int depth, double Rmin, double Rmax, double Dmin, double Dmax));
SkyList   *SkyListChildrenByBounds PROTO((SkyTable *table, int No, int depth, double Rmin, double Rmax, double Dmin, double Dmax));

int        SkyListMerge     	   PROTO((SkyList **outlist, SkyList *newlist));
int        SkyListFree             PROTO((SkyList *list));
int        SkyTableFree            PROTO((SkyTable *table));
int        SkyListSetFilenames     PROTO((SkyList *list, char *path, char *ext));
int        SkyTableSetFilenames    PROTO((SkyTable *sky, char *path, char *ext));

SkyList   *SkyRegionByPoint_List   PROTO((SkyList *inList, int depth, double ra, double dec));
SkyList   *SkyListByBounds_List    PROTO((SkyList *table, int depth, double Rmin, double Rmax, double Dmin, double Dmax));
SkyList   *SkyListChildrenByBounds_List PROTO((SkyList *table, int depth, double Rmin, double Rmax, double Dmin, double Dmax));

/* APIs to split and extend a skytable */
void       SkyTableL5fromL4_List   PROTO((SkyRegion *L4, SkyTable *L5, int Nfirst));
void       SkyTableExtend          PROTO((SkyTable *old, SkyTable *new, int depth));


int set_skyregion(double Rs, double Re, double Ds, double De);
int get_skyregion (double *Rs, double *Re, double *Ds, double *De);

void dvo_set_catdir(char *catdir);
char *dvo_get_catdir();

/* dvo-specific sorting functions */
void sortave (Average *ave, off_t N);
void sort_image_subset (Image *image, off_t *subset, off_t N);
void sort_coords_index (double *X, double *Y, off_t *S, off_t N);
void sort_coords_indexonly (double *X, double *Y, off_t *S, off_t N);
void sort_IDs_indexonly (opihi_int *X, off_t *S, off_t N);
void sort_regions (SkyRegion *region, off_t N);

#ifdef MOVED_TO_LIBOHANA
int  print_error(void);
int  init_error(void);
int  push_error(char *);
#endif

// functions for parallel DVO
int HostTableExists (char *catdir, char *rootname);
HostTable    *HostTableLoad (char *catdir, char *rootname);
int HostTableWaitJobs (HostTable *table, char *file, int lineno);
int HostTableWaitJobsGetIO (HostTable *table, char *file, int lineno, int VERBOSE);
int HostTableTestHost (SkyRegion *region, int hostID);

void InitHost (HostInfo *host);
void FreeHostTable (HostTable *table);
void FreeHostTableGroup (HostTableGroup *table);

HostTableGroup *HostTableGroupsUniqueMachines (HostTable *table, int *ngroups);
HostTableGroup *HostTableGroupsMaxNumber (HostTable *table, int *ngroups, int Nmax);

int HostTableGroupWaitJobsGetIO (HostTableGroup *table, char *file, int lineno, int VERBOSE);

// functions to support tiny versions of Average and Measure
void CopyAverageToTiny (AverageTiny *averageT, Average *average);
void CopyMeasureToTiny (MeasureTiny *measureT, Measure *measure);
int populate_tiny_values (Catalog *catalog, DVOTinyValueMode mode);
int free_tiny_values (Catalog *catalog);

BoundaryTree *BoundaryTreeLoad(char *filename);
BoundaryTree *BoundaryTreeRead(Header *headerPHU, Header *headerZone, FILE *f);
void BoundaryTreeFree(BoundaryTree *tree);

int BoundaryTreeSave(char *filename, BoundaryTree *tree);
int BoundaryTreeWrite(FILE *f, BoundaryTree *tree);

int BoundaryTreeCellCoords (BoundaryTree *tree, int *zone, int *band, double ra, double dec);
int BoundaryTreeProjection (double *x, double *y, double r, double d, BoundaryTree *tree, int zone, int band);

TessellationTable *TessellationTableLoad(char *filename, int *Ntess);
int TessellationTableSave(char *filename, TessellationTable *tess, int Ntess);
int TessellationPrimaryCellIDs (TessellationTable *tess, int Ntess, int *tessID, int *projID, int *skycellID, double ra, double dec);
void TessellationTableInit (TessellationTable *tess, int Ntess);
void TessellationTableFree (TessellationTable *tess, int Ntess);

float dvoOffsetR (Measure *measure, Average *average);
float dvoOffsetD (Measure *measure, Average *average);
double dvoMeanR (float dR, Average *average);
double dvoMeanD (float dD, Average *average);

void dvo_average_init (Average *average);
void dvo_averageT_init (AverageTiny *average);
void dvo_secfilt_init (SecFilt *secfilt, SecFiltInitMode mode);
void dvo_measure_init (Measure *measure);
void dvo_measureT_init (MeasureTiny *measure);

void dvo_lensing_init (Lensing *lensing);
void dvo_lensobj_init (Lensobj *lensobj, int toZero);
void dvo_starpar_init (StarPar *starpar);
void dvo_galphot_init (GalPhot *galphot);

void InitRegionHosts (RegionHostInfo *hosts, int Nhosts, int NHOSTS);
void FreeRegionHosts (RegionHostInfo *hosts, int Nhosts);
void FreeRegionHostTable (RegionHostTable *table);
RegionHostTable *RegionHostTableLoad (char *catdir, char *rootname);
int RegionHostTableWaitJobs (RegionHostTable *regionHosts, char *file, int lineno);
int RegionHostTableWaitJobsGetIO (RegionHostTable *regionHosts, char *file, int lineno, int VERBOSE);
int RegionHostFindNeighbors (RegionHostTable *table, int Nhost);

// galaxy_model:
int TransformProperMotion_radians (double *uR, double *uD, double uL, double uB, double Rrad, double Drad, CoordTransform *transform);
int TransformProperMotionForewards (double *uL, double *uB, double uR, double uD, double R, double D, CoordTransform *transform);
int TransformProperMotionBackwards (double *uR, double *uD, double uL, double uB, double R, double D, CoordTransform *transform);
int SolarMotionModel_radians (double *uL_sol, double *uB_sol, double Lrad, double Brad, double distance);
int SolarMotionModel (double *uL_sol, double *uB_sol, double L, double B, double distance);
int GalaxyMotionModel_radians (double *uL_gal, double *uB_gal, double Lrad, double Brad);
int GalaxyMotionModel (double *uL_gal, double *uB_gal, double L, double B);
int InitGalaxyModel (char *version);

# define LENSFIELD(NAME) float LensValue_##NAME (PhotCode *code, Lensobj *lensobj, int Nlensobj);

LENSFIELD(X11_sm_obj)
LENSFIELD(X12_sm_obj)
LENSFIELD(X22_sm_obj)
LENSFIELD(E1_sm_obj)
LENSFIELD(E2_sm_obj)

LENSFIELD(X11_sh_obj)
LENSFIELD(X12_sh_obj)
LENSFIELD(X22_sh_obj)
LENSFIELD(E1_sh_obj)
LENSFIELD(E2_sh_obj)

LENSFIELD(X11_sm_psf)
LENSFIELD(X12_sm_psf)
LENSFIELD(X22_sm_psf)
LENSFIELD(E1_sm_psf)
LENSFIELD(E2_sm_psf)

LENSFIELD(X11_sh_psf)
LENSFIELD(X12_sh_psf)
LENSFIELD(X22_sh_psf)
LENSFIELD(E1_sh_psf)
LENSFIELD(E2_sh_psf)

LENSFIELD( F_ApR5)
LENSFIELD(dF_ApR5)
LENSFIELD(sF_ApR5)
LENSFIELD(fF_ApR5)

LENSFIELD( F_ApR6)
LENSFIELD(dF_ApR6)
LENSFIELD(sF_ApR6)
LENSFIELD(fF_ApR6)

LENSFIELD( F_ApR7)
LENSFIELD(dF_ApR7)
LENSFIELD(sF_ApR7)
LENSFIELD(fF_ApR7)

LENSFIELD(E1)
LENSFIELD(E2)

# undef LENSFIELD

# define GALPHOT_FIELD(NAME, VALUE, TYPE, DEFAULT) TYPE GalphotValue_##NAME (PhotCode *code, dvoMagClassType class, GalPhot *galphot, int Ngalphot);

//GALPHOT_FIELD(GAL_MAG, 	       mag);	       
//GALPHOT_FIELD(GAL_MAG_ERR,     magErr);    
//GALPHOT_FIELD(GAL_MAJ, 	       majorAxis);	       
//GALPHOT_FIELD(GAL_MAJ_ERR,     majorAxisErr);
//GALPHOT_FIELD(GAL_MIN, 	       minorAxis);	       
//GALPHOT_FIELD(GAL_MIN_ERR,     minorAxisErr);    
//GALPHOT_FIELD(GAL_THETA,       theta);      
//GALPHOT_FIELD(GAL_THETA_ERR,   thetaErr);  
//GALPHOT_FIELD(GAL_INDEX,       index);      
//GALPHOT_FIELD(GAL_CHISQ,       chisq);      
//GALPHOT_FIELD(GAL_NPIX,        Npix);       
//GALPHOT_FIELD(GAL_FLAGS,       flags);       
//GALPHOT_FIELD(GAL_TYPE,        modelType);       

GALPHOT_FIELD(GAL_MAG,         mag,          float, NAN) 
GALPHOT_FIELD(GAL_MAG_ERR,     magErr,       float, NAN) 
GALPHOT_FIELD(GAL_MAJ, 	       majorAxis,    float, NAN) 
GALPHOT_FIELD(GAL_MAJ_ERR,     majorAxisErr, float, NAN)
GALPHOT_FIELD(GAL_MIN, 	       minorAxis,    float, NAN) 
GALPHOT_FIELD(GAL_MIN_ERR,     minorAxisErr, float, NAN) 
GALPHOT_FIELD(GAL_THETA,       theta,        float, NAN) 
GALPHOT_FIELD(GAL_THETA_ERR,   thetaErr,     float, NAN) 
GALPHOT_FIELD(GAL_INDEX,       index,        float, NAN)      
GALPHOT_FIELD(GAL_CHISQ,       chisq,        float, NAN)      
GALPHOT_FIELD(GAL_NPIX,        Npix,         float, NAN)       

GALPHOT_FIELD(GAL_TYPE,        modelType,    short, 0)       

GALPHOT_FIELD(GAL_FLAGS,       flags,        unsigned int, 0)       
GALPHOT_FIELD(GAL_OBJ_ID,      objID,        unsigned int, 0)       
GALPHOT_FIELD(GAL_CAT_ID,      catID,        unsigned int, 0)       
GALPHOT_FIELD(GAL_DET_ID,      detID,        unsigned int, 0)       
GALPHOT_FIELD(GAL_IMAGE_ID,    imageID,      unsigned int, 0)       

# undef GALPHOT_FIELD

# endif // DVO_H
