#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <pslib.h>
#include "pmHDU.h"
#include "pmFPA.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmPSFtry.h"
#include "pmDetections.h"
#include "pmDetEff.h"

static void detEffFree(pmDetEff *de)
{
    psFree(de->magOffsets);
    psFree(de->counts);
    psFree(de->magDiffMean);
    psFree(de->magDiffStdev);
    psFree(de->magErrMean);
}


pmDetEff *pmDetEffAlloc(float magRef, int numSources, int numBins)
{
    pmDetEff *de = psAlloc(sizeof(pmDetEff)); // Detection efficiency, to return
    psMemSetDeallocator(de, (psFreeFunc)detEffFree);

    de->magRef = magRef;
    de->numSources = numSources;
    de->numBins = numBins;

    de->magOffsets = NULL;
    de->counts = NULL;
    de->magDiffMean = NULL;
    de->magDiffStdev = NULL;
    de->magErrMean = NULL;

    return de;
}


bool pmDetEffWrite(psFits *fits, pmDetEff *de, const psMetadata *header, const char *extname)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PM_ASSERT_DETEFF_RESULTS(de, false);

    psArray *table = psArrayAlloc(de->numBins); // Table to write
    for (int i = 0; i < de->numBins; i++) {
        psMetadata *row = psMetadataAlloc(); // Table row
        psMetadataAddF32(row, PS_LIST_TAIL, "OFFSET", 0, "Magnitude offset from reference",
                         de->magOffsets->data.F32[i]);
        psMetadataAddS32(row, PS_LIST_TAIL, "COUNTS", 0, "Number of sources recovered",
                         de->counts->data.S32[i]);
        psMetadataAddF32(row, PS_LIST_TAIL, "DIFF.MEAN", 0, "Mean magnitude difference",
                         de->magDiffMean->data.F32[i]);
        psMetadataAddF32(row, PS_LIST_TAIL, "DIFF.STDEV", 0, "Stdev magnitude difference",
                         de->magDiffStdev->data.F32[i]);
        psMetadataAddF32(row, PS_LIST_TAIL, "ERR.MEAN", 0, "Mean magnitude error",
                         de->magErrMean->data.F32[i]);
        table->data[i] = row;
    }

    psMetadata *deHeader = psMetadataCopy(NULL, header); // Header for detection efficiency
    psMetadataAddF32(deHeader, PS_LIST_TAIL, "DETEFF.MAGREF", PS_META_REPLACE, "Magnitude reference",
                     de->magRef);
    psMetadataAddS32(deHeader, PS_LIST_TAIL, "DETEFF.NUM", PS_META_REPLACE, "Number of fake sources injected",
                     de->numSources);

    if (!psFitsWriteTable(fits, deHeader, table, extname)) {
        psError(PS_ERR_IO, false, "Unable to write detection efficiency table.");
        psFree(table);
        psFree(deHeader);
        return false;
    }

    psFree(table);
    psFree(deHeader);

    return true;
}

bool pmReadoutWriteDetEff(psFits *fits, const pmReadout *readout,
                          const psMetadata *header, const char *extname)
{
    PM_ASSERT_READOUT_NON_NULL(readout, false);

    bool mdok;                          // Status of MD lookup
    pmDetEff *de = psMetadataLookupPtr(&mdok, readout->analysis, PM_DETEFF_ANALYSIS); // Detection efficiency
    if (!mdok || !de) {
        // Wrote everything there was to write
        return true;
    }
    return pmDetEffWrite(fits, de, header, extname);
}


pmDetEff *pmDetEffRead(psFits *fits, const char *extname)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_STRING_NON_EMPTY(extname, false);

    if (!psFitsMoveExtNameClean(fits, extname)) {
        // Nothing to read
        return NULL;
    }

    psMetadata *header = psFitsReadHeader(NULL, fits); // Header for table
    if (!header) {
        psError(PS_ERR_IO, false, "Unable to read FITS header");
        return NULL;
    }

    int numBins = psFitsTableSize(fits);// Size of table
    bool mdok;                          // Status of MD lookup
    int numSources = psMetadataLookupS32(&mdok, header, "DETEFF.NUM"); // Number of fake sources injected
    if (!mdok) {
        psError(PS_ERR_UNKNOWN, false, "Unable to find number of sources");
        psFree(header);
        return NULL;
    }
    float magRef = psMetadataLookupF32(&mdok, header, "DETEFF.MAGREF"); // Magnitude reference
    if (!mdok) {
        psError(PS_ERR_UNKNOWN, false, "Unable to find magnitude reference");
        psFree(header);
        return NULL;
    }
    psFree(header);

    pmDetEff *de = pmDetEffAlloc(magRef, numSources, numBins); // Detection efficiency
    de->magOffsets = psVectorAlloc(numBins, PS_TYPE_F32);
    de->counts = psVectorAlloc(numBins, PS_TYPE_S32);
    de->magDiffMean = psVectorAlloc(numBins, PS_TYPE_F32);
    de->magDiffStdev = psVectorAlloc(numBins, PS_TYPE_F32);
    de->magErrMean = psVectorAlloc(numBins, PS_TYPE_F32);

    psArray *table = psFitsReadTable(fits); // FITS table
    if (!table) {
        psError(PS_ERR_IO, false, "Unable to read detection efficiency table.");
        psFree(de);
        return false;
    }

    for (int i = 0; i < numBins; i++) {
        psMetadata *row = table->data[i]; // Table row
        de->magOffsets->data.F32[i] = psMetadataLookupF32(NULL, row, "OFFSET");
        de->counts->data.S32[i] = psMetadataLookupS32(NULL, row, "COUNTS");
        de->magDiffMean->data.F32[i] = psMetadataLookupF32(NULL, row, "DIFF.MEAN");
        de->magDiffStdev->data.F32[i] = psMetadataLookupF32(NULL, row, "DIFF.STDEV");
        de->magErrMean->data.F32[i] = psMetadataLookupF32(NULL, row, "ERR.MEAN");
    }

    psFree(table);
    return de;
}

bool pmReadoutReadDetEff(psFits *fits, const pmReadout *readout, const char *extname)
{
    PM_ASSERT_READOUT_NON_NULL(readout, false);

    pmDetEff *de = pmDetEffRead(fits, extname);
    if (!de) {
        if (psErrorCodeLast() != PS_ERR_NONE) {
            return false;
        }
        return true;
    }

    bool status = psMetadataAddPtr(readout->analysis, PS_LIST_TAIL, PM_DETEFF_ANALYSIS, PS_META_REPLACE | PS_DATA_UNKNOWN, "Detection efficiency", de);
    psFree (de);
    return status;
}
