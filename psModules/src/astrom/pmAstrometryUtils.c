/** @file  pmAstrometryUtils.c
 *
 *  @brief utility functions for transform and distort functions
 *
 *  @ingroup Astrometry
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-10-08 20:18:00 $
 *
 *  Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmAstrometryUtils.h"

static int transform_extra_orders = 3;

int pmAstrometryGetExtraOrders (void) {
  return transform_extra_orders;
}

int pmAstrometrySetExtraOrders (int orders) {
  transform_extra_orders = orders;
  return transform_extra_orders;
}

// this is used by the test output block
static int Nout = 0;

// given a 2D transformation -- L(x,y),M(x,y) -- find the coordinates x,y
// for which L,M = 0,0. tol is the allowed error on x,y.
psPlane *psPlaneTransformGetCenter (psPlaneTransform *trans, double tol)
{

    // crpix1,2 = X,Y(crval1,2)
    // start with linear solution for Xo,Yo:
    double R  = (trans->x->coeff[1][0]*trans->y->coeff[0][1] - trans->x->coeff[0][1]*trans->y->coeff[1][0]);
    double Xo = (trans->y->coeff[0][0]*trans->x->coeff[0][1] - trans->x->coeff[0][0]*trans->y->coeff[0][1])/R;
    double Yo = (trans->x->coeff[0][0]*trans->y->coeff[1][0] - trans->y->coeff[0][0]*trans->x->coeff[1][0])/R;

    if (0) {
      // this is a test output block, not used in normal operations
      char filename[64];
      snprintf (filename, 64, "trans.%03d.md", Nout);
      FILE *f = fopen (filename, "w");

      psMetadata *md = psMetadataAlloc();

      psPolynomial2DtoMetadata (md, trans->x, "X");
      psPolynomial2DtoMetadata (md, trans->y, "Y");
      psMetadataConfigPrint (f, md);
      psFree (md);
      fclose (f);
      Nout ++;
    }

    // iterate to actual solution: requires small non-linear terms
    if (trans->x->nX > 1) {
        psPolynomial2D *XdX = psPolynomial2D_dX(NULL, trans->x);
        psPolynomial2D *XdY = psPolynomial2D_dY(NULL, trans->x);

        psPolynomial2D *YdX = psPolynomial2D_dX(NULL, trans->y);
        psPolynomial2D *YdY = psPolynomial2D_dY(NULL, trans->y);

        psImage *Alpha = psImageAlloc (2, 2, PS_DATA_F32);
        psVector *Beta = psVectorAlloc (2, PS_DATA_F32);

        /* this loop uses the Newton-Raphson method to solve for Xo,Yo
	 * it needs the high order terms to be small 
	 * Xo,Yo are in pixels;
	 */
        double dPos = tol + 1;
        for (int i = 0; (dPos > tol) && (i < 20); i++) {
            // NOTE: order for Alpha is: [y][x]
            // original Alpha->data.F32[1][0] = psPolynomial2DEval (XdY, Xo, Yo);
            // original Alpha->data.F32[0][1] = psPolynomial2DEval (YdX, Xo, Yo);
            Alpha->data.F32[0][0] = psPolynomial2DEval (XdX, Xo, Yo);
            Alpha->data.F32[0][1] = psPolynomial2DEval (XdY, Xo, Yo);
            Alpha->data.F32[1][0] = psPolynomial2DEval (YdX, Xo, Yo);
            Alpha->data.F32[1][1] = psPolynomial2DEval (YdY, Xo, Yo);

	    // Beta = (-L,-M) for the current guess
            Beta->data.F32[0] = -1.0 * psPolynomial2DEval (trans->x, Xo, Yo);
            Beta->data.F32[1] = -1.0 * psPolynomial2DEval (trans->y, Xo, Yo);
            // fprintf (stderr, " Beta: %f %f\n", Beta->data.F32[0], Beta->data.F32[1]);

	    // since we want (L,M) = (0,0), Beta is also the offset
            if (!psMatrixGJSolve (Alpha, Beta)) {
		psError(PS_ERR_UNKNOWN, false, "Unable to solve for center.");
		psFree (Alpha);
		psFree (Beta);
		psFree (XdX);
		psFree (XdY);
		psFree (YdX);
		psFree (YdY);
		return NULL;
	    }

            Xo += Beta->data.F32[0];
            Yo += Beta->data.F32[1];
            dPos = hypot(Beta->data.F32[0], Beta->data.F32[1]);
	    psTrace ("psastro", 5, "i: %d, Xo,Yo : %f %f  dX, dY: %f %f dPos: %f\n", i, Xo, Yo, Beta->data.F32[0], Beta->data.F32[1], dPos);
        }
        psFree (Alpha);
        psFree (Beta);
        psFree (XdX);
        psFree (XdY);
        psFree (YdX);
        psFree (YdY);

	if (dPos > tol) {
	    psError(PS_ERR_UNKNOWN, false, "Newton-Raphson method did not converge after 20 iterations (%f)\n", dPos);
	    return NULL;
	}
    }
    psPlane *center = psPlaneAlloc ();
    center->x = Xo;
    center->y = Yo;

    return center;
}

