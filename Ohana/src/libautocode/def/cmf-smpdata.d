# name of structure type
STRUCT  SMPData
EXTNAME SMPFILE
TYPE    BINTABLE
SIZE    44

# elements of data structure / FITS table
FIELD X,      X_PIX,      float,    x coord,              pixels
FIELD Y,      Y_PIX,      float,    y coord,              pixels
FIELD M,      MAG_RAW,    float,    inst mags,            mags
FIELD dM,     MAG_ERR,    float,    inst mag error,       mags
FIELD Mgal,   MAG_GAL,    float,    galaxy mag,           mags
FIELD Map,    MAG_AP,     float,    aperture mag,         mags
FIELD sky,    LOG_SKY,    float,    log-10 of sky,        cnts/sec
FIELD fx,     FWHM_X,     float,    semi-major,           pixels
FIELD fy,     FWHM_Y,     float,    semi-minor,           pixels
FIELD df,     THETA,      float,    ellipse angle,        degrees
FIELD dophot, DOPHOT,     char,     dophot type,          none
FIELD dummy,  DUMMY,      char[3],  padding,              none
