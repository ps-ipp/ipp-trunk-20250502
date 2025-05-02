STRUCT       Average_PS1_DEV_2
EXTNAME      DVO_AVERAGE_PS1_DEV_2
TYPE         BINTABLE
SIZE         80
DESCRIPTION  DVO Average Object Table

# elements of data structure / FITS table

FIELD R,              RA,         double,           RA,                	       	  decimal degrees 
FIELD D,              DEC,        double,           DEC,               	       	  decimal degrees 
FIELD dR,             RA_ERR,     float,            RA error                      arcsec
FIELD dD,             DEC_ERR,    float,            DEC error                     arcsec

FIELD uR,             U_RA,       float,            RA*cos(D) proper-motion,      arcsec/year
FIELD uD,             U_DEC,      float,            DEC proper-motion,            arcsec/year
FIELD duR,            V_RA_ERR,   float,            RA*cos(D) p-m error,          arcsec/year
FIELD duD,            V_DEC_ERR,  float,            DEC p-m error,                arcsec/year
FIELD P,              PAR,        float,            parallax,			  arcsec
FIELD dP,             PAR_ERR,    float,            parallax error,               arcsec

FIELD Xp,             SIGMA_POS,   short, 	    position scatter,   	  1/100 arcsec
FIELD Nmeasure,       NMEASURE,    unsigned short,  number of psf measurements
FIELD Nmissing,       NMISSING,    unsigned short,  number of missings
FIELD Nextend,        NEXTEND,     unsigned short,  number of extended measurements
FIELD measureOffset,  OFF_MEASURE, int,     	    offset to first psf measurement
FIELD missingOffset,  OFF_MISSING, int,     	    offset to first missing obs
FIELD refColor,       REF_COLOR,   float,   	    color of astrometry ref stars

FIELD code,           code,       unsigned short,   ID code (star; ghost; etc)
FIELD dummy,          DUMMY,      char[2],          padding

# Pan-STARRS uses a 64-bit detection ID.  keep this in two 32 bit ints
# for C89 compatibility.  The objID is constructed based on the
# position of first instatiation.  this is actually quite expensive
# because we need to include the uniqueness test to construct this,
# which requires a select for each new object.  Therefore, I will use
# a table based ID (table ID + object ID), and we will have to
# re-number the object IDs if we change the table density, OR treat
# all subdivisions as entries which are from a foreign table.

FIELD objID,          OBJ_ID,    unsigned int,   unique ID for object in table
FIELD catID,          CAT_ID,    unsigned int,   unique ID for table in which object was first realized

# this structure should only be used for internal representations
# the average-FORMAT structures should be used for external representations
# note that the average magnitudes are stored in the 'secfilt' table (change this name??)
# the index for the secfilt table is just Nsecfilt times the index for the average table.

# the DVO object IDs are generated internally and are not equivalent to the PSPS object IDs
# probably need to add position chisq

# XXX include the number of measurements used to determine the positional information?
