#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

int main (void)
{
    plan_tests(34);

//    diag("psImageShiftKernel() tests");

    // tests using BILINEAR interpolation
    // skip until we implement the integer shift portion
    if (0) {
        psMemId id = psMemGetId();

//        diag("shift a delta function by an integer offset");

        // generate simple image (delta function)
        psImage *image = psImageAlloc(32, 32, PS_TYPE_F32);
        ok(image != NULL, "psImage successfully allocated");
        skip_start(image == NULL, 5, "Skipping tests because psImageAlloc() failed");

        // use delta function as input test image
        image->data.F32[10][10] = 1;

        // integer shifts should be exact
        psImage *shift = psImageShiftKernel (NULL, image, 1.0, 1.0, PS_INTERPOLATE_BILINEAR);
        is_float (shift->data.F32[10][10], 0.0, "point 10,10 should be 0.0 : %f", shift->data.F32[10][10]);
        is_float (shift->data.F32[10][11], 0.0, "point 10,11 should be 0.0 : %f", shift->data.F32[10][11]);
        is_float (shift->data.F32[10][12], 0.0, "point 10,12 should be 0.0 : %f", shift->data.F32[10][12]);
        is_float (shift->data.F32[11][10], 0.0, "point 11,10 should be 0.0 : %f", shift->data.F32[11][10]);
        is_float (shift->data.F32[11][11], 1.0, "point 11,11 should be 1.0 : %f", shift->data.F32[11][11]);
        is_float (shift->data.F32[11][12], 0.0, "point 11,12 should be 0.0 : %f", shift->data.F32[11][12]);
        is_float (shift->data.F32[12][10], 0.0, "point 12,10 should be 0.0 : %f", shift->data.F32[12][10]);
        is_float (shift->data.F32[12][11], 0.0, "point 12,11 should be 0.0 : %f", shift->data.F32[12][11]);
        is_float (shift->data.F32[12][12], 0.0, "point 12,12 should be 0.0 : %f", shift->data.F32[12][12]);
        psFree(shift);
        psFree(image);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // tests using BILINEAR interpolation
    // XXX skip until we implement BILINEAR
    if (0) {
        psMemId id = psMemGetId();

//        diag("shift a delta function by an fractional pixel offset");

        // generate simple image (delta function)
        psImage *image = psImageAlloc(32, 32, PS_TYPE_F32);
        ok(image != NULL, "psImage successfully allocated");
        skip_start(image == NULL, 5, "Skipping tests because psImageAlloc() failed");

        // XXX is this needed?
        psImageInit (image, 0.0);

        // use delta function as input test image
        image->data.F32[10][10] = 1;

        // fractional shifts introduce smoothing,
        psImage *shift = psImageShiftKernel (NULL, image, 0.2, 0.4, PS_INTERPOLATE_BICUBE);
//        diag("these require tolerance of 4 epsilon (why?)");
        is_float_tol (shift->data.F32[10][10], 0.8*0.6, 4*FLT_EPSILON, "point 10,10 should be %f : %f", 0.8*0.6, shift->data.F32[10][10]);
        is_float_tol (shift->data.F32[10][11], 0.2*0.6, 4*FLT_EPSILON, "point 10,11 should be %f : %f", 0.2*0.6, shift->data.F32[10][11]);
        is_float_tol (shift->data.F32[10][12], 0.0,     4*FLT_EPSILON, "point 10,12 should be %f : %f", 0.0,     shift->data.F32[10][12]);
        is_float_tol (shift->data.F32[11][10], 0.8*0.4, 4*FLT_EPSILON, "point 11,10 should be %f : %f", 0.8*0.4, shift->data.F32[11][10]);
        is_float_tol (shift->data.F32[11][11], 0.2*0.4, 4*FLT_EPSILON, "point 11,11 should be %f : %f", 0.2*0.4, shift->data.F32[11][11]);
        is_float_tol (shift->data.F32[11][12], 0.0,     4*FLT_EPSILON, "point 11,12 should be %f : %f", 0.0,     shift->data.F32[11][12]);
        is_float_tol (shift->data.F32[12][10], 0.0,     4*FLT_EPSILON, "point 12,10 should be %f : %f", 0.0,     shift->data.F32[12][10]);
        is_float_tol (shift->data.F32[12][11], 0.0,     4*FLT_EPSILON, "point 12,11 should be %f : %f", 0.0,     shift->data.F32[12][11]);
        is_float_tol (shift->data.F32[12][12], 0.0,     4*FLT_EPSILON, "point 12,12 should be %f : %f", 0.0,     shift->data.F32[12][12]);
        psFree(shift);
        psFree(image);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test by measuring gaussian centroid before and after
    // using BILINEAR interpolation
    // XXX skip until we implement BILINEAR
    if (0) {
        psMemId id = psMemGetId();

//        diag("shift a gaussian and measure centroid");

        // generate simple image (gauss function)
        psImage *image = psImageAlloc(32, 32, PS_TYPE_F32);
        ok(image != NULL, "psImage successfully allocated");
        skip_start(image == NULL, 5, "Skipping tests because psImageAlloc() failed");

        // build a simple gaussian (16,16),(2,1) as input test image
        for (int j = 0; j < image->numRows; j++) {
            for (int i = 0; i < image->numCols; i++) {
                float z = PS_SQR(i - 16.0)/8.0 + PS_SQR(j - 16.0)/2.0;
                image->data.F32[j][i] = exp(-z);
            }
        }

        // measure centroid of unshifted gaussian (should be 16.0,16.0)
        float xo = 0.0;
        float yo = 0.0;
        float no = 0.0;
        for (int j = 0; j < image->numRows; j++) {
            for (int i = 0; i < image->numCols; i++) {
                xo += i*image->data.F32[j][i];
                yo += j*image->data.F32[j][i];
                no += image->data.F32[j][i];
            }
        }
        xo /= no;
        yo /= no;

//        diag ("these require a tolerance of 100 epsilon");
        is_float_tol (xo, 16.0, 100*FLT_EPSILON, "x-centroid should be %f : %f", 16.0, xo);
        is_float_tol (xo, 16.0, 100*FLT_EPSILON, "y-centroid should be %f : %f", 16.0, yo);

        // fractional shifts introduce smoothing,
        psImage *shift = psImageShiftKernel (NULL, image, 1.2, -1.2, PS_INTERPOLATE_BILINEAR);

        // measure centroid of shifted gaussian (should be 17.2,14.8)
        xo = yo = no = 0.0;
        for (int j = 0; j < shift->numRows; j++) {
            for (int i = 0; i < shift->numCols; i++) {
                xo += i*shift->data.F32[j][i];
                yo += j*shift->data.F32[j][i];
                no += shift->data.F32[j][i];
            }
        }
        xo /= no;
        yo /= no;

//        diag ("these require a tolerance of 100 epsilon");
        is_float_tol (xo, 17.2, 100*FLT_EPSILON, "x-centroid should be %f : %f", 17.2, xo);
        is_float_tol (yo, 14.8, 100*FLT_EPSILON, "y-centroid should be %f : %f", 14.8, yo);

        psFree(shift);
        psFree(image);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // tests using BICUBE interpolation
    {
        psMemId id = psMemGetId();

        // generate simple image (x ramp)
        psImage *image = psImageAlloc(32, 32, PS_TYPE_F32);
        ok(image != NULL, "psImage successfully allocated");
        skip_start(image == NULL, 5, "Skipping tests because psImageAlloc() failed");

        // use delta function as input test image
        image->data.F32[11][11] = 1;

        // fractional shifts introduce smoothing,
        psImage *shift = psImageShiftKernel (NULL, image, 0.2, 0.4, PS_INTERPOLATE_BICUBE);
//        diag ("please check these values: this kernel is very steep and ringy.  what is the right BICUBE kernel?");
        is_float_tol (shift->data.F32[10][10], -1.444445, 4*FLT_EPSILON, "point 10,10 should be %f : %f", -1.444445, shift->data.F32[10][10]);
        is_float_tol (shift->data.F32[10][11], -0.277778, 4*FLT_EPSILON, "point 10,11 should be %f : %f", -0.277778, shift->data.F32[10][11]);
        is_float_tol (shift->data.F32[10][12], -0.777778, 4*FLT_EPSILON, "point 10,12 should be %f : %f", -0.777778, shift->data.F32[10][12]);
        is_float_tol (shift->data.F32[11][10], +0.055556, 4*FLT_EPSILON, "point 11,10 should be %f : %f", +0.055556, shift->data.F32[11][10]);
        is_float_tol (shift->data.F32[11][11], +1.222222, 4*FLT_EPSILON, "point 11,11 should be %f : %f", +1.222222, shift->data.F32[11][11]);
        is_float_tol (shift->data.F32[11][12], +0.722222, 4*FLT_EPSILON, "point 11,12 should be %f : %f", +0.722222, shift->data.F32[11][12]);
        is_float_tol (shift->data.F32[12][10], -0.111111, 4*FLT_EPSILON, "point 12,10 should be %f : %f", -0.111111, shift->data.F32[12][10]);
        is_float_tol (shift->data.F32[12][11], +1.055556, 4*FLT_EPSILON, "point 12,11 should be %f : %f", +1.055556, shift->data.F32[12][11]);
        is_float_tol (shift->data.F32[12][12], +0.555556, 4*FLT_EPSILON, "point 12,12 should be %f : %f", +0.555556, shift->data.F32[12][12]);

        psFree(shift);
        psFree(image);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test by measuring gaussian centroid before and after
    // using BICUBE interpolation
    {
        psMemId id = psMemGetId();

//        diag("shift a gaussian and measure centroid");

        // generate simple image (gauss function)
        psImage *image = psImageAlloc(32, 32, PS_TYPE_F32);
        ok(image != NULL, "psImage successfully allocated");
        skip_start(image == NULL, 5, "Skipping tests because psImageAlloc() failed");

        // build a simple gaussian (16,16),(2,1) as input test image
        for (int j = 0; j < image->numRows; j++)
        {
            for (int i = 0; i < image->numCols; i++) {
                float z = PS_SQR(i - 16.0)/8.0 + PS_SQR(j - 16.0)/2.0;
                image->data.F32[j][i] = exp(-z);
            }
        }

        // measure centroid of unshifted gaussian (should be 16.0,16.0)
        float xo = 0.0;
        float yo = 0.0;
        float no = 0.0;
        for (int j = 0; j < image->numRows; j++)
        {
            for (int i = 0; i < image->numCols; i++) {
                xo += i*image->data.F32[j][i];
                yo += j*image->data.F32[j][i];
                no += image->data.F32[j][i];
            }
        }
        xo /= no;
        yo /= no;

//        diag ("these require a tolerance of 100 epsilon");
        is_float_tol (xo, 16.0, 100*FLT_EPSILON, "x-centroid should be %f : %f", 16.0, xo);
        is_float_tol (xo, 16.0, 100*FLT_EPSILON, "y-centroid should be %f : %f", 16.0, yo);

        // fractional shifts introduce smoothing,
        psImage *shift = psImageShiftKernel (NULL, image, 0.2, -0.2, PS_INTERPOLATE_BICUBE);

        // measure centroid of shifted gaussian (should be 17.2,14.8)
        xo = yo = no = 0.0;
        for (int j = 0; j < shift->numRows; j++)
        {
            for (int i = 0; i < shift->numCols; i++) {
                xo += i*shift->data.F32[j][i];
                yo += j*shift->data.F32[j][i];
                no += shift->data.F32[j][i];
            }
        }
        xo /= no;
        yo /= no;

//        diag ("these require a tolerance of 100 epsilon");
        is_float_tol (xo, 16.2, 100*FLT_EPSILON, "x-centroid should be %f : %f", 16.2, xo);
        is_float_tol (yo, 15.8, 100*FLT_EPSILON, "y-centroid should be %f : %f", 15.8, yo);

        psFree(shift);
        psFree(image);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // tests using GAUSS interpolation
    {
        psMemId id = psMemGetId();

        // generate simple image (x ramp)
        psImage *image = psImageAlloc(32, 32, PS_TYPE_F32);
        ok(image != NULL, "psImage successfully allocated");
        skip_start(image == NULL, 5, "Skipping tests because psImageAlloc() failed");

        // use delta function as input test image
        image->data.F32[11][11] = 1;

        // fractional shifts introduce smoothing,
        psImage *shift = psImageShiftKernel (NULL, image, 0.0, 0.0, PS_INTERPOLATE_GAUSS);
        is_float_tol (shift->data.F32[10][10], -0.157778, 4*FLT_EPSILON, "point 10,10 should be %f : %f", -0.157778, shift->data.F32[10][10]);
        is_float_tol (shift->data.F32[10][11], +0.168889, 4*FLT_EPSILON, "point 10,11 should be %f : %f", +0.168889, shift->data.F32[10][11]);
        is_float_tol (shift->data.F32[10][12], -0.131111, 4*FLT_EPSILON, "point 10,12 should be %f : %f", -0.131111, shift->data.F32[10][12]);
        is_float_tol (shift->data.F32[11][10], +0.142221, 4*FLT_EPSILON, "point 11,10 should be %f : %f", +0.142221, shift->data.F32[11][10]);
        is_float_tol (shift->data.F32[11][11], +0.488884, 4*FLT_EPSILON, "point 11,11 should be %f : %f", +0.488884, shift->data.F32[11][11]);
        is_float_tol (shift->data.F32[11][12], +0.208878, 4*FLT_EPSILON, "point 11,12 should be %f : %f", +0.208878, shift->data.F32[11][12]);
        is_float_tol (shift->data.F32[12][10], -0.064500, 4*FLT_EPSILON, "point 12,10 should be %f : %f", -0.064500, shift->data.F32[12][10]);
        is_float_tol (shift->data.F32[12][11], +0.302066, 4*FLT_EPSILON, "point 12,11 should be %f : %f", +0.302066, shift->data.F32[12][11]);
        is_float_tol (shift->data.F32[12][12], +0.041884, 4*FLT_EPSILON, "point 12,12 should be %f : %f", +0.041884, shift->data.F32[12][12]);
        psFree(shift);
        psFree(image);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test by measuring gaussian centroid before and after
    // using GAUSS interpolation
    {
        psMemId id = psMemGetId();

//        diag("shift a gaussian and measure centroid");

        // generate simple image (gauss function)
        psImage *image = psImageAlloc(32, 32, PS_TYPE_F32);
        ok(image != NULL, "psImage successfully allocated");
        skip_start(image == NULL, 5, "Skipping tests because psImageAlloc() failed");

        // build a simple gaussian (16,16),(2,1) as input test image
        for (int j = 0; j < image->numRows; j++)
        {
            for (int i = 0; i < image->numCols; i++) {
                float z = PS_SQR(i - 16.0)/8.0 + PS_SQR(j - 16.0)/2.0;
                image->data.F32[j][i] = exp(-z);
            }
        }

        // measure centroid of unshifted gaussian (should be 16.0,16.0)
        float xo = 0.0;
        float yo = 0.0;
        float no = 0.0;
        for (int j = 0; j < image->numRows; j++)
        {
            for (int i = 0; i < image->numCols; i++) {
                xo += i*image->data.F32[j][i];
                yo += j*image->data.F32[j][i];
                no += image->data.F32[j][i];
            }
        }
        xo /= no;
        yo /= no;

//        diag ("these require a tolerance of 100 epsilon");
        is_float_tol (xo, 16.0, 100*FLT_EPSILON, "x-centroid should be %f : %f", 16.0, xo);
        is_float_tol (xo, 16.0, 100*FLT_EPSILON, "y-centroid should be %f : %f", 16.0, yo);

        // fractional shifts introduce smoothing,
        psImage *shift = psImageShiftKernel (NULL, image, 0.2, 0.4, PS_INTERPOLATE_GAUSS);

        // measure centroid of shifted gaussian (should be 17.2,14.8)
        xo = yo = no = 0.0;
        for (int j = 0; j < shift->numRows; j++)
        {
            for (int i = 0; i < shift->numCols; i++) {
                xo += i*shift->data.F32[j][i];
                yo += j*shift->data.F32[j][i];
                no += shift->data.F32[j][i];
            }
        }
        xo /= no;
        yo /= no;

//        diag ("these require a tolerance of 100 epsilon");
        is_float_tol (xo, 16.2, 100*FLT_EPSILON, "x-centroid should be %f : %f", 16.2, xo);
        is_float_tol (yo, 15.8, 100*FLT_EPSILON, "y-centroid should be %f : %f", 15.8, yo);

        psFits *fits = psFitsOpen ("gauss.fits", "w");
        psFitsWriteImage (fits, NULL, image, 0, NULL);
        psFitsClose (fits);

        fits = psFitsOpen ("shift.fits", "w");
        psFitsWriteImage (fits, NULL, shift, 0, NULL);
        psFitsClose (fits);

        psFree(shift);
        psFree(image);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    return exit_status();
}
