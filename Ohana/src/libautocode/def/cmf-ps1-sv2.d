# name of structure type
STRUCT  CMF_PS1_SV2
EXTNAME CMF_PS1_SV2
TYPE    BINTABLE
SIZE    208

## note that this definition is inconsistent with the definition in
## pmModules/src/objects/pmSourceIO_CMF.c.in.  The version in
## pmSourceIO*.c creates a table with data not properly aligned with
## the 8-byte boundaries. The structure defined by libautocode does
## this, but has a different order of elements (and adds padding2 to
## fix things). We have generated may files with PS1_SV1 as is, so
## I'll leave it. But in future a PS1_SV2 should be forced to match
## libautocode. Note that addstar knows to detect the alternate
## version of PS1_SV1 and correctly interpret its fields.

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
FIELD apFlux,         AP_FLUX,           float,    aperture flux,         counts
FIELD apFluxErr,      AP_FLUX_SIG,       float,    error on ap flux,      counts

FIELD Mcalib,         CAL_PSF_MAG,       float,    calibrated psf mag,    mags
FIELD dMcal,          CAL_PSF_MAG_SIG,   float,    zero point scatter,    mags

# NOTE: RA & DEC (both double) need to be on an 8-byte boundary...
FIELD RA,             RA_PSF,            double,   PSF RA coord,          degrees
FIELD DEC,            DEC_PSF,           double,   PSF DEC coord,         degrees

FIELD Mpeak,          PEAK_FLUX_AS_MAG,  float,    peak flux as a mag,    mags
FIELD sky,            SKY,               float,    sky flux,              cnts/sec
FIELD dSky,           SKY_SIG,           float,    sky flux error,        cnts/sec
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

FIELD M3c,            MOMENTS_M3C,       float,    third moment cos(t),   pixels^3
FIELD M3s,            MOMENTS_M3S,       float,    third moment sin(t),   pixels^3
FIELD M4c,            MOMENTS_M4C,       float,    fourth moment cos(t),  pixels^4
FIELD M4s,            MOMENTS_M4S,       float,    fourth moment sin(t),  pixels^4

FIELD Mr1,            MOMENTS_R1,        float,    first radial moment,   pixels
FIELD Mrh,            MOMENTS_RH,        float,    half radial moment,    pixels^1/2
FIELD kronFlux,       KRON_FLUX,         float,    kron flux,             counts
FIELD kronFluxErr,    KRON_FLUX_ERR,     float,    kron flux error,       counts

FIELD kronInner,      KRON_FLUX_INNER,   float,    kron flux 1<R<2.5,     counts
FIELD kronOuter,      KRON_FLUX_OUTER,   float,    kron flux 2.5<R<4,     counts
FIELD flags,          FLAGS,             int,      analysis flags
FIELD flags2,         FLAGS2,            int,      analysis flags (2)

FIELD padding2,       PADDING2,          int,      padding for 8byte records
FIELD nFrames,        N_FRAMES,          short,    images overlapping peak
FIELD padding,        PADDING,           short,    padding for 8byte records

# for an object in an image, we have three triplets that tell us about the shape:
# second moments: Mxx, Mxy, Myy 
# model shape parameters: F_major, F_minor, F_theta
# centroid errors: sigma_X, sigma_Y, sigma_XY

