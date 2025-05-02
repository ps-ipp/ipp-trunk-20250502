# name of structure type
STRUCT  PS1_DEV_1
EXTNAME PS1_DEV_1
TYPE    BINTABLE
SIZE    72

# elements of data structure / FITS table
FIELD detID,     IPP_IDET,     	   unsigned int, detection ID
FIELD X,      	 X_PSF,    	   float,    x coord,              pixels
FIELD Y,      	 Y_PSF,    	   float,    y coord,              pixels
FIELD dX,      	 X_PSF_SIG,    	   float,    x coord error,        pixels
FIELD dY,      	 Y_PSF_SIG,    	   float,    y coord error,        pixels
FIELD M,      	 PSF_INST_MAG,     float,    inst mags,            mags
FIELD dM,     	 PSF_INST_MAG_SIG, float,    inst mag error,       mags
FIELD Mpeak,     PEAK_FLUX_AS_MAG, float,    inst mag error,       mags
FIELD sky,    	 SKY,              float,    sky flux,             cnts/sec
FIELD dSky,    	 SKY_SIG,          float,    sky flux errorf       cnts/sec
FIELD psfChisq,  PSF_CHISQ,        float,    psf fit chisq
FIELD crNsigma,  CR_NSIGMA,        float,    Nsigma deviations from PSF to CF
FIELD extNsigma, EXT_NSIGMA,       float,    Nsigma deviations from PSF to EXT
FIELD fx,     	 PSF_WIDTH_X,      float,    semi-major,           pixels
FIELD fy,     	 PSF_WIDTH_Y,      float,    semi-minor,           pixels
FIELD df,     	 PSF_THETA,        float,    ellipse angle,        degrees
FIELD psfQF, 	 PSF_QF,           float,    quality factor
FIELD nFrames, 	 N_FRAMES,         short,    images overlapping peak
FIELD flags,  	 FLAGS,            short,    padding
