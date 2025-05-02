# ifndef DVODB_H
# define DVODB_H

# define MEASURE_HAS_XCCD 1

// Some values used by code moved to libdvo from opihi.
typedef enum {OPIHI_NOTYPE, OPIHI_FLT, OPIHI_INT, OPIHI_STR} opihiVectorTypes;

# define opihi_flt double
# define opihi_int long long int
# define OPIHI_INT_FMT "%lld"

// # define opihi_int int64_t

typedef enum {
  DB_STACK_NONE  = 0,
  DB_STACK_INT   = 0x01,
  DB_STACK_FIELD = 0x02,
  DB_STACK_TEMP  = 0x04,
  DB_STACK_VALUE = 0x08,
  DB_STACK_CLOSE_PAR = 0x10,
  DB_STACK_OPEN_PAR,
  DB_STACK_LOGIC,
  DB_STACK_COMPARE,
  DB_STACK_BITWISE,
  DB_STACK_SUM,
  DB_STACK_MULTIPLY,
  DB_STACK_POWER,
  DB_STACK_UNARY,
} dbStackTypes;

/* magnitude types */

typedef enum {
  MAG_SRC_NONE,
  MAG_SRC_CHP,
  MAG_SRC_WRP,
  MAG_SRC_STK,
} dvoMagSourceType;

typedef enum {
  MAG_LEVEL_NONE,	// -2.5*log(DN) [ie, not DN/sec]
  MAG_LEVEL_INST,		// -2.5*log(DN) [ie, not DN/sec]
  MAG_LEVEL_CAT,		// MAG_INST + 2.5*log(exptime) + C_lambda + K_lambda*(airmass - 1)
  MAG_LEVEL_SYS, // MAG_CAT + \sum_i A_i * (color_i - color_o) [color correction for measure photcode]
  MAG_LEVEL_REL, // MAG_SYS - Mcal [specific zero point for image]
  MAG_LEVEL_CAL, // MAG_REL + \sum_i A_i * (color_i - color_o) [color correction for average photcode]
  MAG_LEVEL_AVE, 
  MAG_LEVEL_REF, 
} dvoMagLevelType;

typedef enum {
  MAG_OPTION_NONE,
  MAG_OPTION_MAG,
  MAG_OPTION_ERR,
  MAG_OPTION_FLUX,
  MAG_OPTION_FLUX_ERR,
  MAG_OPTION_STDEV,
  MAG_OPTION_CHISQ,
  MAG_OPTION_MIN,
  MAG_OPTION_MAX,
  MAG_OPTION_NCODE,
  MAG_OPTION_NPHOT, // Nused
  MAG_OPTION_NWARP,
  MAG_OPTION_NWARP_GOOD,
  MAG_OPTION_NSTACK,
  MAG_OPTION_NSTACK_DET,
  MAG_OPTION_UC_DIST, 
  MAG_OPTION_FLAGS, 
  MAG_OPTION_PSF_QF, 
  MAG_OPTION_PSF_QF_PERFECT, 

  MAG_OPTION_X11_SM_OBJ, 
  MAG_OPTION_X12_SM_OBJ, 
  MAG_OPTION_X22_SM_OBJ, 
  MAG_OPTION_E1_SM_OBJ, 
  MAG_OPTION_E2_SM_OBJ, 

  MAG_OPTION_X11_SH_OBJ, 
  MAG_OPTION_X12_SH_OBJ, 
  MAG_OPTION_X22_SH_OBJ, 
  MAG_OPTION_E1_SH_OBJ, 
  MAG_OPTION_E2_SH_OBJ, 

  MAG_OPTION_X11_SM_PSF, 
  MAG_OPTION_X12_SM_PSF, 
  MAG_OPTION_X22_SM_PSF, 
  MAG_OPTION_E1_SM_PSF, 
  MAG_OPTION_E2_SM_PSF, 

  MAG_OPTION_X11_SH_PSF, 
  MAG_OPTION_X12_SH_PSF, 
  MAG_OPTION_X22_SH_PSF, 
  MAG_OPTION_E1_SH_PSF, 
  MAG_OPTION_E2_SH_PSF, 

  MAG_OPTION_E1_PSF, 
  MAG_OPTION_E2_PSF, 

  MAG_OPTION_F_AP_R5, 
  MAG_OPTION_F_ERR_AP_R5, 
  MAG_OPTION_F_STDEV_AP_R5, 
  MAG_OPTION_F_FILL_AP_R5, 

  MAG_OPTION_F_AP_R6, 
  MAG_OPTION_F_ERR_AP_R6, 
  MAG_OPTION_F_STDEV_AP_R6, 
  MAG_OPTION_F_FILL_AP_R6, 

  MAG_OPTION_F_AP_R7, 
  MAG_OPTION_F_ERR_AP_R7, 
  MAG_OPTION_F_STDEV_AP_R7, 
  MAG_OPTION_F_FILL_AP_R7, 

  MAG_OPTION_E1, 
  MAG_OPTION_E2, 

  MAG_OPTION_GAL_MAG,
  MAG_OPTION_GAL_MAG_ERR,
  MAG_OPTION_GAL_MAJ,
  MAG_OPTION_GAL_MAJ_ERR,
  MAG_OPTION_GAL_MIN,
  MAG_OPTION_GAL_MIN_ERR,
  MAG_OPTION_GAL_THETA,
  MAG_OPTION_GAL_THETA_ERR,
  MAG_OPTION_GAL_INDEX,
  MAG_OPTION_GAL_CHISQ,
  MAG_OPTION_GAL_NPIX,
  MAG_OPTION_GAL_FLAGS,        
  MAG_OPTION_GAL_TYPE,        

  MAG_OPTION_GAL_OBJ_ID,  
  MAG_OPTION_GAL_CAT_ID,  
  MAG_OPTION_GAL_DET_ID, 
  MAG_OPTION_GAL_IMAGE_ID,
} dvoMagOptionType;

