STRUCT       Measure_PS1_SIM
EXTNAME      DVO_MEASURE_PS1_SIM
TYPE         BINTABLE
SIZE         136
DESCRIPTION  DVO Detection Measurement Table 

FIELD R,              RA,            double,         RA at epoch,                  degrees
FIELD D,              DEC,           double,         DEC at epoch,                 degrees
FIELD M,              MAG,           float,          catalog mag,                    mag
FIELD dM,             MAG_ERR,       float,          mag error,                      mag
FIELD Mcal,           M_CAL,         float,          image cal mag,                  mag
FIELD dt,             M_TIME,        float,          exposure time,                  2.5*log(exptime)

# note that with airmass = 1.0 / cos(90 - alt), we have full alt/az representation
FIELD airmass,        AIRMASS,      float,          (airmass - 1),                  airmass
FIELD az,             AZ,           float,          telescope azimuth

# new field elements needed for Pan-STARRS:
FIELD Xccd,           X_CCD,        float,          X coord on chip (raw value),  pixels
FIELD Yccd,           Y_CCD,        float,          Y coord on chip (raw value),  pixels

FIELD Xfix,           X_FIX,        float,          X coord after correction,     pixels
FIELD Yfix,           Y_FIX,        float,          Y coord after correction,     pixels

FIELD XoffKH,         X_OFF_KH,      float,          X offset from correction,     pixels
FIELD YoffKH,         Y_OFF_KH,      float,          Y offset from correction,     pixels
FIELD XoffDCR,        X_OFF_DCR,     float,          X offset from correction,     pixels
FIELD YoffDCR,        Y_OFF_DCR,     float,          Y offset from correction,     pixels

FIELD Mflat,          M_FLAT,        float,          Static Flat-field offset,     mag
FIELD dummy2,         PADDING,       int,            unused 4 bytes

FIELD t,              TIME,         int,            time in seconds (UNIX)
FIELD averef,         AVE_REF,      unsigned int,   reference to average entry      

FIELD detID,          DET_ID,       unsigned int,   detection ID
FIELD objID,          OBJ_ID,       unsigned int,   unique ID for object in table
FIELD catID,          CAT_ID,       unsigned int,   unique ID for table in which object was first realized

FIELD imageID,        IMAGE_ID,     unsigned int,   reference to DVO image ID

# do we need more resolution than a short? should this be a log?
FIELD psfQF,          PSF_QF,        float,          psf coverage/quality factor
FIELD psfQFperf,      PSF_QF_PEFECT, float,          psf coverage / quality factor (all mask bits)

FIELD photcode,       PHOTCODE,     unsigned short, photcode
FIELD dXccd,          X_CCD_ERR,    short,          X coord error on chip,          1/100 of pixels
FIELD dYccd,          Y_CCD_ERR,    short,          Y coord error on chip,          1/100 of pixels
FIELD dRsys,          POS_SYS_ERR,  short,          systematic error from astrom,   1/100 of pixels

FIELD dummy,          PADDING2,     short,          padding
FIELD posangle,       POSANGLE,     short,          position angle sky to chip,     (0xffff/360) deg
FIELD pltscale,       PLTSCALE,     float,          plate scale,                    arcsec/pixel

FIELD dbFlags,        DB_FLAGS,     unsigned int,   flags supplied by analysis in database
FIELD photFlags,      PHOT_FLAGS,   unsigned int,   flags supplied by photometry program

