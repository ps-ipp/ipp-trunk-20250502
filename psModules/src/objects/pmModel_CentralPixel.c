/* @file  pmModel_CentralPixel.c
 * @brief Functions to manage the central pixel for sersic-like models
 * @author EAM, IfA
 *
 * @version $Revision: 1.19 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-16 22:30:50 $
 *
 * Copyright 2013 Institute for Astronomy, University of Hawaii
 */

/******************************************************************************
 * this file contains functions to determine the flux of the central pixel(s) for an
 * exponential / devaucouleur / sersic style galaxy model.  The problem is that (a) these
 * models are so centrally-peaked that it is necessary to determine the true mean flux in the
 * central pixel by integration of fractional pixels (at least 0.02 pixels in the case of a
 * DEV model) and (b) the process of integrating the central pixel is too slow to be used for
 * real processing.

 * we bypass this problem by defining a set of pre-calculated central pixel images, with
 * subpixel resolution > 1 pixel (maybe 11 subpixels per real pixel).  These pre-calculated
 * images are generated for a series of values for the following parameters: sersic index,
 * effective radius, axial ratio.  We then select the closest image to our specific case, and
 * integrate over the true sub-pixels relevant for our position and model.  We have thus turned
 * problem from 2500 evaluations of the full sersic model to ~100 straight additions (possibly
 * x 6 if we need to interpolate in each of the dimensions).  

 * we need a number of support functions:

 * pmModelCP_Load : load CP model data from the specified file. 
 * pmModelCP_GetImage : choose an appropriate CP model image for a given set of parameters
 * pmModelCP_GetValue : calculate the true CP value for the given image and parameters

   *****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pslib.h>
#include <pslib.h>

#include "pmModel_CentralPixel.h"

static void pmModelCP_Free(pmModelCP *cp) {
    psFree (cp->flux);
}

pmModelCP *pmModelCP_Alloc(void)
{
    pmModelCP *tmp = (pmModelCP *) psAlloc(sizeof(pmModelCP));
    psMemSetDeallocator(tmp, (psFreeFunc) pmModelCP_Free);

    tmp->flux = NULL;
    tmp->Rmajor = NAN;
    tmp->Aratio = NAN;
    tmp->Sindex = NAN;

    return tmp;
}

static void pmModelCPset_Free(pmModelCPset *tmp) {

    for (int i = 0; i < tmp->RmajorNitem; i++) {
	for (int j = 0; j < tmp->AratioNitem; j++) {
	    psFree (tmp->lookupCube[i][j]);
	}
	psFree (tmp->lookupCube[i]);
    }
    psFree (tmp->lookupCube);
    psFree (tmp->images);
}

pmModelCPset *pmModelCPset_Alloc(void)
{
    pmModelCPset *tmp = (pmModelCPset *) psAlloc(sizeof(pmModelCPset));
    psMemSetDeallocator(tmp, (psFreeFunc) pmModelCPset_Free);

    tmp->RmajorMin = NAN;
    tmp->RmajorMax = NAN;
    tmp->RmajorDel = NAN;

    tmp->AratioMin = NAN;
    tmp->AratioMax = NAN;
    tmp->AratioDel = NAN;

    tmp->SindexMin = NAN;
    tmp->SindexMax = NAN;
    tmp->SindexDel = NAN;

    tmp->RmajorNitem = 0;
    tmp->AratioNitem = 0;
    tmp->SindexNitem = 0;

    tmp->lookupCube = NULL;
    tmp->images = NULL;

    return tmp;
}

// load the central-pixel maps as an array of pmModelCP 
pmModelCPset *pmModelCP_Load (char *filename) {

    bool status;

    psFits *fits = psFitsOpen (filename, "r");
    if (!fits) {
	return false;
    }
    
    // read the PHU -- it defines descriptive metadata
    psMetadata *PHU = psFitsReadHeader (NULL, fits);
    if (!PHU) {
	psFitsClose (fits);
	return false;
    }

    pmModelCPset *CPset = pmModelCPset_Alloc();

    // NOTE : RMAJOR refers to the LOG_10 of the major axis
    CPset->RmajorMin = psMetadataLookupF32 (&status, PHU, "RMAJ_MIN"); psAssert (status, "missing keyword RMAJ_MIN");
    CPset->RmajorMax = psMetadataLookupF32 (&status, PHU, "RMAJ_MAX"); psAssert (status, "missing keyword RMAJ_MAX");
    CPset->RmajorDel = psMetadataLookupF32 (&status, PHU, "RMAJ_DEL"); psAssert (status, "missing keyword RMAJ_DEL");
    CPset->AratioMin = psMetadataLookupF32 (&status, PHU, "ARAT_MIN"); psAssert (status, "missing keyword ARAT_MIN");
    CPset->AratioMax = psMetadataLookupF32 (&status, PHU, "ARAT_MAX"); psAssert (status, "missing keyword ARAT_MAX");
    CPset->AratioDel = psMetadataLookupF32 (&status, PHU, "ARAT_DEL"); psAssert (status, "missing keyword ARAT_DEL");
    CPset->SindexMin = psMetadataLookupF32 (&status, PHU, "SIDX_MIN"); psAssert (status, "missing keyword SIDX_MIN");
    CPset->SindexMax = psMetadataLookupF32 (&status, PHU, "SIDX_MAX"); psAssert (status, "missing keyword SIDX_MAX");
    CPset->SindexDel = psMetadataLookupF32 (&status, PHU, "SIDX_DEL"); psAssert (status, "missing keyword SIDX_DEL");

    CPset->RmajorNitem = 1 + (int)(0.5 + (CPset->RmajorMax - CPset->RmajorMin) / CPset->RmajorDel);
    CPset->AratioNitem = 1 + (int)(0.5 + (CPset->AratioMax - CPset->AratioMin) / CPset->AratioDel);
    CPset->SindexNitem = 1 + (int)(0.5 + (CPset->SindexMax - CPset->SindexMin) / CPset->SindexDel);

    // array entry = lookupCube[RmajorBin][AratioBin][SindexBin]

    CPset->lookupCube = (int ***) psAlloc (sizeof(int **)*CPset->RmajorNitem);
    for (int i = 0; i < CPset->RmajorNitem; i++) {
	CPset->lookupCube[i] = (int **) psAlloc (sizeof(int *)*CPset->AratioNitem);
	for (int j = 0; j < CPset->AratioNitem; j++) {
	    CPset->lookupCube[i][j] = (int *) psAlloc (sizeof(int)*CPset->SindexNitem);
	    for (int k = 0; k < CPset->SindexNitem; k++) {
		CPset->lookupCube[i][j][k] = -1;
	    }
	}
    }
    
    CPset->images = psArrayAllocEmpty (CPset->RmajorNitem*CPset->AratioNitem*CPset->SindexNitem);

    // the CP file contains a set of 2D images; load them all 

    psRegion fullImage = psRegionSet (0, 0, 0, 0);
    while (true) {
	bool status = psFitsMoveExtNum (fits, 1, true);
	if (!status) break;

	psMetadata *header = psFitsReadHeader (NULL, fits);
	if (!header) {
	    fprintf (stderr, "error reading header\n");
	    return false;
	}

	pmModelCP *cp = pmModelCP_Alloc ();

	cp->flux = psFitsReadImageBuffer (NULL, fits, fullImage, 0);
	if (!cp->flux) {
	    fprintf (stderr, "error reading image\n");
	    return false;
	}

	cp->Rmajor = psMetadataLookupF32 (&status, header, "R_MAJOR");
	cp->Aratio = psMetadataLookupF32 (&status, header, "A_RATIO");
	cp->Sindex = psMetadataLookupF32 (&status, header, "S_INDEX");
	
	int RmajorBin = (int)((cp->Rmajor - CPset->RmajorMin) / CPset->RmajorDel); psAssert ((RmajorBin < CPset->RmajorNitem) && (RmajorBin >= 0), "bad bin");
	int AratioBin = (int)((cp->Aratio - CPset->AratioMin) / CPset->AratioDel); psAssert ((AratioBin < CPset->AratioNitem) && (AratioBin >= 0), "bad bin");
	int SindexBin = (int)((cp->Sindex - CPset->SindexMin) / CPset->SindexDel); psAssert ((SindexBin < CPset->SindexNitem) && (SindexBin >= 0), "bad bin");

	CPset->lookupCube[RmajorBin][AratioBin][SindexBin] = CPset->images->n;

	psArrayAdd (CPset->images, 121, cp);
	psFree (cp);
	psFree (header);
    }

    psFree (PHU);
    psFitsClose (fits);

    return CPset;
}

// choose the closest image to the given coords
pmModelCP *pmModelCP_GetImage (pmModelCPset *CPset, float Rmajor, float Aratio, float Sindex) {

    // the pmModelCP set is defined for a grid of Rmajor, Aratio, Sindex values

    int RmajorBin = (int)((Rmajor - CPset->RmajorMin) / CPset->RmajorDel); psAssert ((RmajorBin < CPset->RmajorNitem) && (RmajorBin >= 0), "bad bin");
    int AratioBin = (int)((Aratio - CPset->AratioMin) / CPset->AratioDel); psAssert ((AratioBin < CPset->AratioNitem) && (AratioBin >= 0), "bad bin");
    int SindexBin = (int)((Sindex - CPset->SindexMin) / CPset->SindexDel); psAssert ((SindexBin < CPset->SindexNitem) && (SindexBin >= 0), "bad bin");
    
    int entry = CPset->lookupCube[RmajorBin][AratioBin][SindexBin];
    
    pmModelCP *cp = CPset->images->data[entry];

    return (cp);
}

// XXX for test purposes only:
# define TEST_IMAGE 0
# if (TEST_IMAGE)
static psImage *map = NULL;
# endif

float pmModelCP_GetFlux_RotSquare (pmModelCP *cp, float dx, float dy, float theta);

float pmModelCP_GetFlux (pmModelCP *cp, float dx, float dy, float theta) {

# if (TEST_IMAGE) 
    map = psImageCopy (map, cp->flux, PS_TYPE_S32);
    psImageInit (map, 0.0);
# endif

    // float flux = pmModelCP_GetFlux_Bresen (cp, dx, dy, theta);
    // float flux = pmModelCP_GetFlux_Old (cp, dx, dy, theta);
    float flux = pmModelCP_GetFlux_RotSquare (cp, dx, dy, theta);
    
    // RotSquare for theta = 0.0 & Bresen give the same answer 
    // if I count from x[0] <= ix < x[1]

# if (TEST_IMAGE) 
    psFits *fits = psFitsOpen ("map.fits", "w");
    psFitsWriteImage (fits, NULL, map, 0, NULL);
    psFitsClose (fits);
    psFree (map);
# endif

    return flux;
}

float pmModelCP_GetFlux_Old (pmModelCP *cp, float dx, float dy, float theta) {

    // the cp data is defined for the central 3x3 pixels.  we allow dx,dy to have values of
    // -1.0 <= dx,dy <= +1.0

    // Xsub = (Xim * cos(theta) - Yim * sin(theta) + 1.5) * Nsub 
    // Ysub = (Yim * cos(theta) + Xim * sin(theta) + 1.5) * Nsub 
    
    // integrate from (dx - 0.5 to dx + 0.5), (dy - 0.5 to dy + 0.5), 

    // get the Xsub,Ysub values for the 4 corners, find the Xmin,Xmax, Ymin,Ymax in the
    // subrastered image

    float cs = cos(theta*PS_RAD_DEG);
    float sn = sin(theta*PS_RAD_DEG);

    float Nsub = 11.0;
    int Xsub00 = ((dx - 0.5)*cs - (dy - 0.5)*sn + 1.5)*Nsub;
    int Ysub00 = ((dx - 0.5)*sn + (dy - 0.5)*cs + 1.5)*Nsub;
    int Xsub01 = ((dx - 0.5)*cs - (dy + 0.5)*sn + 1.5)*Nsub;
    int Ysub01 = ((dx - 0.5)*sn + (dy + 0.5)*cs + 1.5)*Nsub;
    int Xsub10 = ((dx + 0.5)*cs - (dy - 0.5)*sn + 1.5)*Nsub;
    int Ysub10 = ((dx + 0.5)*sn + (dy - 0.5)*cs + 1.5)*Nsub;
    int Xsub11 = ((dx + 0.5)*cs - (dy + 0.5)*sn + 1.5)*Nsub;
    int Ysub11 = ((dx + 0.5)*sn + (dy + 0.5)*cs + 1.5)*Nsub;

    int Xmin, Xmax, Ymin, Ymax;

    Xmin = PS_MIN(Xsub00,Xsub01);
    Xmin = PS_MIN(Xsub10,Xmin);
    Xmin = PS_MIN(Xsub11,Xmin);
    Xmin = PS_MIN(Xmin, cp->flux->numCols - 1);
    Xmin = PS_MAX(Xmin, 0);
    Xmax = PS_MAX(Xsub00,Xsub01);
    Xmax = PS_MAX(Xsub10,Xmax);
    Xmax = PS_MAX(Xsub11,Xmax);
    Xmax = PS_MIN(Xmax, cp->flux->numCols - 1);
    Xmax = PS_MAX(Xmax, 0);
    Ymin = PS_MIN(Ysub00,Ysub01);
    Ymin = PS_MIN(Ysub10,Ymin);
    Ymin = PS_MIN(Ysub11,Ymin);
    Ymin = PS_MIN(Ymin, cp->flux->numRows - 1);
    Ymin = PS_MAX(Ymin, 0);
    Ymax = PS_MAX(Ysub00,Ysub01);
    Ymax = PS_MAX(Ysub10,Ymax);
    Ymax = PS_MAX(Ysub11,Ymax);
    Ymax = PS_MIN(Ymax, cp->flux->numRows - 1);
    Ymax = PS_MAX(Ymax, 0);

    // integrate pixels from Xmin,Ymin to Xmax,Ymax, only include pixels contained in the
    // target pixel

    float flux = 0.0;
    int   npix = 0;
    for (int i = Xmin; i < Xmax; i++) {
	float dX = i / Nsub - 1.5;
	for (int j = Ymin; j < Ymax; j++) {
	    float dY = j / Nsub - 1.5;

	    float Xim =  dX*cs + dY*sn;
	    if (Xim < (dx - 0.5)) continue;
	    if (Xim > (dx + 0.5)) continue;

	    float Yim = -dX*sn + dY*cs;
	    if (Yim < (dy - 0.5)) continue;
	    if (Yim > (dy + 0.5)) continue;

	    flux += cp->flux->data.F32[j][i];
	    npix ++;
	}
    }
	   
    float normFlux = flux / npix;
    return normFlux;
}

// *** pmSourceRadialProfileSortPair is a utility function for sorting a pair of vectors
# define COMPARE_INDEX(A,B) (y[A] < y[B])
# define SWAP_INDEX(TYPE,A,B) {				\
	int tmp;					\
	if (A != B) {					\
	    tmp = x[A];					\
	    x[A] = x[B];				\
	    x[B] = tmp;					\
	    tmp = y[A];					\
	    y[A] = y[B];				\
	    y[B] = tmp;					\
	}						\
    }

bool pmModelCP_SortCorners (int *x, int *y, int Npar) {

    if (Npar < 2) return true;

    // sort the vector set by the radius
    PSSORT (Npar, COMPARE_INDEX, SWAP_INDEX, NONE);
    return true;
}

float pmModelCP_GetFlux_RotSquare (pmModelCP *cp, float dx, float dy, float theta) {

    // the cp data is defined for the central 3x3 pixels.  we allow dx,dy to have values of
    // -1.0 <= dx,dy <= +1.0

    // Xsub = (Xim * cos(theta) - Yim * sin(theta) + 1.5) * Nsub 
    // Ysub = (Yim * cos(theta) + Xim * sin(theta) + 1.5) * Nsub 
    
    // integrate from (dx - 0.5 to dx + 0.5), (dy - 0.5 to dy + 0.5), 

    // get the Xsub,Ysub values for the 4 corners, find the Xmin,Xmax, Ymin,Ymax in the
    // subrastered image

    float cs = cos(theta*PS_RAD_DEG);
    float sn = sin(theta*PS_RAD_DEG);
    float Nsub = 11.0;

    int Xsub[4], Ysub[4];

    Xsub[0] = ((dx - 0.5)*cs - (dy - 0.5)*sn + 1.5)*Nsub;
    Ysub[0] = ((dx - 0.5)*sn + (dy - 0.5)*cs + 1.5)*Nsub;
    Xsub[1] = ((dx - 0.5)*cs - (dy + 0.5)*sn + 1.5)*Nsub;
    Ysub[1] = ((dx - 0.5)*sn + (dy + 0.5)*cs + 1.5)*Nsub;
    Xsub[2] = ((dx + 0.5)*cs - (dy - 0.5)*sn + 1.5)*Nsub;
    Ysub[2] = ((dx + 0.5)*sn + (dy - 0.5)*cs + 1.5)*Nsub;
    Xsub[3] = ((dx + 0.5)*cs - (dy + 0.5)*sn + 1.5)*Nsub;
    Ysub[3] = ((dx + 0.5)*sn + (dy + 0.5)*cs + 1.5)*Nsub;

    // first, sort the corners in order of the Y coordinate:
    pmModelCP_SortCorners (Xsub, Ysub, 4);

    float flux = 0.0;
    float npix = 0.0;

    // if (Ysub[0] == Ysub[1]), we have a simple square
    if (Ysub[0] == Ysub[1]) {
	psAssert (Ysub[2] == Ysub[3], "not square?");
	int Xmin = PS_MIN(Xsub[0], Xsub[1]);
	int Xmax = PS_MAX(Xsub[0], Xsub[1]);
	for (int iy = Ysub[0]; iy < Ysub[3]; iy++) {
	    for (int ix = Xmin; ix < Xmax; ix++) {
		flux += cp->flux->data.F32[iy][ix];
		npix += 1.0;
# if (TEST_IMAGE) 
		fprintf (stderr, "%d %d | %f %f | %f\n", ix, iy, flux, npix, cp->flux->data.F32[iy][ix]);
		map->data.S32[iy][ix] ++;
# endif
	    }
	}
	float normFlux = flux / npix;
	return normFlux;
    }
    
    // second case: Xsub[1] > Xsub[2]:
    if (Xsub[1] > Xsub[2]) {
	float dYdXp, dYdXm;
	// first segment, Ysub[0] to Ysub[1]:
	dYdXp = (Ysub[1] - Ysub[0]) / (float) (Xsub[1] - Xsub[0]);
	dYdXm = (Ysub[2] - Ysub[0]) / (float) (Xsub[2] - Xsub[0]);
	for (int iy = Ysub[0]; iy < Ysub[1]; iy++) {
	    int Xs = (iy - Ysub[0]) / dYdXm + Xsub[0];
	    int Xe = (iy - Ysub[0]) / dYdXp + Xsub[0];
	    for (int ix = Xs; ix < Xe; ix ++) {
		flux += cp->flux->data.F32[iy][ix];
		npix += 1.0;
# if (TEST_IMAGE) 
		fprintf (stderr, "%d %d | %f %f | %f\n", ix, iy, flux, npix, cp->flux->data.F32[iy][ix]);
		map->data.S32[iy][ix] ++;
# endif
	    }
	}
	// 2nd segment, Ysub[1] to Ysub[2]:
	dYdXp = (Ysub[3] - Ysub[1]) / (float) (Xsub[3] - Xsub[1]);
	dYdXm = (Ysub[2] - Ysub[0]) / (float) (Xsub[2] - Xsub[0]);
	for (int iy = Ysub[1]; iy < Ysub[2]; iy++) {
	    int Xs = (iy - Ysub[0]) / dYdXm + Xsub[0];
	    int Xe = (iy - Ysub[1]) / dYdXp + Xsub[1];
	    for (int ix = Xs; ix < Xe; ix ++) {
		flux += cp->flux->data.F32[iy][ix];
		npix += 1.0;
# if (TEST_IMAGE) 
		fprintf (stderr, "%d %d | %f %f | %f\n", ix, iy, flux, npix, cp->flux->data.F32[iy][ix]);
		map->data.S32[iy][ix] ++;
# endif
	    }
	}
	// first segment, Ysub[0] to Ysub[1]:
	dYdXp = (Ysub[3] - Ysub[1]) / (float) (Xsub[3] - Xsub[1]);
	dYdXm = (Ysub[3] - Ysub[2]) / (float) (Xsub[3] - Xsub[2]);
	for (int iy = Ysub[2]; iy < Ysub[3]; iy++) {
	    int Xs = (iy - Ysub[2]) / dYdXm + Xsub[2];
	    int Xe = (iy - Ysub[1]) / dYdXp + Xsub[1];
	    for (int ix = Xs; ix < Xe; ix ++) {
		flux += cp->flux->data.F32[iy][ix];
		npix += 1.0;
# if (TEST_IMAGE) 
		fprintf (stderr, "%d %d | %f %f | %f\n", ix, iy, flux, npix, cp->flux->data.F32[iy][ix]);
		map->data.S32[iy][ix] ++;
# endif
	    }
	}
	float normFlux = flux / npix;
	return normFlux;
    }

    // third case: Xsub[1] < Xsub[2]:
    if (Xsub[2] > Xsub[1]) {
	// first segment, Ysub[0] to Ysub[1]:
	float dYdXp, dYdXm;
	dYdXp = (Ysub[2] - Ysub[0]) / (float) (Xsub[2] - Xsub[0]);
	dYdXm = (Ysub[1] - Ysub[0]) / (float) (Xsub[1] - Xsub[0]);
	for (int iy = Ysub[0]; iy < Ysub[1]; iy++) {
	    int Xs = (iy - Ysub[0]) / dYdXm + Xsub[0];
	    int Xe = (iy - Ysub[0]) / dYdXp + Xsub[0];
	    for (int ix = Xs; ix < Xe; ix ++) {
		flux += cp->flux->data.F32[iy][ix];
		npix += 1.0;
# if (TEST_IMAGE) 
		fprintf (stderr, "%d %d | %f %f | %f\n", ix, iy, flux, npix, cp->flux->data.F32[iy][ix]);
		map->data.S32[iy][ix] ++;
# endif
	    }
	}
	// 2nd segment, Ysub[1] to Ysub[2]:
	dYdXp = (Ysub[2] - Ysub[0]) / (float) (Xsub[2] - Xsub[0]);
	dYdXm = (Ysub[3] - Ysub[1]) / (float) (Xsub[3] - Xsub[1]);
	for (int iy = Ysub[1]; iy < Ysub[2]; iy++) {
	    int Xs = (iy - Ysub[1]) / dYdXm + Xsub[1];
	    int Xe = (iy - Ysub[0]) / dYdXp + Xsub[0];
	    for (int ix = Xs; ix < Xe; ix ++) {
		flux += cp->flux->data.F32[iy][ix];
		npix += 1.0;
# if (TEST_IMAGE) 
		fprintf (stderr, "%d %d | %f %f | %f\n", ix, iy, flux, npix, cp->flux->data.F32[iy][ix]);
		map->data.S32[iy][ix] ++;
# endif
	    }
	}
	// first segment, Ysub[0] to Ysub[1]:
	dYdXp = (Ysub[3] - Ysub[2]) / (float) (Xsub[3] - Xsub[2]);
	dYdXm = (Ysub[3] - Ysub[1]) / (float) (Xsub[3] - Xsub[1]);
	for (int iy = Ysub[2]; iy < Ysub[3]; iy++) {
	    int Xs = (iy - Ysub[1]) / dYdXm + Xsub[1];
	    int Xe = (iy - Ysub[2]) / dYdXp + Xsub[2];
	    for (int ix = Xs; ix < Xe; ix ++) {
		flux += cp->flux->data.F32[iy][ix];
		npix += 1.0;
# if (TEST_IMAGE) 
		fprintf (stderr, "%d %d | %f %f | %f\n", ix, iy, flux, npix, cp->flux->data.F32[iy][ix]);
		map->data.S32[iy][ix] ++;
# endif
	    }
	}
	float normFlux = flux / npix;
	return normFlux;
    }
    myAbort ("impossible case?");
}

float pmModelCP_GetFlux_Bresen (pmModelCP *cp, float dx, float dy, float theta) {

    // the cp data is defined for the central 3x3 pixels.  we allow dx,dy to have values of
    // -1.0 <= dx,dy <= +1.0

    // Xsub = ( Xim * cos(theta) + Yim * sin(theta) + 1.5) * Nsub 
    // Ysub = (-Yim * cos(theta) + Xim * sin(theta) + 1.5) * Nsub 
    
    // integrate from (dx - 0.5 to dx + 0.5), (dy - 0.5 to dy + 0.5), 

    // get the Xsub,Ysub values for the 4 corners, find the Xmin,Xmax, Ymin,Ymax in the
    // subrastered image

    float cs = cos(theta*PS_RAD_DEG);
    float sn = sin(theta*PS_RAD_DEG);

    float Nsub = 11.0;
    int Xsub00 = 0.5 + ((dx - 0.5)*cs + (dy - 0.5)*sn + 1.5)*Nsub;
    int Ysub00 = 0.5 + ((dy - 0.5)*cs - (dx - 0.5)*sn + 1.5)*Nsub;
    int Xsub01 = 0.5 + ((dx - 0.5)*cs + (dy + 0.5)*sn + 1.5)*Nsub;
    int Ysub01 = 0.5 + ((dy + 0.5)*cs - (dx - 0.5)*sn + 1.5)*Nsub;
    int Xsub10 = 0.5 + ((dx + 0.5)*cs + (dy - 0.5)*sn + 1.5)*Nsub;
    int Ysub10 = 0.5 + ((dy - 0.5)*cs - (dx + 0.5)*sn + 1.5)*Nsub;
    int Xsub11 = 0.5 + ((dx + 0.5)*cs + (dy + 0.5)*sn + 1.5)*Nsub;
    int Ysub11 = 0.5 + ((dy + 0.5)*cs - (dx + 0.5)*sn + 1.5)*Nsub;

    float flux = pmModelCP_GetFlux_BresenSquare (cp, Xsub00, Ysub00, Xsub10, Ysub10, Xsub01, Ysub01, Xsub11, Ysub11);
    return flux;
}

// first line is (X00,Y00) - (X10,Y1) : last line is (X01,Y01) - (X11,Y11)
float pmModelCP_GetFlux_BresenSquare (pmModelCP *cp, int X00, int Y00, int X10, int Y10, int X01, int Y01, int X11, int Y11) {

    int dX0 = X01 - X00;
    int dY0 = Y01 - Y00;

    // int dX1 = X11 - X10;
    // int dY1 = Y11 - Y10;

    // myAssert ((dX0 == dX1) && (dY0 == dY1), "pixel is not square?");

    bool FlipCoords = (abs(dX0) < abs(dY0));
    bool FlipDirect = FlipCoords ? (Y00 > Y10) : (X00 > X10);

    float flux = 0.0;
    if (!FlipDirect && !FlipCoords) flux = pmModelCP_GetFlux_BresenSquareBase (cp, X00, Y00, X10, Y10, X01, Y01, X11, Y11, false);
    if ( FlipDirect && !FlipCoords) flux = pmModelCP_GetFlux_BresenSquareBase (cp, X10, Y10, X00, Y00, X11, Y11, X01, Y01, false);
    if (!FlipDirect &&  FlipCoords) flux = pmModelCP_GetFlux_BresenSquareBase (cp, Y00, X00, Y10, X10, Y01, X01, Y11, X11, true);
    if ( FlipDirect &&  FlipCoords) flux = pmModelCP_GetFlux_BresenSquareBase (cp, Y10, X10, Y00, X00, Y11, X11, Y01, X01, true);
    return flux;
}

// draw a line between (X00,Y00) & (X01,Y01) and increment to the next line segment until endpoints (X10,Y10) & (X11,Y11)
float pmModelCP_GetFlux_BresenSquareBase (pmModelCP *cp, int X00, int Y00, int X10, int Y10, int X01, int Y01, int X11, int Y11, bool swapcoords) {

    int dX0 = X01 - X00;
    int dY0 = Y01 - Y00;

    // int dX1 = X11 - X10;
    // int dY1 = Y11 - Y10;

    // myAssert ((dX0 == dX1) && (dY0 == dY1), "pixel is not square?");

    float flux = 0.0;
    float npix = 0.0;

    int Ys = Y00;
    int Ye = Y10;
    int e = 0;
    for (int Xs = X00, Xe = X10; Xs < X01; Xs++, Xe++) {
	if (swapcoords) {
	    pmModelCP_GetFlux_BresenLine (&flux, &npix, cp, Ys, Xs, Ye, Xe);
	} else {
	    pmModelCP_GetFlux_BresenLine (&flux, &npix, cp, Xs, Ys, Xe, Ye);
	}
	e += dY0;
	float e2 = 2 * e;
	if (e2 > dX0) {
	    Ys++;
	    Ye++;
	    e -= dX0;
	} 
	if (e2 < -dX0) {
	    Ys--;
	    Ye--;
	    e += dX0;
	}
    }
    float normFlux = flux / npix;
    // fprintf (stderr, "bres: %f %f %f\n", flux, (float) npix, normFlux);
    return normFlux;
}

// get the sequence right: 
// if abs(dY) > abs(dX) : we will run in the Y direction not the X direction (we swap X and Y going in)
// if the direction (dX or dY) is negative, go the opposite direction
bool pmModelCP_GetFlux_BresenLine (float *flux, float *npix, pmModelCP *cp, int X0, int Y0, int X1, int Y1) {

  int dX = X1 - X0;
  int dY = Y1 - Y0;

  bool FlipCoords = (abs(dX) < abs(dY));
  bool FlipDirect = FlipCoords ? (Y0 > Y1) : (X0 > X1);

  if (!FlipDirect && !FlipCoords) pmModelCP_GetFlux_BresenLineBase (flux, npix, cp, X0, Y0, X1, Y1, false);
  if ( FlipDirect && !FlipCoords) pmModelCP_GetFlux_BresenLineBase (flux, npix, cp, X1, Y1, X0, Y0, false);
  if (!FlipDirect &&  FlipCoords) pmModelCP_GetFlux_BresenLineBase (flux, npix, cp, Y0, X0, Y1, X1, true);
  if ( FlipDirect &&  FlipCoords) pmModelCP_GetFlux_BresenLineBase (flux, npix, cp, Y1, X1, Y0, X0, true);
  return true;
}

bool pmModelCP_GetFlux_BresenLineBase (float *flux, float *npix, pmModelCP *cp, int X0, int Y0, int X1, int Y1, bool swapcoords) {

    int dX = X1 - X0;
    int dY = Y1 - Y0;

    int Y = Y0;
    int e = 0;
    for (int X = X0; X < X1; X++) {
	if (swapcoords) {
	    *flux += cp->flux->data.F32[X][Y];
	    *npix += 1.0;
# if (TEST_IMAGE) 
	    fprintf (stderr, "%d %d | %f %f | %f\n", X, Y, *flux, *npix, cp->flux->data.F32[X][Y]);
	    map->data.S32[X][Y] ++;
# endif
	} else {
	    *flux += cp->flux->data.F32[Y][X];
	    *npix += 1.0;
# if (TEST_IMAGE) 
	    fprintf (stderr, "%d %d | %f %f | %f\n", X, Y, *flux, *npix, cp->flux->data.F32[Y][X]);
	    map->data.S32[Y][X] ++;
# endif
	}
	e += dY;
	float e2 = 2 * e;
	if (e2 > dX) {
	    Y++;
	    e -= dX;
	} 
	if (e2 < -dX) {
	    Y--;
	    e += dX;
	}
    }
    return true;
}

// this is a test function which generates a full sersic model evaluation with sub-pixel sampling
float pmModelCP_FullSersic (float dx, float dy, float theta, float Rmajor, float Aratio, float Sindex) {

    float flux = 0.0;
    int   npix = 0;

    float Rminor = Aratio * Rmajor;
    float f1 = 1.0 / PS_SQR(Rminor) + 1.0 / PS_SQR(Rmajor);
    float f2 = 1.0 / PS_SQR(Rminor) - 1.0 / PS_SQR(Rmajor);
    
    float sxr = 0.5*f1 - 0.5*f2*cos(2.0*theta*PS_RAD_DEG);
    float syr = 0.5*f1 + 0.5*f2*cos(2.0*theta*PS_RAD_DEG);
    
    float Rxx  = +1.0 / sqrt(sxr);
    float Ryy  = +1.0 / sqrt(syr);
    float Rxy = -f2*sin(2.0*theta*PS_RAD_DEG);
    
    float kappa = -0.275552 + 1.972625*Sindex + 0.003487 * PS_SQR(Sindex);
    float rindex = 0.5 / Sindex;

    float off = -60.0/(11*11);
    float delta = 1.0 / (11*11);
    for (float ix = off; ix < 0.5; ix += delta) {
	for (float iy = off; iy < 0.5; iy += delta) {

	    float dX = dx + ix;
	    float dY = dy + iy;
	    float z = PS_SQR(dX / Rxx) + PS_SQR(dY / Ryy) + dX * dY * Rxy;

	    float q = pow (z, rindex);
	    float f = exp(-kappa*q);

	    flux += f;
	    npix ++;
	}
    }
    float normFlux = flux / npix;
    // fprintf (stderr, "full : %f %f %f\n", flux, (float) npix, normFlux);
    return normFlux;
}

// this is a test function which generates a full sersic model evaluation with sub-pixel sampling
// Nsub is the number of sub-pixel samplings and must be odd
// dx,dy are the centroid offset
float pmModelCP_SersicSubpix (float dx, float dy, float Rxx, float Rxy, float Ryy, float Sindex, int Nsub) {

    float flux = 0.0;
    int   npix = 0;

    // -0.275552 + 1.972625*Sindex + 0.003487 * PS_SQR(Sindex);
    float kappa = pmSersicKappa (Sindex);
    float rindex = 0.5 / Sindex;

    // have the resolution be a user-parameter?
    psAssert (Nsub % 2 == 1, "Nsub is not odd");
    int Nsub2 = (Nsub - 1) / 2;

    float delta = 1.0 / (float) Nsub;
    // float off = -Nsub2 * delta;

    int Sx = (int) floor(dx / delta);
    int Sy = (int) floor(dy / delta);

    for (int ix = -Nsub2; ix <= Nsub2; ix++) {
      float dX = delta * (Sx + ix);
      for (int iy = -Nsub2; iy <= Nsub2; iy++) {
	float dY = delta * (Sy + iy);
	    float z = PS_SQR(dX / Rxx) + PS_SQR(dY / Ryy) + dX * dY * Rxy;

	    float q = pow (z, rindex);
	    float f = exp(-kappa*q);

	    // if ((ix == 0) && (iy == 0)) {
	    //   // fprintf (stderr, "this: %f  %f  %f  --  full : %f %f\n", z, q, f, flux, (float) npix);
	    // }

	    flux += f;
	    npix ++;
	}
    }
    float normFlux = flux / npix;
    // fprintf (stderr, "full : %f %f %f\n", flux, (float) npix, normFlux);
    return normFlux;
}

float pmSersicKappa (float Sindex) {
    // this function is empirically derived from a fit to data for Sindex = 0.5 - 5.5
    // constrain Sindex or kappa?
    float kappa = -0.275552 + 1.972625*Sindex + 0.003487 * PS_SQR(Sindex);
    return kappa;
}

float pmSersicNorm (float Sindex) {

    float C0 = NAN;
    float C1 = NAN;
    float C2 = NAN;

    // y = 0.201545 x^0 -0.950965 x^1 -0.315248 x^2 
    // y = 0.402084 x^0 -1.357775 x^1 -0.105102 x^2 
    // y = 0.619093 x^0 -1.591674 x^1 -0.041576 x^2 
    // y = 0.770263 x^0 -1.696421 x^1 -0.023363 x^2 
    // y = 0.885891 x^0 -1.755684 x^1 -0.015753 x^2 

    if ((Sindex >= 0.0) && (Sindex < 1.0)) { 
	C0 = 0.201545; C1 = -0.950965; C2 = -0.315248;
	// y = 0.201545 x^0 -0.950965 x^1 -0.315248 x^2 
    }
    if ((Sindex >= 1.0) && (Sindex < 2.0)) { 
	C0 = 0.402084; C1 = -1.357775; C2 = -0.105102;
	// y = 0.402084 x^0 -1.357775 x^1 -0.105102 x^2 
    }
    if ((Sindex >= 2.0) && (Sindex < 3.0)) { 
	C0 = 0.619093; C1 = -1.591674; C2 = -0.041576;
	// y = 0.619093 x^0 -1.591674 x^1 -0.041576 x^2 
    }
    if ((Sindex >= 3.0) && (Sindex < 4.0)) { 
	C0 = 0.770263; C1 = -1.696421; C2 = -0.023363;
	// y = 0.770263 x^0 -1.696421 x^1 -0.023363 x^2 
    }
    if ((Sindex >= 4.0) && (Sindex < 5.5)) { 
	C0 = 0.885891; C1 = -1.755684; C2 = -0.015753; 
	// y = 0.885891 x^0 -1.755684 x^1 -0.015753 x^2 
    }

    if (isnan(C0)) return NAN;

    float lnorm = C0 + C1*Sindex + C2*Sindex*Sindex;
    float norm = exp(lnorm);
    return norm;
}

# if (0)
// create a vector containing only the unique entries in the input vector
psVector *psVectorUniqueSubset (psVector *input) {

    // sort the input vector (to new temp vector)
    // run through the sorted vector, copying to a new output vector if the current value is
    // new

    psVector *temp = psVectorSort (input);
    
    psVector *output = psVectorAllocEmpty (0.5*input->n, PS_TYPE_F32);
    
    psVectorAppend (output, temp->data.F32[0]);
    float lastValue = temp->data.F32[0];
    for (int i = 0; i < temp->n; i++) {
	if (temp->data.F32[i] == lastValue) continue;
	psVectorAppend (output, temp->data.F32[i]);
	float lastValue = temp->data.F32[i];
    }
    psFree (temp);
    return output;
}

getUnique() {
    // we need to convert the collection of Rmajor, Aratio, Sindex values to a cube
    // such that entry[RmajorBin][AratioBin][SindexBin] is the CPset array element

    // create full vectors will all Rmajor, Aratio, Sindex values:
    psVector *RmajorAll = psVectorAllocEmpty (CPset->n, PS_TYPE_F32);
    psVector *AratioAll = psVectorAllocEmpty (CPset->n, PS_TYPE_F32);
    psVector *SindexAll  = psVectorAllocEmpty (CPset->n, PS_TYPE_F32);
    for (int i = 0; i < CPset->n; i++) {
	pmModelCP *cp = CPset->data[i];
	psVectorAppend (RmajorAll, cp->Rmajor);
	psVectorAppend (AratioAll, cp->Aratio);
	psVectorAppend (SindexAll,  cp->Sindex);
    }
    psVector *RmajorUniq = psVectorUniqueSubset (RmajorAll);
    psVector *AratioUniq = psVectorUniqueSubset (AratioAll);
    psVector *SindexUniq  = psVectorUniqueSubset (SindexAll);
}
# endif
