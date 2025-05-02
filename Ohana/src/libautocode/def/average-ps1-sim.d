STRUCT       Average_PS1_SIM
EXTNAME      DVO_AVERAGE_PS1_SIM
TYPE         BINTABLE
SIZE         104
DESCRIPTION  DVO Average Object Table

# elements of data structure / FITS table

FIELD R,              RA,          double,          RA,                           decimal degrees 
FIELD D,              DEC,         double,          DEC,                          decimal degrees 
FIELD dR,             RA_ERR,      float,           RA error,                     arcsec
FIELD dD,             DEC_ERR,     float,           DEC error,                    arcsec
				   
FIELD uR,             U_RA,        float,           RA*cos(D) proper-motion,      arcsec/year
FIELD uD,             U_DEC,       float,           DEC proper-motion,            arcsec/year
FIELD duR,            V_RA_ERR,    float,           RA*cos(D) p-m error,          arcsec/year
FIELD duD,            V_DEC_ERR,   float,           DEC p-m error,                arcsec/year
FIELD P,              PAR,         float,           parallax,                     arcsec
FIELD dP,             PAR_ERR,     float,           parallax error,               arcsec

FIELD ChiSqAve,       CHISQ_POS,   float,           astrometry analysis chisq
FIELD ChiSqPM,        CHISQ_PM,    float,           astrometry analysis chisq
FIELD ChiSqPar,       CHISQ_PAP,   float,           astrometry analysis chisq
FIELD Tmean,          MEAN_EPOCH,  int,   	    mean epoch (PM,PAR ref),       unix time seconds
FIELD Trange,         TIME_RANGE,  int,   	    mean epoch (PM,PAR ref),       unix time seconds

FIELD Npos,           NUMBER_POS,  unsigned short,  number of detections used for astrometry
FIELD dummy,          DUMMY,       short,           padding

# this limits us to a max of 64k measurements per object
FIELD Nmeasure,       NMEASURE,    unsigned short,  number of psf measurements
FIELD Nstarpar,       NSTARPAR,    short,           number of stellar parameter entries

FIELD measureOffset,  OFF_MEASURE, int,             offset to first psf measurement
FIELD starparOffset,  OFF_STARPAR, int,   	    offset to stellar parameter data

FIELD refColorBlue,   REF_COLOR_BLUE, float,   	    color of astrometry ref stars
FIELD refColorRed,    REF_COLOR_RED,  float,   	    color of astrometry ref stars

# 'flags' was called 'code' prior to 2009.02.07
FIELD flags,          FLAGS,       uint32_t,        average object flags (star; ghost; etc)

# objID + catID gives a unique ID for all objects in the database
FIELD objID,          OBJ_ID,      unsigned int,    unique ID for object in table
FIELD catID,          CAT_ID,      unsigned int,    unique ID for table in which object was first realized

