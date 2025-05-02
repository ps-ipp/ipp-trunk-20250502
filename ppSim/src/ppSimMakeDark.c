# include "ppSim.h"

// XXX add bounds to the inputs?
bool ppSimMakeDark (pmReadout *readout, pmConfig *config) {

    bool mdok;

    psImage *signal = readout->image;
    psImage *variance = readout->variance;

    psMetadata *recipe = psMetadataLookupMetadata(&mdok, config->recipes, PPSIM_RECIPE); // Recipe

    bool dark = psMetadataLookupBool(&mdok, recipe, "DARK"); // Generate a DARK?
    if (!dark) return true;

    float darkRate = psMetadataLookupF32(&mdok, recipe, "DARK.RATE"); // Dark rate
    if (isnan(darkRate)) {
      psError(PS_ERR_UNKNOWN, false, "missing DARK.RATE\n");
      return false;
    }

    float detTemp   = psMetadataLookupF32(&mdok, recipe, "DET.TEMP"); // Detector Temperature
    float darkTemp  = psMetadataLookupF32(&mdok, recipe, "DARK.TEMP"); // Dark temperature trend
    float darkTemp2 = psMetadataLookupF32(&mdok, recipe, "DARK.TEMP2"); // Dark temperature trend 2nd order term
    if (!isfinite(detTemp)) detTemp = 0.0;
    if (!isfinite(darkTemp)) darkTemp = 0.0;
    if (!isfinite(darkTemp2)) darkTemp2 = 0.0;

    float expTime  = psMetadataLookupF32(&mdok, recipe, "EXPTIME"); // Exposure time
    if (isnan(expTime)) {
      psError(PS_ERR_UNKNOWN, false, "missing EXPTIME\n");
      return false;
    }

    // Dark current
    float darkCurrent = (darkRate + darkTemp*detTemp + darkTemp2*PS_SQR(detTemp)) * expTime; // Dark current accumulated
    
    psTrace("ppSim", 6, "dark current: %f, darkRate: %f, darkTempTrend: %f, darkTempSecondOrder: %f, detTemp: %f, expTime: %f\n", darkCurrent, darkRate, darkTemp, darkTemp2, detTemp, expTime);

    for (int y = 0; y < signal->numRows; y++) {
        for (int x = 0; x < signal->numCols; x++) {
            signal->data.F32[y][x] += darkCurrent;
            variance->data.F32[y][x] += darkCurrent;
        }
    }
    return true;
}

