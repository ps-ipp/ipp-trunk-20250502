/** @file  tst_psCoord.c
*
*  @brief The code will ...
*
*  @author GLG, MHPCC
*
*  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
*  @date $Date: 2007-06-05 01:10:22 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define ORDER_X 2
#define ORDER_Y 3
#define ORDER_Z 4
#define ORDER_T 5
#define N 10
#define COLOR 1.0
#define MAGNITUDE 1.0

psS32 main( psS32 argc, char* argv[] )
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(57);

    // Test psPlaneAlloc()
    {
        psMemId id = psMemGetId();
        psPlane *myP = psPlaneAlloc();
        ok(myP != NULL, "psPlaneAlloc() returned non-NULL");
        skip_start(myP == NULL, 4, "Skipping tests because psPlaneAlloc() returned NULL");
        ok(isnan(myP->x), "psPlane->x is NAN");
        ok(isnan(myP->y), "psPlane->y is NAN");
        ok(isnan(myP->xErr), "psPlane->xErr is NAN");
        ok(isnan(myP->yErr), "psPlane->yErr is NAN");
        skip_end();
        psFree(myP);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test psPlaneTransformAlloc()
    {
        psMemId id = psMemGetId();
        psPlaneTransform *myPT = psPlaneTransformAlloc(ORDER_X, ORDER_Y);
        ok(myPT != NULL, "psPlaneTransformAlloc() returned non-NULL");
        skip_start(myPT == NULL, 4, "Skipping tests because psPlaneTransformAlloc() returned NULL");
        ok(myPT->x->nX == ORDER_X, "psPlaneTransform->x->nX set correctly");
        ok(myPT->y->nX == ORDER_X, "psPlaneTransform->y->nX set correctly");
        ok(myPT->x->nY == ORDER_Y, "psPlaneTransform->x->nY set correctly");
        ok(myPT->y->nY == ORDER_Y, "psPlaneTransform->y->nY set correctly");
        psFree(myPT);

        // Attempt to specify negative x order and verify NULL returned and
        // errror message generated
        myPT = psPlaneTransformAlloc(-1, 1);
        ok(myPT == NULL, "psPlaneTransformAlloc(-1, 1) returned NULL");
        psFree(myPT);

        myPT = psPlaneTransformAlloc(1, -1);
        ok(myPT == NULL, "psPlaneTransformAlloc(1, -1) returned NULL");
        psFree(myPT);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test psPlaneDistortAlloc()
    {
        psMemId id = psMemGetId();
        psPlaneDistort *myPD = psPlaneDistortAlloc(ORDER_X, ORDER_Y, ORDER_Z, ORDER_T);
        ok(myPD != NULL, "psPlaneDistortAlloc() returned non-NULL");
        skip_start(myPD == NULL, 12, "Skipping tests because psPlaneDistortAlloc() returned NULL");
        ok(myPD->x->nX == ORDER_X, "psPlaneDistort->x->nX set correctly");
        ok(myPD->x->nY == ORDER_Y, "psPlaneDistort->x->nY set correctly");
        ok(myPD->x->nZ == ORDER_Z, "psPlaneDistort->x->nZ set correctly");
        ok(myPD->x->nT == ORDER_T, "psPlaneDistort->x->nT set correctly");
        ok(myPD->y->nX == ORDER_X, "psPlaneDistort->y->nX set correctly");
        ok(myPD->y->nY == ORDER_Y, "psPlaneDistort->y->nY set correctly");
        ok(myPD->y->nZ == ORDER_Z, "psPlaneDistort->y->nZ set correctly");
        ok(myPD->y->nT == ORDER_T, "psPlaneDistort->y->nT set correctly");
        psFree(myPD);

        myPD = psPlaneDistortAlloc(-1, 1, 1, 1);
        ok(myPD == NULL, "psPlaneDistortAlloc(-1, 1, 1, 1) returned NULL");
        psFree(myPD);
        myPD = psPlaneDistortAlloc(1, -1, 1, 1);
        ok(myPD == NULL, "psPlaneDistortAlloc(1, -1, 1, 1) returned NULL");
        psFree(myPD);
        myPD = psPlaneDistortAlloc(1, 1, -1, 1);
        ok(myPD == NULL, "psPlaneDistortAlloc(1, 1, -1, 1) returned NULL");
        psFree(myPD);
        myPD = psPlaneDistortAlloc(1, 1, 1, -1);
        ok(myPD == NULL, "psPlaneDistortAlloc(1, 1, 1, -1) returned NULL");
        psFree(myPD);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // We test psPlaneTransformApply() on a simple identity transformation for few x,y pairs.
    {
        psMemId id = psMemGetId();
        psPlane* in = psPlaneAlloc();
        ok(in != NULL, "psPlaneAlloc() returned non-NULL");
        skip_start(in == NULL, 2, "Skipping tests because psPlaneAlloc() returned NULL");
        psPlaneTransform* pt = psPlaneTransformAlloc(2,2);
        ok(pt != NULL, "psPlaneTransformAlloc() returned non-NULL");
        skip_start(pt == NULL, 1, "Skipping tests because psPlaneTransformAlloc() returned NULL");

        // Set transform coefficients so the x coord input will equal x coord output, same for y
        pt->x->coeff[1][0] = 1.0;
        pt->y->coeff[0][1] = 1.0;

        // Apply transform for several points
        bool errorFlag = false;
        for (psS32 i = 0; i < N; i++)
        {
            in->x = (psF64) i;
            in->y = (psF64) (i + 5.0);
            in->xErr = 0.0;
            in->yErr = 0.0;

            // XXX: psPlane *out = psPlaneTransformApply(out, pt, in); causes a seg-fault.
            // Why?
            psPlane *out = psPlaneTransformApply(NULL, pt, in);
            if(out == NULL) {
                diag("ERROR: psPlaneTransformApply() returned NULL");
                errorFlag = true;
            } else {
                if (FLT_EPSILON < fabs(out->x - in->x)) {
                    diag("ERROR: out.x is %lf, should be %lf", out->x, in->x);
                    errorFlag = true;
                }

                if (FLT_EPSILON < fabs(out->y - in->y)) {
                    diag("ERROR: out.y is %lf, should be %lf", out->y, in->y);
                    errorFlag = true;
                }
            }
            psFree(out);
        }
        ok(!errorFlag, "psPlaneTransformApply() successful on several data points");
        psFree(pt);
        psFree(in);
        skip_end();
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // psPlaneTransformApply should generate error message for NULL psPlaneTransform
    {
        psMemId id = psMemGetId();
        psPlane* in = psPlaneAlloc();
        psPlane *tmpPL = psPlaneTransformApply(NULL, NULL, in);
        ok(tmpPL == NULL, "psPlaneTransformApply(NULL, NULL, in) returned NULL");
        psFree(in);
        psFree(tmpPL);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // psPlaneTransformApply should generate error message for NULL x coeff psPlaneTransform
    {
        psMemId id = psMemGetId();
        psPlane *in = psPlaneAlloc();
        psPlaneTransform *pt = psPlaneTransformAlloc(2,2);
        psFree(pt->x);
        pt->x = NULL;
        psPlane *tmpPL = psPlaneTransformApply(NULL, pt, in);
        ok(tmpPL == NULL, "psPlaneTransformApply(NULL, pt, in) returned NULL with NULL x coeff psPlaneTransform");
        psFree(in);
        psFree(pt);
        psFree(tmpPL);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    // psPlaneTransformApply Should generate error message for NULL y coeff psPlaneTransform");
    {
        psMemId id = psMemGetId();
        psPlane* in = psPlaneAlloc();
        psPlaneTransform* pt = psPlaneTransformAlloc(2,2);
        psFree(pt->y);
        pt->y = NULL;
        psPlane *tmpPL = psPlaneTransformApply(NULL, pt, in);
        ok(tmpPL == NULL, "psPlaneTransformApply(NULL, pt, in) returned NULL with NULL y coeff psPlaneTransform");
        psFree(tmpPL);
        psFree(pt);
        psFree(in);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // psPlaneTransformApply() should generate error message for NULL psPlane");
    {
        psMemId id = psMemGetId();
        psPlaneTransform* pt = psPlaneTransformAlloc(2,2);
        psPlane *tmpPL = psPlaneTransformApply(NULL, pt, NULL);
        ok(tmpPL == NULL, "psPlaneTransformApply(NULL, pt, NULL) did not return NULL");
        psFree(tmpPL);
        psFree(pt);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // We test psPlaneDistortApply() on a simple identity transformation for few x,y pairs.
    {
        psMemId id = psMemGetId();
        psPlane *in = psPlaneAlloc();
        ok(in != NULL, "psPlaneAlloc() returned non-NULL");
        skip_start(in == NULL, 1, "Skipping tests because psPlaneAlloc() returned NULL");
        psPlaneDistort *pt = psPlaneDistortAlloc(2, 2, 2, 2);
        ok(pt != NULL, "psPlaneDistortAlloc() returned non-NULL");
        skip_start(pt == NULL, 1, "Skipping tests because psPlaneTransformAlloc() returned NULL");

        pt->x->coeff[1][0][0][0] = 1.0;
        pt->x->coeff[0][0][1][0] = 1.0;
        pt->x->coeff[0][0][0][1] = 1.0;
        pt->y->coeff[0][1][0][0] = 1.0;
        pt->y->coeff[0][0][1][0] = 1.0;
        pt->y->coeff[0][0][0][1] = 1.0;

        bool errorFlag = false;
        for (psS32 i = 0; i < N; i++)
        {
            in->x = (psF64) i;
            in->y = (psF64) (i + 5.0);
            in->xErr = 0.0;
            in->yErr = 0.0;

            // XXX: psPlane *out = psPlaneDistortApply(out, pt, in, COLOR, MAGNITUDE); generates a seg-fault.  Why?
            psPlane *out = psPlaneDistortApply(NULL, pt, in, COLOR, MAGNITUDE);
            if(out == NULL) {
                diag("ERROR: psPlaneDistortApply() returned NULL");
                errorFlag = true;
            } else {
                if (FLT_EPSILON < fabs(out->x - COLOR - MAGNITUDE - in->x)) {
                    diag("ERROR: out->x is %lf, should be %lf",
                         out->x, in->x + COLOR + MAGNITUDE);
                    errorFlag = true;
                }
                if (FLT_EPSILON < fabs(out->y - COLOR - MAGNITUDE - in->y)) {
                    diag("ERROR: out->y is %lf, should be %lf",
                         out->y, in->y + COLOR + MAGNITUDE);
                    errorFlag = true;
                }
            }
            psFree(out);
        }
        ok(!errorFlag, "psPlaneDistortApply() successful on several data points");
        psFree(in);
        psFree(pt);
        skip_end();
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // psPlaneDistortApply() should generate an error message for null psPlaneDistort
    if (1) {
        psMemId id = psMemGetId();
        psPlane *in = psPlaneAlloc();
        psPlane *tmpPL = psPlaneDistortApply(NULL, NULL, in, COLOR, MAGNITUDE);
        ok(tmpPL == NULL, "psPlaneDistortApply(NULL, NULL, in, COLOR, MAGNITUDE) returned NULL");
        psFree(tmpPL);
        psFree(in);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // psPlaneDistortApply() should generate an error message for null x member psPlaneDistort
    if (1) {
        psMemId id = psMemGetId();
        psPlane *in = psPlaneAlloc();
        psPlaneDistort *pt = psPlaneDistortAlloc(2, 2, 2, 2);
        psFree(pt->x);
        pt->x = NULL;
        psPlane *tmpPL = psPlaneDistortApply(NULL, pt, in, COLOR, MAGNITUDE);
        ok(tmpPL == NULL, "psPlaneDistortApply(NULL, pt, in, COLOR, MAGNITUDE) with NULL pt->x member");
        psFree(tmpPL);
        psFree(pt);
        psFree(in);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // psPlaneDistortApply() should generate an error message for null y member psPlaneDistort
    if (1) {
        psMemId id = psMemGetId();
        psPlane *in = psPlaneAlloc();
        psPlaneDistort *pt = psPlaneDistortAlloc(2, 2, 2, 2);
        psFree(pt->y);
        pt->y = NULL;
        psPlane *tmpPL = psPlaneDistortApply(NULL, pt, in, COLOR, MAGNITUDE);
        ok(tmpPL == NULL, "psPlaneDistortApply(NULL, pt, in, COLOR, MAGNITUDE) with NULL pt->y member");
        psFree(tmpPL);
        psFree(pt);
        psFree(in);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // psPlaneDistortApply() should generate an error message for null input psPlane
    if (1) {
        psMemId id = psMemGetId();
        psPlaneDistort *pt = psPlaneDistortAlloc(2, 2, 2, 2);
        psPlane *tmpPL = psPlaneDistortApply(NULL, pt, NULL, COLOR, MAGNITUDE);
        ok(tmpPL == NULL, "psPlaneDistortApply(NULL, pt, NULL, COLOR, MAGNITUDE) returned NULL");
        psFree(tmpPL);
        psFree(pt);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test psPixelsTransform()
    // psPixelsTransform() should generate error message for NULL input pixels
    {
        psMemId id = psMemGetId();
        psPlaneTransform *trans = psPlaneTransformAlloc(1, 3);
        psPixels *output = psPixelsTransform(NULL, NULL, trans);
        ok(output == NULL, "psPixelsTransform(NULL, NULL, trans) returned NULL");
        psFree(trans);
        psFree(output);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // psPixelsTransform() should generate error message for NULL input PlaneTransform
    {
        psMemId id = psMemGetId();
        psPixels *input = psPixelsAlloc(2);
        psPixels *output = psPixelsTransform(NULL, input, NULL);
        ok(output == NULL, "psPixelsTransform(NULL, input, NULL) returned NULL");
        psFree(output);
        psFree(input);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // We test psPixelsTransform() on several data points
    {
        psMemId id = psMemGetId();
        psPixels *input = psPixelsAlloc(2);
        input->n = 2;
        input->data[0].x = 1.0;
        input->data[0].y = 1.0;
        input->data[1].x = 1.0;
        input->data[1].y = 6.0;
        psPlaneTransform *trans = psPlaneTransformAlloc(1, 3);
        trans->x->coeff[0][0] = 0;
        trans->x->coeff[1][0] = 1.0;
        trans->y->coeff[0][0] = 0;
        trans->y->coeff[0][0] = 0;
        trans->y->coeff[0][2] = 0.5;

        // XXX: Fix this
        psPixels *output = psPixelsTransform(NULL, input, trans);
        int nExpected = 9;
        if (output->n != nExpected)
        {
            diag("ERROR: psPixelsTransform failed to return the expected number of pixels.\n");
            printf("\n output returned with %ld pixels\n\n", output->n);
            for (int i = 0; i < output->n; i++) {
                printf("  (%6.2lf, %6.2lf) pixel %d\n", output->data[i].x, output->data[i].y, i+1);
            }
            return 3;
        }

        psFree(trans);
        psFree(input);
        psFree(output);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

