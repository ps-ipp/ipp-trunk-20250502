STRUCT       SecFilt_PS1_V5
EXTNAME      DVO_SECFILT_PS1_V5
TYPE         BINTABLE
SIZE         176
DESCRIPTION  DVO SecFilt : Secondary Filter Data 

## *** this section is for per-exposure mean values *** (unlabled values are implicitly PSF values)
FIELD  M,             MAG,               float,    average mag in this band,              mags
FIELD  dM,            MAG_ERR,           float,    formal error on average mag,           mags
FIELD  Map,           MAG_AP,            float,    ave aperture mag in this band,         mags
FIELD  dMap,          MAG_AP_ERR,        float,    ave aperture mag in this band,         mags
FIELD  sMap,          MAG_AP_STDEV,      float,      standard deviation of ap mags,         mags
FIELD  Mkron,         MAG_KRON,          float,    ave kron mag in this band,             mags
FIELD  dMkron,        MAG_KRON_ERR,      float,    formal error on average kron mag,      mags
FIELD  sMkron,        MAG_KRON_STDEV,    float,      standard deviation of kron mags,       mags
				         
# XXX I could add these fields to secfilt or calculate in dvopsps?
FIELD  psfQfMax,      PSF_QF_MAX,      float,      best psfQf for this filter
FIELD  psfQfPerfMax,  PSF_QF_PERF_MAX, float,      best psfQfPerfect for this filter

# these statistics are PSF-specific      
FIELD  Mstdev,        MAG_STDEV,         float,    standard deviation of measurements,    mags
FIELD  Mmin,          MAG_MIN,           float,    min accepted mag,                      mags
FIELD  Mmax,          MAG_MAX,           float,    max accepted mag,                      mags
FIELD  Mchisq,        MAG_CHI,           float,    chisq on average mag,                  value
				         
FIELD  Ncode,         NCODE,             short,    number of detections in band
FIELD  Nused,         NUSED,             short,    number of detections used in average
FIELD  NusedKron,     NUSED_KRON,        short,    number of detections used in average
FIELD  NusedAp,       NUSED_AP,          short,    number of detections used in average
				         
FIELD  flags,         FLAGS,             uint32_t, photometry flags

## *** this section is for stack values ***

FIELD  MpsfStk,       MAG_PSF_STK,       float,    magnitude from stack (primary if available)
FIELD  FpsfStk,       FLUX_PSF_STK,      float,    flux from stack (primary if available)
FIELD  dFpsfStk,      FLUX_PSF_STK_ERR,  float,    mean flux psf error

FIELD  MkronStk,      MAG_KRON_STK,      float,    magnitude from stack (primary if available)
FIELD  FkronStk,      FLUX_KRON_STK,     float,    flux from stack (primary if available)
FIELD  dFkronStk,     FLUX_KRON_STK_ERR, float,    mean flux kron error

FIELD  MapStk,        MAG_AP_STK,        float,    magnitude from stack (primary if available)
FIELD  FapStk,        FLUX_AP_STK,       float,    flux from stack (primary if available)
FIELD  dFapStk,       FLUX_AP_STK_ERR,   float,    mean flux ap error

FIELD  Nstack,        NSTACK,            short,    number of measurements in band (gpc1 stack only)
FIELD  NstackDet,     NSTACK_DET,        short,    number of stack detections in band (gpc1 stack only)

FIELD  stackPrmryOff, STACK_PRIMARY_OFF,  int,     measure entry which is primary stack detection
FIELD  stackBestOff,  STACK_BEST_OFF,     int,     measure entry which is best stack detection

## *** this section is for forced-warp mean values ***

FIELD  MpsfWrp,       MAG_PSF_WRP,       float,    psf magnitude from stack (primary if available)
FIELD  FpsfWrp,       FLUX_PSF_WRP,      float,    psf flux from stack (primary if available)
FIELD  dFpsfWrp,      FLUX_PSF_WRP_ERR,  float,    mean flux psf error
FIELD  sFpsfWrp,      FLUX_PSF_WRP_STD,  float,    mean flux psf stdev

FIELD  MkronWrp,      MAG_KRON_WRP,      float,    kron magnitude from stack (primary if available)
FIELD  FkronWrp,      FLUX_KRON_WRP,     float,    kron flux from stack (primary if available)
FIELD  dFkronWrp,     FLUX_KRON_WRP_ERR, float,    mean flux kron error
FIELD  sFkronWrp,     FLUX_KRON_WRP_STD, float,    mean flux kron stdev

FIELD  MapWrp,        MAG_AP_WRP,        float,    aper magnitude from stack (primary if available)
FIELD  FapWrp,        FLUX_AP_WRP,       float,    aper flux from stack (primary if available)
FIELD  dFapWrp,       FLUX_AP_WRP_ERR,   float,    mean flux ap error
FIELD  sFapWrp,       FLUX_AP_WRP_STD,   float,    mean flux ap stdev

FIELD  NusedWrp,      NUSED_WRP,         short,    number of detections used in average
FIELD  NusedKronWrp,  NUSED_KRON_WRP,    short,    number of detections used in average
FIELD  NusedApWrp,    NUSED_AP_WRP,      short,    number of detections used in average

FIELD  Nwarp,         NWARP,             short,    number of measurements in band (gpc1 warp only)
FIELD  NwarpGood,     NWARP_GOOD,        short,    number of meas with psf_qf > 0.85 in band (gpc1 warp only)

FIELD  ubercalDist,   UBERCAL_DIST,      short,    number of images from an ubercal-image

# 31*float + uint32_t + uint64_t + 8*short = 152 bytes / object / photcode
# for 5G objects, expect ~6TB total (over 60 machines, this is 100G)