//  MAG_OPTION_STACK_PRIMARY_OFF, 
//  MAG_OPTION_STACK_BEST_OFF, 

typedef enum {
  MAG_CLASS_NONE,
  MAG_CLASS_PSF,
  MAG_CLASS_KRON,
  MAG_CLASS_APER,
  MAG_CLASS_DEV, // DeVaucouleur Model (only for galphot)
  MAG_CLASS_EXP, // Exponential Model (only for galphot)
  MAG_CLASS_SER, // Sersic Model (only for galphot)
} dvoMagClassType;

/* measure fields */
typedef enum {MEAS_ZERO, 
      MEAS_GLON, 
      MEAS_GLAT, 
      MEAS_GLON_AVE, 
      MEAS_GLAT_AVE,
      MEAS_ELON, 
      MEAS_ELAT, 
      MEAS_ELON_AVE, 
      MEAS_ELAT_AVE,
      MEAS_RA, 
      MEAS_DEC, 
      MEAS_RA_AVE, 
      MEAS_DEC_AVE,
      MEAS_RA_AVE_ERR, 
      MEAS_DEC_AVE_ERR, 
      MEAS_U_RA, 
      MEAS_U_DEC, 
      MEAS_U_RA_ERR, 
      MEAS_U_DEC_ERR, 
      MEAS_PAR, 
      MEAS_PAR_ERR, 
      MEAS_RA_OFFSET, 
      MEAS_DEC_OFFSET, 
      MEAS_RA_FIT_OFFSET, 
      MEAS_DEC_FIT_OFFSET, 
      MEAS_RA_OFFSET_ERR, 
      MEAS_DEC_OFFSET_ERR, 
      MEAS_CHISQ_POS, 
      MEAS_CHISQ_PM,  
      MEAS_CHISQ_PAR, 
      MEAS_TMEAN, 
      MEAS_TRANGE, 
      MEAS_NMEAS, 
      MEAS_NMISS, 
      MEAS_NPOS, 
      MEAS_OBJ_FLAGS, 
      MEAS_SECFILT_FLAGS, 
      MEAS_PHOT, // photometry class of measurements
      MEAS_MINST, 
      MEAS_MCAT, 
      MEAS_MSYS, 
      MEAS_MREL, 
      MEAS_MCAL, 
      MEAS_EXPTIME, 
      MEAS_AIRMASS, 
      MEAS_MEAN_AIRMASS, 
      MEAS_ALT, 
      MEAS_AZ, 
      MEAS_PHOTCODE, 
      MEAS_PHOTCODE_EQUIV, 
      MEAS_PHOTCODE_KLAM, 
      MEAS_PHOTCODE_C, 
      MEAS_TIME, 
      MEAS_FWHM, 
      MEAS_FWHM_MAJ, 
      MEAS_FWHM_MIN, 
      MEAS_THETA, 
      MEAS_POSANGLE, 
      MEAS_PLATESCALE, 
      MEAS_MXX, 
      MEAS_MXY, 
      MEAS_MYY, 
      MEAS_DOPHOT, 
      MEAS_DB_FLAGS, 
      MEAS_PHOT_FLAGS, 
      MEAS_PHOT_FLAGS2, 
      MEAS_XCCD, 
      MEAS_YCCD, 
      MEAS_XCCD_ERR, 
      MEAS_YCCD_ERR, 
      MEAS_XFIX, 
      MEAS_YFIX, 
      MEAS_XOFF_KH, 
      MEAS_YOFF_KH, 
      MEAS_XOFF_DCR, 
      MEAS_YOFF_DCR, 
      MEAS_XOFF_CAM, 
      MEAS_YOFF_CAM, 
      MEAS_ROFF_GAL, 
      MEAS_DOFF_GAL, 
      MEAS_POS_SYS_ERR, 
      MEAS_XFIELD, 
      MEAS_YFIELD, 
      MEAS_XMOSAIC, 
      MEAS_YMOSAIC, 
      MEAS_SKY, 
      MEAS_dSKY, 
      MEAS_DET_ID, 
      MEAS_OBJ_ID, 
      MEAS_CAT_ID, 
      MEAS_EXT_ID, 
      MEAS_IMAGE_ID, 
      MEAS_PSF_QF, 
      MEAS_PSF_QF_PERFECT, 
      MEAS_PSF_CHISQ, 
      MEAS_PSF_NDOF,
      MEAS_PSF_NPIX,
      MEAS_CR_NSIGMA, 
      MEAS_EXT_NSIGMA, 
      MEAS_IMAGE_EXTERN_ID, // return image.externID
      MEAS_EXPNAME_AS_INT,
      MEAS_MCAL_OFFSET_PSF, // make this a dvoMagOption?
      MEAS_MCAL_OFFSET_APER, // make this a dvoMagOption?
      MEAS_FLAT,
      MEAS_CENTER_OFFSET,
      MEAS_REF_COLOR_BLUE,
      MEAS_REF_COLOR_RED,

      MEAS_X11_SM_OBJ, 
      MEAS_X12_SM_OBJ, 
      MEAS_X22_SM_OBJ, 
      MEAS_E1_SM_OBJ, 
      MEAS_E2_SM_OBJ, 
      MEAS_X11_SH_OBJ, 
      MEAS_X12_SH_OBJ, 
      MEAS_X22_SH_OBJ, 
      MEAS_E1_SH_OBJ, 
      MEAS_E2_SH_OBJ, 
      MEAS_X11_SM_PSF, 
      MEAS_X12_SM_PSF, 
      MEAS_X22_SM_PSF, 
      MEAS_E1_SM_PSF, 
      MEAS_E2_SM_PSF, 
      MEAS_X11_SH_PSF, 
      MEAS_X12_SH_PSF, 
      MEAS_X22_SH_PSF, 
      MEAS_E1_SH_PSF, 
      MEAS_E2_SH_PSF, 

      MEAS_E1_PSF, 
      MEAS_E2_PSF, 

      MEAS_F_AP_R5, 
      MEAS_F_ERR_AP_R5, 
      MEAS_F_STDEV_AP_R5, 
      MEAS_F_FILL_AP_R5, 

      MEAS_F_AP_R6, 
      MEAS_F_ERR_AP_R6, 
      MEAS_F_STDEV_AP_R6, 
      MEAS_F_FILL_AP_R6, 

      MEAS_F_AP_R7, 
      MEAS_F_ERR_AP_R7, 
      MEAS_F_STDEV_AP_R7, 
      MEAS_F_FILL_AP_R7, 

      MEAS_E_BV,		      // extinction (mags)
      MEAS_E_BV_ERR,
      MEAS_DISTANCE_MOD, // distance modulus (mags)
      MEAS_DISTANCE_MOD_ERR,
      MEAS_M_R,		      // absolute mag in r-band
      MEAS_M_R_ERR,
      MEAS_FEH,		      // metallicity
      MEAS_FEH_ERR,
      MEAS_URA_GALMODEL,      // model pm prediction
      MEAS_UDEC_GALMODEL,     // model pm prediction
      MEAS_RA_GALMODEL,      // model pm prediction
      MEAS_DEC_GALMODEL,     // model pm prediction
} dvoMeasureType;

