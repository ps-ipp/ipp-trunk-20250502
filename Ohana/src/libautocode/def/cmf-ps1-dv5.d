# name of structure type
STRUCT  CMF_PS1_DV5
EXTNAME CMF_PS1_DV5
TYPE    BINTABLE
SIZE    248

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
FIELD apNpix,         AP_NPIX,           int,      number of pixels in aperture
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
FIELD psfCore,        PSF_CORE,          float,    radial slope term
FIELD psfFwhmMajor,   PSF_FWHM_MAJ,      float,    true fwhm in major axis direction
FIELD psfFwhmMinor,   PSF_FWHM_MIN,      float,    true fwhm in minor axis direction
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
FIELD chipNum,        SRC_CHIP_NUM,      short,    chip which supplied detection
FIELD chipX,          SRC_CHIP_X,        short,    x-coord on chip
FIELD chipY,          SRC_CHIP_Y,        short,    y-coord on chip
FIELD padding3,       PADDING3,          short,    padding to fill 8byte boundary
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


IPP_IDET             1J     none     none    FIELD detID,          IPP_IDET,          unsigned int, detection ID                     
X_PSF                1E     none     none    FIELD X,              X_PSF,             float,    x coord,               pixels
Y_PSF                1E     none     none    FIELD Y,              Y_PSF,             float,    y coord,               pixels
X_PSF_SIG            1E     none     none    FIELD dX,             X_PSF_SIG,         float,    x coord error,         pixels
Y_PSF_SIG            1E     none     none    FIELD dY,             Y_PSF_SIG,         float,    y coord error,         pixels
POSANGLE             1E     none     none    FIELD posangle,       POSANGLE,          float,    Posangle at source,    degrees
PLTSCALE             1E     none     none    FIELD pltscale,       PLTSCALE,          float,    Plate Scale at source, arcsec/pixel
PSF_INST_MAG         1E     none     none    FIELD M,              PSF_INST_MAG,      float,    inst mags,             mags
PSF_INST_MAG_SIG     1E     none     none    FIELD dM,             PSF_INST_MAG_SIG,  float,    inst mag error,        mags
PSF_INST_FLUX        1E     none     none    FIELD Flux,           PSF_INST_FLUX,     float,    psf flux,	       counts
PSF_INST_FLUX_SIG    1E     none     none    FIELD dFlux,          PSF_INST_FLUX_SIG, float,    psf flux error,        counts      
AP_MAG               1E     none     none    FIELD Map,            AP_MAG,   	 float,    standard aperture mag, mags
AP_MAG_RAW           1E     none     none    FIELD MapRaw,         AP_MAG_RAW,        float,    raw aperture mag,      mags
AP_MAG_RADIUS        1E     none     none    FIELD apRadius,       AP_MAG_RADIUS,     float,    radius used for fit,   pixels
AP_FLUX              1E     none     none    FIELD apFlux,         AP_FLUX,           float,    ap flux
AP_FLUX_SIG          1E     none     none    FIELD apFluxErr,      AP_FLUX_SIG,       float,    ap flux err
*AP_NPIX             1J     none     none    
PEAK_FLUX_AS_MAG     1E     none     none    FIELD Mpeak,          PEAK_FLUX_AS_MAG,  float,    
CAL_PSF_MAG          1E     none     none    FIELD Mcalib,         CAL_PSF_MAG,       float,    
CAL_PSF_MAG_SIG      1E     none     none    FIELD dMcal,          CAL_PSF_MAG_SIG,   float,    
RA_PSF               1D     none     none    FIELD RA,             RA_PSF,            double,   
DEC_PSF              1D     none     none    FIELD DEC,            DEC_PSF,           double,   
SKY                  1E     none     none    FIELD sky,            SKY,               float,    
SKY_SIGMA            1E     none     none    FIELD dSky,           SKY_SIGMA,         float,    
PSF_CHISQ            1E     none     none    FIELD psfChisq,       PSF_CHISQ,         float,    
CR_NSIGMA            1E     none     none    FIELD crNsigma,       CR_NSIGMA,         float,    
EXT_NSIGMA           1E     none     none    FIELD extNsigma,      EXT_NSIGMA,        float,    
PSF_MAJOR            1E     none     none    FIELD fx,             PSF_MAJOR,         float,    
PSF_MINOR            1E     none     none    FIELD fy,             PSF_MINOR,         float,    
PSF_THETA            1E     none     none    FIELD df,             PSF_THETA,         float,    
PSF_CORE             1E     none     none    
PSF_FWHM_MAJ         1E     none     none    
PSF_FWHM_MIN         1E     none     none    
PSF_QF               1E     none     none    FIELD psfQF,          PSF_QF,            float,    
PSF_QF_PERFECT       1E     none     none    FIELD psfQFperf,      PSF_QF_PERFECT,    float,    
PSF_NDOF             1J     none     none    FIELD psfNdof,        PSF_NDOF,          int,      
PSF_NPIX             1J     none     none    FIELD psfNpix,        PSF_NPIX,          int,      
MOMENTS_XX           1E     none     none    FIELD Mxx,            MOMENTS_XX,        float,    
MOMENTS_XY           1E     none     none    FIELD Mxy,            MOMENTS_XY,        float,    
MOMENTS_YY           1E     none     none    FIELD Myy,            MOMENTS_YY,        float,    
MOMENTS_R1           1E     none     none    FIELD Mr1,            MOMENTS_R1,        float,    
MOMENTS_RH           1E     none     none    FIELD Mrh,            MOMENTS_RH,        float,    
KRON_FLUX            1E     none     none    FIELD kronFlux,       KRON_FLUX,         float,    
KRON_FLUX_ERR        1E     none     none    FIELD kronFluxErr,    KRON_FLUX_ERR,     float,    
KRON_FLUX_INNER      1E     none     none    FIELD kronInner,      KRON_FLUX_INNER,   float,    
KRON_FLUX_OUTER      1E     none     none    FIELD kronOuter,      KRON_FLUX_OUTER,   float,    
SRC_CHIP_NUM         1I     none     none    FIELD chipNum,        SRC_CHIP_NUM,      short,    
SRC_CHIP_X           1I     none     none    FIELD chipX,          SRC_CHIP_X,        short,    
SRC_CHIP_Y           1I     none     none    FIELD chipY,          SRC_CHIP_Y,        short,    
PADDING3             1I     none     none    FIELD padding3,       PADDING3,          short,    
DIFF_NPOS            1J     none     none    FIELD D_Npos,         DIFF_NPOS,         int,      
DIFF_FRATIO          1E     none     none    FIELD D_Fratio,       DIFF_FRATIO,       float,    
DIFF_NRATIO_BAD      1E     none     none    FIELD D_Nratio_bad,   DIFF_NRATIO_BAD,   float,    
DIFF_NRATIO_MASK     1E     none     none    FIELD D_Nratio_mask,  DIFF_NRATIO_MASK,  float,    
DIFF_NRATIO_ALL      1E     none     none    FIELD D_Nratio_all,   DIFF_NRATIO_ALL,   float,    
DIFF_R_P             1E     none     none    FIELD D_Rp,           DIFF_R_P,          float,    
DIFF_SN_P            1E     none     none    FIELD D_SNp,          DIFF_SN_P,         float,    
DIFF_R_M             1E     none     none    FIELD D_Rm,           DIFF_R_M,          float,    
DIFF_SN_M            1E     none     none    FIELD D_SNm,          DIFF_SN_M,         float,    
FLAGS                1J     none     none    FIELD flags,          FLAGS,             int,      
FLAGS2               1J     none     none    FIELD flags2,         FLAGS2,            int,      
N_FRAMES             1I     none     none    FIELD nFrames,        N_FRAMES,          short,    
PADDING              1I     none     none    FIELD padding,        PADDING,           short,    
