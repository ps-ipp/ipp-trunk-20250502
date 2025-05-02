STRUCT       Measure_PS1_DEV_1
EXTNAME      DVO_MEASURE_PS1_DEV_1
TYPE         BINTABLE
SIZE         104
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
FIELD detID,          DET_ID,       unsigned int,   detection ID
FIELD imageID,        IMAGE_ID,     unsigned int,   reference to image

# do we need more resolution than a short? should this be a log?
FIELD psfQF,          PSF_QF,       float,          psf coverage/quality factor
FIELD psfChisq,       PSF_CHISQ,    float,          psf coverage/quality factor
FIELD crNsigma,       CR_NSIGMA,    float,          psf coverage/quality factor
FIELD extNsigma,      EXT_NSIGMA,   float,          psf coverage/quality factor

FIELD FWx,            FWHM_MAJOR,   short,          object fwhm major axis,       1/100 of arcsec 
FIELD FWy,            FWHM_MINOR,   short,          object fwhm minor axis,       1/100 of arcsec 
FIELD theta,          PSF_THETA,    short,          angle wrt ccd X dir,          (0xffff/360) deg
FIELD photcode,       PHOTCODE,     unsigned short, photcode

FIELD dXccd,          X_CCD_ERR,    short,          X coord error on chip,         pixels
FIELD dYccd,          Y_CCD_ERR,    short,          Y coord error on chip,         pixels

FIELD dbFlags,        DB_FLAGS,     unsigned short, flags for various uses  
FIELD photFlags,      PHOT_FLAGS,   unsigned short, flags supplied by photometry program

FIELD stargal,        STAR_GAL,     char,           star-galaxy separator

# absorb these into photFlags?
FIELD dophot,         DOPHOT,       char,           dophot type

FIELD dummy,          DUMMY,        char[2],        padding
