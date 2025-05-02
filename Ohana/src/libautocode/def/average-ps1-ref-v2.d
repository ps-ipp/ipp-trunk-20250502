STRUCT       Average_PS1_REF_V2
EXTNAME      DVO_AVERAGE_PS1_REF_V2
TYPE         BINTABLE
SIZE         72
DESCRIPTION  DVO Average Object Table

# elements of data structure / FITS table

FIELD R,              RA,          double,          RA,                	       	  decimal degrees 
FIELD D,              DEC,         double,          DEC,               	       	  decimal degrees 
FIELD dR,             RA_ERR,      float,           RA error                      arcsec
FIELD dD,             DEC_ERR,     float,           DEC error                     arcsec

FIELD uR,             U_RA,        float,           RA*cos(D) proper-motion,      arcsec/year
FIELD uD,             U_DEC,       float,           DEC proper-motion,            arcsec/year
FIELD duR,            V_RA_ERR,    float,           RA*cos(D) p-m error,          arcsec/year
FIELD duD,            V_DEC_ERR,   float,           DEC p-m error,                arcsec/year
FIELD P,              PAR,         float,           parallax,                     arcsec
FIELD dP,             PAR_ERR,     float,           parallax error,               arcsec

FIELD measureOffset,  OFF_MEASURE, uint32_t,   	    offset to first psf measurement
FIELD missingOffset,  OFF_MISSING, uint32_t,   	    offset to first missing obs

# objID + catID gives a unique ID for all objects in the database
FIELD objID,          OBJ_ID,      unsigned int,    unique ID for object in table
FIELD catID,          CAT_ID,      unsigned int,    unique ID for table in which object was first realized

# this limits us to a max of 64k measurements per object
FIELD Nmeasure,       NMEASURE,    unsigned short,  number of psf measurements
FIELD Nmissing,       NMISSING,    unsigned short,  number of missings
FIELD pad,            PAD,         char[4],         padding

# 2 double, 8 float, 2 int, 2 short, 4 char, 2 unit32_t
# 2*8 +     8*4 +    2*4 +  2*2 +    4*1 +   2*4       = 72

