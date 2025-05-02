STRUCT  RegImage
EXTNAME IMAGE_DATABASE
TYPE    BINTABLE
SIZE    360

# elements of data structure / FITS table

FIELD filename,         FILE,       char[64],     filename in db
FIELD pathname,     	PATH,       char[128],    fullpath in db
FIELD filter,       	FILTER,     char[32],     filter name
FIELD instrument,   	INSTRUMENT, char[32],     instrument
FIELD ccd,	        CCD,        char,   	  ccd identifier
FIELD mode,	        MODE,       char,   	  mef/split/etc
FIELD type,	        TYPE,       char,   	  object/flat/bias/etc
FIELD flag,	        FLAG,       char,   	  data flags
FIELD seqtime,          SEQTIME,    float,        exposure time per slice,   seconds
FIELD seq,	        SEQ,        char,   	  sequence number
FIELD junk,	        JUNK,       char[19],     space for expansion
FIELD exptime,	        EXPTIME,    float,  	  exposure time,             seconds
FIELD airmass,	        AIRMASS,    float,  	  airmass
FIELD sky,	        SKY,        float,  	  background level,          counts / pixel
FIELD bias,	        BIAS,       float,  	  bias level,                counts / pixel
FIELD fwhm,	        FWHM,       float,  	  image quality,             pixels
FIELD telfocus,	        TELFOCUS,   float,  	  telescope focus,           microns
FIELD xprobe,	        XPROBE,     float,  	  bonnette probe x pos,      microns
FIELD yprobe,	        YPROBE,     float,  	  bonnette probe y pos,      microns
FIELD zprobe,	        ZPROBE,     float,  	  bonnette focus,            microns
FIELD dettemp,	        DETTEMP,    float,  	  detector temperature,      deg celcius
FIELD teltemp_0,       	TELTEMP0,   float,  	  other temperature,         deg celcius
FIELD teltemp_1,       	TELTEMP1,   float,  	  other temperature,         deg celcius
FIELD teltemp_2,       	TELTEMP2,   float,  	  other temperature,         deg celcius
FIELD teltemp_3,       	TELTEMP3,   float,  	  other temperature,         deg celcius
FIELD rotangle,	        ROTANGLE,   float,  	  camera rotation angle,     degrees
FIELD ra,	        RA,         float,  	  image ra,                  degrees
FIELD dec,	        DEC,        float,  	  image dec,                 degrees
FIELD obstime,	        OBS_TIME,   e_time, 	  time of measurement,       seconds since 01 Jan 1970 UT
FIELD regtime,	        REG_TIME,   e_time, 	  time of registration,      seconds since 01 Jan 1970 UT

# take care of the memory padding boundaries when using 'junk' for new elements
# which are not of type char!
