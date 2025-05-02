STRUCT       Measure_PS1_V4
EXTNAME      DVO_MEASURE_PS1_V4
TYPE         BINTABLE
SIZE         184
DESCRIPTION  DVO Detection Measurement Table 

FIELD dR,             D_RA,         float,          RA offset,                	  arcsec
FIELD dD,             D_DEC,        float,          DEC offset,               	  arcsec
FIELD M,              MAG,          float,          catalog mag,       	       	  mag
FIELD Mcal,           M_CAL,        float,          image cal mag,	          mag
FIELD Map,            M_APER,       float,          aperture mag,		  mag
FIELD Mkron,	      M_KRON,	    float,	    kron magnitude,		  mag
FIELD dMkron,	      M_KRON_ERR,   float,	    kron magnitude error,	  mag
FIELD dM,             MAG_ERR,      float,          mag error,                    mag
FIELD dMcal,          MAG_CAL_ERR,  float,          systematic calibration error, mag
FIELD dt,             M_TIME,       float,          exposure time,                2.5*log(exptime)

# for stacks only?
FIELD FluxPSF,        FLUX_PSF,     float,          flux from psf fit,              counts/sec?
FIELD dFluxPSF,       FLUX_PSF_ERR, float,          error on psf flux,              counts/sec?
FIELD FluxKron,       FLUX_KRON,     float,         flux from kron ap,              counts/sec?
FIELD dFluxKron,      FLUX_KRON_ERR, float,         error on kron flux,             counts/sec?

# note that with airmass = 1.0 / cos(90 - alt), we have full alt/az representation
FIELD airmass,        AIRMASS,      float,          (airmass - 1),		  airmass
FIELD az,             AZ,           float,          telescope azimuth

# new field elements needed for Pan-STARRS:
FIELD Xccd,           X_CCD,        float,          X coord on chip (raw value),  pixels
FIELD Yccd,           Y_CCD,        float,          Y coord on chip (raw value),  pixels

FIELD Xfix,           X_FIX,        float,          X coord after correction,     pixels
FIELD Yfix,           Y_FIX,        float,          Y coord after correction,     pixels

# could these be packed into fewer bits?
FIELD Sky,            SKY_FLUX,     float,          local estimate of sky flux,    counts/sec
FIELD dSky,           SKY_FLUX_ERR, float,          local estimate of sky flux,    counts/sec

FIELD t,              TIME,         int,   	    time in seconds (UNIX)
FIELD averef,         AVE_REF,      unsigned int,   reference to average entry      

# internally, this is an unsigned int; however, we do NOT convert with TZERO/TSCAL on output
FIELD detID,          DET_ID,       unsigned int,   detection ID
FIELD imageID,        IMAGE_ID,     unsigned int,   reference to DVO image ID
FIELD objID,          OBJ_ID,       unsigned int,   unique ID for object in table
FIELD catID,          CAT_ID,       unsigned int,   unique ID for table in which object was first realized

# PSPS uses a 64-bit detection ID
FIELD extID,          EXT_ID,       uint64_t,       external ID (eg PSPS detID)

# do we need more resolution than a short? should this be a log?
FIELD psfQF,          PSF_QF,        float,          psf coverage/quality factor
FIELD psfQFperf,      PSF_QF_PEFECT, float,          psf coverage / quality factor (all mask bits)
FIELD psfChisq,       PSF_CHISQ,     float,          psf fit chisq

FIELD psfNdof,        PSF_NDOF,      int,            psf degrees of freedom
FIELD psfNpix,        PSF_NPIX,      int,            psf number of pixels
FIELD crNsigma,       CR_NSIGMA,     float,          Nsigma deviation towards CR
FIELD extNsigma,      EXT_NSIGMA,    float,          Nsigma deviation towards EXT

# model shape parameters
FIELD FWx,            FWHM_MAJOR,   short,          object fwhm major axis,         1/100 of pixels
FIELD FWy,            FWHM_MINOR,   short,          object fwhm minor axis,         1/100 of pixels 
FIELD theta,          PSF_THETA,    short,          angle wrt ccd X dir,            (0xffff/360) deg

# moments
FIELD Mxx,            MXX,          short,          second moments in pixel coords, 1/100 of pixels
FIELD Mxy,            MXY,          short,          second moments in pixel coords, 1/100 of pixels
FIELD Myy,            MYY,          short,          second moments in pixel coords, 1/100 of pixels

# fractional exposure time
FIELD t_msec,         TIME_MSEC,    unsigned short, time fraction of second,       milliseconds
FIELD photcode,       PHOTCODE,     unsigned short, photcode

# position errors
FIELD dXccd,          X_CCD_ERR,    short,          X coord error on chip,          1/100 of pixels
FIELD dYccd,          Y_CCD_ERR,    short,          Y coord error on chip,          1/100 of pixels
FIELD dRsys,          POS_SYS_ERR,  short,          systematic error from astrom,   1/100 of pixels

# local astrometry scales
FIELD posangle,       POSANGLE,     short,          position angle sky to chip,     (0xffff/360) deg
FIELD pltscale,       PLTSCALE,     float,          plate scale,                    arcsec/pixel

FIELD dbFlags,        DB_FLAGS,     unsigned int,   flags supplied by analysis in database
FIELD photFlags,      PHOT_FLAGS,   unsigned int,   flags supplied by photometry program


# 19 x float
# 6 x int or unsigned int
# 11 x short or unsigned short
# 3 x uint64_t

# = 
