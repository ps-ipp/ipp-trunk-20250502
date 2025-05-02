STRUCT  Measure_Loneos
EXTNAME DVO_MEASURE_LONEOS
TYPE    BINTABLE
SIZE    20

# elements of data structure / FITS table

FIELD dR,             D_RA,       short,          RA offset,                	  1/100 arcsec
FIELD dD,             D_DEC,      short,          DEC offset,               	  1/100 arcsec
FIELD M,              MAG,        short,          catalog mag,       	       	  millimag
FIELD Mcal,           Mcal,       short,          image cal mag,	          millimag
FIELD dM,             dM,         unsigned char,  mag error,                      millimag
FIELD dophot,         dophot,     char,           dophot type
FIELD source,         source,     unsigned short, photcode
FIELD t,              t,          unsigned int,   time in seconds (UNIX)
FIELD averef,         averef,     unsigned int,   reference to average entry      