/* average fields */
typedef enum {AVE_ZERO, 
      AVE_RA, 
      AVE_DEC, 
      AVE_RA_ERR, 
      AVE_DEC_ERR, 
      AVE_GLON, 
      AVE_GLAT, 
      AVE_ELON, 
      AVE_ELAT, 
      AVE_U_RA, 
      AVE_U_DEC, 
      AVE_U_RA_ERR, 
      AVE_U_DEC_ERR, 
      AVE_PAR, 
      AVE_PAR_ERR, 
      AVE_CHISQ_POS, 
      AVE_CHISQ_PM, 
      AVE_CHISQ_PAR, 
      AVE_TMEAN,
      AVE_TRANGE, 
      AVE_PSF_QF,
      AVE_PSF_QF_PERF,
      AVE_STARGAL,
      AVE_Xp, 
      AVE_NMEAS, 
      AVE_NMISS, 
      AVE_NLENSING, 
      AVE_NLENSOBJ, 
      AVE_NSTARPAR, 
      AVE_NGALPHOT, 
      AVE_NPOS, 
      AVE_NWARP_OK, 
      AVE_OBJ_FLAGS, 
      AVE_TYPE, 
      AVE_TYPEFRAC,
      AVE_OBJID,
      AVE_CATID,
      AVE_EXTID_HI,
      AVE_EXTID_LO,
      AVE_PHOT_FLAGS_HI,
      AVE_PHOT_FLAGS_LO,
      AVE_REF_COLOR_BLUE,
      AVE_REF_COLOR_RED,
      AVE_PHOT, // photometry class of values
      AVE_E_BV,		      // extinction (mags)
      AVE_E_BV_ERR,
      AVE_DISTANCE_MOD, // distance modulus (mags)
      AVE_DISTANCE_MOD_ERR,
      AVE_M_R,		      // absolute mag in r-band
      AVE_M_R_ERR,
      AVE_FEH,		      // metallicity
      AVE_FEH_ERR,
      AVE_URA_GALMODEL,      // model pm prediction
      AVE_UDEC_GALMODEL,     // model pm prediction
      AVE_RA_GALMODEL,      // model pm prediction
      AVE_DEC_GALMODEL,     // model pm prediction
} dvoAverageType;

