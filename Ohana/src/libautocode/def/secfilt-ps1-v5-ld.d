STRUCT       SecFilt_PS1_V5_LOAD
EXTNAME      DVO_SECFILT_PS1_V5_LOAD
TYPE         BINTABLE
SIZE         8
DESCRIPTION  DVO SecFilt : Secondary Filter Data 

## *** this section is for per-exposure mean values *** (unlabled values are implicitly PSF values)
FIELD  M,             MAG,               float,    average mag in this band,              mags
FIELD  Ncode,         NCODE,             short,    number of detections in band
FIELD  Nused,         NUSED,             short,    number of detections used in average
