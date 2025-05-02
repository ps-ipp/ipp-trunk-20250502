STRUCT       Measure_PS1_REF_V2
EXTNAME      DVO_MEASURE_PS1_REF_V2
TYPE         BINTABLE
SIZE         48
DESCRIPTION  DVO Detection Measurement Table 

# this format is a stripped-down version of ps1-v1, appropriate to a static reference database
# we make the trade-off towards small data volume vs detailed metadata

FIELD dR,             D_RA,         float,          RA offset,                	  arcsec
FIELD dD,             D_DEC,        float,          DEC offset,               	  arcsec
FIELD M,              MAG,          float,          catalog mag,       	       	  mag
FIELD dM,             MAG_ERR,      float,          mag error,                    mag
FIELD Mcal,           M_CAL,        float,          image cal mag,                mag
FIELD dMcal,          MAG_CAL_ERR,  float,          systematic calibration error, mag
FIELD dt,             M_TIME,       float,          exposure time,                2.5*log(exptime)

FIELD t,              TIME,         int,   	    time in seconds (UNIX)
FIELD averef,         AVE_REF,      unsigned int,   reference to average entry      

# might be able to drop these:
FIELD objID,          OBJ_ID,       unsigned int,   unique ID for object in table
FIELD catID,          CAT_ID,       unsigned int,   unique ID for table in which object was first realized

FIELD photcode,       PHOTCODE,     unsigned short, photcode

FIELD pad,            PAD,          char[2],        padding

# 7 x float 
# 4 x int
# 2 + 2
# = 48 bytes

# 7 float, 4 int, 1 short, 2 char
# 7*4 +    4*4 +  1*2 +    2*1   = 48


