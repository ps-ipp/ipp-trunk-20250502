/** @file  psCoord.c
 *
 *  @brief Contains basic coordinate transformation definitions and operations
 *
 *  This file defines the basic types for astronomical coordinate
 *  transformation
 *
 *  @ingroup CoordinateTransform
 *
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.142 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-09-12 00:59:00 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */
/******************************************************************************/
/*  INCLUDE FILES                                                             */
/******************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <math.h>
#include <float.h>
#include <string.h>

#include "psType.h"
#include "psCoord.h"
#include "psMemory.h"
#include "psTime.h"
#include "psAssert.h"
#include "psError.h"
#include "psLogMsg.h"
#include "psTrace.h"
#include "psAbort.h"

#include "psMatrix.h"
#include "psMinimizePolyFit.h"

# define TEST_SAVE_INVERSE_TRANSFORM 0

# if (TEST_SAVE_INVERSE_TRANSFORM)
# include "psBinaryOp.h"
# endif

# define ELIXIR_CODE 1

static void planeFree(psPlane *p)
{
    // There are non dynamic allocated items
}

static void sphereFree(psSphere* s)
{
    // There are non dynamic allocated items
}

static void cubeFree(psCube* c)
{
    // There are non dynamic allocated items
}

static void planeTransformFree(psPlaneTransform *pt)
{
    psFree(pt->x);
    psFree(pt->y);
}

/*****************************************************************************
p_psPlaneTransformLinearInvert(transform): : this is a private function which
simply inverts the supplied psPlaneTransform transform.  It assumes that
"transform" is linear.

XXX: This code no longer makes sense.  The merge must be reviewed.

XXX: below is the code using the standard matrix representation.  note that
this inversion requires x->nX == 1, y->nY == 1 and x->nY <= 1, y->nX <= 1
*****************************************************************************/
psPlaneTransform *p_psPlaneTransformLinearInvert(psPlaneTransform *transform)
{
    PS_ASSERT_PTR_NON_NULL(transform, NULL);
    PS_ASSERT_PTR_NON_NULL(transform->x, NULL);
    PS_ASSERT_PTR_NON_NULL(transform->y, NULL);
    if ((transform->x->nX < 1) ||
	(transform->x->nY < 1) ||
	(transform->y->nX < 1) ||
	(transform->y->nY < 1)) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: input transform is not invertible.");
        return(NULL);
    }

    // since the output polynomial is 1st order, a Chebyshev is not really useful
    psPlaneTransform *out = psPlaneTransformAlloc(1, 1, PS_POLYNOMIAL_ORD); 

    psF64 r12 = 0.0;
    if (transform->x->nY == 1) {
        r12 = transform->x->coeff[0][1];
    }
    psF64 r21 = 0.0;
    if (transform->y->nX == 1) {
        r21 = transform->y->coeff[1][0];
    }
    psF64 r11 = transform->x->coeff[1][0];
    psF64 r22 = transform->y->coeff[0][1];
    psF64 xo  = transform->x->coeff[0][0];
    psF64 yo  = transform->y->coeff[0][0];

    psF64 invDet = 1.0 / (r11 * r22 - r12 * r21);

    // apply the results back to the polynomials
    out->x->coeff[0][0] = -invDet * (r22 * xo - r12 * yo);
    out->y->coeff[0][0] = -invDet * (r11 * yo - r21 * xo);
    out->x->coeff[1][0] = +invDet * r22;
    out->y->coeff[0][1] = +invDet * r11;
    if (transform->x->nY == 1) {
        out->x->coeff[0][1] = -invDet * r12;
    }
    if (transform->y->nX == 1) {
        out->y->coeff[1][0] = -invDet * r21;
    }
    return(out);
}

/*****************************************************************************
p_psIsProjectionLinear(): this is a private function which simply determines
if the supplied psPlaneTransform transform is linear: if any of the
cooefficients of order 2 are higher are non-zero, then it is not linear.

Returns:
    true: if linear
    false: otherwise

Why isn't this called p_psIsPlaneTransformLinear()?
*****************************************************************************/
bool p_psIsProjectionLinear(psPlaneTransform *transform)
{
    PS_ASSERT_PTR_NON_NULL(transform, false);
    PS_ASSERT_PTR_NON_NULL(transform->x, false);
    PS_ASSERT_PTR_NON_NULL(transform->y, false);

    for (psS32 i=0;i<(1 + transform->x->nX);i++) {
        for (psS32 j=0;j<(1 + transform->x->nY);j++) {
            if (transform->x->coeff[i][j] != 0.0) {
                if (!(((i == 0) && (j == 0)) ||
		      ((i == 0) && (j == 1)) ||
		      ((i == 1) && (j == 0)))) {
                    return(false);
                }
            }
        }
    }

    for (psS32 i=0;i<(1 + transform->y->nX);i++) {
        for (psS32 j=0;j<(1 + transform->y->nY);j++) {
            if (transform->y->coeff[i][j] != 0.0) {
                if (!(((i == 0) && (j == 0)) ||
		      ((i == 0) && (j == 1)) ||
		      ((i == 1) && (j == 0)))) {
                    return(false);
                }
            }
        }
    }

    return(true);
}

psPlane* psPlaneAlloc(void)
{
    psPlane *p = psAlloc(sizeof(psPlane));
    psMemSetDeallocator(p, (psFreeFunc) planeFree);

    p->x = NAN;
    p->y = NAN;
    p->xErr = NAN;
    p->yErr = NAN;

    return(p);
}


psSphere* psSphereAlloc(void)
{
    psSphere *s = psAlloc(sizeof(psSphere));
    psMemSetDeallocator(s, (psFreeFunc) sphereFree);

    return(s);
}

psCube* psCubeAlloc(void)
{
    psCube *c = psAlloc(sizeof(psCube));

    psMemSetDeallocator(c, (psFreeFunc) cubeFree);
    return(c);
}

psPlaneTransform* psPlaneTransformAlloc(int order1, int order2, psPolynomialType type)
{
    PS_ASSERT_INT_NONNEGATIVE(order1, NULL);
    PS_ASSERT_INT_NONNEGATIVE(order2, NULL);

    psPlaneTransform *pt = psAlloc(sizeof(psPlaneTransform));

    pt->x = psPolynomial2DAlloc(type, order1, order2);
    pt->y = psPolynomial2DAlloc(type, order1, order2);

    psMemSetDeallocator(pt, (psFreeFunc) planeTransformFree);
    return(pt);
}


bool psMemCheckPlane(psPtr ptr)
{
    PS_ASSERT_PTR_NON_NULL(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)planeFree );
}

bool psMemCheckSphere(psPtr ptr)
{
    PS_ASSERT_PTR_NON_NULL(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)sphereFree );
}

bool psMemCheckCube(psPtr ptr)
{
    PS_ASSERT_PTR_NON_NULL(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)cubeFree );
}

