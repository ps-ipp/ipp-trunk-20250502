#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

int main (void)
{
    plan_tests(54);

    note("psPolynomial2D Derivative tests");

    // test psPolynomial2D_dX (no supplied output)
    {
        psMemId id = psMemGetId();

        note ("test psPolynomial2D_dX (no supplied output)");

        psPolynomial2D *poly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 2, 2);
        ok(poly != NULL, "psPolynomial2D successfully allocated");
        skip_start(poly == NULL, 5, "Skipping tests because psPolynomial2DAlloc() failed");

        // build sample polynomial (upper-left elements only)
        // z = 5 + 2x + 3y - 3x^2 + 4xy - 2y^2
        // dz/dx = 2 - 6x + 4y
        poly->coeff[0][0] = 5.0;
        poly->coeff[1][0] = 2.0;
        poly->coeff[0][1] = 3.0;
        poly->coeff[2][0] = -3.0;
        poly->coeff[1][1] = 4.0;
        poly->coeff[0][2] = -2.0;

        // mask remaining elements
        poly->coeffMask[2][1] = 1;
        poly->coeffMask[1][2] = 1;
        poly->coeffMask[2][2] = 1;

        psPolynomial2D *dX = psPolynomial2D_dX (NULL, poly);

        ok(dX->nX == 1, "new x order is %d", dX->nX);
        ok(dX->nY == 2, "new y order is %d", dX->nY);

        is_float(dX->coeff[0][0], +2.0, "x^0 y^0 coeff is %f", dX->coeff[0][0]);
        is_float(dX->coeff[1][0], -6.0, "x^1 y^0 coeff is %f", dX->coeff[1][0]);
        is_float(dX->coeff[0][1], +4.0, "x^0 y^1 coeff is %f", dX->coeff[0][1]);

        ok(!dX->coeffMask[0][0], "x^0 y^0 coeff is unmasked");
        ok(!dX->coeffMask[1][0], "x^1 y^0 coeff is unmasked");
        ok(!dX->coeffMask[0][1], "x^0 y^1 coeff is unmasked");

        ok(dX->coeffMask[1][1], "x^1 y^1 coeff is masked");
        ok(dX->coeffMask[1][2], "x^1 y^2 coeff is masked");

        psFree (dX);
        psFree (poly);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test psPolynomial2D_dX (supplied output)
    {
        psMemId id = psMemGetId();

        note ("test psPolynomial2D_dX (supplied output)");

        psPolynomial2D *poly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 2, 2);
        ok(poly != NULL, "psPolynomial2D successfully allocated");
        skip_start(poly == NULL, 5, "Skipping tests because psPolynomial2DAlloc() failed");

        // build sample polynomial (upper-left elements only)
        // z = 5 + 2x + 3y - 3x^2 + 4xy - 2y^2
        // dz/dx = 2 - 6x + 4y
        poly->coeff[0][0] = 5.0;
        poly->coeff[1][0] = 2.0;
        poly->coeff[0][1] = 3.0;
        poly->coeff[2][0] = -3.0;
        poly->coeff[1][1] = 4.0;
        poly->coeff[0][2] = -2.0;

        // mask remaining elements
        poly->coeffMask[2][1] = 1;
        poly->coeffMask[1][2] = 1;
        poly->coeffMask[2][2] = 1;

        psPolynomial2D *dX = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 2, 2);
        psPolynomial2D_dX (dX, poly);

        ok(dX->nX == 1, "new x order is %d", dX->nX);
        ok(dX->nY == 2, "new y order is %d", dX->nY);

        is_float(dX->coeff[0][0], +2.0, "x^0 y^0 coeff is %f", dX->coeff[0][0]);
        is_float(dX->coeff[1][0], -6.0, "x^1 y^0 coeff is %f", dX->coeff[1][0]);
        is_float(dX->coeff[0][1], +4.0, "x^0 y^1 coeff is %f", dX->coeff[0][1]);

        ok(!dX->coeffMask[0][0], "x^0 y^0 coeff is unmasked");
        ok(!dX->coeffMask[1][0], "x^1 y^0 coeff is unmasked");
        ok(!dX->coeffMask[0][1], "x^0 y^1 coeff is unmasked");

        ok(dX->coeffMask[1][1], "x^1 y^1 coeff is masked");
        ok(dX->coeffMask[1][2], "x^1 y^2 coeff is masked");

        psFree (dX);
        psFree (poly);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test psPolynomial2D_dX (inPlace: supplied output == supplied input)
    {
        psMemId id = psMemGetId();

        note ("test psPolynomial2D_dX (supplied output)");

        psPolynomial2D *poly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 2, 2);
        ok(poly != NULL, "psPolynomial2D successfully allocated");
        skip_start(poly == NULL, 5, "Skipping tests because psPolynomial2DAlloc() failed");

        // build sample polynomial (upper-left elements only)
        // z = 5 + 2x + 3y - 3x^2 + 4xy - 2y^2
        // dz/dx = 2 - 6x + 4y
        poly->coeff[0][0] = 5.0;
        poly->coeff[1][0] = 2.0;
        poly->coeff[0][1] = 3.0;
        poly->coeff[2][0] = -3.0;
        poly->coeff[1][1] = 4.0;
        poly->coeff[0][2] = -2.0;

        // mask remaining elements
        poly->coeffMask[2][1] = 1;
        poly->coeffMask[1][2] = 1;
        poly->coeffMask[2][2] = 1;

        psPolynomial2D *result = psPolynomial2D_dX (poly, poly);
        ok (result == NULL, "psPolynomial2D_dX failed as expected: cannot assign output to input");

        psFree (poly);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test psPolynomial2D_dY (no supplied output)
    {
        psMemId id = psMemGetId();

        note ("test psPolynomial2D_dY (no supplied output)");

        psPolynomial2D *poly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 2, 2);
        ok(poly != NULL, "psPolynomial2D successfully allocated");
        skip_start(poly == NULL, 5, "Skipping tests because psPolynomial2DAlloc() failed");

        // build sample polynomial (upper-left elements only)
        // z = 5 + 2x + 3y - 3x^2 + 4xy - 2y^2
        // dz/dy = 3 + 4x - 4y
        poly->coeff[0][0] = 5.0;
        poly->coeff[1][0] = 2.0;
        poly->coeff[0][1] = 3.0;
        poly->coeff[2][0] = -3.0;
        poly->coeff[1][1] = 4.0;
        poly->coeff[0][2] = -2.0;

        // mask remaining elements
        poly->coeffMask[2][1] = 1;
        poly->coeffMask[1][2] = 1;
        poly->coeffMask[2][2] = 1;

        psPolynomial2D *dY = psPolynomial2D_dY (NULL, poly);

        ok(dY->nX == 2, "new x order is %d", dY->nX);
        ok(dY->nY == 1, "new y order is %d", dY->nY);

        is_float(dY->coeff[0][0], +3.0, "x^0 y^0 coeff is %f", dY->coeff[0][0]);
        is_float(dY->coeff[1][0], +4.0, "x^1 y^0 coeff is %f", dY->coeff[1][0]);
        is_float(dY->coeff[0][1], -4.0, "x^0 y^1 coeff is %f", dY->coeff[0][1]);

        ok(!dY->coeffMask[0][0], "x^0 y^0 coeff is unmasked");
        ok(!dY->coeffMask[1][0], "x^1 y^0 coeff is unmasked");
        ok(!dY->coeffMask[0][1], "x^0 y^1 coeff is unmasked");

        ok(dY->coeffMask[1][1], "x^1 y^1 coeff is masked");
        ok(dY->coeffMask[1][2], "x^1 y^2 coeff is masked");

        psFree (dY);
        psFree (poly);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test psPolynomial2D_dY (supplied output)
    {
        psMemId id = psMemGetId();

        note ("test psPolynomial2D_dY (supplied output)");

        psPolynomial2D *poly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 2, 2);
        ok(poly != NULL, "psPolynomial2D successfully allocated");
        skip_start(poly == NULL, 5, "Skipping tests because psPolynomial2DAlloc() failed");

        // build sample polynomial (upper-left elements only)
        // z = 5 + 2x + 3y - 3x^2 + 4xy - 2y^2
        // dz/dy = 3 + 4x - 4y
        poly->coeff[0][0] = 5.0;
        poly->coeff[1][0] = 2.0;
        poly->coeff[0][1] = 3.0;
        poly->coeff[2][0] = -3.0;
        poly->coeff[1][1] = 4.0;
        poly->coeff[0][2] = -2.0;

        // mask remaining elements
        poly->coeffMask[2][1] = 1;
        poly->coeffMask[1][2] = 1;
        poly->coeffMask[2][2] = 1;

        psPolynomial2D *dY = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 2, 2);
        psPolynomial2D_dY (dY, poly);

        ok(dY->nX == 2, "new x order is %d", dY->nX);
        ok(dY->nY == 1, "new y order is %d", dY->nY);

        is_float(dY->coeff[0][0], +3.0, "x^0 y^0 coeff is %f", dY->coeff[0][0]);
        is_float(dY->coeff[1][0], +4.0, "x^1 y^0 coeff is %f", dY->coeff[1][0]);
        is_float(dY->coeff[0][1], -4.0, "x^0 y^1 coeff is %f", dY->coeff[0][1]);

        ok(!dY->coeffMask[0][0], "x^0 y^0 coeff is unmasked");
        ok(!dY->coeffMask[1][0], "x^1 y^0 coeff is unmasked");
        ok(!dY->coeffMask[0][1], "x^0 y^1 coeff is unmasked");

        ok(dY->coeffMask[1][1], "x^1 y^1 coeff is masked");
        ok(dY->coeffMask[1][2], "x^1 y^2 coeff is masked");

        psFree (dY);
        psFree (poly);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test psPolynomial2D_dY (supplied output)
    {
        psMemId id = psMemGetId();

        note ("test psPolynomial2D_dY (supplied output)");

        psPolynomial2D *poly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 2, 2);
        ok(poly != NULL, "psPolynomial2D successfully allocated");
        skip_start(poly == NULL, 5, "Skipping tests because psPolynomial2DAlloc() failed");

        // build sample polynomial (upper-left elements only)
        // z = 5 + 2x + 3y - 3x^2 + 4xy - 2y^2
        // dz/dy = 3 + 4x - 4y
        poly->coeff[0][0] = 5.0;
        poly->coeff[1][0] = 2.0;
        poly->coeff[0][1] = 3.0;
        poly->coeff[2][0] = -3.0;
        poly->coeff[1][1] = 4.0;
        poly->coeff[0][2] = -2.0;

        // mask remaining elements
        poly->coeffMask[2][1] = 1;
        poly->coeffMask[1][2] = 1;
        poly->coeffMask[2][2] = 1;

        psPolynomial2D *result = psPolynomial2D_dY (poly, poly);
        ok (result == NULL, "psPolynomial2D_dY failed as expected: cannot assign output to input");

        psFree (poly);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    return exit_status();
}
