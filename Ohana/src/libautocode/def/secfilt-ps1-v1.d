STRUCT       SecFilt_PS1_V1
EXTNAME      DVO_SECFILT_PS1_V1
TYPE         BINTABLE
SIZE         20
DESCRIPTION  DVO SecFilt : Secondary Filter Data 

# elements of data structure / FITS table
FIELD  M,      MAG,      float,                average mag in this band, mags
FIELD  dM,     MAG_ERR,  float,                error on average mag,     mags
FIELD  Mchisq, MAG_CHI,  float,                chisq on average mag,                  value
FIELD  Ncode,  NCODE,    short,                number of detections in band
FIELD  Nused,  NUSED,    short,                number of detections used in average
FIELD  M_20,   MAG_20,   short,                lower 20percent mag,      millimags
FIELD  M_80,   MAG_80,   short,                upper 20percent mag,      millimags