bool psMemCheckPlaneTransform(psPtr ptr)
{
    PS_ASSERT_PTR_NON_NULL(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)planeTransformFree );
}

psPlane* psPlaneTransformApply(
    psPlane* out,
    const psPlaneTransform* transform,
    const psPlane* coords)
{
    PS_ASSERT_PTR_NON_NULL(transform, NULL);
    PS_ASSERT_PTR_NON_NULL(transform->x, NULL);
    PS_ASSERT_PTR_NON_NULL(transform->y, NULL);
    PS_ASSERT_PTR_NON_NULL(coords, NULL);

    if (out == NULL) {
        out = psPlaneAlloc();
    }

    out->x = psPolynomial2DEval(transform->x, coords->x, coords->y);
    out->y = psPolynomial2DEval(transform->y, coords->x, coords->y);

    return (out);
}

static void planeDistortFree(psPlaneDistort *pt)
{
    psFree(pt->x);
    psFree(pt->y);
}

psPlaneDistort* psPlaneDistortAlloc(int order1, int order2, int order3, int order4)
{
    PS_ASSERT_INT_NONNEGATIVE(order1, NULL);
    PS_ASSERT_INT_NONNEGATIVE(order2, NULL);
    PS_ASSERT_INT_NONNEGATIVE(order3, NULL);
    PS_ASSERT_INT_NONNEGATIVE(order4, NULL);

    psPlaneDistort *pt = psAlloc(sizeof(psPlaneDistort));
    pt->x = psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, order1, order2, order3, order4);
    pt->y = psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, order1, order2, order3, order4);

    psMemSetDeallocator(pt, (psFreeFunc) planeDistortFree);
    return(pt);
}

bool psMemCheckPlaneDistort(psPtr ptr)
{
    PS_ASSERT_PTR_NON_NULL(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)planeDistortFree );
}



/******************************************************************************
This transformation takes into account parameters beyond an objects spatial
coordinates: term3 and term4 (magnitude and color).
*****************************************************************************/
psPlane* psPlaneDistortApply(
    psPlane* out,
    const psPlaneDistort* distort,
    const psPlane* coords,
    float mag,
    float color)
{
    PS_ASSERT_PTR_NON_NULL(distort, NULL);
    PS_ASSERT_PTR_NON_NULL(distort->x, NULL);
    PS_ASSERT_PTR_NON_NULL(distort->y, NULL);
    PS_ASSERT_PTR_NON_NULL(coords, NULL);

    if (out == NULL) {
        out = psPlaneAlloc();
    }
    out->x = psPolynomial4DEval(distort->x, coords->x, coords->y, mag, color);
    out->y = psPolynomial4DEval(distort->y, coords->x, coords->y, mag, color);
    return (out);
}


void projectionFree(psProjection *p)
{
  if (!p) return;
  if (!p->radial) return;
  psFree (p->radial);
}

psProjection* psProjectionAlloc(
    double R,
    double D,
    double Xs,
    double Ys,
    psProjectionType type)
{
    psProjection *p = psAlloc(sizeof(psProjection));
    p->D = D;
    p->R = R;
    p->Xs = Xs;
    p->Ys = Ys;
    p->type = type;
    p->radial = NULL;

    psMemSetDeallocator(p, (psFreeFunc) projectionFree);
    return(p);
}

bool psMemCheckProjection(psPtr ptr)
{
    PS_ASSERT_PTR_NON_NULL(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)projectionFree );
}

typedef enum {
    PS_PROJECTION_CLASS_NONE,
    PS_PROJECTION_CLASS_CARTESIAN,
    PS_PROJECTION_CLASS_ZENITHAL,
    PS_PROJECTION_CLASS_PSEUDOCYL,
}psProjectionClass;

