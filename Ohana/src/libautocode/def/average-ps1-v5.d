STRUCT       Average_PS1_V5
EXTNAME      DVO_AVERAGE_PS1_V5
TYPE         BINTABLE
SIZE         192
DESCRIPTION  DVO Average Object Table

# elements of data structure / FITS table

FIELD R,              RA,          double,          RA,                	       	  decimal degrees 
FIELD D,              DEC,         double,          DEC,               	       	  decimal degrees 
FIELD dR,             RA_ERR,      float,           RA error,                     arcsec
FIELD dD,             DEC_ERR,     float,           DEC error,                    arcsec

FIELD uR,             U_RA,        float,           RA*cos(D) proper-motion,      arcsec/year
FIELD uD,             U_DEC,       float,           DEC proper-motion,            arcsec/year
FIELD duR,            V_RA_ERR,    float,           RA*cos(D) p-m error,          arcsec/year
FIELD duD,            V_DEC_ERR,   float,           DEC p-m error,                arcsec/year
FIELD P,              PAR,         float,           parallax,			  arcsec
FIELD dP,             PAR_ERR,     float,           parallax error,               arcsec

FIELD Rstk,           RA_STK,      double,          RA on stack,      	       	  decimal degrees 
FIELD Dstk,           DEC_STK,     double,          DEC on stack,      	       	  decimal degrees 
FIELD dRstk,          RA_STK_ERR,  float,           RA error on stack,            arcsec
FIELD dDstk,          DEC_STK_ERR, float,           DEC error on stack,           arcsec

FIELD ChiSqAve,       CHISQ_POS,   float,           astrometry analysis chisq
FIELD ChiSqPM,        CHISQ_PM,    float,           astrometry analysis chisq
FIELD ChiSqPar,       CHISQ_PLX,   float,           astrometry analysis chisq
FIELD Tmean,          MEAN_EPOCH,  int,   	    mean epoch (PM,PAR ref),       unix time seconds
FIELD Trange,         TIME_RANGE,  int,   	    mean epoch (PM,PAR ref),       unix time seconds

FIELD psfQF,          PSF_QF,      float,           psf coverage (bad masks)
FIELD psfQFperf,      PSF_QF_PERF, float,           psf coverage (all masks)

FIELD stargal,        STARGAL_SEP, float,           star / galaxy separator,       1/100 arcsec
FIELD Npos,           NUMBER_POS,  unsigned short,  number of detections used for astrometry

# this limits us to a max of 64k measurements per object
FIELD Nmeasure,       NMEASURE,    unsigned short,  number of psf measurements
FIELD Nmissing,       NMISSING,    unsigned short,  number of missings
FIELD Nlensing,       NLENSING,    unsigned short,  number of lensing measurements
FIELD Nlensobj,       NLENSOBJ,    unsigned short,  number of lensing measurements
FIELD Ngalphot,       NGALPHOT,    unsigned short,  number of galphot measurements

FIELD measureOffset,  OFF_MEASURE,  int,   	    offset to first psf measurement
FIELD missingOffset,  OFF_MISSING,  int,   	    offset to first missing obs
FIELD lensingOffset,  OFF_LENSING,  int,   	    offset to first lensing obs
FIELD lensobjOffset,  OFF_LENSOBJ,  int,   	    offset to mean lensing data
FIELD starparOffset,  OFF_STARPAR,  int,   	    offset to stellar parameter data
FIELD galphotOffset,  OFF_GALPHOT, int,   	    offset to galphot object entry

FIELD refColorBlue,   REF_COLOR_BLUE, float,   	    color of astrometry ref stars
FIELD refColorRed,    REF_COLOR_RED,  float,   	    color of astrometry ref stars

FIELD tessID,         TESS_ID,       char,	    ID of tessellation for primary skycell
FIELD skycellID,      SKYCELL_ID,    char,    	    ID of skycell for primary skycell
FIELD projectionID,   PROJECTION_ID, short,	    ID of projection for primary skycell

FIELD Nstarpar,       NSTARPAR,      short,         number of stellar parameter entries
FIELD NwarpOK,        NWARP_OK,      short,         total number of warp measurements with psf_qf > 0.0

# 'flags' was called 'code' prior to 2009.02.07
FIELD flags,          FLAGS,       uint32_t,        average object flags (star; ghost; etc)
FIELD photFlagsUpper, PHOTFLAGS_U, uint32_t,        upper bit of 2 bit summary of per-measure photflags
FIELD photFlagsLower, PHOTFLAGS_L, uint32_t,        lower bit of 2 bit summary of per-measure photflags

# objID + catID gives a unique ID for all objects in the database
FIELD objID,          OBJ_ID,      unsigned int,    unique ID for object in table
FIELD catID,          CAT_ID,      unsigned int,    unique ID for table in which object was first realized
FIELD extID,          EXT_ID,      uint64_t,        external ID for object (eg PSPS objID)

# replace extIDgc (unused) with uRgal, uDgal:
# FIELD extIDgc,      EXT_ID_GC,   uint64_t,        external ID for object in galactic coords
FIELD uRgal,          U_RA_GAL,    float,           modeled proper motion based on galactic motion
FIELD uDgal,          U_DEC_GAL,   float,           modeled proper motion based on galactic motion

# 4 double, 2 uint64_t, 17 float, 12 int, 6 short
# 4*8 + 2*8 + 17*4 + 12*4 + 6*2 = 176
# for 5G objects, expect 840G

# photflagsUpper & photflagsLower: we have Nmeasures of a given source
# using psphot. each of these as a photFlag field, with bits
# describing the detection quality.  photflagsUpper and photFlagsLower
# represent a 2 bit value (0-3) which defines the frequency of a given
# bit in the measurements:
# for a given bit, if that bit is raised for these percentile ranges,
# the following bits are set:
# min |  max | L | U
#  0% |  25% | 0 | 0 
# 25% |  50% | 1 | 0 
# 50% |  75% | 0 | 1 
# 75% | 100% | 1 | 1 
