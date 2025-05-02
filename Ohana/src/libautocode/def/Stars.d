*** note that this file / structure has been deprecated


STRUCT  Stars
EXTNAME STARS
TYPE    BINTABLE
SIZE    278

* FIELD     Xccd,             X,          double,    x coordinate on image,	     pixels
* FIELD     Yccd,             Y,          double,    y coordinate on image,	     pixels
* FIELD     dXccd,            dX,         double,    x coordinate error,	             pixels
* FIELD     dYccd,            dY,         double,    y coordinate error,  	     pixels
* FIELD     posangle,         POSANGLE,   float
* FIELD     pltscale,         PLTSCALE,   float
FIELD     R,                R,          double,    ra coordinate on sky,	     decimal degrees
FIELD     D,                D,          double,    dec coordinate on sky,	     decimal degrees
FIELD     dR,               dR,         double,    ra error,			     arcsec
FIELD     dD,               dD,         double,    dec error,			     arcsec
FIELD     uR,               U_RA,       double,    proper motion in RA,		     milliarcsec/year
FIELD     uD,               U_DEC,      double,    proper motion in DEC,	     milliarcsec/year
FIELD     duR,              U_RA_ERR,   double,    p-m error in RA,		     milliarcsec/year
FIELD     duD,              U_DEC_ERR,  double,    p-m error in DEC,		     milliarcsec/year
FIELD     P,                PAR,        double,    parallax,			     milliarcsec
FIELD     dP,               PAR_ERR,    double,    parallax error,		     milliarcsec

* FIELD     M,                M,          double,    instrumental mag
* FIELD     dM,               DM,         double,    error on mag
* FIELD     dMcal,            DMCAL,      double,    systematic error on mag
* FIELD     Sky,              SKY,        double,    local sky counts
* FIELD     dSky,             dSKY,       double,    local sky error counts
* FIELD     fx,               FX,         double,    object FWHM x-dir,		     pixels?
* FIELD     fy,               FY,         double,    object FWHM y-dir,		     pixels?
* FIELD     df,               DF,         double,    object position angle,	     degrees
* FIELD     Mcal,             MCAL,       float,     image cal magnitude
* FIELD     Map,              MAP,        double,    alternative (aperture) magnitude
FIELD     Mpeak,            MPEAK,      double,    alternative (peak) magnitude
* FIELD     detID,            ID,         int,       detection identifier
* FIELD     imageID,          IMAGE_ID,   int,       image identifier
FIELD     found,            FOUND,      int,       found in database catalog?
* FIELD     t,                T,          e_time,    date/time of exposure (UNIX)
* FIELD     t_msec,           T_MSEC,     short,     milliseconds of exposure
* FIELD     dt,               EXPTIME,    float,     exposure time,                    2.5*log(exptime)
* FIELD     psfQual,          PSF_QUAL,   float
* FIELD     psfChisq,         PSF_CHISQ,  float
* FIELD     psfNdof,          PSF_NDOF,   int
* FIELD     psfNpix,          PSF_NPIX,   int
* FIELD     crNsigma,         CR_NSIGMA,  float
* FIELD     extNsigma,        EXT_NSIGMA, float
* FIELD     Mxx,              MOMENTS_XX, float,     second moment
* FIELD     Mxy,              MOMENTS_XY, float,     second moment
* FIELD     Myy,              MOMENTS_YY, float,     second moment
* FIELD     airmass,          AIRMASS,    float,     (airmass - 1),		     airmass
* FIELD     az,     	    AZ,         float,     azimuth
* FIELD     photcode,         CODE,       short
* FIELD     nFrames,          N_FRAMES,   short
* FIELD     photFlags,        FLAGS,      short
FIELD     dummy,            DUMMY,      char[2]

# double:   24 * 8 : 192
# int/float:10 * 4 :  40
# short:     2 * 2 :   4
# char:      4 * 1 :   4

# this structure is only used internally and for interprocess communication (addstar)
# dR, uR, etc should be better defined at the pole...
# define down the types to floats where reasonable (all but R,D)?
# R,D should be in ICRS, J2000, epoch 2000 : precess as needed, apply p-m as needed

# XXX I'd like to merge this with Measure, but need to be careful about addstarClient -> addstarServer
# XXX extend photFlags to match actual psphot output