psPlane* psProject(
    psPlane *outPlane,
    const psSphere* coord,
    const psProjection* projection)
{
    PS_ASSERT_PTR_NON_NULL(coord, NULL);
    PS_ASSERT_PTR_NON_NULL(projection, NULL);

    psF64 phi = 0;
    psF64 theta = 0;
    psF64 sinTheta = 0;
    psF64 cosPhiCT = 0;
    psF64 sinPhiCT = 0;
    psF64 sinDp, cosDp, sinAlpha, cosAlpha, sinDelta, cosDelta, zeta;

    psProjectionClass class = PS_PROJECTION_CLASS_NONE;
    switch (projection->type) {
      case PS_PROJ_LIN:
      case PS_PROJ_PLY:
      case PS_PROJ_WRP:
        class = PS_PROJECTION_CLASS_CARTESIAN;
        break;
      case PS_PROJ_TAN:
      case PS_PROJ_TNX:
      case PS_PROJ_DIS:
      case PS_PROJ_SIN:
      case PS_PROJ_STG:
      case PS_PROJ_ZEA:
      case PS_PROJ_ZPL:
      case PS_PROJ_ZPN:
        class = PS_PROJECTION_CLASS_ZENITHAL;
        break;
      case PS_PROJ_AIT:
      case PS_PROJ_PAR:
      case PS_PROJ_GLS:
      case PS_PROJ_MER:
      case PS_PROJ_CAR:
        class = PS_PROJECTION_CLASS_PSEUDOCYL;
        break;

      default:
        psAbort("invalid projection");
        break;
    }

    // Allocate return value
    psPlane* out = NULL;
    if (outPlane == NULL) {
        out = psPlaneAlloc();
    } else {
        out = outPlane;
    }

    switch (class) {
      case PS_PROJECTION_CLASS_CARTESIAN:
        out->x = (coord->r - projection->R);
        out->y = (coord->d - projection->D);
        break;

      case PS_PROJECTION_CLASS_ZENITHAL:
        sinDp = sin(projection->D);
        cosDp = cos(projection->D);
        sinAlpha = sin(coord->r-projection->R);
        cosAlpha = cos(coord->r-projection->R);
        sinDelta = sin(coord->d);
        cosDelta = cos(coord->d);

# if ELIXIR_CODE

        sinTheta =  cosDelta*cosAlpha*cosDp + sinDelta*sinDp;
	sinPhiCT =  cosDelta*sinAlpha;
	cosPhiCT =  cosDelta*cosAlpha*sinDp - sinDelta*cosDp;
# else

        sinTheta =  cosDelta*cosAlpha*cosDp + sinDelta*sinDp;
	sinPhiCT = -cosDelta*sinAlpha;
	cosPhiCT = -cosDelta*cosAlpha*sinDp + sinDelta*cosDp;
# endif
	// Perform the specified projection
	switch (projection->type) {
	  case PS_PROJ_TAN:
	  case PS_PROJ_TNX: // XXX this is not quite right
	  case PS_PROJ_DIS:
	    // Gnomonic projection
	    out->x = +sinPhiCT / sinTheta;
	    out->y = -cosPhiCT / sinTheta;
	    break;
	  case PS_PROJ_SIN:
	    // Othrographic projection
	    out->x = +sinPhiCT;
	    out->y = -cosPhiCT;
	    break;
	  case PS_PROJ_STG:
	    // Othrographic projection
	    out->x = +2*sinPhiCT / (1 + sinTheta);
	    out->y = -2*cosPhiCT / (1 + sinTheta);
	    break;
	  case PS_PROJ_ZEA:
	  case PS_PROJ_ZPL:
	    // Zenithal Equal-Area
	    zeta = M_SQRT2 / sqrt (1 + sinTheta);
	    out->x = +zeta * sinPhiCT;
	    out->y = -zeta * cosPhiCT;
	    break;

	  case PS_PROJ_ZPN: {
	    // the forward projection is:
	    // theta = atan2(stht, ctht)
	    // gamma = (pi/2 - theta) : theta in radians
	    // Ro = sum (P_i gamma^i)
	    // R  = (180/pi) Ro
	    
	    // Ro = (pi/180)(90 - theta)
	    // R = (180/pi)sum (P_i R^i)
	    
	    // is ZPN defined for Npolyterms = 0 or 1?

	    double cosTheta = hypot(sinPhiCT, cosPhiCT);
	    double theta = atan2 (sinTheta, cosTheta);
	    
	    double gamma = M_PI_2 - theta;
	    
	    // i = 0 .. N - 1 (1 <= Npolyterms <= 21)
	    double Ro = 0.0;
	    for (int i = projection->radial->n - 1; i > 0; i--) {
	      double Pi = projection->radial->data.F64[i];
	      Ro = (Ro + Pi)*gamma;
	    }
	    Ro += projection->radial->data.F64[0];
	    
	    out->x = (cosTheta == 0.0) ? 0.0 : +Ro * sinPhiCT / cosTheta ;
	    out->y = (cosTheta == 0.0) ? 0.0 : -Ro * cosPhiCT / cosTheta ;
	    break;
	  }

	  default:
	    psAbort("invalid projection");
	    break;
	}
	break;

      case PS_PROJECTION_CLASS_PSEUDOCYL:
        phi = coord->r - projection->R;
        theta = coord->d - projection->D;
        switch (projection->type)
        {
	  case PS_PROJ_AIT:
            // Hammer-Aitoff projection
            zeta = 1.0/sqrt(0.5*(1.0+cos(theta)*cos(phi/2.0)));
            out->x = 2.0*zeta*cos(theta)*sin(phi/2.0);
            out->y = zeta*sin(theta);
            break;
	  case PS_PROJ_GLS:
            // projection name?
            out->x = phi * cos(theta);
            out->x = theta;
            break;
	  case PS_PROJ_PAR:
            // Parabolic projection
            out->x = phi*(2.0*cos(2.0*theta/3.0) - 1.0);
            out->y = M_PI*sin(theta/3.0);
            break;
	  case PS_PROJ_CAR:
	  case PS_PROJ_MER:
            psAbort("projection not yet implemented");
            break;
	  default:
            psAbort("invalid projection");
            break;
        }
        break;

      default:
        psAbort("invalid projection");
        break;
    }

    // Apply plate scales
    out->x /= projection->Xs;
    out->y /= projection->Ys;

    // Return output
    return out;
}

psSphere* psDeproject(
    psSphere *outSphere,
    const psPlane* coord,
    const psProjection* projection)
{
    PS_ASSERT_PTR_NON_NULL(coord, NULL);
    PS_ASSERT_PTR_NON_NULL(projection, NULL);

    psF64 rho      = 0.0;
    psF64 rho2      = 0.0;
    psF64 sinTheta = 0.0;
    psF64 cosTheta = 0.0;
    psF64 sinPhi   = 0.0;
    psF64 cosPhi   = 0.0;

    psF64  theta = 0.0;
    psF64  phi   = 0.0;
    psF64  x = 0, y = 0, R = 0;

    psProjectionClass class = PS_PROJECTION_CLASS_NONE;
    switch (projection->type) {
      case PS_PROJ_LIN:
      case PS_PROJ_PLY:
      case PS_PROJ_WRP:
        class = PS_PROJECTION_CLASS_CARTESIAN;
        break;
      case PS_PROJ_TAN:
      case PS_PROJ_TNX:
      case PS_PROJ_DIS:
      case PS_PROJ_SIN:
      case PS_PROJ_STG:
      case PS_PROJ_ZEA:
      case PS_PROJ_ZPL:
      case PS_PROJ_ZPN:
        class = PS_PROJECTION_CLASS_ZENITHAL;
        break;
      case PS_PROJ_AIT:
      case PS_PROJ_PAR:
      case PS_PROJ_GLS:
      case PS_PROJ_CAR:
      case PS_PROJ_MER:
        class = PS_PROJECTION_CLASS_PSEUDOCYL;
        break;

      default:
        psAbort("invalid projection");
        break;
    }

    // Allocate return sphere structure
    psSphere *out = NULL;
    if (outSphere == NULL) {
        out = psSphereAlloc();
    } else {
        out = outSphere;
    }

    // Perform inverse projection
    switch (class) {
      case PS_PROJECTION_CLASS_CARTESIAN:
        out->r = coord->x*projection->Xs + projection->R;
        out->d = coord->y*projection->Ys + projection->D;
        break;

      case PS_PROJECTION_CLASS_ZENITHAL:
        // Remove plate scales
        x = coord->x*projection->Xs;
        y = coord->y*projection->Ys;
        R = sqrt(x*x + y*y);
        sinPhi   = (R == 0) ? 0.0 : +x / R;
        cosPhi   = (R == 0) ? 1.0 : -y / R;

        switch (projection->type) {
	  case PS_PROJ_TAN:
	  case PS_PROJ_TNX:// XXX this is not quite right
	  case PS_PROJ_DIS:
            // Gnonomic deprojection
            rho      = sqrt (1 + R*R);
            sinTheta = 1 / rho;
            cosTheta = R / rho;
            break;
	  case PS_PROJ_SIN:
            // Orhtographic deprojection
            cosTheta = R;
            sinTheta = sqrt (1 - R*R);
            break;
	  case PS_PROJ_STG:
	    sinTheta = (4 - R) / (4 + R);
	    cosTheta = sqrt (1 - sinTheta*sinTheta);
            break;
	  case PS_PROJ_ZEA:
	  case PS_PROJ_ZPL:
            if (R > 2)
                return NULL;
            sinTheta = 1 - 0.5*PS_SQR(R);
            cosTheta = sqrt (1 - PS_SQR(sinTheta));
            break;

      case PS_PROJ_ZPN:

	// the forward projection is:
	// theta = atan2(stht, ctht)
	// gamma = (pi/2 - theta) : theta in radians
	// Ro = sum (P_i gamma^i)
	// R  = (180/pi) Ro

	// given R, we need to find theta:
	// Ro = R * (pi / 180) = sum (P_i gamma^i)
	// solve sum (P_i gamma^i) - Ro = 0 using Newton-Raphson

	// use Ro to get a guess for gamma and iterate

	{
	  // find the roots of f(gamma) - R = 0
	  // starting guess for gamma is (R - P0) / P1
	  double gamma = (R - projection->radial->data.F64[0]) / projection->radial->data.F64[1];
	  
	  for (int iter = 0; iter < 5; iter++) {
	    
	    double Rc = 0.0; // this will hold the ander 
	    double dR = 0.0;
	    for (int i = projection->radial->n - 1; i > 1; i--) {
	      double Pi = projection->radial->data.F64[i];
	      Rc = (Rc + Pi)*gamma;
	      dR = (dR + i*Pi)*gamma;
	    }
	    double P0 = projection->radial->data.F64[0];
	    double P1 = projection->radial->data.F64[1];
	    Rc = (Rc + P1)*gamma + P0;
	    dR = (dR + P1);

	    double gamma_new = gamma - (Rc - R) / dR;
	    gamma = gamma_new;
	  }
	  
	  double theta = M_PI_2 - gamma ;
	  cosTheta = cos (theta);
	  sinTheta = sin (theta);
	  break;
	}
	
	  default:
            psAbort("invalid projection");
            break;
        }

        psF64 sinDp = sin(projection->D);
        psF64 cosDp = cos(projection->D);

        // Convert from projection spherical coordinates
        // psLib versions:
# if ELIXIR_CODE
        // XXX the elixir version : does the ADD have a sign error?
        psF64 delta    = asin(sinTheta*sinDp - cosTheta*cosPhi*cosDp);
        psF64 sinAlpha = +cosTheta*sinPhi;
        psF64 cosAlpha = +cosTheta*cosPhi*sinDp + sinTheta*cosDp;
# else

	psF64 delta    = asin(sinTheta*sinDp + cosTheta*cosPhi*cosDp);
        psF64 sinAlpha = -cosTheta*sinPhi;
        psF64 cosAlpha = -cosTheta*cosPhi*sinDp + sinTheta*cosDp;
# endif

        out->d = delta;
        out->r = atan2(sinAlpha, cosAlpha) + projection->R;
        break;

      case PS_PROJECTION_CLASS_PSEUDOCYL:
        switch (projection->type)
        {
	  case PS_PROJ_AIT:
            // Hammer-Aitoff deprojection
            // XXX EAM : need range check on z^2 : must be > 0
            // XXX EAM : old code, ADD, and elixir code are discrepant re x/4, y/2
            rho2 = 1.0 - PS_SQR(0.25*x) - PS_SQR(0.5*y);
            if (rho2 < 0)
                return (NULL);
            rho = sqrt(rho2);
            phi = 2.0*atan2(0.5*x*rho, 2.0*rho2 - 1.0);
            theta = asin(y*rho);
            break;
	  case PS_PROJ_PAR:
            // Parabolic deprojection
            rho = y/M_PI;
            phi = x/(1.0 - 4.0*rho*rho);
            theta = 3.0*asin(rho);
            break;
	  case PS_PROJ_GLS:
            phi = x/cos(y);
            theta = y;
            break;
	  default:
            psAbort("invalid projection");
            break;
        }
        out->r = phi   + projection->R;
        out->d = theta + projection->D;
        break;
      default:
        psAbort("invalid projection");
        break;
    }

    // Return sphere coordinate
    return out;
}

