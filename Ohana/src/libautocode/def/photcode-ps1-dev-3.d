STRUCT       PhotCode_PS1_DEV_3
EXTNAME      DVO_PHOTCODE_PS1_DEV_3
TYPE         BINTABLE
SIZE         104
DESCRIPTION  DVO Photcode Description Table 

# elements of data structure / FITS table
FIELD  code,         	  CODE,          	 unsigned short, code number (stored in Measure.source) 
FIELD  name,         	  NAME,          	 char[32],       name for filter combination 
FIELD  type,         	  TYPE,          	 char,           PRI/SEC/DEP/REF 
FIELD  dummy,        	  DUMMY,         	 char[3],        padding
FIELD  C,            	  C_LAM,         	 short,          primary phot calibration terms (millimags) 
FIELD  dC,           	  C_LAM_ERR,     	 short,          primary phot calibration terms (millimags) 
FIELD  dX,           	  X_ERR,         	 short,          primary phot calibration terms (millimags) 
FIELD  K,            	  K,             	 float,          secondary phot calibration terms (millimags) 
FIELD  c1,           	  C1,            	 int,            color is average.M[c1] - average.M[c2] 
FIELD  c2,           	  C2,            	 int,            color is average.M[c1] - average.M[c2] 
FIELD  equiv,        	  EQUIV,         	 int,            this dependent filter is equivalent to equiv PRI/SEC
FIELD  Nc,           	  NC,            	 int,            number of color terms 
FIELD  X,            	  X,             	 float[4],       color terms $X[0]*mc + X[1]*mc^2 + X[2]*mc^3$, etc 
FIELD  astromErrSys,      ASTROM_ERR_SYS,  	 float,          systematic astrometry error (arcsec)
FIELD  astromErrScale,    ASTROM_ERR_SCALE,  	 float,          astrometric error scale
FIELD  astromErrMagScale, ASTROM_ERR_MAG_SCALE,  float,          astrometric error / mag error scale
FIELD  astromPoorMask,    ASTROM_POOR_MASK,      short,          detections matching this mask should only be used in emergencies
FIELD  astromBadMask,     ASTROM_BAD_MASK,       short,          detections matching this mask should not be used
FIELD  photomErrSys,   	  PHOTOM_ERR_SYS,  	 float,          systematic photometric error
FIELD  photomPoorMask, 	  PHOTOM_POOR_MASK,  	 short,          detections matching this mask should only be used in emergencies
FIELD  photomBadMask,  	  PHOTOM_BAD_MASK,  	 short,          detections matching this mask should not be used

#   dR_total^2 =  dR_sys^2 + AS * dR_obs^2 + MS * dM_obs^2
#   dR_sys : systematicAstrometryError
#   AS     : astrometryErrorScale
#   MS     : astrometryErrorMagScale
