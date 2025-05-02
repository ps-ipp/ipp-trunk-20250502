# name of structure type
STRUCT  CMF_PS1_V5
EXTNAME CMF_PS1_V5
TYPE    BINTABLE
SIZE    320

# elements of data structure / FITS table
FIELD detID,          IPP_IDET,          unsigned int, detection ID                     
FIELD X,              X_PSF,             float,    x coord,               pixels
FIELD Y,              Y_PSF,             float,    y coord,               pixels
FIELD dX,             X_PSF_SIG,         float,    x coord error,         pixels
FIELD dY,             Y_PSF_SIG,         float,    y coord error,         pixels
FIELD posangle,       POSANGLE,          float,    Posangle at source,    degrees
FIELD pltscale,       PLTSCALE,          float,    Plate Scale at source, arcsec/pixel
FIELD M,              PSF_INST_MAG,      float,    inst mags,             mags
FIELD dM,             PSF_INST_MAG_SIG,  float,    inst mag error,        mags
FIELD Flux,           PSF_INST_FLUX,     float,    psf flux,              counts
FIELD dFlux,          PSF_INST_FLUX_SIG, float,    psf flux error,        counts      
FIELD Map,            AP_MAG,            float,    standard aperture mag, mags
FIELD MapRaw,         AP_MAG_RAW,        float,    raw aperture mag,      mags
FIELD apRadius,       AP_MAG_RADIUS,     float,    radius used for aper,  pixels
FIELD apFlux,         AP_FLUX,           float,    aperture flux,         counts
FIELD apFluxErr,      AP_FLUX_SIG,       float,    error on ap flux,      counts
FIELD apNpix,         AP_NPIX,           int,      pixels used by aper,   pixels
FIELD Mcalib,         CAL_PSF_MAG,       float,    calibrated psf mag,    mags
FIELD dMcal,          CAL_PSF_MAG_SIG,   float,    zero point scatter,    mags

# this field is in the wrong order (is between DEC and sky in cmf, but breaks byte boundary).
# I need to move these bytes around on read (fix PS1_V5?)
FIELD Mpeak,          PEAK_FLUX_AS_MAG,  float,    peak flux as a mag,    mags

# NOTE: RA & DEC (both double) need to be on an 8-byte boundary...
FIELD RA,             RA_PSF,            double,   PSF RA coord,          degrees
FIELD DEC,            DEC_PSF,           double,   PSF DEC coord,         degrees

FIELD sky,            SKY,               float,    sky flux,              cnts/sec
FIELD dSky,           SKY_SIGMA,         float,    sky flux error,        cnts/sec
FIELD psfChisq,       PSF_CHISQ,         float,    psf fit chisq
FIELD crNsigma,       CR_NSIGMA,         float,    Nsigma deviations from PSF to CF
FIELD extNsigma,      EXT_NSIGMA,        float,    Nsigma deviations from PSF to EXT
FIELD fx,             PSF_MAJOR,         float,    psf fit major axis,    pixels
FIELD fy,             PSF_MINOR,         float,    psf fit minor axis,    pixels
FIELD df,             PSF_THETA,         float,    ellipse angle,         degrees
FIELD k,              PSF_CORE,          float,    extra PSF parameter,   unitless
FIELD fwhmMaj,        PSF_FWHM_MAJ,      float,    true fwhm of psf,      pixels
FIELD fwhmMin,        PSF_FWHM_MIN,      float,    true fwhm (minor),     pixels
FIELD psfQF,          PSF_QF,            float,    quality factor
FIELD psfQFperf,      PSF_QF_PERFECT,    float,    quality factor perfect
FIELD psfNdof,        PSF_NDOF,          int,      psf degrees of freedom
FIELD psfNpix,        PSF_NPIX,          int,      psf number of pixels
FIELD Mxx,            MOMENTS_XX,        float,    second moment X,       pixels^2
FIELD Mxy,            MOMENTS_XY,        float,    second moment Y,       pixels^2
FIELD Myy,            MOMENTS_YY,        float,    second moment XY,      pixels^2
FIELD M3c,            MOMENTS_M3C,       float,    third moment cos(t),   pixels^3
FIELD M3s,            MOMENTS_M3S,       float,    third moment sin(t),   pixels^3
FIELD M4c,            MOMENTS_M4C,       float,    fourth moment cos(t),  pixels^4
FIELD M4s,            MOMENTS_M4S,       float,    fourth moment sin(t),  pixels^4