/*****************************************************************************
multiplyDPoly2D(trans1, trans2): Takes two 2-D polynomials as input and
multiplies them.  Basically, for each non-zero coeff in the trans1 coeff[][]
array, you must multiply by all non-zero coeffs in trans2.
*****************************************************************************/
static psPolynomial2D *multiplyDPoly2D(
    psPolynomial2D *trans1,
    psPolynomial2D *trans2)
{
    psTrace("psLib.astro", 4, "---- %s() begin ----\n", __func__);
    psTrace("psLib.astro", 5, "multiplyDPoly2D(%d %d: %d %d)\n", trans1->nX, trans1->nY, trans2->nX, trans2->nY);
    psS32 orderX = trans1->nX + trans2->nX;
    psS32 orderY = trans1->nY + trans2->nY;
    psTrace("psLib.astro", 5, "out poly (nX, nY) is (%d, %d)\n", orderX, orderY);

    psPolynomial2D *out = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, orderX, orderY);
    psTrace("psLib.astro", 5, "Creating poly (%d, %d)\n", orderX, orderY);

    for (psS32 t1x = 0 ; t1x < (1 + trans1->nX) ; t1x++) {
        for (psS32 t1y = 0 ; t1y < (1 + trans1->nY) ; t1y++) {
            if (0.0 != trans1->coeff[t1x][t1y]) {
                for (psS32 t2x = 0 ; t2x < (1 + trans2->nX) ; t2x++) {
                    for (psS32 t2y = 0 ; t2y < (1 + trans2->nY) ; t2y++) {
                        out->coeff[t1x+t2x][t1y+t2y]+= (trans1->coeff[t1x][t1y] * trans2->coeff[t2x][t2y]);
                    }
                }
            }
        }
    }
    psTrace("psLib.astro", 4, "---- %s() end ----\n", __func__);
    return(out);
}

