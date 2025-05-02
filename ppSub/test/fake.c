#include <stdio.h>
#include <pslib.h>

// Quick and dirty program to generate cross-directed PSFs for testing ppSub.
// To build: gcc fake.c -o fake -g -std=gnu99 `pslib-config --cflags --libs`


// PSF for images
#define X_AXIS_1 5.0                    // Length of x axis (FWHM) for image 1
#define Y_AXIS_1 3.0                    // Length of y axis (FWHM) for image 1
#define X_AXIS_2 5.0                    // Length of x axis (FWHM) for image 1
#define Y_AXIS_2 5.0                    // Length of y axis (FWHM) for image 1

// Image parameters
#define NUMCOLS 1024                    // Number of columns
#define NUMROWS 1024                    // Number of rows
#define IMAGE_SCALE 0.2                 // Arcsec per pixel

// Source parameters
#define SOURCES_LUM 0.4                 // Luminosity function
#define SOURCES_MAG 14.5                // Magnitude for density
#define SOURCES_DENSITY 30000.0         // Source density at nominated magnitude (per deg^2)
#define SOURCES_ZP 25.0                 // Magnitude ZP
#define SOURCES_GAUSSIAN 0              // Gaussian sources (boolean)?

// Noise properties
#define NOISE_1 10.0                    // Noise in image 1
#define NOISE_2 10.0                    // Noise in image 2
#define NOISE_FRAC 0.1                  // Go out to this fraction of the noise

// Conversion factor by which to multiply sigma to get FWHM.  Roughly 2.35
#define SIGMA_FWHM (2.0*sqrt((double)2.0*(log((double)2.0))))

// Add a star to an image
static void addstar(float x, float y,   // Coordinates of source
                    float flux,         // Total (integrated) flux of source
                    float xSigma, float ySigma, // Gaussian width of source
                    float noise,        // Lowest noise level to consider
                    psImage *image      // Image to which to add source
                    )
{
    float peak = flux / 2.0 / M_PI / sqrtf(PS_SQR(xSigma) + PS_SQR(ySigma)); // Peak flux
    if (peak < noise) {
        return;
    }

#if SOURCES_GAUSSIAN
    // Gaussian
    float radius = sqrtf(2.0 * (PS_SQR(xSigma) + PS_SQR(ySigma)) * logf(peak / noise)); // Radius for source
#else
    // Basic Waussian
    float radius;
    if (peak / noise > 10.0) {
        // Need to treat the bright limit differently because the wings are much more noticeable
        radius = sqrtf(2.0 * (PS_SQR(xSigma) + PS_SQR(ySigma)) * peak / noise);
    } else {
        radius = sqrtf(2.0) * hypot(1.0 / xSigma, 1.0 / ySigma) * sqrtf(2.0 * logf(peak / noise));
    }
#endif

    // Bounds of source
    int xMin = PS_MAX(floorf(x - radius), 0);
    int xMax = PS_MIN(ceilf(x + radius), NUMCOLS - 1);
    int yMin = PS_MAX(floorf(y - radius), 0);
    int yMax = PS_MIN(ceilf(y + radius), NUMROWS - 1);

    for (int j = yMin; j <= yMax; j++) {
        float dy = j - y;
        for (int i = xMin; i <= xMax; i++) {
            float dx = i - x;
#if SOURCES_GAUSSIAN
            // Gaussian
            float value = peak * expf(- PS_SQR(dx) / 2.0 / PS_SQR(xSigma) -
                                      PS_SQR(dy) / 2.0 / PS_SQR(ySigma));
#else
            // Basic Waussian
            float z = 0.5 * (PS_SQR(dx / xSigma) + PS_SQR(dy / ySigma));
            float value = peak / (1.0 + z + PS_SQR(z));
#endif

            image->data.F32[j][i] += value;
        }
    }

    return;
}


