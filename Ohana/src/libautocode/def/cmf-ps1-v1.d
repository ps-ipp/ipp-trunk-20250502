# name of structure type
STRUCT  CMF_PS1_V1
EXTNAME CMF_PS1_V1
TYPE    BINTABLE
SIZE    128

# elements of data structure / FITS table
FIELD detID,     IPP_IDET,     	   unsigned int, detection ID
FIELD X,      	 X_PSF,    	   float,    x coord,               pixels
FIELD Y,      	 Y_PSF,    	   float,    y coord,               pixels
FIELD dX,      	 X_PSF_SIG,    	   float,    x coord error,         pixels
FIELD dY,      	 Y_PSF_SIG,    	   float,    y coord error,         pixels
FIELD RA,      	 RA_PSF,    	   float,    PSF RA coord,          degrees
FIELD DEC,     	 DEC_PSF,    	   float,    PSF DEC coord,         degrees
FIELD posangle,  POSANGLE,    	   float,    Posangle at source,    degrees
FIELD pltscale,  PLTSCALE,    	   float,    Plate Scale at source, arcsec/pixel
FIELD M,      	 PSF_INST_MAG,     float,    inst mags,             mags
FIELD dM,     	 PSF_INST_MAG_SIG, float,    inst mag error,        mags
FIELD Map,       AP_MAG,  	   float,    standard aperture mag, mags
FIELD apRadius,  AP_MAG_RADIUS,    float,    radius used for fit,   pixels
FIELD Mpeak,     PEAK_FLUX_AS_MAG, float,    peak flux as a mag,    mags
FIELD Mcalib,    CAL_PSF_MAG,      float,    calibrated psf mag,    mags
FIELD dMcal,     CAL_PSF_MAG_SIG,  float,    zero point scatter,    mags
FIELD sky,    	 SKY,              float,    sky flux,              cnts/sec
FIELD dSky,    	 SKY_SIGMA,        float,    sky flux error,        cnts/sec
FIELD psfChisq,  PSF_CHISQ,        float,    psf fit chisq
FIELD crNsigma,  CR_NSIGMA,        float,    Nsigma deviations from PSF to CF
FIELD extNsigma, EXT_NSIGMA,       float,    Nsigma deviations from PSF to EXT
FIELD fx,     	 PSF_MAJOR,        float,    psf fit major axis,    pixels
FIELD fy,     	 PSF_MINOR,        float,    psf fit minor axis,    pixels
FIELD df,     	 PSF_THETA,        float,    ellipse angle,         degrees
FIELD psfQF, 	 PSF_QF,           float,    quality factor
FIELD psfNdof, 	 PSF_NDOF,         int,      psf degrees of freedom
FIELD psfNpix, 	 PSF_NPIX,         int,      psf number of pixels
FIELD Mxx,     	 MOMENTS_XX,       float,    second moment X,       pixels^2
FIELD Mxy,     	 MOMENTS_XY,       float,    second moment Y,       pixels^2
FIELD Myy,     	 MOMENTS_YY,       float,    second moment XY,      pixels^2
FIELD flags,  	 FLAGS,            int,      analysis flags
FIELD nFrames, 	 N_FRAMES,         short,    images overlapping peak
FIELD padding,   PADDING,	   short,    padding for 8byte records

# for an object in an image, we have three triplets that tell us about the shape:
# second moments: Mxx, Mxy, Myy 
# model shape parameters: F_major, F_minor, F_theta
# centroid errors: sigma_X, sigma_Y, sigma_XY