/*****************************************************************************
psPlaneTransformCombine(out, trans1, trans2)
*****************************************************************************/
psPlaneTransform *psPlaneTransformCombine(
    psPlaneTransform *out,
    const psPlaneTransform *trans1,
    const psPlaneTransform *trans2,
    psRegion region,
    int nSamples)
{
    psTrace("psLib.astro", 3, "---- %s() begin ----\n", __func__);
    psTrace("psLib.astro", 3, "tracelevel(%s) is %d\n", __func__, psTraceGetLevel("psLib.astro"));

    PS_ASSERT_PTR_NON_NULL(trans1, NULL);
    PS_ASSERT_PTR_NON_NULL(trans2, NULL);
    psTrace("psLib.astro", 5, "trans1->x is (%d, %d) order.\n", trans1->x->nX, trans1->x->nY);
    psTrace("psLib.astro", 5, "trans1->y is (%d, %d) order.\n", trans1->y->nX, trans1->y->nY);
    psTrace("psLib.astro", 5, "trans2->x is (%d, %d) order.\n", trans2->x->nX, trans2->x->nY);
    psTrace("psLib.astro", 5, "trans2->y is (%d, %d) order.\n", trans2->y->nX, trans2->y->nY);
    if (psTraceGetLevel("psLib.astro") >= 6) {
        PS_POLY_PRINT_2D(trans1->x);
        PS_POLY_PRINT_2D(trans1->y);
        PS_POLY_PRINT_2D(trans2->x);
        PS_POLY_PRINT_2D(trans2->y);
    }

    // both polynomials in both input transforms must match type -- and for now be ORD
    psAssert (trans1->x->type == PS_POLYNOMIAL_ORD, "fix for CHEB");
    psAssert (trans1->y->type == PS_POLYNOMIAL_ORD, "fix for CHEB");
    psAssert (trans2->x->type == PS_POLYNOMIAL_ORD, "fix for CHEB");
    psAssert (trans2->y->type == PS_POLYNOMIAL_ORD, "fix for CHEB");

    //
    // Determine the size of the new psPlaneTransform.
    //
    psS32 orderXnX = (trans2->x->nX * trans1->x->nX) + (trans2->x->nY * trans1->y->nX);
    psS32 orderXnY = (trans2->x->nX * trans1->x->nY) + (trans2->x->nY * trans1->y->nY);
    psS32 orderYnX = (trans2->y->nX * trans1->x->nX) + (trans2->y->nY * trans1->y->nX);
    psS32 orderYnY = (trans2->y->nX * trans1->x->nY) + (trans2->y->nY * trans1->y->nY);
    psS32 orderX = PS_MAX(orderXnX, orderYnX);
    psS32 orderY = PS_MAX(orderXnY, orderYnY);
    psTrace("psLib.astro", 5, "The new (orderX, orderY) is (%d, %d)\n", orderX, orderY);

    //
    // Allocate the new psPlaneTransform, if necessary.
    //

    psPlaneTransform *myPT = NULL;
    if (out == NULL) {
	myPT = psPlaneTransformAlloc(orderX, orderY, PS_POLYNOMIAL_ORD);
    } else {
        if ((out->x->nX == orderX) &&
	    (out->x->nY == orderY) &&
	    (out->y->nX == orderX) &&
	    (out->y->nY == orderY)) {
            myPT = out;
            //
            // Initialize the new psPlaneTransform, if necessary.
            //
            for (psS32 i = 0 ; i < orderX+1 ; i++) {
                for (psS32 j = 0 ; j < orderY+1 ; j++) {
                    myPT->x->coeff[i][j] = 0.0;
                    myPT->y->coeff[i][j] = 0.0;
                    myPT->x->coeffMask[i][j] = PS_POLY_MASK_NONE;
                    myPT->y->coeffMask[i][j] = PS_POLY_MASK_NONE;
                }
            }
        } else {
            psFree(out);
            myPT = psPlaneTransformAlloc(orderX, orderY, PS_POLYNOMIAL_ORD);
        }
    }
    psTrace("psLib.astro", 5, "New polynomial ranks are (%d %d %d %d)\n", myPT->x->nX, myPT->x->nY, myPT->y->nX, myPT->y->nY);

    //
    // For each term (a * x^i * y^j) in trans2, we substitute the appropriate
    // equation from trans1, and raise it to the appropriate power.  This is
    // done via the multiplyDPoly2D().  The result is a polynomial (currPoly)
    // and its coefficients are added into the myPT coeff matrix.
    //
    // trans1XPolys[i]: contains a polynomial corresponding to trans1->x raised to the i-th power.
    //

    psS32 order = PS_MAX(PS_MAX(PS_MAX(trans2->x->nX, trans2->x->nY), trans2->y->nX), trans2->y->nY);
    psPolynomial2D **trans1XPolys = (psPolynomial2D **) psAlloc((order + 1) * sizeof(psPolynomial2D *));
    psPolynomial2D **trans1YPolys = (psPolynomial2D **) psAlloc((order + 1) * sizeof(psPolynomial2D *));

    //
    // Raise the trans1 polynomials to whatever power is need in the trans2 polynomials.
    //
    trans1XPolys[0] = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 0, 0);
    trans1XPolys[0]->coeff[0][0] = 1.0;
    trans1YPolys[0] = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 0, 0);
    trans1YPolys[0]->coeff[0][0] = 1.0;

    for (psS32 c = 1 ; c < (order + 1) ; c++) {
        trans1XPolys[c] = multiplyDPoly2D(trans1XPolys[c-1], trans1->x);
        trans1YPolys[c] = multiplyDPoly2D(trans1YPolys[c-1], trans1->y);
    }

    psTrace("psLib.astro", 5, "Determine the new x-polynomial\n");
    for (psS32 t2x = 0 ; t2x < (trans2->x->nX + 1) ; t2x++) {
        for (psS32 t2y = 0 ; t2y < (trans2->x->nY + 1) ; t2y++) {
            psTrace("psLib.astro", 6, "X: -------------------- (t2x, t2y) (%d, %d) --------------------\n", t2x, t2y);
            if (trans2->x->coeffMask[t2x][t2y] & PS_POLY_MASK_SET) {
		continue;
	    }

	    psTrace("psLib.astro", 6, "In this iteration, we raise trans1->x to the %d power and trans1->y to the %d-power.\n", t2x, t2y);
	    psPolynomial2D *newPoly = multiplyDPoly2D(trans1XPolys[t2x], trans1YPolys[t2y]);

	    if (psTraceGetLevel("psLib.astro") >= 6) {
		PS_POLY_PRINT_2D(newPoly);
	    }

	    // Set the appropriate coeffs in myPT->x
	    for (psS32 i = 0 ; i < (1 + newPoly->nX) ; i++) {
		for (psS32 j = 0 ; j < (1 + newPoly->nY) ; j++) {
		    myPT->x->coeff[i][j]+= newPoly->coeff[i][j] * trans2->x->coeff[t2x][t2y];
		}
	    }

	    if (psTraceGetLevel("psLib.astro") >= 6) {
		PS_POLY_PRINT_2D(myPT->x);
	    }
	    psFree(newPoly);
        }
    }
    if (psTraceGetLevel("psLib.astro") >= 6) {
        psTrace("psLib.astro", 6, "The final x-polynomial\n");
        PS_POLY_PRINT_2D(myPT->x);
    }

    //
    // Determine the new y-polynomial
    //
    psTrace("psLib.astro", 5, "Determine the new y-polynomial\n");
    for (psS32 t2x = 0 ; t2x < (trans2->y->nX + 1) ; t2x++) {
        for (psS32 t2y = 0 ; t2y < (trans2->y->nY + 1) ; t2y++) {
            psTrace("psLib.astro", 5, "Y: -------------------- (t2x, t2y) (%d, %d) --------------------\n", t2x, t2y);
            if (trans2->y->coeffMask[t2x][t2y] & PS_POLY_MASK_SET) {
		continue;
	    }
	    psTrace("psLib.astro", 5, "In this iteration, we raise trans1->x to the %d power and trans1->y to the %d-power.\n", t2x, t2y);
	    psPolynomial2D *newPoly = multiplyDPoly2D(trans1XPolys[t2x], trans1YPolys[t2y]);

	    if (psTraceGetLevel("psLib.astro") >= 6) {
		PS_POLY_PRINT_2D(newPoly);
	    }

	    // Set the appropriate coeffs in myPT->x
	    for (psS32 i = 0 ; i < (1 + newPoly->nX) ; i++) {
		for (psS32 j = 0 ; j < (1 + newPoly->nY) ; j++) {
		    myPT->y->coeff[i][j]+= newPoly->coeff[i][j] * trans2->y->coeff[t2x][t2y];
		}
	    }
	    if (psTraceGetLevel("psLib.astro") >= 6) {
		PS_POLY_PRINT_2D(myPT->x);
	    }
	    psFree(newPoly);
        }
    }
    if (psTraceGetLevel("psLib.astro") >= 6) {
        psTrace("psLib.astro", 6, "The final y-polynomial\n");
        PS_POLY_PRINT_2D(myPT->y);
    }

    for (psS32 c = 0 ; c < (order + 1) ; c++) {
        psFree(trans1XPolys[c]);
        psFree(trans1YPolys[c]);
    }
    psFree(trans1XPolys);
    psFree(trans1YPolys);

    psTrace("psLib.astro", 3, "---- %s() end ----\n", __func__);
    return(myPT);
}

