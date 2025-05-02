STRUCT       ExtModel_PS1_DEV_2
EXTNAME      DVO_EXTMODEL_PS1_DEV_2
TYPE         BINTABLE
SIZE         104
DESCRIPTION  DVO Detection Extended Model Fits

FIELD dR,             D_RA,         	  float,          RA offset,                	  arcsec
FIELD dD,             D_DEC,        	  float,          DEC offset,               	  arcsec
FIELD M,              MAG,          	  float,          catalog mag,       	       	  mag
FIELD dM,             MAG_ERR,      	  float,          mag error,                    mag

# Pan-STARRS uses a 64-bit detection ID.  keep this in two 32 bit ints for backwards compatibility?
# I don't actually have these indexed, but this lets us match back to the measurement
FIELD detID,          DET_ID,       	  unsigned int,   detection ID
FIELD imageID,        IMAGE_ID,     	  unsigned int,   reference to image
FIELD photcode,       PHOTCODE,     	  unsigned short, photcode

FIELD FWx,            EXT_MAJOR,    	  float,          object fwhm major axis
FIELD FWy,            EXT_MINOR,    	  float,          object fwhm minor axis
FIELD theta,          EXT_THETA,    	  float,          angle wrt ccd X dir

## XXX other model parameter values here
