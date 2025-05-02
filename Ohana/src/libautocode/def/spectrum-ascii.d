# name of structure type
STRUCT  SpectrumASCII
EXTNAME SPECTRUM
TYPE    TABLE
SIZE    262

# elements of data structure / FITS table
FIELD filename,   FILENAME,   char[32],    filename in db,        
FIELD pathname,   PATHNAME,   char[64],    fullpath in db,        
FIELD instrument, INSTRUMENT, char[16],    instrument,            
FIELD telescope,  TELESCOPE,  char[16],    telescope,             
FIELD objname,    OBJNAME,    char[16],    object name,           
FIELD extname,    EXTNAME,    char[16],    extname in file,       

FIELD ra,         RA,         float[10.6], ra,                    degrees
FIELD dec,        DEC,        float[10.6], dec,                   degrees
FIELD exptime,    EXPTIME,    float[6.1],  exposure time,         seconds
FIELD airmass,    AIRMASS,    float[5.3],  airmass,               none
FIELD Ws,         Ws,         float[7.2],  spectral range start,  Angstrom
FIELD We,         We,         float[7.2],  spectral range end,    Angstrom
FIELD dW,         dW,         float[7.2],  spectral resolution,   Angstrom / pix
	       			      
FIELD Nspec,      NSPECTRA,   int[3],      number of spectra,     none
FIELD obstime,    OBS_TIME,   char[20],    time of measurement,   yyyy/mm/dd
FIELD regtime,    REG_TIME,   char[20],    time of registration,  yyyy/mm/dd
	       			      
FIELD mode,       MODE,       int[2],      phu/mef/ext,           
FIELD state,      STATE,      int[2],      raw/wav/flx/etc,       
FIELD flag,       FLAG,       int[3],      status flags,          
