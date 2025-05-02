STRUCT       Getstar_PS1_DEV_1
EXTNAME      GETSTAR_PS1_DEV_1
TYPE         BINTABLE
SIZE         48
DESCRIPTION  Getstar output file

# elements of data structure / FITS table

# average R,D are epoch & equinox J2000.0
FIELD R,              RA,         double,           RA,                	       	  decimal degrees 
FIELD D,              DEC,        double,           DEC,               	       	  decimal degrees 
FIELD uR,             U_RA,       float,            RA*cos(D) proper-motion,      arcsec/year
FIELD uD,             U_DEC,      float,            DEC proper-motion,            arcsec/year
FIELD P,              PAR,        float,            parallax,			  arcsec

FIELD mag,            MAG,        float,            average magnitude in requested photcode
FIELD c1,             MAG_C1,     float,            average magnitude in color term 1
FIELD c2,             MAG_C2,     float,            average magnitude in color term 2

FIELD photcode,       PHOTCODE,   unsigned short,   photcode for this mag
FIELD code,           CODE,       unsigned short,   ID code (star; ghost; etc)
FIELD dummy,          DUMMY,      char[4],          padding
