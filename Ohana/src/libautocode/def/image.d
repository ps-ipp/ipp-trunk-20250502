STRUCT       Image
EXTNAME      DVO_IMAGE
TYPE         BINTABLE
SIZE         392
DESCRIPTION  DVO Image Table 

## XXX NOTE: as of 2015.01.11 (r 37807), I had to move this structure from the autocode
## version to libdvo/include/libdvo_astro.h to support self-references 

# elements of the image structure
# careful of 8-byte boundaries

# Coords is an internal structure defined in def/common.h
SUBSTRUCT coords,           COORDS,               Coords,        astrometric data
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
SUBFIELD  mosaic,           MOSAIC,               e_void,     	 pointer to parent mosaic
SUBFIELD  imageMap,         IMAGE_MAP,            e_void,     	 pointer to image map
# 136 bytes

# change this to a double?
FIELD 	  tzero,            TZERO,                e_time,         readout time (row 0)
FIELD 	  nstar,            NSTAR,                unsigned int,   number of stars on image
FIELD 	  secz,             SECZ,                 float,      	  airmass,                   mag
FIELD 	  NX,               NX,                   unsigned short, image width
FIELD 	  NY,               NY,                   unsigned short, image height
FIELD 	  apmifit,          APMIFIT,              float,      	  aperture correction,       mag
FIELD 	  dapmifit,         DAPMIFIT,             float,      	  apmifit error,             mag
FIELD 	  McalPSF,          MCAL_PSF,             float,      	  calibration mag for PSF, mag
FIELD 	  McalAPER,         MCAL_APER,            float,      	  calibration mag for Aperture, mag
FIELD 	  dMcal,            DMCAL,                float,      	  error on Mcal,             mag
FIELD 	  McalChiSq,        XM,                   float,      	  image chisq
FIELD 	  padding_1,        PADDING,              short,      	  identifier for CCD,
FIELD 	  photcode,         PHOTCODE,             short,      	  identifier for CCD,
FIELD 	  exptime,          EXPTIME,              float,          exposure time,             seconds
FIELD     sidtime,          ST,			  float,          sidereal time of exposure
FIELD     latitude,         LAT,		  float,          observatory latitude,      degrees
# 56 bytes

FIELD     RAo,              RA_CENTER,            float,          image center,              degrees
FIELD     DECo,             DEC_CENTER,           float,          image center,              degrees
FIELD     Radius,           RADIUS,               float,          image radius,              degrees
FIELD     refColorBlue,     REF_COLOR_BLUE,       float,          median astrometry ref color
FIELD     refColorRed,      REF_COLOR_RED,        float,          median astrometry ref color

# should we define the max length of name as a macro?
FIELD 	  name,             NAME,                 char[117],      name of original image 
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
FIELD 	  ubercalDist,      UBERCAL_DIST,         short,      	  distance to nearest ubercal image

FIELD 	  dXpixSys,         XPIX_SYS_ERR,         float,      	  systematic astrometry error in X
FIELD 	  dYpixSys,         YPIX_SYS_ERR,         float,      	  systematic astrometry error in Y

FIELD 	  dMagSys,          MAG_SYS_ERR,          float,      	  systematic photometry error
FIELD 	  nFitAstrom,       N_FIT_ASTROM,         unsigned short, number of stars used for astrometry cal
FIELD 	  nFitPhotom,       N_FIT_PHOTOM,         unsigned short, number of stars used for photometry cal

FIELD 	  photom_map_id,    PHOTOM_MAP_ID,        unsigned int,   reference to 2D zero point map
FIELD 	  astrom_map_id,    ASTROM_MAP_ID,        unsigned int,   reference to 2D astrometry map

FIELD    *parent,           PARENT,               e_void,     	  pointer to parent mosaic (not save to disk)
# nFitPhotom lands on the old location of Mxxxx, which was used to mean nFitPhotom in some cases

# old image structure:
# FIELD 	  order,            ORDER,                short,      	  Mrel 2D polynomical order 
# FIELD 	  Mx,               MX,                   short,      	  Mrel polyterm
# FIELD 	  My,               MY,                   short,      	  Mrel polyterm
# FIELD 	  Mxx,              MXX,                  short,      	  Mrel polyterm
# FIELD 	  Mxy,              MXY,                  short,      	  Mrel polyterm
# FIELD 	  Myy,              MYY,                  short,      	  Mrel polyterm
# FIELD 	  Mxxx,             MXXX,                 short,      	  Mrel polyterm
# FIELD 	  Mxxy,             MXXY,                 short,      	  Mrel polyterm
# FIELD 	  Mxyy,             MXYY,                 short,      	  Mrel polyterm
# FIELD 	  Myyy,             MYYY,                 short,      	  Mrel polyterm
# FIELD 	  Mxxxx,            MXXXX,                short,      	  Mrel polyterm
# FIELD 	  Mxxxy,            MXXXY,                short,      	  Mrel polyterm
# FIELD 	  Mxxyy,            MXXYY,                short,      	  Mrel polyterm
# FIELD 	  Mxyyy,            MXYYY,                short,      	  Mrel polyterm
# FIELD 	  Myyyy,            MYYYY,                short,      	  Mrel polyterm
# 40 bytes

# *** 20090206 : new fields : parentID, flags (was code char), changed name to 121 bytes.
# *** 20100331 : new fields : RAo, DECo. Radius
# *** 20110203 : replace the old zero point polynomial terms (Mx,My,... Mxxxx,Myyyy) with dummy1 - astrom_map_id
