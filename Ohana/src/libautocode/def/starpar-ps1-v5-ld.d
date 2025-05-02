STRUCT       StarPar_PS1_V5_LOAD
EXTNAME      DVO_STARPAR_PS1_V5_LOAD
TYPE         BINTABLE
SIZE         80
DESCRIPTION  DVO Table of Stellar Properties

FIELD R,              RA,            double,         ra
FIELD D,              DEC,           double,         dec
FIELD galLat,         GAL_LAT,       float,          galactic latitude
FIELD galLon,         GAL_LON,       float,          galactic longitude
FIELD Ebv,            E_BV,          float,          extinction
FIELD dEbv,           E_BV_ERR,      float,          extinction error
FIELD DistMag,        DISTANCE_MOD,     float,       mag
FIELD dDistMag,       DISTANCE_MOD_ERR, float,       mag
FIELD M_r,            M_R_ABS,       float,          r-band abs magnitude
FIELD dM_r,           M_R_ABS_ERR,   float,          r-band abs magnitude error
FIELD FeH,            F_E_H,         float,          metallicity
FIELD dFeH,           F_E_H_ERR,     float,          metallicity error
FIELD uRA,            U_RA,          float,          model guess for proper motion
FIELD uDEC,           U_DEC,         float,          model guess for proper motion
FIELD averef,         AVE_REF,       unsigned int,   reference to average entry      
FIELD objID,          OBJ_ID,        unsigned int,   unique ID for object in table
FIELD catID,          CAT_ID,        unsigned int,   unique ID for table in which object was first realized
FIELD dummy,          DUMMY,         unsigned int,   dummy