// convert a transformation L(x,y) to L'(x-xo,y-yo)
// is this used for an upward (e.g., chip->fpa) or a downward (fpa->chip) transform?
psPlaneTransform *psPlaneTransformSetCenter (psPlaneTransform *output, psPlaneTransform *input, double Xo, double Yo)
{

    // validate fit type:
    // polynomial in input transforms must match type -- and for now be ORD
    psAssert (input->x->type == PS_POLYNOMIAL_ORD, "fix for CHEB");
    psAssert (input->y->type == PS_POLYNOMIAL_ORD, "fix for CHEB");

    if (output == NULL) {
        output = psPlaneTransformAlloc(input->x->nX, input->x->nY, PS_POLYNOMIAL_ORD);
    }

    /* given two equivalent polynomial representations L(x,y) = \sum_i \sum_j A_{i,j} x^i y^j
     * we can transform L(x,y) into L'(x-xo,y-yo) by taking the derivatives of both sides and 
     * noting that the constant term in each is the coefficient in the case of L(x,y) and is the 
     * value of L'(-xo,-yo) in the second case.
     */

    psPolynomial2D *tmp;

    psPolynomial2D *xPx = psPolynomial2DCopy (NULL, input->x);
    psPolynomial2D *yPx = psPolynomial2DCopy (NULL, input->y);

    for (int i = 0; i <= input->x->nX; i++) {
        psPolynomial2D *xPy = psPolynomial2DCopy (NULL, xPx);
        psPolynomial2D *yPy = psPolynomial2DCopy (NULL, yPx);
        for (int j = 0; j <= input->x->nY; j++) {
            output->x->coeffMask[i][j] = input->x->coeffMask[i][j];
            output->y->coeffMask[i][j] = input->y->coeffMask[i][j];
            output->x->coeff[i][j] = (output->x->coeffMask[i][j] & PS_POLY_MASK_SET) ? 0 : psPolynomial2DEval (xPy, Xo, Yo) / tgamma(i+1) / tgamma(j+1);
            output->y->coeff[i][j] = (output->y->coeffMask[i][j] & PS_POLY_MASK_SET) ? 0 : psPolynomial2DEval (yPy, Xo, Yo) / tgamma(i+1) / tgamma(j+1);

            // take the next derivative wrt y, catch output (is NULL on last pass)
            tmp = psPolynomial2D_dY(NULL, xPy);
            psFree (xPy);
            xPy = tmp;
            tmp = psPolynomial2D_dY(NULL, yPy);
            psFree (yPy);
            yPy = tmp;
        }
        // take the next derivative wrt x, catch output (is NULL on last pass)
        tmp = psPolynomial2D_dX(NULL, xPx);
        psFree (xPx);
        xPx = tmp;
        tmp = psPolynomial2D_dX(NULL, yPx);
        psFree (yPx);
        yPx = tmp;
    }
    return output;
}

