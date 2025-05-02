# name of structure type
STRUCT  ImageASCII
EXTNAME IMAGE
TYPE    TABLE
SIZE    328

# elements of data structure / FITS table
FIELD obstime,    START_TIME, char[20],    start time of measurement, yyyy/mm/dd,hh:mm:ss
FIELD filter,  	  FILTER,     char[10],    filter and camera name    
FIELD ZP,      	  ZP_OBS,     float[8.4],  measured zero point,       mag
FIELD dZP,     	  ZP_ERR,     float[7.4],  error on zero point,       mag
FIELD ra,      	  RA,         float[11.6], RA (J2000),                dec. degrees
FIELD dec,     	  DEC,        float[11.6], DEC (J2000),               dec. degrees
FIELD airmass, 	  C_AIRMASS,  float[7.3],  airmass coeff,             mag per airmass 
FIELD sky,     	  SKY,        float[7.1],  median sky flux,           counts
FIELD Nstar,   	  NSTAR,      int[6],      Number of stars in image,  stars

## XXX is this used?
