#include <stdio.h>
#include "tap.h"
#include "pstap.h"
#include <pslib.h>

#define NUMCOLS 12                      // Number of columns for image
#define NUMROWS 34                      // Number of rows for image
#define BITPIX 32                       // Bits per pixel
#define OUTTYPE PS_TYPE_F64             // Expected output type

// Generate an image
static psImage *generateImage(void)
{
    psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 12345); // Random number generator
    psImage *image = psImageAlloc(NUMCOLS, NUMROWS, PS_TYPE_F32); // Generated image
    for (int y = 0; y < NUMROWS; y++) {
        for (int x = 0; x < NUMCOLS; x++) {
            image->data.F32[y][x] = ((x + y) % 2) ? NAN : psRandomUniform(rng);
        }
    }
    psFree(rng);
    return image;
}


int main(int argc, const char *argv[])
{
    plan_tests(7);

    {
        psMemId id = psMemGetId();

        psString name = NULL;           // Name of FITS image
        psStringAppend(&name, "%s.fits", argv[0]);

        psImage *original = generateImage(); // Image to test
        psMetadata *header = psMetadataAlloc(); // FITS header

        psFits *fits = psFitsOpen(name, "w");

        // Set compression
        psVector *tiles = psVectorAlloc(2, PS_TYPE_S32);
        tiles->data.S32[0] = 0;
        tiles->data.S32[1] = 1;
        ok(psFitsSetCompression(fits, PS_FITS_COMPRESS_RICE, tiles, 4, 0, 0), "Set compression");
        psFree(tiles);

        // Set quantisation
        fits->options = psFitsOptionsAlloc();
        fits->options->bitpix = BITPIX;
        fits->options->scaling = PS_FITS_SCALE_STDEV_BOTH;
        fits->options->stdevBits = 24;

        bool write = psFitsWriteImage(fits, header, original, 0, NULL);
        psFitsClose(fits);

        ok(write, "Write image");
        skip_start(!fits || !write, 4, "Unable to write image");
        {
            psFits *fits = psFitsOpen(name, "r");
            psImage *image = psFitsReadImage(fits, psRegionSet(0,0,0,0), 0);
            psFitsClose(fits);

            ok(image, "Read image");
            skip_start(!image, 3, "Unable to read image");
            {
                ok(image->type.type == OUTTYPE, "Image type");
                ok(image->numCols == NUMCOLS && image->numRows == NUMROWS, "Image size");
                bool consistent = true; // Images are consistent?
                double tol = 1.0 / (double)(1 << (BITPIX - 1)) + FLT_EPSILON; // Expected tolerance
                for (int y = 0; y < NUMROWS; y++) {
                    for (int x = 0; x < NUMCOLS; x++) {
                        double imageVal = psImageGet(image, x, y);
                        if ((!isfinite(original->data.F32[y][x]) && isfinite(imageVal)) ||
                            (isfinite(original->data.F32[y][x]) && !isfinite(imageVal) &&
                             original->data.F32[y][x] < 1.0 - 0.5 * tol) ||
                            (fabs(original->data.F32[y][x] - imageVal) > tol)) {
                            consistent = false;
                            diag("Inconsistent at %d,%d: %lf vs %f --> %le", x, y,
                                 imageVal, original->data.F32[y][x], imageVal - original->data.F32[y][x]);
                        }
                    }
                }
                ok(consistent, "Images consistent.");
                psFree(image);
            }
            skip_end();
        }
        skip_end();
        psFree(original);
        psFree(header);
        psFree(name);

        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}
