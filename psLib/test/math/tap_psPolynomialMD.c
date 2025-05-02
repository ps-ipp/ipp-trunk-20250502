#include <stdio.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

#define XORDER 3
#define YORDER 2
static float constant = 1.0;
static float xCoeff[XORDER] = { 5.0, -3.0, 2.0 };
static float yCoeff[YORDER] = { -6.0, 4.0 };
#define XNUM 10
#define YNUM 10
#define NUM ((XNUM)*(YNUM))
#define TOL 1.0e-8

int main(int argc, char *argv[])
{
    plan_tests(2 * (1 + (1 + XORDER + YORDER)) + 1);

    {
        psMemId id = psMemGetId();

        psArray *ordinates = psArrayAlloc(NUM);
        psVector *values = psVectorAlloc(NUM, PS_TYPE_F32);

        for (int i = 0, index = 0; i < XNUM; i++) {
            int x = i - XNUM/2;
            for (int j = 0; j < YNUM; j++, index++) {
                int y = j - YNUM/2;

                float value = constant;
                for (int k = 0; k < XORDER; k++) {
                    value += xCoeff[k] * powf(x, k+1);
                }
                for (int k = 0; k < YORDER; k++) {
                    value += yCoeff[k] * powf(y, k+1);
                }

                values->data.F32[index] = value;
                psVector *ord = psVectorAlloc(2, PS_TYPE_F32);
                ord->data.F32[0] = x;
                ord->data.F32[1] = y;
                ordinates->data[index] = ord;
            }
        }

        psVector *orders = psVectorAlloc(2, PS_TYPE_S32);
        orders->data.S32[0] = XORDER;
        orders->data.S32[1] = YORDER;

        {
            psPolynomialMD *poly = psPolynomialMDAlloc(orders);
            bool polyOK = psPolynomialMDFit(poly, values, NULL, NULL, 0, ordinates);
            ok(polyOK, "Fit polynomial");
            skip_start(!polyOK, 1+XORDER+YORDER, "Skipping coefficient checks since fit failed.");
            int index = 0;
            is_double_tol(poly->coeff->data.F64[index], constant, TOL, "Coefficient %d", index);
            index++;
            for (int i = 0; i < XORDER; i++, index++) {
                is_double_tol(poly->coeff->data.F64[index], xCoeff[i], TOL, "Coefficient %d", index);
            }
            for (int i = 0; i < YORDER; i++, index++) {
                is_double_tol(poly->coeff->data.F64[index], yCoeff[i], TOL, "Coefficient %d", index);
            }
            skip_end();
            psFree(poly);
        }

        {
            psPolynomialMD *poly = psPolynomialMDAlloc(orders);
            values->data.F32[NUM/2] *= 10;
            bool polyOK = psPolynomialMDClipFit(poly, values, NULL, NULL, 0, ordinates, 1, 3.0);
            ok(polyOK, "Clip-fit polynomial");
            skip_start(!polyOK, 1+XORDER+YORDER, "Skipping coefficient checks since clip-fit failed.");
            int index = 0;
            is_double_tol(poly->coeff->data.F64[index], constant, TOL, "Coefficient %d", index);
            index++;
            for (int i = 0; i < XORDER; i++, index++) {
                is_double_tol(poly->coeff->data.F64[index], xCoeff[i], TOL, "Coefficient %d", index);
            }
            for (int i = 0; i < YORDER; i++, index++) {
                is_double_tol(poly->coeff->data.F64[index], yCoeff[i], TOL, "Coefficient %d", index);
            }
            skip_end();
            psFree(poly);
        }

        psFree(orders);
        psFree(values);
        psFree(ordinates);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    exit(EXIT_SUCCESS);
}
