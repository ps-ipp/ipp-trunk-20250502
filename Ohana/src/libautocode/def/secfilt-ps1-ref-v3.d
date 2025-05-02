STRUCT       SecFilt_PS1_REF_V3
EXTNAME      DVO_SECFILT_PS1_REF_V3
TYPE         BINTABLE
SIZE         12
DESCRIPTION  DVO SecFilt : Secondary Filter Data 

# elements of data structure / FITS table
FIELD  M,     MAG,      float,                average mag in this band, mags
FIELD  dM,    MAG_ERR,  float,                error on average mag,     mags
FIELD  flags, FLAGS,    uint32_t,             photometry flags

