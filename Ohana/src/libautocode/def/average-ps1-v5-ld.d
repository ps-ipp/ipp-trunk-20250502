STRUCT       Average_PS1_V5_LOAD
EXTNAME      DVO_AVERAGE_PS1_V5_LOAD
TYPE         BINTABLE
SIZE         56
DESCRIPTION  DVO Average Object Table

# elements of data structure / FITS table

FIELD R,              RA,          double,          RA,                	       	  decimal degrees 
FIELD D,              DEC,         double,          DEC,               	       	  decimal degrees 

# objID + catID gives a unique ID for all objects in the database
FIELD objID,          OBJ_ID,      unsigned int,    unique ID for object in table
FIELD catID,          CAT_ID,      unsigned int,    unique ID for table in which object was first realized
FIELD extID,          EXT_ID,      uint64_t,        external ID for object (eg PSPS objID)

# offsets to starting point
FIELD measureOffset,  OFF_MEASURE, int,   	    offset to first measurement
FIELD lensingOffset,  OFF_LENSING, int,   	    offset to first lensing obs
FIELD galphotOffset,  OFF_GALPHOT, int,   	    offset to galphot object entry

# this limits us to a max of 64k measurements per object
FIELD Nmeasure,       NMEASURE,    unsigned short,  number of psf measurements
FIELD Nlensing,       NLENSING,    unsigned short,  number of lensing measurements
FIELD Ngalphot,       NGALPHOT,    unsigned short,  number of galphot measurements
FIELD dummy,  	      DUMMY,       char[6],         padding

# 3*8 + 5*4 + 3*2 + 6 = 56