//       AVE_NPHOT, 
//       AVE_NCODE, 
//       AVE_MAG, 
//       AVE_dMAG, 
//       AVE_Xm, 

typedef enum {IMAGE_ZERO, 
      IMAGE_RA, 
      IMAGE_DEC, 
      IMAGE_GLON, 
      IMAGE_GLAT, 
      IMAGE_ELON, 
      IMAGE_ELAT, 
      IMAGE_XM, 
      IMAGE_AIRMASS, 
      IMAGE_MCAL_PSF, 
      IMAGE_MCAL_APER, 
      IMAGE_dMCAL, 
      IMAGE_PHOTCODE, 
      IMAGE_TIME, 
      IMAGE_FWHM, 
      IMAGE_FWHM_MEDIAN, 
      IMAGE_EXPTIME, 
      IMAGE_EXPNAME_AS_INT,
      IMAGE_NSTAR, 
      IMAGE_NCAL, 
      IMAGE_SKY, 
      IMAGE_FLAGS, 
      IMAGE_CCDNUM, 
      IMAGE_NX_PIX, 
      IMAGE_NY_PIX, 
      IMAGE_THETA, 
      IMAGE_SKEW, 
      IMAGE_SCALE, 
      IMAGE_DSCALE, 
      IMAGE_APRESID,
      IMAGE_DAPRESID,
      IMAGE_SIDTIME,
      IMAGE_LATITUDE,
      IMAGE_DET_LIMIT,
      IMAGE_SAT_LIMIT,
      IMAGE_CERROR,
      IMAGE_FWHM_MAJ,
      IMAGE_FWHM_MIN,
      IMAGE_FWHM_MAJ_MEDIAN,
      IMAGE_FWHM_MIN_MEDIAN,
      IMAGE_TRATE,
      IMAGE_IMAGE_ID,
      IMAGE_EXTERN_ID,
      IMAGE_SOURCE_ID,
      IMAGE_X_LL_CHIP,
      IMAGE_X_LR_CHIP,
      IMAGE_X_UL_CHIP,
      IMAGE_X_UR_CHIP,
      IMAGE_Y_LL_CHIP,
      IMAGE_Y_LR_CHIP,
      IMAGE_Y_UL_CHIP,
      IMAGE_Y_UR_CHIP,
      IMAGE_X_LL_FP,
      IMAGE_X_LR_FP,
      IMAGE_X_UL_FP,
      IMAGE_X_UR_FP,
      IMAGE_Y_LL_FP,
      IMAGE_Y_LR_FP,
      IMAGE_Y_UL_FP,
      IMAGE_Y_UR_FP,
      IMAGE_R_LL,
      IMAGE_R_LR,
      IMAGE_R_UL,
      IMAGE_R_UR,
      IMAGE_D_LL,
      IMAGE_D_LR,
      IMAGE_D_UL,
      IMAGE_D_UR,
      IMAGE_X_ERR_SYS,
      IMAGE_Y_ERR_SYS,
      IMAGE_MAG_ERR_SYS,
      IMAGE_UBERCAL_DIST,
      IMAGE_NFIT_PHOTOM,
      IMAGE_NFIT_ASTROM,
      IMAGE_NLINK_PHOTOM,
      IMAGE_NLINK_ASTROM,
      IMAGE_REF_COLOR_BLUE,
      IMAGE_REF_COLOR_RED
} dvoImageType;

