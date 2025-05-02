STRUCT  Measure_Elixir
EXTNAME DVO_MEASURE_ELIXIR
TYPE    BINTABLE
SIZE    32

# elements of data structure / FITS table

FIELD dR,             D_RA,       short,          RA offset,                	  1/100 arcsec
FIELD dD,             D_DEC,      short,          DEC offset,               	  1/100 arcsec
FIELD M,              MAG,        short,          catalog mag,       	       	  millimag
FIELD Mcal,           Mcal,       short,          image cal mag,	          millimag
FIELD Mgal,           Mgal,       short,          galaxy mag,			  millimag
FIELD airmass,        airmass,    short,          (airmass - 1),		  milliairmass
FIELD FWx,            FWx,        short,          object fwhm major axis,         1/100 of arcsec 
FIELD dM,             dM,         unsigned char,  mag error,                      millimag
FIELD fwy,            fwy,        unsigned char,  object fwhm minor/major ratio
FIELD theta,          theta,      unsigned char,  angle wrt ccd X dir,            (0xff/360) deg
FIELD dophot,         dophot,     char,           dophot type
FIELD source,         source,     unsigned short, photcode
FIELD t,              t,          unsigned int,   time in seconds (UNIX)
FIELD averef,         averef,     unsigned int,   reference to average entry      
FIELD dt,             dt,         short,          exposure time,                  2500*log(exptime)
FIELD flags,          flags,      unsigned short, flags for various uses  
