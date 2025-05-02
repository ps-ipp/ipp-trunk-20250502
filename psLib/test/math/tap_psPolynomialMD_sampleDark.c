#include <stdio.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define TOL 1.0e-4

int main(int argc, char *argv[])
{
    plan_tests(13);
    {
        psMemId id = psMemGetId();

	FILE *f = fopen ("data/polyMD.dat", "r");
	ok (f, "open datafile");
	skip_start (!f, 10, "skipping tests using polyMD.dat");

	psVector *exptime = psVectorAllocEmpty (100, PS_TYPE_F32);
	psVector *temp    = psVectorAllocEmpty (100, PS_TYPE_F32);
	psVector *ord1    = psVectorAllocEmpty (100, PS_TYPE_F32);
	psVector *ord2    = psVectorAllocEmpty (100, PS_TYPE_F32);
	psVector *ord3    = psVectorAllocEmpty (100, PS_TYPE_F32);
	psVector *flux    = psVectorAllocEmpty (100, PS_TYPE_F32);

	float Exptime, Temp, Ord1, Ord2, Ord3, Flux;

	// load data: exptime temp ord1 ord2 ord3 f
	for (int i = 0; true; i++) {
	    int status = fscanf (f, "%f %f %f %f %f %f", &Exptime, &Temp, &Ord1, &Ord2, &Ord3, &Flux);
	    if (status != 6) break;
	    psVectorAppend(exptime, Exptime);
	    psVectorAppend(temp,    Temp);
	    psVectorAppend(ord1,    Ord1);
	    psVectorAppend(ord2,    Ord2);
	    psVectorAppend(ord3,    Ord3);
	    psVectorAppend(flux,    Flux);
	    psAssert (i < exptime->nalloc, "error reading data");
	}
	ok (exptime->n == 5000, "read %d elements", exptime->n);

	// fit f to ord1, ord2, ord3 (1st order only)
        {
	    psArray *ordinates = psArrayAlloc(exptime->n);
	    psVector *values = psVectorAlloc(exptime->n, PS_TYPE_F32);

	    for (int i = 0; i < exptime->n; i++) {
		values->data.F32[i] = flux->data.F32[i];
		psVector *ord = psVectorAlloc(3, PS_TYPE_F32);
		ord->data.F32[0] = ord1->data.F32[i];
		ord->data.F32[1] = ord2->data.F32[i];
		ord->data.F32[2] = ord3->data.F32[i];
		ordinates->data[i] = ord;
	    }

	    int XORDER = 1;
	    int YORDER = 1;
	    int ZORDER = 1;

	    psVector *orders = psVectorAlloc(3, PS_TYPE_S32);
	    orders->data.S32[0] = XORDER;
	    orders->data.S32[1] = YORDER;
	    orders->data.S32[2] = ZORDER;

            psPolynomialMD *poly = psPolynomialMDAlloc(orders);
            bool polyOK = psPolynomialMDFit(poly, values, NULL, NULL, 0, ordinates);
            ok(polyOK, "Fit polynomial");
            skip_start(!polyOK, 4, "Skipping coefficient checks since fit failed.");
            is_double_tol(poly->coeff->data.F64[0], 1000.0, TOL, "Coefficient %d", 0);
	    is_double_tol(poly->coeff->data.F64[1], 5.0,    TOL, "Coefficient %d", 1);
	    is_double_tol(poly->coeff->data.F64[2], 0.025,  TOL, "Coefficient %d", 2);
	    is_double_tol(poly->coeff->data.F64[3], 0.005,  TOL, "Coefficient %d", 3);
            skip_end();
            psFree(poly);
	    psFree(orders);
	    psFree(values);
	    psFree(ordinates);
        }

	// fit f to exptime (1st order) and temp (2nd order)
	// f = D0 + D1*exptime + D2*exptime*temp + D3*exptime*temp^2
        {
            psPolynomial2D *poly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 1, 2);
	    poly->coeffMask[0][1] = 1; // x^0 y^1
	    poly->coeffMask[0][2] = 1; // x^0 y^2
	    
	    bool polyOK = psVectorFitPolynomial2D(poly, NULL, 0, flux, NULL, exptime, temp);
            ok(polyOK, "Fit polynomial");

            skip_start(!polyOK, 4, "Skipping coefficient checks since fit failed.");
            is_double_tol(poly->coeff[0][0], 1000.0, TOL, "Coefficient %d %d", 0, 0);
	    is_double_tol(poly->coeff[1][0], 5.0,    TOL, "Coefficient %d %d", 1, 0);
	    is_double_tol(poly->coeff[1][1], 0.025,  TOL, "Coefficient %d %d", 1, 1);
	    is_double_tol(poly->coeff[1][2], 0.005,  TOL, "Coefficient %d %d", 1, 2);
            skip_end();
            psFree(poly);
        }
	psFree(exptime);
	psFree(temp);
	psFree(ord1);
	psFree(ord2);
	psFree(ord3);
	psFree(flux);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
	skip_end();
    }
    exit(EXIT_SUCCESS);
}
