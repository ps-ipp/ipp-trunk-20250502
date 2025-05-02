STRUCT       Lensobj_PS1_V5_LOAD
EXTNAME      DVO_LENSOBJ_PS1_V5_LOAD
TYPE         BINTABLE
SIZE         152
DESCRIPTION  DVO Lensobj Table 

FIELD X11_sm_obj,     X11_SM_OBJ,    float,          mean lensing smear object X11
FIELD X12_sm_obj,     X12_SM_OBJ,    float,          mean lensing smear object X12
FIELD X22_sm_obj,     X22_SM_OBJ,    float,          mean lensing smear object X22
FIELD  E1_sm_obj,      E1_SM_OBJ,    float,          mean lensing smear object E1
FIELD  E2_sm_obj,      E2_SM_OBJ,    float,          mean lensing smear object E2

FIELD X11_sh_obj,     X11_SH_OBJ,    float,          mean lensing shear object X11
FIELD X12_sh_obj,     X12_SH_OBJ,    float,          mean lensing shear object X12
FIELD X22_sh_obj,     X22_SH_OBJ,    float,          mean lensing shear object X22
FIELD  E1_sh_obj,      E1_SH_OBJ,    float,          mean lensing shear object E1
FIELD  E2_sh_obj,      E2_SH_OBJ,    float,          mean lensing shear object E2

FIELD X11_sm_psf,     X11_SM_PSF,    float,          mean lensing smear psf stars X11
FIELD X12_sm_psf,     X12_SM_PSF,    float,          mean lensing smear psf stars X12
FIELD X22_sm_psf,     X22_SM_PSF,    float,          mean lensing smear psf stars X22
FIELD  E1_sm_psf,      E1_SM_PSF,    float,          mean lensing smear psf stars E1
FIELD  E2_sm_psf,      E2_SM_PSF,    float,          mean lensing smear psf stars E2

FIELD X11_sh_psf,     X11_SH_PSF,    float,          mean lensing shear psf stars X11
FIELD X12_sh_psf,     X12_SH_PSF,    float,          mean lensing shear psf stars X12
FIELD X22_sh_psf,     X22_SH_PSF,    float,          mean lensing shear psf stars X22
FIELD  E1_sh_psf,      E1_SH_PSF,    float,          mean lensing shear psf stars E1
FIELD  E2_sh_psf,      E2_SH_PSF,    float,          mean lensing shear psf stars E2

FIELD  F_ApR5,           FLUX_AP_R5, float,          Flux inside r = 5
FIELD dF_ApR5,       FLUX_ERR_AP_R5, float,          error on Flux inside r = 5
FIELD sF_ApR5,       FLUX_STD_AP_R5, float,          stdev of Flux inside r = 5
FIELD fF_ApR5,       FLUX_FIL_AP_R5, float,          fill factor for Flux inside r = 5

FIELD  F_ApR6,           FLUX_AP_R6, float,          Flux inside r = 6
FIELD dF_ApR6,       FLUX_ERR_AP_R6, float,          error on Flux inside r = 6
FIELD sF_ApR6,       FLUX_STD_AP_R6, float,          stdev of Flux inside r = 6
FIELD fF_ApR6,       FLUX_FIL_AP_R6, float,          fill factor for Flux inside r = 6

FIELD  F_ApR7,           FLUX_AP_R7, float,          Flux inside r = 7
FIELD dF_ApR7,       FLUX_ERR_AP_R7, float,          error on Flux inside r = 7
FIELD sF_ApR7,       FLUX_STD_AP_R7, float,          stdev of Flux inside r = 7
FIELD fF_ApR7,       FLUX_FIL_AP_R7, float,          fill factor for Flux inside r = 7

FIELD  gamma,            GAMMA,      float
FIELD     E1,               E1,      float
FIELD     E2,               E2,      float

FIELD    objID,          OBJ_ID,       unsigned int,   unique ID for object in table
FIELD    catID,          CAT_ID,       unsigned int,   unique ID for table in which object was first realized
FIELD photcode,         PHOTCODE,    short
FIELD    Nmeas,            NMEAS,    short

# 31 x float
#  3 x int
# = 136 bytes / detection
# for 2.5G objects + 60 overlaps, expect 15TB for forced warp
