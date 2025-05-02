# name of structure type
STRUCT  CMF_PS1_DV4
EXTNAME CMF_PS1_DV4
TYPE    BINTABLE
SIZE    224

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
FIELD Flux,           PSF_INST_FLUX,     float,    psf flux,	       counts
FIELD dFlux,          PSF_INST_FLUX_SIG, float,    psf flux error,        counts      
FIELD Map,            AP_MAG,   	 float,    standard aperture mag, mags
FIELD MapRaw,         AP_MAG_RAW,        float,    raw aperture mag,      mags
FIELD apRadius,       AP_MAG_RADIUS,     float,    radius used for fit,   pixels
FIELD apFlux,         AP_FLUX,           float,    ap flux
FIELD apFluxErr,      AP_FLUX_SIG,       float,    ap flux err
FIELD Mpeak,          PEAK_FLUX_AS_MAG,  float,    peak flux as a mag,    mags
FIELD Mcalib,         CAL_PSF_MAG,       float,    calibrated psf mag,    mags
FIELD dMcal,          CAL_PSF_MAG_SIG,   float,    zero point scatter,    mags

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
FIELD psfQF,          PSF_QF,            float,    quality factor
FIELD psfQFperf,      PSF_QF_PERFECT,    float,    quality factor perfect
FIELD psfNdof,        PSF_NDOF,          int,      psf degrees of freedom
FIELD psfNpix,        PSF_NPIX,          int,      psf number of pixels
FIELD Mxx,            MOMENTS_XX,        float,    second moment X,       pixels^2
FIELD Mxy,            MOMENTS_XY,        float,    second moment Y,       pixels^2
FIELD Myy,            MOMENTS_YY,        float,    second moment XY,      pixels^2
FIELD Mr1,            MOMENTS_R1,        float,    first radial moment,   pixels
FIELD Mrh,            MOMENTS_RH,        float,    half radial moment,    pixels^1/2
FIELD kronFlux,       KRON_FLUX,         float,    kron flux,             counts
FIELD kronFluxErr,    KRON_FLUX_ERR,     float,    kron flux error,       counts
FIELD kronInner,      KRON_FLUX_INNER,   float,    kron flux 1<R<2.5,     counts
FIELD kronOuter,      KRON_FLUX_OUTER,   float,    kron flux 2.5<R<4,     counts
FIELD D_Npos,         DIFF_NPOS,         int,      diff param
FIELD D_Fratio,       DIFF_FRATIO,       float,    diff param
FIELD D_Nratio_bad,   DIFF_NRATIO_BAD,   float,    diff param
FIELD D_Nratio_mask,  DIFF_NRATIO_MASK,  float,    diff param
FIELD D_Nratio_all,   DIFF_NRATIO_ALL,   float,    diff param
FIELD D_Rp,           DIFF_R_P,          float,    diff param
FIELD D_SNp,          DIFF_SN_P,         float,    diff param
FIELD D_Rm,           DIFF_R_M,          float,    diff param
FIELD D_SNm,          DIFF_SN_M,         float,    diff param
FIELD flags,          FLAGS,             int,      analysis flags
FIELD flags2,         FLAGS2,            int,      analysis flags (2)
FIELD nFrames,        N_FRAMES,          short,    images overlapping peak
FIELD padding,        PADDING,           short,    padding for 8byte records