/*****************************************************************************
psPlaneTransformFit(trans, source, dest, nRejIter, sigmaClip)

XXX: This code ignores nRejIter and sigmaClip.  We must call the ClipFit
routines instead.
*****************************************************************************/
bool psPlaneTransformFit(
    psPlaneTransform *trans,
    const psArray *source,
    const psArray *dest,
    int nRejIter,
    float sigmaClip)
{
    PS_ASSERT_PTR_NON_NULL(trans, NULL);
    PS_ASSERT_PTR_NON_NULL(source, NULL);
    PS_ASSERT_PTR_NON_NULL(dest, NULL);

    //
    // Create the x and y vectors for the psVectorFitPolynomial2D() function.
    //
    psS32 numCoords = PS_MIN(source->n, dest->n);
    psVector *xIn = psVectorAlloc(numCoords, PS_TYPE_F64);
    psVector *yIn = psVectorAlloc(numCoords, PS_TYPE_F64);
    psVector *xOut = psVectorAlloc(numCoords, PS_TYPE_F64);
    psVector *yOut = psVectorAlloc(numCoords, PS_TYPE_F64);
    for (int g = 0; g < numCoords; g++) {
        xIn->data.F64[g] = ((psPlane *) source->data[g])->x;
        yIn->data.F64[g] = ((psPlane *) source->data[g])->y;
        xOut->data.F64[g] = ((psPlane *) dest->data[g])->x;
        yOut->data.F64[g] = ((psPlane *) dest->data[g])->y;
    }

    bool result = true;
    result &= psVectorFitPolynomial2D(trans->x, NULL, 0, xOut, NULL, xIn, yIn);
    result &= psVectorFitPolynomial2D(trans->y, NULL, 0, yOut, NULL, xIn, yIn);
    psFree(xIn);
    psFree(yIn);
    psFree(xOut);
    psFree(yOut);

    if (!result) {
        psError( PS_ERR_UNKNOWN, true, "psVectorFitPolynomial2D() returned NULL: could not fit a 2-D polynomial to the data.\n");
        return(false);
    }

    return(true);
}


/*****************************************************************************
psPlaneTransformInvert(out, in, region, nSamples)

*****************************************************************************/
psPlaneTransform *psPlaneTransformInvert(
    psPlaneTransform *out,
    const psPlaneTransform *in,
    psRegion region,
    int nSamples, int extraOrders)
{
    PS_ASSERT_PTR_NON_NULL(in, NULL);
    PS_ASSERT_PTR_NON_NULL(in->x, NULL);
    PS_ASSERT_PTR_NON_NULL(in->y, NULL);
    PS_ASSERT_INT_LARGER_THAN(nSamples, 0, NULL);

    // Reject a trivially non-invertible case.
    if ((in->x->nX < 1) || (in->x->nY < 1) || (in->y->nX < 1) || (in->y->nY < 1)) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: input transform is not invertible.");
        return(NULL);
    }

    // Ensure that the input transformation is symmetrical.
    if ((in->x->nX != in->x->nY) || (in->y->nX != in->y->nY) || (in->x->nX != in->y->nX)) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Input transformation must have same nX==nY.");
    }

    //
    // If the transform is linear, then invert it exactly and return.
    //
    if (p_psIsProjectionLinear((psPlaneTransform *) in)) {
        return(p_psPlaneTransformLinearInvert((psPlaneTransform *) in));
    }
    PS_ASSERT_INT_LARGER_THAN_OR_EQUAL(nSamples, 1, NULL);


    // Allocate a new psPlaneTransform if "out" is NULL, or has the wrong size.  For a
    // non-linear transformation, the inverted solution must have a higher order than the
    // forward solution.  Since we are fitting a dense grid (generated below), it is not a
    // problem (in terms of over-fitting) to use a higher order

    // XXX : the extra orders supplied here needs to be something supplied by the user

    psS32 order = in->x->nX + extraOrders;
    psPlaneTransform *myPT = NULL;
    if (out == NULL) {
      myPT = psPlaneTransformAlloc(order, order, PS_POLYNOMIAL_CHEB);
    } else {
      // the user has supplied a model with a specific order : fit that order
      myPT = psMemIncrRefCounter(out); // we need to return something which can be freed
    }

    //
    // Initialize the grid.  Since we want the inverse of the transformation, the
    // inCoords are written to the outData vector, and the outCoords are written
    // to the inData vector.
    //
    psVector *xIn = psVectorAlloc(nSamples*nSamples, PS_TYPE_F64);
    psVector *yIn = psVectorAlloc(nSamples*nSamples, PS_TYPE_F64);
    psVector *xOut = psVectorAlloc(nSamples*nSamples, PS_TYPE_F64);
    psVector *yOut = psVectorAlloc(nSamples*nSamples, PS_TYPE_F64);
    psPlane *inCoord = psPlaneAlloc();
    psPlane *outCoord = psPlaneAlloc();
    psS32 cnt = 0;
    for (int yint = 0; yint < nSamples; yint++) {
        inCoord->y = region.y0 + ((psF32) yint) * ((region.y1 - region.y0) / ((psF32) nSamples));
        for (int xint = 0; xint < nSamples; xint++) {
            inCoord->x = region.x0 + ((psF32) xint) * ((region.x1 - region.x0) / ((psF32) nSamples));
            (void)psPlaneTransformApply(outCoord, in, inCoord);
            xOut->data.F64[cnt] = inCoord->x;
            yOut->data.F64[cnt] = inCoord->y;
            xIn->data.F64[cnt] = outCoord->x;
            yIn->data.F64[cnt] = outCoord->y;
            cnt++;
        }
    }

    bool result = true;
    result &= psVectorFitPolynomial2D(myPT->x, NULL, 0, xOut, NULL, xIn, yIn);
    result &= psVectorFitPolynomial2D(myPT->y, NULL, 0, yOut, NULL, xIn, yIn);