enum {DVO_TABLE_AVERAGE, DVO_TABLE_MEASURE, DVO_TABLE_IMAGE};
enum {DVO_DB_CMDLINE_ERROR, DVO_DB_CMDLINE_IS_END, DVO_DB_CMDLINE_IS_WHERE, DVO_DB_CMDLINE_IS_MATCH}; 

// options for selecting the ra,dec limits of the db selections
typedef struct {
  char *name;
  char *list;
  int useDisplay;
  int useSkyregion;
} SkyRegionSelection;

// a single db field 
typedef struct {
  char *name;
  int extract;
  int table;
  int ID; // may be either dvoMeasureType or dvoAverageType

  dvoMagSourceType magSource; // chip, (forced) warp, stack [only relevant for averages]
  dvoMagLevelType magLevel;  // inst, cat, sys, rel, ave, ref, err, min, max, stdev, nphot, chisq, 
  dvoMagOptionType magOption;  // psf, kron, aper
  dvoMagClassType magClass;

  char type;
  PhotCode *photcode;
} dbField;

// db boolean operations
typedef struct {
  char   *name;
  char    type;
  int     field;
  opihi_flt FltValue;
  opihi_int IntValue;
  // double FltValue;
  // int IntValue;
} dbStack;

typedef struct {
  opihi_flt Flt;
  opihi_int Int;
  // double Flt;
  // int Int;
} dbValue;

typedef struct {
  double crval1;
  double crval2;
  float theta;
  unsigned int imageID;
  unsigned int externID;
  unsigned int expname;
  float Mcal;
  float secz;
  float Xcenter;
  float Ycenter;
} ImageMetadata;

Image        *LoadImagesDVO         PROTO((off_t *Nimage));
void          FreeImagesDVO         PROTO((Image *images));
Image        *MatchImageDVO         PROTO((unsigned int time, short int source, unsigned int imageID));
Coords       *MatchMosaic           PROTO((unsigned int time, short int source));
int           GetTimeSelection      PROTO((time_t *tz, time_t *te));
int           GetTimeFormat         PROTO((time_t *TimeReference, int *TimeFormat));
double        TimeValue             PROTO((time_t time, time_t TimeReference, int TimeFormat));
double        GetTimeRange          PROTO((time_t time, int TimeFormat));

