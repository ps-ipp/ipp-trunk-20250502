STRUCT  DetReg
EXTNAME DETREND_DATABASE
TYPE    BINTABLE
SIZE    416

# elements of data structure / FITS table

FIELD tstart,         START_TIME, e_time,    start time of measurement, seconds since 1970 Jan 01 UT
FIELD tstop,          STOP_TIME,  e_time,    stop time of measurement,  seconds since Jan 1, 1970 UT
FIELD treg,           REG_TIME,   e_time,    time of registration,      seconds since Jan 1, 1970 UT
FIELD exptime,        EXPTIME,    float,     exposure time,             seconds
FIELD type,           IMAGETYP,   int,       detrend type number
FIELD filter,         FILTER,     int,       filter number
FIELD ccd,            CCDNUM,     int,       ccd number
FIELD Nentry,         VERSION,    int,       image version number
FIELD Norder,         ORDER,      int,       selection order
FIELD mode,           MODE,       char,      image mode
FIELD altpath,        ALTPATH,    char,      available on alt db paths, true for data on alt db paths
FIELD dummy,          RESERVED,   char[58],  space for additions,       for future expansion
FIELD label,          LABEL,      char[64],  data label
FIELD filename,       PATH,       char[256], filename in db
