STRUCT       Measure_Panstarrs_DEV_0
EXTNAME      DVO_MEASURE_PANSTARRS_DEV_0
TYPE         BINTABLE
SIZE         96
DESCRIPTION  DVO Detection Measurement Table 

FIELD dR,             D_RA,         float,          RA offset,                	  arcsec
FIELD dD,             D_DEC,        float,          DEC offset,               	  arcsec
FIELD M,              MAG,          float,          catalog mag,       	       	  mag
FIELD Mcal,           M_CAL,        float,          image cal mag,	          mag
FIELD Mgal,           M_GAL,        float,          galaxy mag,			  mag
FIELD dM,             MAG_ERR,      float,          mag error,                    mag
FIELD dt,             M_TIME,       float,          exposure time,                2.5*log(exptime)

# note that with airmass = 1.0 / cos(90 - alt), we have full alt/az representation
FIELD airmass,        AIRMASS,      float,          (airmass - 1),		  airmass
FIELD az,             AZ,           float,          telescope azimuth

# new field elements needed for Pan-STARRS:
FIELD Xccd,           X_CCD,        float,          X coord on chip,               pixels
FIELD Yccd,           Y_CCD,        float,          Y coord on chip,               pixels

# could these be packed into fewer bits?
FIELD Sky,            SKY_FLUX,     float,          local estimate of sky flux,    counts/sec
FIELD dSky,           SKY_FLUX_ERR, float,          local estimate of sky flux,    counts/sec

FIELD t,              TIME,         unsigned int,   time in seconds (UNIX)
FIELD averef,         AVE_REF,      unsigned int,   reference to average entry      

# Pan-STARRS uses a 64-bit detection ID.  keep this in two 32 bit ints for backwards compatibility?
FIELD detID_hi,       DET_ID_HI,    unsigned int,   ID upper bytes
FIELD detID_lo,       DET_ID_LO,    unsigned int,   ID lower bytes

FIELD imageID_hi,     IMAGE_ID_HI,  unsigned int,   reference to image
FIELD imageID_lo,     IMAGE_ID_LO,  unsigned int,   reference to image

FIELD FWx,            FWHM_MAJOR,   short,          object fwhm major axis,       1/100 of arcsec 
FIELD FWy,            FWHM_MINOR,   short,          object fwhm minor axis,       1/100 of arcsec 
FIELD theta,          PSF_THETA,    short,          angle wrt ccd X dir,          (0xffff/360) deg
FIELD photcode,       PHOTCODE,     unsigned short, photcode

FIELD flags,          FLAGS,        unsigned short, flags for various uses  

FIELD dXccd,          X_CCD_ERR,    short,          X coord error on chip,         pixels
FIELD dYccd,          Y_CCD_ERR,    short,          Y coord error on chip,         pixels

# do we need more resolution than a short? should this be a log?
FIELD psfQF,          PSF_QF,       short,          psf coverage/quality factor

FIELD dophot,         DOPHOT,       char,           dophot type
FIELD stargal,        STAR_GAL,     char,           star-galaxy separator

# we need extra bytes for padding purposes...
FIELD dummy,          DUMMY,        char[2],        padding