// rotate a transformation L(x,y) by theta
psPlaneTransform *psPlaneTransformRotate  (psPlaneTransform *output, psPlaneTransform *input, double theta)
{
    /* given the polynomial transformations:
     *  L(x,y) = \sum_i \sum_j A_{i,j} x^i y^j and 
     *  M(x,y) = \sum_i \sum_j B_{i,j} x^i y^j 
     * we can rotate L,M to L',M' by applying the rotation matrix (c,s),(-s,c).
     * the resulting terms of L and M are:
     * A'_{i,j} = c A_{i,j} + s B_{i,j}
     * B'_{i,j} = c B_{i,j} - s A_{i,j}
     */

    if (output == NULL) {
	// generate a new transform using the same order and type as the input
        output = psPlaneTransformAlloc(input->x->nX, input->x->nY, input->x->type);
    }

    float cs = cos(theta);
    float sn = sin(theta);

    for (int i = 0; i <= input->x->nX; i++) {
        for (int j = 0; j <= input->x->nY; j++) {
	    // XXX what about inconsistent x and y masking?
            output->x->coeffMask[i][j] = input->x->coeffMask[i][j];
	    output->y->coeffMask[i][j] = input->y->coeffMask[i][j];
	    if (output->x->coeffMask[i][j]) {
		output->x->coeff[i][j] = 0.0;
		output->y->coeff[i][j] = 0.0;
	    } else {
		output->x->coeff[i][j] = cs*input->x->coeff[i][j] + sn*input->y->coeff[i][j];
		output->y->coeff[i][j] = cs*input->y->coeff[i][j] - sn*input->x->coeff[i][j];
	    }
        }
    }
    return output;
}

// construct a psPlaneTransform which is the identify transformation
psPlaneTransform *psPlaneTransformIdentity (int order)
{

    psPlaneTransform *transform;

    if (order < 1)
        psAbort("invalid order");
    if (order > 3)
        psAbort("invalid order");

    // all coeffs and masks initially set to 0
    transform = psPlaneTransformAlloc (order, order, PS_POLYNOMIAL_ORD);

    for (int i = 0; i <= order; i++) {
        for (int j = 0; j <= order; j++) {
            if (i + j > order) {
                transform->x->coeffMask [i][j] = PS_POLY_MASK_SET;
                transform->y->coeffMask [i][j] = PS_POLY_MASK_SET;
            }
        }
    }
    transform->x->coeff[1][0] = 1;
    transform->y->coeff[0][1] = 1;
    transform->x->coeffMask[1][0] = PS_POLY_MASK_NONE;
    transform->y->coeffMask[0][1] = PS_POLY_MASK_NONE;

    return transform;
}

// check that the given psPlaneTransform is the identity * (Xs,Ys)
bool psPlaneTransformIsDiagonal (psPlaneTransform *transform)
{

    int order;
    bool status;

    // we currently only support up to 3rd order polynomials
    if (transform->x->nX < 1)
        return false;
    if (transform->x->nY < 1)
        return false;
    if (transform->y->nX < 1)
        return false;
    if (transform->y->nY < 1)
        return false;

    if (transform->x->nX != transform->x->nY)
        return false;
    if (transform->y->nX != transform->y->nY)
        return false;

    // these are not actually valid tests
    if (transform->x->nX > 3)
        return false;
    if (transform->x->nY > 3)
        return false;
    if (transform->y->nX > 3)
        return false;
    if (transform->y->nY > 3)
        return false;

    status = true;
    order = transform->x->nX;
    for (int i = 0; i <= order; i++) {
        for (int j = 0; j <= order; j++) {
            if (i + j > order) {
                // high-order cross terms must be masked (eg, x^3 y^2)
                status &= (transform->x->coeffMask[i][j] & PS_POLY_MASK_SET);
            } else {
                status &= !(transform->x->coeffMask[i][j] & PS_POLY_MASK_SET);
                if ((i == 1) && (i + j == 1)) {
                    // linear, diagonal terms must be non-zero
                    status &= (fabs(transform->x->coeff[i][j]) > FLT_EPSILON);
                } else {
                    // non-linear and off-diagonal terms must be 0 (eg, x^2, x y)
                    status &= (fabs(transform->x->coeff[i][j]) < FLT_EPSILON);
                }
            }
        }
    }

    order = transform->y->nX;
    for (int i = 0; i <= order; i++) {
        for (int j = 0; j <= order; j++) {
            if (i + j > order) {
                // high-order cross terms must be masked (eg, x^3 y^2)
                status &= (transform->y->coeffMask[i][j] & PS_POLY_MASK_SET);
            } else {
                status &= !(transform->y->coeffMask[i][j] & PS_POLY_MASK_SET);
                if ((j == 1) && (i + j == 1)) {
                    // linear, diagonal terms must be 1.0
                    status &= (fabs(transform->y->coeff[i][j]) > FLT_EPSILON);
                } else {
                    // non-linear and off-diagonal terms must be 0 (eg, x^2, x y)
                    status &= (fabs(transform->y->coeff[i][j]) < FLT_EPSILON);
                }
            }
        }
    }
    return status;
}

