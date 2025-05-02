#ifndef PP_SIM_H
#define PP_SIM_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>   // for unlink
#include <pslib.h>
#include <psmodules.h>
#include <psastro.h>
#include <psphot.h>

#define PPSIM_RECIPE "PPSIM"
// #define OUTPUT_FILE "PPSIM.OUTPUT"

// Compare a value with minimum and maximum values, replacing where required.
#define COMPARE(VALUE,MIN,MAX) {                \
        if (VALUE < MIN) { MIN = VALUE; }       \
        if (VALUE > MAX) { MAX = VALUE; }       \
    }

// Return cell position, given an FPA position; calculations are all done in pixel units
#define PPSIM_FPA_TO_CELL(pos, cell0, cellParity, binning, chip0, chipParity) \
    (((pos) - (chip0))*(chipParity) - (cell0))*(cellParity) / (binning)


// Return FPA position, given a cell position; calculations are all done in pixel units
#define PPSIM_CELL_TO_FPA(pos, cell0, cellParity, binning, chip0, chipParity) \
    ((chip0) + (binning)*(chipParity)*((cell0) + (cellParity)*(pos)))

// Type of image to simulate
typedef enum {
    PPSIM_TYPE_NONE,                    // No type set
    PPSIM_TYPE_BIAS,                    // Bias image
    PPSIM_TYPE_DARK,                    // Dark image
    PPSIM_TYPE_FLAT,                    // Flat-field image
    PPSIM_TYPE_OBJECT                   // Object image
} ppSimType;

typedef struct {
    double ra;
    double dec;
    float mag;
    float x;
    float y;
    float flux;
    float peak;
  bool external; // star was supplied (not randomly generated) for this analysis
} ppSimStar;

typedef struct {
    double ra;
    double dec;
    float mag;
    float x;
    float y;
    float flux;

    float peak;
    float Rmaj;
    float Rmin;
    float theta;
    float index;
} ppSimGalaxy;

ppSimStar *ppSimStarAlloc(void);
ppSimGalaxy *ppSimGalaxyAlloc(void);

/// Parse command-line arguments
bool ppSimArguments(int argc, char **argv, ///< Command-line arguments
                    pmConfig *config ///< Configuration
    );

/// Create output file
///
/// Returns a borrowed pointer to the FPA file.
pmFPAfile *ppSimCreate(pmConfig *config ///< Configuration
    );

// Return bounds of a chip, based on the concepts
psRegion *ppSimChipBounds(const pmChip *chip, // Chip for which to determine size
                          pmFPAview *view // View for chip
    );

// Return bounds of an FPA, based on the concepts
psRegion *ppSimFPABounds(const pmFPA *fpa       // FPA for which to determine size
    );

/// Loop over the output file, generating simulated data
bool ppSimLoop(pmConfig *config ///< Configuration
    );

psVector *ppSimMakeBiassec (pmCell *cell, pmConfig *config);
psVector *ppSimMakeBias (bool *status, pmReadout *readout, pmConfig *config, const psRandom *rng) ;
bool ppSimMakeDark (pmReadout *readout, pmConfig *config);
bool ppSimMakeSky (pmReadout *readout, psImage *expCorr, ppSimType type, pmConfig *config);

bool ppSimLoadSpots (pmFPA *fpa, pmConfig *config);

bool ppSimLoadStars (psArray *stars, pmFPA *fpa, pmConfig *config);
bool ppSimMakeStars(psArray *stars, pmFPA *fpa, pmConfig *config, const psRandom *rng);
bool ppSimMakeStarCluster(psArray *stars, pmFPA *fpa, pmConfig *config, const psRandom *rng);
bool ppSimMakeStarGrid(psArray *stars, pmFPA *fpa, pmConfig *config, const psRandom *rng);
bool ppSimInsertStars (pmReadout *readout, psImage *expCorr, psArray *stars, pmConfig *config);

bool ppSimSmoothReadout(pmReadout *input, psMetadata *recipe);

bool ppSimInitHeader(pmConfig *config,
                     pmFPA *fpa,
                     pmChip *chip,
                     pmCell *cell);

bool ppSimSaturate(pmReadout *readout,  // Image to apply saturation
                   const pmConfig *config); // Saturation level

