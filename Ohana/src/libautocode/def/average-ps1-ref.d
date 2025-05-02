STRUCT       Average_PS1_REF
EXTNAME      DVO_AVERAGE_PS1_REF
TYPE         BINTABLE
SIZE         48
DESCRIPTION  DVO Average Object Table

# elements of data structure / FITS table

FIELD R,              RA,          double,          RA,                	       	  decimal degrees 
FIELD D,              DEC,         double,          DEC,               	       	  decimal degrees 
FIELD dR,             RA_ERR,      float,           RA error                      arcsec
FIELD dD,             DEC_ERR,     float,           DEC error                     arcsec

FIELD measureOffset,  OFF_MEASURE, uint32_t,   	    offset to first psf measurement
FIELD missingOffset,  OFF_MISSING, uint32_t,   	    offset to first missing obs

# objID + catID gives a unique ID for all objects in the database
FIELD objID,          OBJ_ID,      unsigned int,    unique ID for object in table
FIELD catID,          CAT_ID,      unsigned int,    unique ID for table in which object was first realized

# this limits us to a max of 64k measurements per object
FIELD Nmeasure,       NMEASURE,    unsigned short,  number of psf measurements
FIELD Nmissing,       NMISSING,    unsigned short,  number of missings
FIELD pad,            PAD,         char[4],         padding

# 2 x double
# 2 x float
# 4 x int
# 2 x short
# 4 x char
# = 48
