/*****************************************************************************
This routine contains code which tests the general math utility functions.
 
XXX: This test never really was complete.  I simply converted the lates version
to libtap format.  It really needs more work.
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define NUM_DATA 10
#define VERBOSE 0
#define EXTRA_VERBOSE 0
#define TS00_X_F32  0x00000001
#define TS00_X_F64  0x00000002
#define TS00_X_NULL  0x00000004
#define TS00_DOMAIN_F32  0x00000008
#define TS00_DOMAIN_F64  0x00000010
#define TS00_DOMAIN_NULL 0x00000020
#define TS00_RANGE_F32  0x00000040
#define TS00_RANGE_F64  0x00000080
#define TS00_RANGE_NULL  0x00000200
#define TOLERANCE 0.01

psS32 genericInterpolateTest(
    psU32 flags,
    psS32 order,
    psS32 numData,
    psF64 xValue,
    psBool expectedRC)
{
    psS32 testStatus = true;
    psScalar *x = NULL;
    psVector *domain = NULL;
    psVector *range = NULL;
    psScalar *out = NULL;

    psMemId id = psMemGetId();
    if (expectedRC == false && VERBOSE) {
        printf("This test should generate an error message, and return NULL.\n");
    }

    if (flags & TS00_X_NULL) {
        if (VERBOSE)
            printf(" using a NULL x scalar\n");
    }

    if (flags & TS00_X_F32) {
        if (VERBOSE)
            printf(" using a psF32 x scalar\n");
        x = psScalarAlloc((psF32) xValue, PS_TYPE_F32);
    }

    if (flags & TS00_X_F64) {
        if (VERBOSE)
            printf(" using a psF64 x scalar\n");
        x = psScalarAlloc((psF64) xValue, PS_TYPE_F64);
    }

    if (flags & TS00_DOMAIN_NULL) {
        if (VERBOSE)
            printf(" using a NULL domain vector\n");
    }

    if (flags & TS00_DOMAIN_F32) {
        if (VERBOSE)
            printf(" using a psF32 domain vector\n");
        domain = psVectorAlloc(numData, PS_TYPE_F32);
        for (psS32 i=0;i<numData;i++) {
            domain->data.F32[i] = (psF32) i;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                if (VERBOSE)
                    printf("Original domain data %d: (%.1f)\n", i, domain->data.F32[i]);
            }
        }
    }

    if (flags & TS00_DOMAIN_F64) {
        if (VERBOSE)
            printf(" using a psF64 domain vector\n");
        domain = psVectorAlloc(numData, PS_TYPE_F64);
        for (psS64 i=0;i<numData;i++) {
            domain->data.F64[i] = (psF64) i;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original domain data %d: (%.1f)\n", i, domain->data.F64[i]);
            }
        }
    }

    if (flags & TS00_RANGE_NULL) {
        if (VERBOSE)
            printf(" using a NULL range vector\n");
    }

    if (flags & TS00_RANGE_F32) {
        if (VERBOSE)
            printf(" using a psF32 range vector\n");
        range = psVectorAlloc(numData, PS_TYPE_F32);
        for (psS32 i=0;i<numData;i++) {
            range->data.F32[i] = (psF32) 2*i;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original range data %d: (%.1f)\n", i, range->data.F32[i]);
            }
        }
    }

    if (flags & TS00_RANGE_F64) {
        if (VERBOSE)
            printf(" using a psF64 range vector\n");
        range = psVectorAlloc(numData, PS_TYPE_F64);
        for (psS64 i=0;i<numData;i++) {
            range->data.F64[i] = (psF64) 2*i;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original range data %d: (%.1f)\n", i, range->data.F64[i]);
            }
        }
    }

    out = p_psVectorInterpolate(NULL, domain, range, order, x);
    if (out == NULL) {
        if (expectedRC == true) {
            diag("TEST ERROR: the p_psVectorInterpolate function returned NULL.\n");
            testStatus = false;
        }
    } else {
        if (expectedRC == false) {
            diag("TEST ERROR: the p_psVectorInterpolate function returned non-NULL.\n");
            testStatus = false;
        }

        if (flags & TS00_X_F32) {
            if ((out->data.F32 - (2.0 * x->data.F32)) > TOLERANCE) {
                diag("The interpolated value was %.2f: should be %.2f\n", out->data.F32, 2.0 * x->data.F32);
                testStatus = false;
            }

        } else if (flags & TS00_X_F64) {
            if ((out->data.F64 - (2.0 * x->data.F64)) > TOLERANCE) {
                diag("The interpolated value was %.2f: should be %.2f\n", out->data.F64, 2.0 * x->data.F64);
                testStatus = false;
            }
        }

        /*
        if (0) {
                if (flags & TS00_F_F32) {
                    expectData = f->data.F32[i];
                } else if (flags & TS00_F_F64) {
                    expectData = (psF32) f->data.F64[i];
                } else if (flags & TS00_F_S32) {
                    expectData = (psF32) f->data.S32[i];
                }
         
                    if (flags & TS00_X_F32) {
                        xData = x->data.F32[i];
                    } else if (flags & TS00_X_F64) {
                        xData = (psF32) x->data.F64[i];
                    } else if (flags & TS00_X_S32) {
                        xData = (psF32) x->data.S32[i];
                    } else if (flags & TS00_X_NULL) {
                        if (flags & TS00_POLY_ORD) {
                            xData = (psF32) i;
                        } else if (flags & TS00_POLY_CHEB) {
                            xData = ((2.0 / ((psF32) (numData - 1))) * ((psF32) i)) - 1.0;
                        }
                    }
         
                    psF32 actualData = psPolynomial1DEval(myPoly, xData);
         
                    if (fabs(actualData-expectData) > fabs(ERROR_TOLERANCE * expectData)) {
                        printf("TEST ERROR: Fitted data %d: (%.1f %.1f), expected was (%.1f)\n",
                               i, xData, actualData, expectData);
                        testStatus = false;
                     } else {
                        if (VERBOSE) {
                            printf("GOOD: Fitted data %d: (%.1f %.1f), expected was (%.1f)\n",
                                     i, xData, actualData, expectData);
                        }
                    }
                }
        }
        */
    }

    psFree(x);
    psFree(out);
    psFree(domain);
    psFree(range);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");

    return (testStatus);
}

/*****************************************************************************
 *****************************************************************************/
psS32 main()
{
    psLogSetFormat("HLNM");
    plan_tests(2);

    //
    // F32 tests:
    //
    // All Vectors non-NULL

    ok(genericInterpolateTest(TS00_X_F32 | TS00_DOMAIN_F32 | TS00_RANGE_F32, 5, NUM_DATA, 5.5, true),
       "p_psVectorInterpolate() worked");
}
