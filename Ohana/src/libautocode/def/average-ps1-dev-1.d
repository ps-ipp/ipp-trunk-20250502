STRUCT       Average_PS1_DEV_1
EXTNAME      DVO_AVERAGE_PS1_DEV_1
TYPE         BINTABLE
SIZE         72
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

FIELD Xp,             SIGMA_POS,  short, 	    position scatter,   	  1/100 arcsec
FIELD Nm,             NMEAS,      unsigned short,   number of measures
FIELD Nn,             NMISS,      unsigned short,   number of missings
FIELD code,           code,       unsigned short,   ID code (star; ghost; etc)
FIELD offset,         offset,     int,     	    offset to first measurement
FIELD missing,        missing,    int,     	    offset to first missing obs

# Pan-STARRS uses a 64-bit detection ID.  keep this in two 32 bit ints for backwards compatibility?
FIELD objID,          OBJ_ID,     unsigned int,   ID upper bytes
FIELD catID,          CAT_ID,     unsigned int,   ID lower bytes

# this structure should only be used for internal representations
# the average-FORMAT structures should be used for external representations
# note that the average magnitudes are stored in the 'secfilt' table (change this name??)
# the index for the secfilt table is just Nsecfilt times the index for the average table.

# the DVO object IDs are generated internally and are not equivalent to the PSPS object IDs
# probably need to add position chisq

# XXX include the number of measurements used to determine the positional information?
