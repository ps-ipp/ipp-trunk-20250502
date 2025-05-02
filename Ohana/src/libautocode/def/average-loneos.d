STRUCT  Average_Loneos
EXTNAME DVO_AVERAGE_LONEOS
TYPE    BINTABLE
SIZE    28

# elements of data structure / FITS table

FIELD R,              RA,         float,            RA,                	       	  decimal degrees 
FIELD D,              DEC,        float,            DEC,               	       	  decimal degrees 
FIELD M,              MAG,        short,            primary mag,       	       	  millimag
FIELD Nm,             NMEAS,      unsigned short,   number of measures
FIELD Nn,             NMISS,      unsigned short,   number of missings
FIELD Xp,             SIGMA_POS,  short, 	    position scatter,   	  1/100 arcsec
FIELD Xm,             CHISQ_MAG,  short, 	    chisq for primary mag,        [1000*value]  ?
FIELD code,           code,       unsigned short,   ID code (star; ghost; etc)
FIELD offset,         offset,     int,     	    offset to first measurement
FIELD missing,        missing,    int,     	    offset to first missing obs
