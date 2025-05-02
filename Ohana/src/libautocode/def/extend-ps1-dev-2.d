STRUCT       Extend_PS1_DEV_2
EXTNAME      DVO_EXTEND_PS1_DEV_2
TYPE         BINTABLE
SIZE         160
DESCRIPTION  DVO Detection Extended Source Parameters Table 

FIELD dR,             D_RA,         	  float,          RA offset,                	  arcsec
FIELD dD,             D_DEC,        	  float,          DEC offset,               	  arcsec
FIELD M,              MAG,          	  float,          catalog mag,       	       	  mag
FIELD dM,             MAG_ERR,      	  float,          mag error,                    mag

# Pan-STARRS uses a 64-bit detection ID.  keep this in two 32 bit ints for backwards compatibility?
# I don't actually have these indexed, but this lets us match back to the measurement
FIELD detID,          DET_ID,       	  unsigned int,   detection ID
FIELD imageID,        IMAGE_ID,     	  unsigned int,   reference to image
FIELD photcode,       PHOTCODE,     	  unsigned short, photcode
FIELD padding,        PADDING,     	  unsigned short, padding

FIELD FWx,            EXT_MAJOR,    	  float,          object fwhm major axis
FIELD FWy,            EXT_MINOR,    	  float,          object fwhm minor axis
FIELD theta,          EXT_THETA,    	  float,          angle wrt ccd X dir

FIELD petroMag,       PETRO_MAG,    	  float,          petrosian mag
FIELD petroMagErr,    PETRO_MAG_ERR,	  float,          petrosian mag
FIELD petroRad,       PETRO_RADIUS,       float,          petrosian mag
FIELD petroRadErr,    PETRO_RADIUS_ERR,   float,          petrosian mag

FIELD kronMag,        KRON_MAG,    	  float,          kron mag
FIELD kronMagErr,     KRON_MAG_ERR,	  float,          kron mag
FIELD kronRad,        KRON_RADIUS,     	  float,          kron mag
FIELD kronRadErr,     KRON_RADIUS_ERR, 	  float,          kron mag

FIELD isophotMag,     ISOPHOT_MAG,    	  float,          isophot mag
FIELD isophotMagErr,  ISOPHOT_MAG_ERR,	  float,          isophot mag
FIELD isophotRad,     ISOPHOT_RADIUS,     float,          isophot mag
FIELD isophotRadErr,  ISOPHOT_RADIUS_ERR, float,          isophot mag

FIELD fluxR0,         FLUX_VAL_R_00,      float,          flux in annulus 0
FIELD fluxR1,         FLUX_VAL_R_01,      float,          flux in annulus 1
FIELD fluxR2,         FLUX_VAL_R_02,      float,          flux in annulus 2
FIELD fluxR3,         FLUX_VAL_R_03,      float,          flux in annulus 3
FIELD fluxR4,         FLUX_VAL_R_04,      float,          flux in annulus 4
FIELD fluxR5,         FLUX_VAL_R_05,      float,          flux in annulus 5

FIELD fluxR0err,      FLUX_ERR_R_00,      float,          flux error in annulus 0
FIELD fluxR1err,      FLUX_ERR_R_01,      float,          flux error in annulus 1
FIELD fluxR2err,      FLUX_ERR_R_02,      float,          flux error in annulus 2
FIELD fluxR3err,      FLUX_ERR_R_03,      float,          flux error in annulus 3
FIELD fluxR4err,      FLUX_ERR_R_04,      float,          flux error in annulus 4
FIELD fluxR5err,      FLUX_ERR_R_05,      float,          flux error in annulus 5

FIELD fluxR0var,      FLUX_VAR_R_00,      float,          flux var in annulus 0
FIELD fluxR1var,      FLUX_VAR_R_01,      float,          flux var in annulus 1
FIELD fluxR2var,      FLUX_VAR_R_02,      float,          flux var in annulus 2
FIELD fluxR3var,      FLUX_VAR_R_03,      float,          flux var in annulus 3
FIELD fluxR4var,      FLUX_VAR_R_04,      float,          flux var in annulus 4
FIELD fluxR5var,      FLUX_VAR_R_05,      float,          flux var in annulus 5

