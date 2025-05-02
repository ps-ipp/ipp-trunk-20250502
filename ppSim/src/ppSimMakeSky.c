# include "ppSim.h"

// this function sets the skyRate to a value for the night sky (SKY.RATE or SKY.MAGS) or for a
// flat-field image (FLAT.RATE).  Include a shutter correction and a scattered light source

bool ppSimMakeSky (pmReadout *readout, psImage *expCorr, ppSimType type, pmConfig *config) {

    bool status;

    psImage *signal = readout->image;
    psImage *variance = readout->variance;

    pmCell *cell = readout->parent;
    pmChip *chip = cell->parent;
    pmFPA  *fpa  = chip->parent;

    psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, PPSIM_RECIPE); // Recipe

    bool sky  = psMetadataLookupBool(&status, recipe, "SKY"); // Generate a SKY flux?
    bool flat = psMetadataLookupBool(&status, recipe, "FLAT"); // Apply flat-field term?

    float expTime      = psMetadataLookupF32(&status, recipe, "EXPTIME"); // Exposure time

    float flatSigma    = psMetadataLookupF32(&status, recipe, "FLAT.SIGMA"); // Flat-field coefficient
    float flatRate     = psMetadataLookupF32(&status, recipe, "FLAT.RATE"); // Flat-field rate
    float shutterTime  = psMetadataLookupF32(&status, recipe, "SHUTTER.TIME"); // Shutter time
    float scatterFrac  = psMetadataLookupF32(&status, recipe, "SCATTER.FRAC"); // scattered light fraction (max)

    float skyRate      = (type == PPSIM_TYPE_FLAT) ? flatRate : ppSimGetSkyRate (recipe);

    int x0Chip        = psMetadataLookupS32(&status, chip->concepts, "CHIP.X0");
    int y0Chip        = psMetadataLookupS32(&status, chip->concepts, "CHIP.Y0");
    int xParityChip   = psMetadataLookupS32(&status, chip->concepts, "CHIP.XPARITY");
    int yParityChip   = psMetadataLookupS32(&status, chip->concepts, "CHIP.YPARITY");

    int x0Cell        = psMetadataLookupS32(&status, cell->concepts, "CELL.X0");
    int y0Cell        = psMetadataLookupS32(&status, cell->concepts, "CELL.Y0");
    int xParityCell   = psMetadataLookupS32(&status, cell->concepts, "CELL.XPARITY");
    int yParityCell   = psMetadataLookupS32(&status, cell->concepts, "CELL.YPARITY");

    int binning = psMetadataLookupS32(NULL, recipe, "BINNING"); // Binning in x and y

    // Size of FPA
    psRegion *bounds = ppSimFPABounds (fpa);
    int dXfpa = bounds->x1 - bounds->x0;
    int dYfpa = bounds->y1 - bounds->y0;

    // Correct chip offsets so that boresight is in the middle of the FPA
    x0Chip -= 0.5 * dXfpa;
    y0Chip -= 0.5 * dYfpa;

    for (int y = 0; y < signal->numRows; y++) {

        float yFPA = PPSIM_CELL_TO_FPA(y, y0Cell, yParityCell, binning, y0Chip, yParityChip) * 2.0 /
            (bounds->y1 - bounds->y0); // Relative y position in FPA

        for (int x = 0; x < signal->numCols; x++) {
            float xFPA = PPSIM_CELL_TO_FPA(x, x0Cell, xParityCell, binning, x0Chip, xParityChip) * 2.0 /
                (bounds->x1 - bounds->x0); // Relative x position in FPA

            // Shutter: adjust exposure time
            float realExpTime = expTime + shutterTime * (xFPA + yFPA + 2.0) / 4.0;

            // Gaussian flat-field over the FPA with flatValue = 1.0 at the field center
            float flatValue = 1.0;
            if (flat) {
                // we make the flat-field have a response of 1.0 at the field center (like a vignetting)
                flatValue = expf(-0.5 / PS_SQR(flatSigma) * (PS_SQR(yFPA) + PS_SQR(xFPA)));
            }

            float scatterRate = 0.0;

            if (sky) {
              // add a scattered light term to the flat-field images
              if (type == PPSIM_TYPE_FLAT) {
                  scatterRate = scatterFrac * PS_SQR(xFPA);
              }

              // Sky background
              float skyFlux = (skyRate * (flatValue + scatterRate)) * realExpTime; // Flux from sky
              signal->data.F32[y][x] += skyFlux;
              variance->data.F32[y][x] += skyFlux;

	      if ((x == (int)(signal->numCols / 2)) && (y == (int)(signal->numRows / 2))) {
		psLogMsg("ppSim", PS_LOG_INFO, "Generating sky at image center = %f cnts / pixel\n", skyFlux);
	      }
            }

            // used later to modify the star and galaxy photometry
            if (expCorr) {
                // exposure correction is (effective exposure time) * (flatValue)
              expCorr->data.F32[y][x] = flatValue * realExpTime / expTime;
            }

            // TO DO: Add fringes

        }
    }

    psFree(bounds);
    return true;
}

