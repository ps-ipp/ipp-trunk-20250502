#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"

#define NUM 10
#define SIZE 100

psArray *generate_images(void)
{
    psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 12345);   // Random Number Generator

    // Generate images
    psArray *images = psArrayAlloc(NUM);// Array of images
    for (int i = 0; i < NUM; i++) {
        psImage *image = psImageAlloc(SIZE, SIZE, PS_TYPE_F32); // Image i
        for (int y = 0; y < SIZE; y++) {
            for (int x = 0; x < SIZE; x++) {
                image->data.F32[y][x] = psRandomGaussian(rng);
            }
        }
        images->data[i] = image;
    }
    psFree(rng);

    return images;
}

int main(int argc, char *argv[])
{
    plan_tests(6);

    diag("Image combination tests");

    // Basic combination
    {
        psArray *images = generate_images();
        psImage *combined = pmCombineImages(NULL, NULL, images, NULL, NULL, 0, NULL, 1, 3.0);
        ok(combined, "Combined image generated");
        skip_start(!combined, 5, "Combination failed.");

        ok(combined->type.type == PS_TYPE_F32, "Correct type");
        ok(combined->numCols == SIZE && combined->numRows == SIZE, "Correct size");
        int discrepant = 0;             // Number of discrepant pixels
        for (int y = 0; y < SIZE; y++)
        {
            for (int x = 0; x < SIZE; x++) {
                if (fabsf(combined->data.F32[y][x]) > 3.0 / sqrt(NUM)) {
                    discrepant++;
                }
            }
        }
        ok(discrepant <= 30, "%d discrepant pixels", discrepant); // Should have 99.7% of 100x100 pixels OK

        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
        psImageStats(stats, combined, NULL, 0);
        ok(stats->sampleMean < 3.0 / sqrt(NUM * SIZE * SIZE), "Sample mean: %e", stats->sampleMean);
        ok(fabs(stats->sampleStdev - 1.0 / sqrt(NUM)) < 1.0e-3, "Sample stdev: %e",
           stats->sampleStdev);
        psFree(stats);

        skip_end();
        psFree(combined);
        psFree(images);
    }

}