void          image_subset          PROTO((Image *image, off_t Nimage, off_t **Subset, off_t *Nsubset, SkyRegionSelection *selection, e_time tzero, double trange, int TimeSelect));
off_t         match_image_subset    PROTO((Image *image, off_t *subset, off_t Nsubset, e_time T, short int S));

// dvo DB field functions
dbField     *dbCmdlineFields        PROTO((int argc, char **argv, int table, int *last, int *nfields));
int          dbCmdlineConditions    PROTO((int argc, char **argv, int first, int *nextField));
dbStack     *dbRPN                  PROTO((int argc, char **argv, int *nstack));
int          dbCheckStack           PROTO((dbStack *stack, int Nstack, int table, dbField **inFields, int *Nfields));
int          dbBooleanCond          PROTO((dbStack *inStack, int NinStack, dbValue *fields));
void         dbInitStack            PROTO((dbStack *stack));
void 	     dbFreeStack            PROTO((dbStack *stack, int Nstack));
void 	     dbFreeEntry            PROTO((dbStack *stack));
void         dbFreeTempEntry        PROTO((dbStack *stack));

dbStack     *dbBinary               PROTO((dbStack *V1, dbStack *V2, char *op, dbValue *fields));
dbStack     *dbUnary                PROTO((dbStack *V1, char *op, dbValue *fields));

int          GetMagMode             PROTO((char *string));
int          ParsePhotcodeField     PROTO((dbField *field, char *fieldName, int fieldID));
int          ParseMeasureField      PROTO((dbField *field, char *fieldName));
int          ParseAverageField      PROTO((dbField *field, char *fieldName));
int          ParseImageField        PROTO((dbField *field, char *fieldName));

dbValue      dbExtractAverages      PROTO((Average *average, SecFilt *secfilt, Measure *measure, Lensobj *lensobj, StarPar *starpar, GalPhot *galphot, dbField *field));
dbValue      dbExtractMeasures      PROTO((Average *average, SecFilt *secfilt, Measure *measure, Lensing *lensing, StarPar *starpar, dbField *field));
dbValue      dbExtractImages        PROTO((Image *image, off_t Nimage, off_t N, dbField *field));

void 	     dbInitField            PROTO((dbField *field));
void 	     dbFreeFields           PROTO((dbField *fields, int Nfields));
int          dbAstroRegionLimits    PROTO((dbStack **stack, int *nstack, SkyRegionSelection *selection, int table));
char        *strfloat               PROTO((float value));

int dbFieldNeedMeasure (dbField *fields, int Nfields);
int dbFieldNeedLensobj (dbField *fields, int Nfields);
int dbFieldNeedLensing (dbField *fields, int Nfields);
int dbFieldNeedStarpar (dbField *fields, int Nfields, int isAverage);
int dbFieldNeedGalphot (dbField *fields, int Nfields);

void FreeImageSelection (void);

void FreeSkyRegionSelection (SkyRegionSelection *selection);
int wordhash (char *word);

int dbExtractMeasuresInitTransform (CoordTransformSystem target);
int dbExtractMeasuresInitAve (void);
int dbExtractMeasuresInitMeas (void);
int dbExtractMeasuresInit (int isRemoteClient);

int dbExtractAveragesInitTransform (CoordTransformSystem target);
int dbExtractAveragesInitAve (void);
int dbExtractAveragesInit (void);

int dbExtractImagesInitTransform (CoordTransformSystem target);
int dbExtractImagesInit (void);
int dbExtractImagesReset (void);

#include "get_graphdata.h"

ImageMetadata *ImageMetadataLoad(char *filename, off_t *nimage);
int ImageMetadataSave(char *filename, Image *image, off_t Nimage);

int SetImageMetadataSelection (char *filename);
void FreeImageMetadataSelection ();
ImageMetadata *MatchImageMetadataDVO (unsigned int imageID);
Coords *MatchMosaicMetadata (unsigned int imageID);
Coords *MatchFieldMetadata (unsigned int imageID);
off_t match_image_by_ID (ImageMetadata *image, off_t Nimage, unsigned int ID);
void sort_image_metadata (ImageMetadata *image, off_t Nimage);

# endif
