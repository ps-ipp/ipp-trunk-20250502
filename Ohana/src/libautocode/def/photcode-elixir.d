STRUCT       PhotCode_Elixir
EXTNAME      DVO_PHOTCODE_ELIXIR
TYPE         BINTABLE
SIZE         80
DESCRIPTION  DVO Photcode Description Table 

# elements of data structure / FITS table
FIELD  code,          CODE,           unsigned short, code number (stored in Measure.source) 
FIELD  name,          NAME,           char[32],       name for filter combination 
FIELD  type,          TYPE,           char,           PRI/SEC/DEP/REF 
FIELD  dummy,         DUMMY,          char[3],        padding
FIELD  C,             C_LAM,          short,          primary phot calibration terms (millimags) 
FIELD  dC,            C_LAM_ERR,      short,          primary phot calibration terms (millimags) 
FIELD  dX,            X_ERR,          short,          primary phot calibration terms (millimags) 
FIELD  K,             K,              float,          secondary phot calibration terms (millimags) 
FIELD  c1,            C1,             int,            color is average.M[c1] - average.M[c2] 
FIELD  c2,            C2,             int,            color is average.M[c1] - average.M[c2] 
FIELD  equiv,         EQUIV,          int,            this dependent filter is equivalent to equiv PRI/SEC
FIELD  Nc,            NC,             int,            number of color terms 
FIELD  X,             X,              float[4],       color terms $X[0]*mc + X[1]*mc^2 + X[2]*mc^3$, etc 
