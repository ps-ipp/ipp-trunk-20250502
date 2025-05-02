STRUCT  AddstarClientOptions
EXTNAME OPTIONS
TYPE    BINTABLE
SIZE    84

FIELD     Nsigma,           NSIGMA,               double,         match radius in terms of astrometric error 
FIELD     radius,           RADIUS,		  double,         match radius in arcsec (default)
FIELD  	  mode,             MODE,                 int,            data source mode
FIELD  	  filelist,         FILELIST,             int,            if true file is a list of input files
FIELD  	  existing_regions, EXISTING_REGIONS,     int,            use only existing regions
FIELD  	  only_match,       ONLY_MATCH,           int,            only update matched stars
FIELD  	  skip_missed,      SKIP_MISSED,          int,            .
FIELD  	  replace,          REPLACE,              int,            .
FIELD  	  closest,          CLOSEST,              int,            .
FIELD  	  nosort,           NOSORT,               int,            .
FIELD  	  update,           UPDATE,               int,            .
FIELD  	  only_images,      ONLY_IMAGES,          int,            .
FIELD  	  calibrate,        CALIBRATE,            int,            .
FIELD  	  quality_airmass,  HQ_AIRMASS,           int,            use high-quality airmass?
FIELD  	  mosaic,           MOSAIC,               int,            does this image require a mosaic coordinate system?
FIELD  	  photcode,         PHOTCODE,             int,     	  photocode of input data
FIELD  	  timeref,          TIMEREF,              e_time,  	  time/date of input data (REFLIST only?)
FIELD  	  imageID,          IMAGE_ID,             unsigned int,   reference to image
FIELD  	  detectionFilter,  DETECTIONFILTER,      unsigned int,   filter mask for detections from smf file
