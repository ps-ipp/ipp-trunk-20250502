/* @file  pmModel_CentralPixel.h
 * @brief Functions to manage the central pixel for sersic-like models
 * @author EAM, IfA
 *
 * @version $Revision: 1.19 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-16 22:30:50 $
 *
 * Copyright 2013 Institute for Astronomy, University of Hawaii
 */

# ifndef PM_MODEL_CENTRAL_PIXEL_H
# define PM_MODEL_CENTRAL_PIXEL_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

typedef struct {
    psImage *flux;
    float Rmajor;
    float Aratio;
    float Sindex;
} pmModelCP;

typedef struct {
    psArray *images;

    float RmajorMin;
    float RmajorMax;
    float RmajorDel;

    float AratioMin;
    float AratioMax;
    float AratioDel;

    float SindexMin;
    float SindexMax;
    float SindexDel;

    int RmajorNitem;
    int AratioNitem;
    int SindexNitem;

    int ***lookupCube;

} pmModelCPset;

pmModelCP    *pmModelCP_Alloc(void);
pmModelCPset *pmModelCPset_Alloc(void);

pmModelCPset *pmModelCP_Load (char *filename);

pmModelCP    *pmModelCP_GetImage (pmModelCPset *CPset, float Rmajor, float Aratio, float Sindex);

float         pmModelCP_GetFlux (pmModelCP *cp, float dx, float dy, float theta);
float         pmModelCP_FullSersic (float dx, float dy, float theta, float Rmajor, float Aratio, float Sindex);

bool pmModelCP_GetFlux_BresenLineBase (float *flux, float *npix, pmModelCP *cp, int X0, int Y0, int X1, int Y1, bool swapcoords);
bool pmModelCP_GetFlux_BresenLine (float *flux, float *npix, pmModelCP *cp, int X0, int Y0, int X1, int Y1);
float pmModelCP_GetFlux_BresenSquareBase (pmModelCP *cp, int X00, int Y00, int X01, int Y01, int X10, int Y10, int X11, int Y11, bool swapcoords);
float pmModelCP_GetFlux_BresenSquare (pmModelCP *cp, int X00, int Y00, int X01, int Y01, int X10, int Y10, int X11, int Y11);
float pmModelCP_GetFlux_Bresen (pmModelCP *cp, float dx, float dy, float theta);
float pmModelCP_GetFlux_Old (pmModelCP *cp, float dx, float dy, float theta);

float pmModelCP_SersicSubpix (float dx, float dy, float Rxx, float Rxy, float Ryy, float Sindex, int Nsub);

float pmSersicKappa (float Sindex);
float pmSersicNorm (float Sindex);

# endif
