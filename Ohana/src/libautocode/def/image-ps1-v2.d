STRUCT       Image_PS1_V2
EXTNAME      DVO_IMAGE_PS1_V2
TYPE         BINTABLE
SIZE         360
DESCRIPTION  DVO Image Table 

# elements of the image structure

SUBSTRUCT coords,           COORDS,               CoordsDisk,    astrometric data
SUBFIELD  crval1,           CRVAL1,               double,   	 coordinate at reference pixel
SUBFIELD  crval2,           CRVAL2,               double,  	 coordinate at reference pixel
SUBFIELD  crpix1,           CRPIX1,               float,   	 coordinate of reference pixel
SUBFIELD  crpix2,           CRPIX2,               float,   	 coordinate of reference pixel
SUBFIELD  cdelt1,           CDELT1,               float,   	 degrees per pixel
SUBFIELD  cdelt2,           CDELT2,               float,    	 degrees per pixel
SUBFIELD  pc1_1,            PC1_1,                float,    	 rotation matrix
SUBFIELD  pc1_2,            PC1_2,                float,    	 rotation matrix
SUBFIELD  pc2_1,            PC2_1,                float,    	 rotation matrix
SUBFIELD  pc2_2,            PC2_2,                float,    	 rotation matrix
SUBFIELD  polyterms,        POLYTERMS,            float[7][2],	 higher order warping terms
SUBFIELD  ctype,            CTYPE,                char[15],      coordinate type
SUBFIELD  Npolyterms,       NPOLYTERMS,           char,     	 order of polynomial
# 120 bytes

FIELD 	  tzero,            TZERO,                e_time,         readout time (row 0)
FIELD 	  nstar,            NSTAR,                unsigned int,   number of stars on image
FIELD 	  secz,             SECZ,                 float,      	  airmass,                   mag
FIELD 	  NX,               NX,                   unsigned short, image width
FIELD 	  NY,               NY,                   unsigned short, image height
FIELD 	  apmifit,          APMIFIT,              float,      	  aperture correction,       mag
FIELD 	  dapmifit,         DAPMIFIT,             float,      	  apmifit error,             mag
FIELD 	  Mcal,             MCAL,                 float,      	  calibration mag,           mag
FIELD 	  dMcal,            DMCAL,                float,      	  error on Mcal,             mag
FIELD 	  Xm,               XM,                   short,      	  image chisq,               10*log(value)
FIELD 	  photcode,         PHOTCODE,             short,      	  identifier for CCD,
FIELD 	  exptime,          EXPTIME,              float,          exposure time,             seconds
FIELD     sidtime,          ST,			  float,          sidereal time of exposure
FIELD     latitude,         LAT,		  float,          observatory latitude,      degrees
# 40 bytes

FIELD     RAo,              RA_CENTER,            float,          image center,              degrees
FIELD     DECo,             DEC_CENTER,           float,          image center,              degrees
FIELD     Radius,           RADIUS,               float,          image radius,              degrees
FIELD     refColor,         REF_COLOR,            float,          dummy

# should we define the max length of name as a macro?
FIELD 	  name,             NAME,                 char[121],      name of original image 
FIELD 	  detection_limit,  DETECTION_LIMIT,      unsigned char,  detection limit,           10*mag
FIELD 	  saturation_limit, SATURATION_LIMIT,     unsigned char,  saturation limit,          10*mag
FIELD 	  cerror,           CERROR,               unsigned char,  astrometric error,         50*arcsec
FIELD 	  fwhm_x,           FWHM_X,               unsigned char,  PSF x width,               25*arcsec
FIELD 	  fwhm_y,           FWHM_Y,               unsigned char,  PSF y width,               25*arcsec
FIELD 	  trate,            TRATE,                unsigned char,  scan rate,                 100 usec/pixel
FIELD 	  ccdnum,           CCDNUM,               unsigned char,  CCD ID number
FIELD 	  flags,            FLAGS,                unsigned int,   image quality flags
FIELD 	  imageID,          IMAGE_ID,             unsigned int,   internal image ID
FIELD 	  parentID,         PARENT_ID,            unsigned int,   associated ref image
FIELD 	  externID,         EXTERN_ID,            unsigned int,   external image ID
FIELD 	  sourceID,         SOURCE_ID,            unsigned short, analysis source ID
# 48 bytes 

FIELD 	  nLinkAstrom,      NLINK_ASTROM,         short,      	  mean number of matched measurements for astrometry
FIELD 	  nLinkPhotom,      NLINK_PHOTOM,         short,      	  mean number of matched measurements for astrometry
FIELD 	  dummy3,           DUMMY3,               short,      	  place holder for byte boundaries

FIELD 	  dXpixSys,         XPIX_SYS_ERR,         float,      	  systematic astrometry error in X
FIELD 	  dYpixSys,         YPIX_SYS_ERR,         float,      	  systematic astrometry error in Y

FIELD 	  dMagSys,          MAG_SYS_ERR,          float,      	  systematic photometry error
FIELD 	  nFitAstrom,       N_FIT_ASTROM,         short,      	  number of stars used for astrometry cal
FIELD 	  nFitPhotom,       N_FIT_PHOTOM,         short,      	  number of stars used for photometry cal

FIELD 	  photom_map_id,    PHOTOM_MAP_ID,        unsigned int,   reference to 2D zero point map
FIELD 	  astrom_map_id,    ASTROM_MAP_ID,        unsigned int,   reference to 2D astrometry map
# nFitPhotom lands on the old location of Mxxxx, which was used to mean nFitPhotom in some cases