# if (TEST_SAVE_INVERSE_TRANSFORM)

    psVector *xFit = psPolynomial2DEvalVector (myPT->x, xIn, yIn);
    psVector *xRes = (psVector *) psBinaryOp (NULL, xOut, "-", xFit);

    psVector *yFit = psPolynomial2DEvalVector (myPT->y, xIn, yIn);
    psVector *yRes = (psVector *) psBinaryOp (NULL, yOut, "-", yFit);

    static int nOut = 0;
    char filename[1024];
    snprintf (filename, 1024, "test.fit.%03d.ply", nOut); 
    FILE *fp = fopen (filename, "w");
    
    fprintf (fp, " ---- xFit ---- \n");

    for (int ix = 0; ix < myPT->x->nX + 1; ix++) {
      for (int iy = 0; iy < myPT->x->nY + 1; iy++) {
	fprintf (fp, "%18.12e ", myPT->x->coeff[ix][iy]);
      }
      fprintf (fp, "\n");
    }
    fprintf (fp, " ---- yFit ---- \n");

    for (int ix = 0; ix < myPT->y->nX + 1; ix++) {
      for (int iy = 0; iy < myPT->y->nY + 1; iy++) {
	fprintf (fp, "%18.12e ", myPT->y->coeff[ix][iy]);
      }
      fprintf (fp, "\n");
    }
    fclose (fp);
    
    snprintf (filename, 1024, "test.fit.%03d.dat", nOut); nOut ++;
    FILE *f1 = fopen (filename, "w");
    for (int i = 0; i < xFit->n; i++) {
      fprintf (f1, "%d : %f %f : %f %f : %f %f : %f %f\n", i,
	       xIn->data.F64[i], yIn->data.F64[i],
	       xOut->data.F64[i], yOut->data.F64[i],
	       xFit->data.F64[i], yFit->data.F64[i],
	       xRes->data.F64[i], yRes->data.F64[i]);
    }
    fclose (f1);

    psStats *myStats = psStatsAlloc (PS_STAT_SAMPLE_STDEV);
    psVectorStats (myStats, xRes, NULL, NULL, 0); float dX = myStats->sampleStdev;
    psVectorStats (myStats, yRes, NULL, NULL, 0); float dY = myStats->sampleStdev;
    fprintf (stderr, "xRes Sigma: %f  --  yRes Sigma %f\n", dX, dY);

    psFree (myStats);
    psFree (xFit);
    psFree (yFit);
    psFree (xRes);
    psFree (yRes);

# endif

    psFree(inCoord);
    psFree(outCoord);
    psFree(xIn);
    psFree(yIn);
    psFree(xOut);
    psFree(yOut);

    if (!result) {
        psError( PS_ERR_UNKNOWN, true, "psVectorFitPolynomial2D() returned NULL: could not fit a 2-D polynomial to the data.\n");
        psFree(myPT);
        return(NULL);
    }

    return(myPT);
}


psPlane *psPlaneTransformDeriv(
    psPlane *out,
    const psPlaneTransform *transformation,
    const psPlane *coord
    )
{
    PS_ASSERT_PTR_NON_NULL(transformation, NULL);
    PS_ASSERT_POLY_NON_NULL(transformation->x, NULL);
    PS_ASSERT_POLY_NON_NULL(transformation->y, NULL);
    PS_ASSERT_PTR_NON_NULL(coord, NULL);

    if (out == NULL) {
        out = psPlaneAlloc();
    }

    out->x = 0.0;
    out->y = 0.0;
    out->xErr = 0.0;
    out->yErr = 0.0;

    psPolynomial2D *xPoly = transformation->x;
    psPolynomial2D *yPoly = transformation->y;

    //
    // Calculate the derivative with respect to x.
    //
    psF32 xSum = 1.0;
    psF32 ySum = 1.0;

    // This loop starts at loop_x=1 since the derivative of the loop_x=0 terms are all 0.0
    for (psS32 loop_x = 1; loop_x < (1 + xPoly->nX); loop_x++) {
        ySum = 1.0;
        for (psS32 loop_y = 0; loop_y < (1 + xPoly->nY); loop_y++) {
            //
            // For each iteration of the loop, we multiply the (x, y) coefficient
            // by (x^(loop_x-1) * y^loop_y) * loop_x
            //

            out->x+= xPoly->coeff[loop_x][loop_y] * xSum * ySum * ((psF32) loop_x);
            psTrace("psLib.astro", 6, "out->x+= (%.2f * %.2f * %.2f * %.2f)\n", xPoly->coeff[loop_x][loop_y], xSum, ySum, ((psF32) loop_x));
            ySum*= coord->y;
        }
        xSum*= coord->x;
    }

    //
    // Calculate the derivative with respect to x.
    //
    xSum = 1.0;

    // This loop starts at loop_y=1 since the derivative of the loop_y=0 terms are all 0.0
    for (psS32 loop_x = 0; loop_x < (1 + yPoly->nX); loop_x++) {
        ySum = 1.0;
        for (psS32 loop_y = 1; loop_y < (1 + yPoly->nY); loop_y++) {
            //
            // For each iteration of the loop, we multiply the (x, y) coefficient
            // by (x^(loop_x-1) * y^loop_y) * loop_y
            // by (x^(loop_x) * y^(loop_y-1))
            //

            out->y+= yPoly->coeff[loop_x][loop_y] * xSum * ySum * ((psF32) loop_y);
            psTrace("psLib.astro", 6, "out->y+= (%.2f * %.2f * %.2f * %.2f)\n", yPoly->coeff[loop_x][loop_y], xSum, ySum, ((psF32) loop_y));
            ySum*= coord->y;
        }
        xSum*= coord->x;
    }

    return(out);
}

psPixels *psPixelsTransform(
    psPixels *out,
    const psPixels *input,
    const psPlaneTransform *inToOut)
{
    PS_ASSERT_PTR_NON_NULL(input, NULL);
    PS_ASSERT_PTR_NON_NULL(inToOut, NULL);
    if (out == NULL) {
        //XXX: Should the length (nalloc) be 1 and append be used everytime a pixel is added?
        //        out = psPixelsAlloc(input->nalloc);
        out = psPixelsAlloc(0);
    }
    psPlane *coord = psPlaneAlloc();
    psPlane *deriv = psPlaneAlloc();
    psPlane *fxnVal = psPlaneAlloc();

    int i = 0;
    //    int m = 0;
    //    while (input->data.x[i] != 0.0 && input->data.y[i] != 0.0) {
    for ( i = 0; i < input->n; i++) {
        coord->x = input->data[i].x;
        coord->y = input->data[i].y;
        deriv = psPlaneTransformDeriv(deriv, inToOut, coord);
        fxnVal = psPlaneTransformApply(fxnVal, inToOut, coord);
        if (fabs(fxnVal->x - coord->x) <= fabs(deriv->x) &&
	    fabs(fxnVal->y - coord->y) <= fabs(deriv->y)) {
            int x = (int)(ceil(fabs(deriv->x)));
            int y = (int)(ceil(fabs(deriv->y)));
            for (int j = -x; j <= x; j++) {
                for (int k = -y; k <= y; k++) {
                    //                    out->data[m].x = fxnVal->x + x;
                    //                    out->data[m].y = fxnVal->y + y;
                    //                    m++;
                    out = psPixelsAdd(out, 1, (float)(fxnVal->x+j),
                                      (float)(fxnVal->y+k) );
                }
            }
        }
    }

    psFree(coord);
    psFree(deriv);
    psFree(fxnVal);
    return out;
}

