STRUCT       SecFilt_PS1_V4
EXTNAME      DVO_SECFILT_PS1_V4
TYPE         BINTABLE
SIZE         64
DESCRIPTION  DVO SecFilt : Secondary Filter Data 

# elements of data structure / FITS table
FIELD  M,             MAG,             float,      average mag in this band,              mags
FIELD  Map,           MAG_AP,          float,      ave aperture mag in this band,         mags
FIELD  Mkron,         MAG_KRON,        float,      ave kron mag in this band,             mags
FIELD  dMkron,        MAG_KRON_ERR,    float,      formal error on average kron mag,      mags
FIELD  dM,            MAG_ERR,         float,      formal error on average mag,           mags
FIELD  Mchisq,        MAG_CHI,         float,      chisq on average mag,                  value
FIELD  FluxPSF,       FLUX_PSF,        float,      mean flux psf fit (PS1: stack)
FIELD  dFluxPSF,      FLUX_PSF_ERR,    float,      mean flux psf error
FIELD  FluxKron,      FLUX_KRON,       float,      mean flux kron ap (PS1: stack)
FIELD  dFluxKron,     FLUX_KRON_ERR,   float,      mean flux kron err
FIELD  flags,         FLAGS,           uint32_t,   photometry flags
FIELD  Ncode,         NCODE,           short,      number of detections in band
FIELD  Nused,         NUSED,           short,      number of detections used in average
FIELD  M_20,          MAG_20,          short,      lower 20percent mag,                   millimags
FIELD  M_80,          MAG_80,          short,      upper 20percent mag,                   millimags
FIELD  ubercalDist,   UBERCAL_DIST,    short,      number of images from an ubercal-image
FIELD  Mstdev,        MAG_STDEV,       short,      standard deviation of measurements,    millimags
FIELD  stackPrmryOff, STACK_PRIMARY_OFF, int,      measure entry which is primary stack detection
FIELD  stackBestOff,  STACK_BEST_OFF,    int,      measure entry which is best stack detection