# # elements of data structure / FITS table
# FIELD detID,          IPP_IDET,          IPP_IDET           1J      unsigned int, detection ID                     
# FIELD X,              X_PSF,             X_PSF              1E      float,    x coord,               pixels
# FIELD Y,              Y_PSF,             Y_PSF              1E      float,    y coord,               pixels
# FIELD dX,             X_PSF_SIG,         X_PSF_SIG          1E      float,    x coord error,         pixels
# FIELD dY,             Y_PSF_SIG,         Y_PSF_SIG          1E      float,    y coord error,         pixels
# FIELD posangle,       POSANGLE,          POSANGLE           1E      float,    Posangle at source,    degrees
# FIELD pltscale,       PLTSCALE,          PLTSCALE           1E      float,    Plate Scale at source, arcsec/pixel
# FIELD M,              PSF_INST_MAG,      PSF_INST_MAG       1E      float,    inst mags,             mags
# FIELD dM,             PSF_INST_MAG_SIG,  PSF_INST_MAG_SIG   1E      float,    inst mag error,        mags
# FIELD flux,           PSF_INST_FLUX,     PSF_INST_FLUX      1E      float,    psf flux,	       counts
# FIELD flux,           PSF_INST_FLUX_SIG, PSF_INST_FLUX_SIG  1E      float,    psf flux error,        counts      
# FIELD Map,            AP_MAG_STANDARD,   AP_MAG             1E      float,    standard aperture mag, mags
# FIELD Map,            AP_MAG_RAW,        AP_MAG_RAW         1E      float,    raw aperture mag,      mags
# FIELD apRadius,       AP_MAG_RADIUS,     AP_MAG_RADIUS      1E      float,    radius used for fit,   pixels
# FIELD Mpeak,          PEAK_FLUX_AS_MAG,  PEAK_FLUX_AS_MAG   1E      float,    peak flux as a mag,    mags
# FIELD Mcalib,         CAL_PSF_MAG,       CAL_PSF_MAG        1E      float,    calibrated psf mag,    mags
# FIELD dMcal,          CAL_PSF_MAG_SIG,   CAL_PSF_MAG_SIG    1E      float,    zero point scatter,    mags
# FIELD RA,             RA_PSF,            RA_PSF             1D      double,   PSF RA coord,          degrees
# FIELD DEC,            DEC_PSF,           DEC_PSF            1D      double,   PSF DEC coord,         degrees
# FIELD sky,            SKY,               SKY                1E      float,    sky flux,              cnts/sec
# FIELD dSky,           SKY_SIG,           SKY_SIGMA          1E      float,    sky flux error,        cnts/sec
# FIELD psfChisq,       PSF_CHISQ,         PSF_CHISQ          1E      float,    psf fit chisq
# FIELD crNsigma,       CR_NSIGMA,         CR_NSIGMA          1E      float,    Nsigma deviations from PSF to CF
# FIELD extNsigma,      EXT_NSIGMA,        EXT_NSIGMA         1E      float,    Nsigma deviations from PSF to EXT
# FIELD fx,             PSF_MAJOR,         PSF_MAJOR          1E      float,    psf fit major axis,    pixels
# FIELD fy,             PSF_MINOR,         PSF_MINOR          1E      float,    psf fit minor axis,    pixels
# FIELD df,             PSF_THETA,         PSF_THETA          1E      float,    ellipse angle,         degrees
# FIELD psfQF,          PSF_QF,            PSF_QF             1E      float,    quality factor
# FIELD psfQFperf,      PSF_QF_PERFECT,    PSF_QF_PERFECT     1E      float,    quality factor perfect
# FIELD psfNdof,        PSF_NDOF,          PSF_NDOF           1J      int,      psf degrees of freedom
# FIELD psfNpix,        PSF_NPIX,          PSF_NPIX           1J      int,      psf number of pixels
# FIELD Mxx,            MOMENTS_XX,        MOMENTS_XX         1E      float,    second moment X,       pixels^2
# FIELD Mxy,            MOMENTS_XY,        MOMENTS_XY         1E      float,    second moment Y,       pixels^2
# FIELD Myy,            MOMENTS_YY,        MOMENTS_YY         1E      float,    second moment XY,      pixels^2
# FIELD M3c,            MOMENTS_M3C,       MOMENTS_M3C        1E      float,    third moment cos(t),   pixels^3
# FIELD M3s,            MOMENTS_M3S,       MOMENTS_M3S        1E      float,    third moment sin(t),   pixels^3
# FIELD M4c,            MOMENTS_M4C,       MOMENTS_M4C        1E      float,    fourth moment cos(t),  pixels^4
# FIELD M4s,            MOMENTS_M4S,       MOMENTS_M4S        1E      float,    fourth moment sin(t),  pixels^4
# FIELD Mr1,            MOMENTS_R1,        MOMENTS_R1         1E      float,    first radial moment,   pixels
# FIELD Mrh,            MOMENTS_RH,        MOMENTS_RH         1E      float,    half radial moment,    pixels^1/2
# FIELD kronFlux,       KRON_FLUX,         KRON_FLUX          1E      float,    kron flux,             counts
# FIELD kronFluxErr,    KRON_FLUX_ERR,     KRON_FLUX_ERR      1E      float,    kron flux error,       counts
# FIELD kronInner,      KRON_FLUX_INNER,   KRON_FLUX_INNER    1E      float,    kron flux 1<R<2.5,     counts
# FIELD kronOuter,      KRON_FLUX_OUTER,   KRON_FLUX_OUTER    1E      float,    kron flux 2.5<R<4,     counts
# FIELD flags,          FLAGS,             FLAGS              1J      int,      analysis flags
# FIELD flags2,         FLAGS,             FLAGS2             1J      int,      analysis flags (2)
# FIELD nFrames,        N_FRAMES,          N_FRAMES           1I      short,    images overlapping peak
# FIELD padding,        PADDING,           PADDING            1I      short,    padding for 8byte records

      