bool ppSimUpdateConceptsFPA (pmFPA *fpa, pmConfig *config);
bool ppSimUpdateConceptsCell (pmCell *cell, pmConfig *config);

bool ppSimAddOverscan (pmReadout *readout, pmConfig *config, psVector *biasCols, psVector *biasRows, psRandom *rng);

bool ppSimAddNoise(psImage *signal,
                   psImage *variance,
                   const pmCell *cell,
                   const pmConfig *config,
                   const psRandom *rng // Random number generator
    );

bool ppSimSetPSF (pmChip *chip, pmConfig *config);

bool ppSimMakeGalaxies(psArray *galaxies, pmFPA *fpa, pmConfig *config, const psRandom *rng);
bool ppSimInsertGalaxies (pmReadout *readout, psImage *expCorr, psArray *galaxies, pmConfig *config);

bool ppSimMosaicChip(pmConfig *config, const psImageMaskType blankMask, const pmFPAview *view, const char *outFile, const char *inFile);

bool ppSimPhotom (pmConfig *config, pmFPAview *view);


// add a bad CTE region
bool ppSimBadCTE(psImage *image,        // Signal image, modified and returned
                 const pmConfig *config // configuration
  );

/// Add bad pixels to an image
bool ppSimBadPixels(pmReadout *readout, ///< Readout for which to generate bad pixels
                    const pmConfig *config, ///< Configuration
                    psRandom *rng       ///< Random number generator
    );

float ppSimStarSkyNoise (float skySigma, float seeingSigma);
float ppSimStarPeakToFlux (float peak, float seeingSigma);
float ppSimStarFluxToPeak (float flux, float seeingSigma);
float ppSimFluxToMag (float flux, float zp);
float ppSimMagToFlux (float mag, float zp);

float ppSimArgToRecipeF32(bool *status,
                          psMetadata *options,    // Target to which to add value
                          const char *recipeName, // Name for value in the recipe
                          psMetadata *arguments,  // Command-line arguments
                          const char *argName    // Argument name in the command-line arguments
    );

int ppSimArgToRecipeS32(bool *status,
                        psMetadata *options,    // Target to which to add value
                        const char *recipeName, // Name for value in the recipe
                        psMetadata *arguments,  // Command-line arguments
                        const char *argName      // Argument name in the command-line arguments
    );

char *ppSimArgToRecipeStr(bool *status,
                          psMetadata *options,    // Target to which to add value
                          const char *recipeName, // Name for value in the recipe
                          psMetadata *arguments,  // Command-line arguments
                          const char *argName    // Argument name in the command-line arguments
    );

bool ppSimArgToRecipeBool(bool *status,
                          psMetadata *options,    // Target to which to add value
                          const char *recipeName, // Name for value in the recipe
                          psMetadata *arguments,  // Command-line arguments
                          const char *argName       // Argument name in the command-line arguments
    );

ppSimType ppSimTypeFromString (char *typeStr);
char *ppSimTypeToString (ppSimType type);

float ppSimGetZeroPoint(psMetadata *recipe, const char *filter);
float ppSimGetSkyRate(psMetadata *recipe);

bool ppSimMergeReadouts (pmConfig *config, pmFPAview *view);

double ppSimRandomGaussian (const psRandom *rnd, double mean, double sigma);
double ppSimRandomGaussianNorm (const psRandom *rnd);
void ppSimRandomGaussianFree(void);

bool ppSimPhotomFiles (pmConfig *config, pmFPAfile *fakeFile, pmFPAfile *forceFile);

bool ppSimPhotomReadoutFake(pmConfig *config, const pmFPAview *view);
bool ppSimPhotomReadoutForce(pmConfig *config, const pmFPAview *view);

psArray *ppSimLoadForceSources(pmConfig *config, const pmFPAview *view);
bool ppSimDetections (psImage *significance, psMetadata *recipe, psArray *sources);
psArray *ppSimMergeSources (psArray *in1, psArray *in2);

psArray *ppSimSelectSources (pmConfig *config, const pmFPAview *view, const char *filename);
bool ppSimDefinePixels (psArray *sources, pmReadout *readout, psMetadata *recipe);

/// Return software version
psString ppSimVersion(void);

/// Return software source
psString ppSimSource(void);

/// Return long version information
psString ppSimVersionLong(void);

#endif
