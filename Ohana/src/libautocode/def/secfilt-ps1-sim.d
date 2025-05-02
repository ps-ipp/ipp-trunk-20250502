STRUCT       SecFilt_PS1_SIM
EXTNAME      DVO_SECFILT_PS1_SIM
TYPE         BINTABLE
SIZE         16
DESCRIPTION  DVO SecFilt : Secondary Filter Data 

## *** this section is for per-exposure mean values *** (unlabled values are implicitly PSF values)
FIELD  M,             MAG,             float,      average mag in this band,              mags
FIELD  dM,            MAG_ERR,         float,      formal error on average mag,           mags

FIELD  Ncode,         NCODE,           short,      number of detections in band (gpc1 chip only)
FIELD  Nused,         NUSED,           short,      number of detections used to calculate secfilt.M

FIELD  flags,         FLAGS,           uint32_t,   photometry flags

