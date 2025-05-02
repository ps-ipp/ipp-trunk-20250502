/** @file psastroMosaicCorrectDistortion.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

bool psastroMosaicCorrectDistortion (pmFPA *fpa, psPlaneTransform *TPtoFP) {

    // invert the linear TPtoFP transform
    psPlaneTransform *FPtoTP = p_psPlaneTransformLinearInvert (TPtoFP);
    if (FPtoTP == NULL) {
        psError (PS_ERR_UNKNOWN, false, "failed to invert TPtoFP\n");
        return false;
    }

    // Store the new coeffs in a new structure.  Forward transformations (chip->fpa->tpa->sky)
    // use Ordinary polynomials
    psPlaneTransform *toTPAnew = psPlaneTransformAlloc(fpa->toTPA->x->nX, fpa->toTPA->x->nY, PS_POLYNOMIAL_ORD);

    // set the new coeffs, or set the mask
    for (int i = 0; i <= toTPAnew->y->nX; i++) {
        for (int j = 0; j <= toTPAnew->y->nY; j++) {

	    // init the coeffs
	    toTPAnew->x->coeff[i][j] = 0.0;
	    toTPAnew->y->coeff[i][j] = 0.0;

	    // if both are masked, mask the outpu
            if ((fpa->toTPA->x->coeffMask[i][j] & PS_POLY_MASK_SET) && (fpa->toTPA->x->coeffMask[i][j] & PS_POLY_MASK_SET)) {
		toTPAnew->x->coeffMask[i][j] = PS_POLY_MASK_SET;
		toTPAnew->y->coeffMask[i][j] = PS_POLY_MASK_SET;
		continue;
	    } 

	    // set the X terms
	    toTPAnew->x->coeff[i][j] += (fpa->toTPA->x->coeffMask[i][j] & PS_POLY_MASK_SET) ? 0.0 : FPtoTP->x->coeff[1][0] * fpa->toTPA->x->coeff[i][j];
	    toTPAnew->x->coeff[i][j] += (fpa->toTPA->y->coeffMask[i][j] & PS_POLY_MASK_SET) ? 0.0 : FPtoTP->x->coeff[0][1] * fpa->toTPA->y->coeff[i][j];

	    // set the Y terms
	    toTPAnew->y->coeff[i][j] += (fpa->toTPA->x->coeffMask[i][j] & PS_POLY_MASK_SET) ? 0.0 : FPtoTP->y->coeff[1][0] * fpa->toTPA->x->coeff[i][j];
	    toTPAnew->y->coeff[i][j] += (fpa->toTPA->y->coeffMask[i][j] & PS_POLY_MASK_SET) ? 0.0 : FPtoTP->y->coeff[0][1] * fpa->toTPA->y->coeff[i][j];
        }
    }

    // adjust the 0,0 terms:
    toTPAnew->x->coeff[0][0] += FPtoTP->x->coeff[0][0];
    toTPAnew->y->coeff[0][0] += FPtoTP->y->coeff[0][0];

    psFree (fpa->toTPA);
    fpa->toTPA = toTPAnew;

    // invert toTPA to determine fromTPA. choose an appropriate region based on the dimensions
    // of the complete FPA
    psRegion *region = pmAstromFPAExtent (fpa);

    psFree (fpa->fromTPA);
    fpa->fromTPA = psPlaneTransformInvert(NULL, fpa->toTPA, *region, 50, 4);
    psFree (region);

    if (fpa->fromTPA == NULL) {
        psError (PS_ERR_UNKNOWN, false, "failed to invert fpa->toTPA\n");
	psFree (FPtoTP);
        return false;
    }

    psFree (FPtoTP);
    return true;
}

// we have three coordinate systems and two polynomial transformations:
// TP is the raw tangent plane coordinate system (aligned with RA,DEC)
// FP is the raw focal plane coordinate system (aligned with camera X,Y)
// dFP is the distorted focal plane coordinate system

// fpa->toTPA is a polynomial transformation from FP to dFP
//   L(x,y) = \sum_i \sum_j A_{i,j} x^i y^j and 
//   M(x,y) = \sum_i \sum_j B_{i,j} x^i y^j 
//   where (x,y) are the FP coords and L,M  are the dFP coords

// FPtoTP is a linear transformation from dFP to TP (we recover this from TPtoFP by inversion)
// P(L,M) = r_xo + r_xx L + r_xy M
// Q(L,M) = r_yo + r_yx L + r_yy M

// we need to merge them into a single transformation:
// P(x,y) = r_xo + r_xx * \sum_i \sum_j A_{i,j} x^i y^j + r_xy * \sum_i \sum_j B_{i,j} x^i y^j 
// Q(x,y) = r_yo + r_yx * \sum_i \sum_j A_{i,j} x^i y^j + r_yy * \sum_i \sum_j B_{i,j} x^i y^j 