// construct a psPlaneDistort which is the identify transformation
psPlaneDistort *psPlaneDistortIdentity (int order)
{

    psPlaneDistort *distort;

    if (order < 1)
        psAbort("invalid order");
    if (order > 3)
        psAbort("invalid order");

    // all coeffs and masks initially set to 0
    distort = psPlaneDistortAlloc (order, order, 0, 0);

    for (int i = 0; i <= order; i++) {
        for (int j = 0; j <= order; j++) {
            if (i + j > order) {
                distort->x->coeffMask [i][j][0][0] = PS_POLY_MASK_SET;
                distort->y->coeffMask [i][j][0][0] = PS_POLY_MASK_SET;
            }
        }
    }
    distort->x->coeff[1][0][0][0] = 1;
    distort->y->coeff[0][1][0][0] = 1;

    return distort;
}

// check that the given psPlaneDistort is the identity * (Xs,Ys)
bool psPlaneDistortIsDiagonal (psPlaneDistort *distort)
{

    int order;
    bool status;

    // we currently only support up to 3rd order polynomials
    if (distort->x->nX < 1)
        return false;
    if (distort->x->nY < 1)
        return false;
    if (distort->y->nX < 1)
        return false;
    if (distort->y->nY < 1)
        return false;

    if (distort->x->nX > 3)
        return false;
    if (distort->x->nY > 3)
        return false;
    if (distort->y->nX > 3)
        return false;
    if (distort->y->nY > 3)
        return false;

    if (distort->x->nZ > 0)
        return false;
    if (distort->x->nT > 0)
        return false;
    if (distort->y->nZ > 0)
        return false;
    if (distort->y->nT > 0)
        return false;

    if (distort->x->nX != distort->x->nY)
        return false;
    if (distort->y->nX != distort->y->nY)
        return false;

    status = true;
    order = distort->x->nX;
    for (int i = 0; i <= order; i++) {
        for (int j = 0; j <= order; j++) {
            if (i + j > order) {
                // high-order cross terms must be masked (eg, x^3 y^2)
                status &= (distort->x->coeffMask[i][j][0][0] & PS_POLY_MASK_SET);
            } else {
                status &= !(distort->x->coeffMask[i][j][0][0] & PS_POLY_MASK_SET);
                if ((i == 1) && (i + j == 1)) {
                    // linear, diagonal terms must be 1.0
                    status &= (fabs(distort->x->coeff[i][j][0][0]) > FLT_EPSILON);
                } else {
                    // non-linear and off-diagonal terms must be 0 (eg, x^2, x y)
                    status &= (fabs(distort->x->coeff[i][j][0][0]) < FLT_EPSILON);
                }
            }
        }
    }

    order = distort->y->nX;
    for (int i = 0; i <= order; i++) {
        for (int j = 0; j <= order; j++) {
            if (i + j > order) {
                // high-order cross terms must be masked (eg, x^3 y^2)
                status &= (distort->y->coeffMask[i][j][0][0] & PS_POLY_MASK_SET);
            } else {
                status &= !(distort->y->coeffMask[i][j][0][0] & PS_POLY_MASK_SET);
                if ((j == 1) && (i + j == 1)) {
                    // linear, diagonal terms must be 1.0
                    status &= (fabs(distort->y->coeff[i][j][0][0]) > FLT_EPSILON);
                } else {
                    // non-linear and off-diagonal terms must be 0 (eg, x^2, x y)
                    status &= (fabs(distort->y->coeff[i][j][0][0]) < FLT_EPSILON);
                }
            }
        }
    }
    return status;
}
