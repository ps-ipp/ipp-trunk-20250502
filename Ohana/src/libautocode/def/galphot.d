STRUCT       GalPhot
EXTNAME      DVO_GALPHOT
TYPE         BINTABLE
SIZE         80
DESCRIPTION  DVO Galaxy Shape Table 

FIELD Xfit,         XFIT,    	    float,          centroid for fit
FIELD Yfit,         YFIT,    	    float,          centroid for fit
FIELD mag,          MAG,    	    float,          galaxy magnitude
FIELD magErr,       MAG_ERR,	    float,          galaxy magnitude error
FIELD majorAxis,    MAJOR_AXIS,     float,          major axis size
FIELD minorAxis,    MINOR_AXIS,     float,          minor axis size
FIELD majorAxisErr, MAJOR_AXIS_ERR, float,          major axis size error
FIELD minorAxisErr, MINOR_AXIS_ERR, float,          minor axis size error
FIELD theta,        THETA,          float,          angle
FIELD thetaErr,     THETA_ERR,      float,          angle error
FIELD index,        INDEX,          float,          sersic index (if relevant)
FIELD chisq,        CHISQ,   	    float,          fit chisq
		    
FIELD Npix,         NPIX,           float,          fitted pixels

FIELD objID,        OBJ_ID,         unsigned int,   unique ID for object in table
FIELD catID,        CAT_ID,         unsigned int,   unique ID for table in which object was first realized
FIELD detID,        DET_ID,         unsigned int,   detection ID
FIELD imageID,      IMAGE_ID,       unsigned int,   reference to GPC1 (external) image ID

FIELD averef,       AVEREF,         unsigned int,   reference to average table
FIELD flags,        FLAGS,          unsigned int,   info flags

FIELD photcode,     PHOTCODE,       short
FIELD modelType,    MODEL_TYPE,     short,          mean lensing smear object E2
