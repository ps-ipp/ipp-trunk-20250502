STRUCT       SecFilt_PS1_DEV_1
EXTNAME      DVO_SECFILT_PS1_DEV_1
TYPE         BINTABLE
SIZE         16
DESCRIPTION  DVO SecFilt : Secondary Filter Data 

# elements of data structure / FITS table
FIELD  M,     MAG,      float,                other mags,       mags
FIELD  dM,    MAG_ERR,  float,                scatter on mag,   mags
FIELD  Xm,    MAG_CHI,  short,                chisq on mag,     [100*log(value)]
FIELD  Ncode, NCODE,    short,                number of detections in band
FIELD  Nused, NUSED,    short,                number of detections used in average
FIELD  dummy, JUNK,     short,                place holder
