STRUCT  Spectrum
EXTNAME SPECTRUM_DATABASE
TYPE    BINTABLE
SIZE    216

# elements of data structure / FITS table
	       			      
FIELD ra,         RA,         float,     ra,                    degrees
FIELD dec,        DEC,        float,     dec,                   degrees
FIELD exptime,    EXPTIME,    float,     exposure time,         seconds
FIELD airmass,    AIRMASS,    float,     airmass,               
FIELD Ws,         Ws,         float,     spectral range start,  Angstrom
FIELD We,         We,         float,     spectral range end,    Angstrom
FIELD dW,         dW,         float,     spectral resolution,   Angstrom / pix
	       			      
FIELD Nspec,      NSPECTRA,   int,       number of spectra,     
FIELD obstime,    OBS_TIME,   int,       time of measurement,   seconds since 1 Jan 1970 UT
FIELD regtime,    REG_TIME,   int,       time of registration,  seconds since 1 Jan 1970 UT
	       			      
FIELD mode,       MODE,       char,      phu/mef/ext           
FIELD state,      STATE,      char,      raw/wav/flx/etc       
FIELD flag,       FLAG,       char,      status flags
FIELD extra,      EXTRA,      char[13],  room for expansion

FIELD pathname,   PATHNAME,   char[64],  fullpath in db
FIELD filename,   FILENAME,   char[32],  filename in db
FIELD extname,    EXTNAME,    char[16],  extname in file,       

FIELD instrument, INSTRUMENT, char[16],  instrument,            
FIELD telescope,  TELESCOPE,  char[16],  telescope,             
FIELD objname,    OBJNAME,    char[16],  object name,           
