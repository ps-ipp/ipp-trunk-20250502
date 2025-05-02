STRUCT       Getstar_PS1_DEV_2
EXTNAME      GETSTAR_PS1_DEV_2
TYPE         BINTABLE
SIZE         64
DESCRIPTION  Getstar output file

# elements of data structure / FITS table

# average R,D are epoch & equinox J2000.0
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

FIELD mag,            MAG,        float,            average magnitude in requested photcode
FIELD c1,             MAG_C1,     float,            average magnitude in color term 1
FIELD c2,             MAG_C2,     float,            average magnitude in color term 2

FIELD photcode,       PHOTCODE,   unsigned short,   photcode for this mag
FIELD code,           CODE,       unsigned short,   ID code (star; ghost; etc)