psCube *psSphereToCube(const psSphere *sphere)
{
    if(sphere == NULL) {
        psError( PS_ERR_UNKNOWN, true, "psSphere argument is NULL.  Returning NULL.\n");
        return NULL;
    }

    psCube *cube = NULL;

    cube = psCubeAlloc();
    cube->x = cos(sphere->d) * cos(sphere->r);
    cube->y = cos(sphere->d) * sin(sphere->r);
    cube->z = sin(sphere->d);
    cube->xErr = cos(sphere->dErr) * cos(sphere->rErr);
    cube->yErr = cos(sphere->dErr) * sin(sphere->rErr);
    cube->zErr = sin(sphere->dErr);

    return(cube);
}

psSphere *psCubeToSphere(const psCube *cube)
{
    if(cube == NULL) {
        psError( PS_ERR_UNKNOWN, true, "psCube argument is NULL.  Returning NULL.\n");
        return NULL;
    }

    psSphere *sphere = NULL;
    sphere = psSphereAlloc();
    //    sphere->r = arctan(cube->x/cube->y);
    //    sphere->d = arctan(sqrt(cube->x*cube->x + cube->y*cube->y)/cube->z);
    //    sphere->rErr = arctan(cube->xErr/cube->yErr);
    //    sphere->dErr = arctan(sqrt(cube->xErr*cube->xErr + cube->yErr*cube->yErr)/cube->zErr);
    //    sphere->r = 1 / (atan(cube->x/cube->y));
    //    sphere->d = 1 / (atan(sqrt(cube->x*cube->x + cube->y*cube->y)/cube->z));
    //    sphere->rErr = 1 / (atan(cube->xErr/cube->yErr));
    //    sphere->dErr = 1 / (atan(sqrt(cube->xErr*cube->xErr + cube->yErr*cube->yErr)/cube->zErr));
    psCube *cube2 = psCubeAlloc();
    *cube2 = *cube;
    double mag = sqrt(cube->x*cube->x + cube->y*cube->y + cube->z*cube->z);
    if (mag > 1.0) {
        cube2->x = cube2->x/mag;
        cube2->y = cube2->y/mag;
        cube2->z = cube2->z/mag;
    }

    sphere->r = atan2(cube2->y, cube2->x);
    sphere->d = asin(cube2->z);
    //    sphere->d = atan2((cube->x*cube->x + cube->y*cube->y), cube->z);
    sphere->rErr = atan2(cube2->yErr, cube2->xErr);
    sphere->dErr = asin(cube2->zErr);
    psFree(cube2);

    return(sphere);
}

psString psProjectTypeToString(psProjectionType type, const char *prefix)
{
    PS_ASSERT_STRING_NON_EMPTY(prefix, NULL);

    char *name = NULL;

    switch (type) {
      case PS_PROJ_LIN: psStringAppend (&name, "%s-LIN", prefix); return name;
      case PS_PROJ_PLY: psStringAppend (&name, "%s-PLY", prefix); return name;
      case PS_PROJ_WRP: psStringAppend (&name, "%s-WRP", prefix); return name;
      case PS_PROJ_TAN: psStringAppend (&name, "%s-TAN", prefix); return name;

      case PS_PROJ_DIS: psStringAppend (&name, "%s-DIS", prefix); return name;
      case PS_PROJ_SIN: psStringAppend (&name, "%s-SIN", prefix); return name;
      case PS_PROJ_STG: psStringAppend (&name, "%s-STG", prefix); return name;
      case PS_PROJ_TNX: psStringAppend (&name, "%s-TNX", prefix); return name;

      case PS_PROJ_ZEA: psStringAppend (&name, "%s-ZEA", prefix); return name;
      case PS_PROJ_ZPL: psStringAppend (&name, "%s-ZPL", prefix); return name;
      case PS_PROJ_ZPN: psStringAppend (&name, "%s-ZPN", prefix); return name;
      case PS_PROJ_AIT: psStringAppend (&name, "%s-AIT", prefix); return name;

      case PS_PROJ_PAR: psStringAppend (&name, "%s-PAR", prefix); return name;
      case PS_PROJ_GLS: psStringAppend (&name, "%s-GLS", prefix); return name;
      case PS_PROJ_CAR: psStringAppend (&name, "%s-CAR", prefix); return name;
      case PS_PROJ_MER: psStringAppend (&name, "%s-MER", prefix); return name;

      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unknown projection type: %d\n", type);
        return NULL;
    }
    return NULL;
}

psProjectionType psProjectTypeFromString(const char *name)
{

  // let higher-level calls decide if this should be an error
  if (!name) return PS_PROJ_NONE;
  if (!name[0]) return PS_PROJ_NONE;

  // PS_ASSERT_STRING_NON_EMPTY(name, PS_PROJ_NONE);

    if (!strcmp (&name[4], "-LIN")) return PS_PROJ_LIN;
    if (!strcmp (&name[4], "-PLY")) return PS_PROJ_PLY;
    if (!strcmp (&name[4], "-WRP")) return PS_PROJ_WRP;
    if (!strcmp (&name[4], "-TAN")) return PS_PROJ_TAN;
    if (!strcmp (&name[4], "-DIS")) return PS_PROJ_DIS;
    if (!strcmp (&name[4], "-SIN")) return PS_PROJ_SIN;
    if (!strcmp (&name[4], "-STG")) return PS_PROJ_STG;
    if (!strcmp (&name[4], "-TNX")) return PS_PROJ_TNX;
    if (!strcmp (&name[4], "-ZEA")) return PS_PROJ_ZEA;
    if (!strcmp (&name[4], "-ZPL")) return PS_PROJ_ZPL;
    if (!strcmp (&name[4], "-ZPN")) return PS_PROJ_ZPN;
    if (!strcmp (&name[4], "-AIT")) return PS_PROJ_AIT;
    if (!strcmp (&name[4], "-PAR")) return PS_PROJ_PAR;
    if (!strcmp (&name[4], "-GLS")) return PS_PROJ_GLS;
    if (!strcmp (&name[4], "-CAR")) return PS_PROJ_CAR;
    if (!strcmp (&name[4], "-MER")) return PS_PROJ_MER;

    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unrecognised projection type: %s", name);
    return PS_PROJ_NONE;
}