FIELD X11_sm_obj,     X11_SM_OBJ,        float,    lensing smear
FIELD X12_sm_obj,     X12_SM_OBJ,        float,    lensing smear
FIELD X22_sm_obj,     X22_SM_OBJ,        float,    lensing smear
FIELD E1_sm_obj,      E1_SM_OBJ,         float,    lensing smear
FIELD E2_sm_obj,      E2_SM_OBJ,         float,    lensing smear

FIELD X11_sh_obj,     X11_SH_OBJ,        float,    lensing shear
FIELD X12_sh_obj,     X12_SH_OBJ,        float,    lensing shear
FIELD X22_sh_obj,     X22_SH_OBJ,        float,    lensing shear
FIELD E1_sh_obj,      E1_SH_OBJ,         float,    lensing shear
FIELD E2_sh_obj,      E2_SH_OBJ,         float,    lensing shear

FIELD X11_sm_psf,     X11_SM_PSF,        float,    lensing smear
FIELD X12_sm_psf,     X12_SM_PSF,        float,    lensing smear
FIELD X22_sm_psf,     X22_SM_PSF,        float,    lensing smear
FIELD E1_sm_psf,      E1_SM_PSF,         float,    lensing smear
FIELD E2_sm_psf,      E2_SM_PSF,         float,    lensing smear

FIELD X11_sh_psf,     X11_SH_PSF,        float,    lensing shear
FIELD X12_sh_psf,     X12_SH_PSF,        float,    lensing shear
FIELD X22_sh_psf,     X22_SH_PSF,        float,    lensing shear
FIELD E1_sh_psf,      E1_SH_PSF,         float,    lensing shear
FIELD E2_sh_psf,      E2_SH_PSF,         float,    lensing shear

FIELD srcChipNum,     SRC_CHIP_NUM,      short,    source chip in warp
FIELD srcChipX,       SRC_CHIP_X,        short,    source chip x coordinate
FIELD srcChipY,       SRC_CHIP_Y,        short,    source chip y coordinate
FIELD padding3,       SRC_CHIP_PAD,      short,    source chip in warp

FIELD Mr1,            MOMENTS_R1,        float,    first radial moment,   pixels
FIELD Mrh,            MOMENTS_RH,        float,    half radial moment,    pixels^1/2
FIELD kronFlux,       KRON_FLUX,         float,    kron flux,             counts
FIELD kronFluxErr,    KRON_FLUX_ERR,     float,    kron flux error,       counts
FIELD kronInner,      KRON_FLUX_INNER,   float,    kron flux 1<R<2.5,     counts
FIELD kronOuter,      KRON_FLUX_OUTER,   float,    kron flux 2.5<R<4,     counts
FIELD skyLimitRad,    SKY_LIMIT_RAD,     float,    profile to sky limit (radius)
FIELD skyLimitFlux,   SKY_LIMIT_FLUX,    float,    profile to sky limit (flux)
FIELD skyLimitSlope,  SKY_LIMIT_SLOPE,   float,    profile to sky limit (slope)
FIELD flags,          FLAGS,             int,      analysis flags
FIELD flags2,         FLAGS2,            int,      analysis flags (2)
FIELD nFrames,        N_FRAMES,          short,    images overlapping peak
FIELD padding,        PADDING,           short,    padding for 8byte records

# for an object in an image, we have three triplets that tell us about the shape:
# second moments: Mxx, Mxy, Myy 
# model shape parameters: F_major, F_minor, F_theta
# centroid errors: sigma_X, sigma_Y, sigma_XY
