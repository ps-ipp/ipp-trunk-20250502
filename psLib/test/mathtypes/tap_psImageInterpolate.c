#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

int main (void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(47);

    // very simple tests: no mask, bilinear mode, xramp image only
    {
        psMemId id = psMemGetId();
        // generate simple image (x ramp)
        psImage *image = psImageAlloc(32, 32, PS_TYPE_F32);
        ok(image != NULL, "psImage successfully allocated");
        skip_start(image == NULL, 5, "Skipping tests because psImageAlloc() failed");

        image->data.F32[10][10] = 1;

        // center of pixels is 0.5, 0.5
        float value;

        value = psImagePixelInterpolate (image, 10.5, 10.5, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 1.0, "pixel center value - %f", value);

        value = psImagePixelInterpolate (image, 10.9, 10.5, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float_tol (value, 0.6, 4.0*FLT_EPSILON, "pixel value - %.20f", value);

        value = psImagePixelInterpolate (image, 10.5, 10.9, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float_tol (value, 0.6, 4.0*FLT_EPSILON, "pixel value - %.20f", value);

        value = psImagePixelInterpolate (image, 10.1, 10.5, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float_tol (value, 0.6, 4.0*FLT_EPSILON, "pixel value - %.20f", value);

        skip_end();

        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // very simple tests: no mask, bilinear mode, xramp image only
    {
        psMemId id = psMemGetId();
        // generate simple image (x ramp)
        psImage *image = psImageAlloc(32, 32, PS_TYPE_F32);
        ok(image != NULL, "psImage successfully allocated");
        skip_start(image == NULL, 5, "Skipping tests because psImageAlloc() failed");

        for (int j = 0; j < image->numRows; j++)
        {
            for (int i = 0; i < image->numCols; i++) {
                image->data.F32[j][i] = i + 0.5;
            }
        }

        // center of pixels is 0.5, 0.5
        float value;

        value = psImagePixelInterpolate (image, 2.5, 2.5, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 2.5, "pixel center value - %f", value);

        value = psImagePixelInterpolate (image, 2.2, 2.5, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 2.2, "coord: 2.2, 2.5, value: %f", value);

        value = psImagePixelInterpolate (image, 2.8, 2.5, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 2.8, "coord: 2.8, value: %f", value);

        value = psImagePixelInterpolate (image, 2.8, 2.2, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 2.8, "coord: 2.8, value: %f", value);

        value = psImagePixelInterpolate (image, 2.8, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 2.8, "coord: 2.8, value: %f", value);

        value = psImagePixelInterpolate (image, 0.8, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 0.8, "coord: 0.8, value: %f", value);

        // no extrapolation
        value = psImagePixelInterpolate (image, 0.3, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 0.5, "coord: 0.3, value: %f", value);

        value = psImagePixelInterpolate (image, -0.2, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 0.5, "coord: -0.2, value: %f", value);

        skip_end();

        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // very simple tests: no mask, bilinear mode, yramp image only
    {
        psMemId id = psMemGetId();
        // generate simple image (y ramp)
        psImage *image = psImageAlloc(32, 32, PS_TYPE_F32);
        ok(image != NULL, "psImage successfully allocated");
        skip_start(image == NULL, 5, "Skipping tests because psImageAlloc() failed");

        for (int j = 0; j < image->numRows; j++)
        {
            for (int i = 0; i < image->numCols; i++) {
                image->data.F32[j][i] = j + 0.5;
            }
        }

        // center of pixels is 0.5, 0.5
        float value;

        value = psImagePixelInterpolate (image, 2.5, 2.5, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 2.5, "pixel center value - %f", value);

        value = psImagePixelInterpolate (image, 2.2, 2.2, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 2.2, "coord: 2.2, 2.5, value: %f", value);

        value = psImagePixelInterpolate (image, 2.5, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 2.8, "coord: 2.8, value: %f", value);

        value = psImagePixelInterpolate (image, 2.2, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 2.8, "coord: 2.8, value: %f", value);

        value = psImagePixelInterpolate (image, 2.8, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BILINEAR);
        is_float (value, 2.8, "coord: 2.8, value: %f", value);

        skip_end();

        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // very simple tests: no mask, bicube mode, xramp image only
    {
        psMemId id = psMemGetId();
        // generate simple image (x ramp)
        psImage *image = psImageAlloc(32, 32, PS_TYPE_F32);
        ok(image != NULL, "psImage successfully allocated");
        skip_start(image == NULL, 5, "Skipping tests because psImageAlloc() failed");

        for (int j = 0; j < image->numRows; j++)
        {
            for (int i = 0; i < image->numCols; i++) {
                image->data.F32[j][i] = i + 0.5;
            }
        }

        // center of pixels is 0.5, 0.5
        float value;

        value = psImagePixelInterpolate (image, 2.5, 2.5, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 2.5, "coord; 2.5, 2.5, value - %f", value);

        value = psImagePixelInterpolate (image, 2.2, 2.5, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 2.2, "coord: 2.2, 2.5, value: %f", value);

        value = psImagePixelInterpolate (image, 2.8, 2.5, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 2.8, "coord: 2.8, 2.5, value: %f", value);

        value = psImagePixelInterpolate (image, 2.8, 2.2, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 2.8, "coord: 2.8, 2.2, value: %f", value);

        value = psImagePixelInterpolate (image, 2.8, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 2.8, "coord: 2.8, 2.8, value: %f", value);

        // no extrapolation: these return the 'uncover' value
        value = psImagePixelInterpolate (image, 0.8, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 0.0, "coord: 0.8, 2.8, value: %f", value);

        value = psImagePixelInterpolate (image, 0.3, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 0.0, "coord: 0.3, 2.8, value: %f", value);

        value = psImagePixelInterpolate (image, -0.2, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 0.0, "coord: -0.2, 2.8, value: %f", value);

        skip_end();

        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // very simple tests: no mask, bilinear mode, yramp image only
    {
        psMemId id = psMemGetId();
        // generate simple image (y ramp)
        psImage *image = psImageAlloc(32, 32, PS_TYPE_F32);
        ok(image != NULL, "psImage successfully allocated");
        skip_start(image == NULL, 5, "Skipping tests because psImageAlloc() failed");

        for (int j = 0; j < image->numRows; j++)
        {
            for (int i = 0; i < image->numCols; i++) {
                image->data.F32[j][i] = j + 0.5;
            }
        }

        // center of pixels is 0.5, 0.5
        float value;

        value = psImagePixelInterpolate (image, 2.5, 2.5, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 2.5, "pixel center value - %f", value);

        value = psImagePixelInterpolate (image, 2.2, 2.2, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 2.2, "coord: 2.2, 2.5, value: %f", value);

        value = psImagePixelInterpolate (image, 2.5, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 2.8, "coord: 2.8, value: %f", value);

        value = psImagePixelInterpolate (image, 2.2, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 2.8, "coord: 2.8, value: %f", value);

        value = psImagePixelInterpolate (image, 2.8, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 2.8, "coord: 2.8, value: %f", value);

        skip_end();

        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // very simple tests: no mask, bilinear mode, x,y 2nd order shape
    {
        psMemId id = psMemGetId();
        // generate simple image (x ramp)
        psImage *image = psImageAlloc(32, 32, PS_TYPE_F32);
        ok(image != NULL, "psImage successfully allocated");
        skip_start(image == NULL, 5, "Skipping tests because psImageAlloc() failed");

        for (int j = 0; j < image->numRows; j++)
        {
            for (int i = 0; i < image->numCols; i++) {
                image->data.F32[j][i] = 0.25*PS_SQR(i + 0.5) + j + 0.5;
            }
        }

        // center of pixels is 0.5, 0.5
        float value;

        value = psImagePixelInterpolate (image, 2.5, 2.5, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 4.0625, "pixel center value - %f", value);

        value = psImagePixelInterpolate (image, 2.2, 2.2, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 3.41, "coord: 2.2, 2.5, value: %f", value);

        value = psImagePixelInterpolate (image, 2.5, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 4.3625002, "coord: 2.5, 2.8, value: %f", value);

        value = psImagePixelInterpolate (image, 2.2, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 4.010000229, "coord: 2.2, 2.8, value: %f", value);

        value = psImagePixelInterpolate (image, 2.8, 2.8, NULL, 0, 0.0, PS_INTERPOLATE_BICUBE);
        is_float (value, 4.75999975, "coord: 2.8, 2.8, value: %f", value);

        skip_end();

        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