int main(int argc, char *argv[])
{
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator

    // Images to generate
    psImage *image1 = psImageAlloc(NUMCOLS, NUMROWS, PS_TYPE_F32);
    psImage *image2 = psImageAlloc(NUMCOLS, NUMROWS, PS_TYPE_F32);

    psImageInit(image1, 0.0);
    psImageInit(image2, 0.0);

    float magNorm = SOURCES_DENSITY * NUMCOLS * NUMROWS * PS_SQR(IMAGE_SCALE / 3600.0) /
        powf(10.0, (SOURCES_LUM * SOURCES_MAG)); // Normalisation for magnitudes

    float faintMag = -2.5 * log10(PS_MIN(NOISE_1 * sqrtf(X_AXIS_1 * Y_AXIS_1),
                                         NOISE_2 * sqrtf(X_AXIS_2 * Y_AXIS_2)) *
                                  NOISE_FRAC) + SOURCES_ZP; // Faintest magnitude
    printf("Faintest mag: %f\n", faintMag);

    long numStars = magNorm * powf(10.0, SOURCES_LUM * faintMag); // Number of stars
    printf("%ld stars\n", numStars);

    // Gaussian widths, in sigma
    float xSigma1 = X_AXIS_1 / SIGMA_FWHM;
    float ySigma1 = Y_AXIS_1 / SIGMA_FWHM;
    float xSigma2 = X_AXIS_2 / SIGMA_FWHM;
    float ySigma2 = Y_AXIS_2 / SIGMA_FWHM;

    for (int i = 0; i < numStars; i++) {
        float x = psRandomUniform(rng) * NUMCOLS; // x coordinate
        float y = psRandomUniform(rng) * NUMROWS; // y coordinate
        float mag = log10((i + 1.0) / magNorm) / SOURCES_LUM; // Magnitude of source
        float flux = powf(10.0, -0.4 * (mag - SOURCES_ZP)); // Total flux of source

        addstar(x, y, flux, xSigma1, ySigma1, NOISE_1 * NOISE_FRAC, image1);
        addstar(x, y, flux, xSigma2, ySigma2, NOISE_2 * NOISE_FRAC, image2);
    }

#if 1
    psImage *var1 = (psImage*)psBinaryOp(NULL, image1, "+", psScalarAlloc(PS_SQR(NOISE_1), PS_TYPE_F32));
    psImage *var2 = (psImage*)psBinaryOp(NULL, image2, "+", psScalarAlloc(PS_SQR(NOISE_2), PS_TYPE_F32));
#else
    psImage *var1 = psImageAlloc(NUMCOLS, NUMROWS, PS_TYPE_F32);
    psImage *var2 = psImageAlloc(NUMCOLS, NUMROWS, PS_TYPE_F32);
    psImageInit(var1, PS_SQR(NOISE_1));
    psImageInit(var2, PS_SQR(NOISE_2));
#endif

    for (int y = 0; y < NUMROWS; y++) {
        for (int x = 0; x < NUMCOLS; x++) {
            image1->data.F32[y][x] += psRandomGaussian(rng) * sqrtf(var1->data.F32[y][x]);
            image2->data.F32[y][x] += psRandomGaussian(rng) * sqrtf(var2->data.F32[y][x]);
        }
    }
#if 1
    var1 = (psImage*)psBinaryOp(var1, image1, "+", psScalarAlloc(PS_SQR(NOISE_1), PS_TYPE_F32));
    var2 = (psImage*)psBinaryOp(var2, image2, "+", psScalarAlloc(PS_SQR(NOISE_2), PS_TYPE_F32));
#else
    psImageInit(var1, PS_SQR(NOISE_1));
    psImageInit(var2, PS_SQR(NOISE_2));
#endif

    {
        psFits *fits = psFitsOpen("image1.fits", "w");
        psFitsWriteImage(fits, NULL, image1, 0, NULL);
        psFitsClose(fits);
    }
    {
        psFits *fits = psFitsOpen("image2.fits", "w");
        psFitsWriteImage(fits, NULL, image2, 0, NULL);
        psFitsClose(fits);
    }

    {
        psFits *fits = psFitsOpen("variance1.fits", "w");
        psFitsWriteImage(fits, NULL, var1, 0, NULL);
        psFitsClose(fits);
    }
    {
        psFits *fits = psFitsOpen("variance2.fits", "w");
        psFitsWriteImage(fits, NULL, var2, 0, NULL);
        psFitsClose(fits);
    }

    psFree(image1);
    psFree(image2);

    psFree(var1);
    psFree(var2);

    psFree(rng);

    return PS_EXIT_SUCCESS;
}
