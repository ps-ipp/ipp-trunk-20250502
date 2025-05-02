STRUCT       SecFilt_PS1_V3
EXTNAME      DVO_SECFILT_PS1_V3
TYPE         BINTABLE
SIZE         32
DESCRIPTION  DVO SecFilt : Secondary Filter Data 

# elements of data structure / FITS table
FIELD  M,     	    MAG,      	  float,      average mag in this band,              mags
FIELD  Map,    	    MAG_AP,    	  float,      ave aperture mag in this band,         mags
FIELD  dM,    	    MAG_ERR,  	  float,      formal error on average mag,           mags
FIELD  Mchisq,      MAG_CHI,      float,      chisq on average mag,                  value
FIELD  flags, 	    FLAGS,    	  uint32_t,   photometry flags
FIELD  Ncode, 	    NCODE,    	  short,      number of detections in band
FIELD  Nused, 	    NUSED,    	  short,      number of detections used in average
FIELD  M_20,  	    MAG_20,   	  short,      lower 20percent mag,                   millimags
FIELD  M_80,  	    MAG_80,   	  short,      upper 20percent mag,                   millimags
FIELD  ubercalDist, UBERCAL_DIST, short,      number of images from an ubercal-image
FIELD  Mstdev,      MAG_STDEV,    short,      standard deviation of measurements,    millimags
