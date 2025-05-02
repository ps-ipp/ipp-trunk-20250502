STRUCT PhotPars
EXTNAME ZERO_POINTS_3.0
TYPE BINTABLE
SIZE 108

# elements of data structure / FITS table

FIELD ZP,    	  ZP_OBS,     float,       measured zero point,       mag
FIELD ZPo,   	  ZP_REF,     float, 	   nominal zero point,        mag
FIELD dZP,   	  ZP_ERR,     float, 	   error on zero point,       mag
FIELD K,     	  C_AIRMASS,  float, 	   airmass coeff,             mag per airmass
FIELD X,     	  C_COLOR,    float, 	   color coeff,               mag per mag
FIELD tstart,	  START_TIME, e_time,      start time of measurement, seconds since 1 Jan 1970 UT
FIELD tstop, 	  STOP_TIME,  e_time,      stop time of measurement,  seconds since 1 Jan 1970 UT
FIELD c1,    	  C1_CODE,    short, 	   code 1 for color,          photcode
FIELD c2,    	  C2_CODE,    short, 	   code 2 for color,          photcode
FIELD photcode,   PHOTCODE,   short, 	   photcode,                  photcode
FIELD label,      LABEL,      char[64],    data label
FIELD refcode,    REFCODE,    rawshort,	   photcode,                  photcode
FIELD Ntime,      N_TIME,     int, 	   number of times
FIELD Nmeas,      N_MEAS,     int, 	   number of measurements
