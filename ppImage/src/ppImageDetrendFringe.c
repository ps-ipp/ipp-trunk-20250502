#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

bool ppImageDetrendFringeMeasure(pmReadout *readout, pmCell *fringe, const bool isResidual, const ppImageOptions *options)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(fringe, false);
    PS_ASSERT_PTR_NON_NULL(options, false);

    // Reference fringe measurements (stored on the reference cell->analysis)
    psArray *references = psMemIncrRefCounter(psMetadataLookupPtr(NULL, fringe->analysis, "FRINGE.MEASUREMENTS"));
    if (!references) {
        psError(PS_ERR_IO, false, "Unable to find fringe references.\n");
        return false;
    }

    pmFringeStats *reference = references->data[0]; // Take the first as representative
    pmFringeRegions *regions = reference->regions; // Regions to measure
    pmFringeStats *measurements = pmFringeStatsMeasure(regions, readout, options->maskValue); // Fringe stats

    // Normalise measurements by the exposure time
    bool mdok;                          // Status of MD lookup
    float expTime = psMetadataLookupF32(&mdok, readout->parent->concepts, "CELL.EXPOSURE"); // Exp. time
    if (!mdok || !isfinite(expTime)) {
        psError(PS_ERR_UNKNOWN, false, "CELL.EXPOSURE is not set for --- can't normalise fringes\n");
        psFree(measurements);
        return false;
    }
    if (expTime == 0) {
        psWarning("Exposure time is zero --- are you sure you want fringe subtraction?\n");
        expTime = 1.0;
    }
    // normalize by exposure time : NOTE df is 1/sigma
    psBinaryOp(measurements->f, measurements->f, "*", psScalarAlloc(1.0 / expTime, PS_TYPE_F32));
    psBinaryOp(measurements->df, measurements->df, "*", psScalarAlloc(expTime, PS_TYPE_F32));

    char *scienceFringes = NULL;
    if (isResidual) {
        scienceFringes = psStringCopy ("FRINGE.RESIDUALS");
    } else {
        scienceFringes = psStringCopy ("FRINGE.MEASUREMENTS");
    }

    // Science fringe measurements
    pmFringeStats *previous = psMetadataLookupPtr(NULL, readout->parent->analysis, scienceFringes);
    if (previous) {
        // Multiple readouts: concatenate
        psArray *concatenate = psArrayAlloc(2); // Array to hold fringes

        // Concatenate science measurements
        concatenate->data[0] = previous;
        concatenate->data[1] = measurements;
        pmFringeStats *new = pmFringeStatsConcatenate(concatenate, NULL, NULL); // New measurements
        psFree(measurements);
        measurements = new;

        // Concatenate reference measurements (duplication, so the science and reference line up)
        for (int i = 0; i < references->n; i++) {
            concatenate->data[0] = concatenate->data[1] = references->data[i];
            pmFringeStats *refNew = pmFringeStatsConcatenate(concatenate, NULL, NULL);
            psFree(references->data[i]);
            references->data[i] = refNew;
        }
        concatenate->data[0] = concatenate->data[1] = NULL;
        psFree(concatenate);
    }

    psMetadataAdd(readout->parent->analysis, PS_LIST_TAIL, scienceFringes, PS_DATA_UNKNOWN | PS_META_REPLACE, "Fringe measurements", measurements);
    psFree(measurements);
    psFree(scienceFringes);
    psFree(references);

    return true;
}


// Pull the fringes out of the cell analysis FRINGE.MEASUREMENTS for a chip
// XXX need some error checks
static psArray *getFringes(const pmChip *chip, const char *source)
{
    psArray *cells = chip->cells;       // Component cells
    psArray *fringes = psArrayAlloc(cells->n); // Fringes, to return
    int video_cell_zero = 0;

    for (int i = 0; i < cells->n; i++) {
        fringes->data[i] = NULL;

        pmCell *cell = cells->data[i];  // Cell of interest

        psTrace("psModules.detrend",7,"Readouts: Cell %d chip: %ld\n",i,cell->readouts->n);
        // XXX for now, skip the video cells (cell->readouts->n > 1)
        // CZW: This mess creates a fake set of fringe stats by stealing the previous one.
        // We let the fitting code know that this is all lies by scaling the weights by a crazy amount.

        if ( (cell->readouts->n > 1) ) {
          psTrace("psModules.detrend",7,"Should be skipping scichip: %d\n",i);
          psWarning ("Skipping Video Cell (%d) for ppImageDetrendFringe.c:getFringes", i);

          if (i == 0) {
            video_cell_zero = 1;
          }
          else {
            pmFringeStats *prevFringe = fringes->data[i-1];
            pmFringeStats *fringe     = pmFringeStatsAlloc(prevFringe->regions);
            for (int j = 0; j < fringe->regions->nRequested; j++) {
              fringe->f->data.F32[j] = prevFringe->f->data.F32[j];
              fringe->df->data.F32[j] = prevFringe->df->data.F32[j] / 1e6;
            }
            fringes->data[i] = fringe;
          }

          continue;
        }

        fringes->data[i] = psMemIncrRefCounter(psMetadataLookupPtr(NULL, cell->analysis, source));
    }

    if (video_cell_zero == 1) {
      pmFringeStats *prevFringe = fringes->data[1];
      pmFringeStats *fringe     = pmFringeStatsAlloc(prevFringe->regions);
      for (int j = 0; j < fringe->regions->nRequested; j++) {
        fringe->f->data.F32[j] = NAN;
        fringe->df->data.F32[j] = 1.0;
      }
      fringes->data[0] = fringe;
    }

    return fringes;
}


// Solve the fringe system: we have science fringe measurements for each cell, and an array of reference
// fringe measurements for each cell.  Need to concatenate these together first, and then solve.
bool ppImageDetrendFringeSolve(pmChip *scienceChip, const pmChip *refChip, const bool isResidual, const ppImageOptions *options)
{
    PS_ASSERT_PTR_NON_NULL(scienceChip, NULL);
    PS_ASSERT_PTR_NON_NULL(refChip, NULL);
    PS_ASSERT_PTR_NON_NULL(options, NULL);

    psArray *science = NULL;
    if (isResidual) {
        science = getFringes(scienceChip,  "FRINGE.RESIDUALS"); // Fringe residuals on science chip
    } else {
        science = getFringes(scienceChip, "FRINGE.MEASUREMENTS"); // Fringe measurements on science chip
    }

    pmFringeStats *scienceCat = pmFringeStatsConcatenate(science, NULL, NULL); // Science fringes
    psFree(science);

    // Need to transform the array of cells each with an array of fringes --> array of fringes for the chip as
    // a whole
    psArray *references = getFringes(refChip, "FRINGE.MEASUREMENTS"); // Fringe measurements on reference chip
    int numRefs = ((psArray*)references->data[0])->n; // Number of reference fringes
    psArray *referencesCat = psArrayAlloc(numRefs);   // Reference fringes
    for (int i = 0; i < numRefs; i++) {               // Iterate over fringes
        psArray *refs = psArrayAlloc(references->n);  // Array of fringes for each cell
        for (int j = 0; j < references->n; j++) {     // Iterate over cells
            psArray *ref = references->data[j];       // Array of references for this cell

            refs->data[j] = psMemIncrRefCounter(ref->data[i]);
        }
        referencesCat->data[i] = pmFringeStatsConcatenate(refs, NULL, NULL);
        psFree(refs);
    }
    psFree(references);

    // Now we can solve
    psTrace("ppImage", 3, "Solving fringe system...\n");
    pmFringeScale *solution = pmFringeScaleMeasure(scienceCat, referencesCat, options->fringeRej,
                                                   options->fringeIter, options->fringeKeep);

    if (isResidual) {
        psMetadataAdd(scienceChip->analysis, PS_LIST_TAIL, "FRINGE.RESIDUAL.SOLUTION", PS_DATA_UNKNOWN, "Fringe solution", solution);
    } else {
        psMetadataAdd(scienceChip->analysis, PS_LIST_TAIL, "FRINGE.SOLUTION", PS_DATA_UNKNOWN, "Fringe solution", solution);
    }

# if (0)
    // write the fringe amplitude or residual amplitude to the header
    // XXX this is measured per cell, but we only have headers per chip
    pmHDU *hdu = pmHDUFromCell(science);// HDU  of interest
    for (int i = 0; i < solution->nFringeFrames; i++) {
        // write metadata header value
        psString keyword = NULL;
        if (isResidual) {
            psStringAppend (&keyword, "FRES_%02dV", i);
        } else {
            psStringAppend (&keyword, "FRNG_%02dV", i);
        }
        psMetadataAddF32(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, "Fringe Amplitude", solution->coeff->data.F32[i + 1]);
        psFree (keyword);

        keyword = NULL;
        if (isResidual) {
            psStringAppend (&keyword, "FRES_%02dE", i);
        } else {
            psStringAppend (&keyword, "FRNG_%02dE", i);
        }
        psMetadataAddF32(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, "Fringe Amplitude error", solution->coeffErr->data.F32[i + 1]);
        psFree (keyword);
    }
# endif

    psFree(solution);
    psFree(scienceCat);
    psFree(referencesCat);

    return true;
}


bool ppImageDetrendFringeGenerate(pmCell *science, pmCell *fringes, const ppImageOptions *options)
{
    PS_ASSERT_PTR_NON_NULL(science, false);
    PS_ASSERT_PTR_NON_NULL(fringes, false);

    pmFringeScale *solution = psMetadataLookupPtr(NULL, science->parent->analysis, "FRINGE.SOLUTION");
    if (!solution) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find fringe solution.\n");
        return false;
    }
    assert(fringes->readouts->n == solution->nFringeFrames);

    psImageMaskType maskVal = options->flatMask;

    bool mdok;                          // Status of MD lookup
    float expTime = psMetadataLookupF32(&mdok, science->concepts, "CELL.EXPOSURE"); // Exp. time
    if (!mdok || !isfinite(expTime)) {
        psError(PS_ERR_UNKNOWN, true, "CELL.EXPOSURE is not set --- can't renormalise fringes\n");
        return false;
    }
    if (expTime == 0) {
        psWarning("Exposure time is zero --- are you sure you want fringe subtraction?\n");
        expTime = 1.0;
    }

    pmHDU *hdu = pmHDUFromCell(science);// HDU  of interest

    // Construct the fringe image from the scale
    psTrace("ppImage", 3, "Generating fringe correction...\n");
    psImage *sumFringe = NULL; // Sum of the fringes
    for (int i = 0; i < solution->nFringeFrames; i++) {
        pmReadout *fringeRO = fringes->readouts->data[i]; // Fringe readout
        psImage *fringe = fringeRO->image; // Fringe image
        psTrace("ppImage", 5, "Scale for fringe component %d is %f\n",
                i, solution->coeff->data.F32[i + 1]);
        psBinaryOp(fringe, fringe, "*", psScalarAlloc(solution->coeff->data.F32[i + 1], PS_TYPE_F32));
        if (!sumFringe) {
            sumFringe = psImageCopy(NULL, fringe, PS_TYPE_F32);
        } else {
            psBinaryOp(sumFringe, sumFringe, "+", fringe);
        }

        psVector *md5 = psImageMD5(fringe); // md5 hash
        psString md5string = psMD5toString(md5); // String
        psFree(md5);
        psStringPrepend(&md5string, "Fringe image %d (scale %.3f) MD5: ",
                        i, solution->coeff->data.F32[i + 1]);
        psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK,
                         md5string, "");
        psFree(md5string);

# if (0)
        // write metadata header value
        // XXX this is measured per cell, but we only have headers per chip
        psString keyword = NULL;
        psStringAppend (&keyword, "FRNG_%02dV", i);
        psMetadataAddF32(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, "Fringe Amplitude", solution->coeff->data.F32[i + 1]);
        psFree (keyword);

        keyword = NULL;
        psStringAppend (&keyword, "FRNG_%02dE", i);
        psMetadataAddF32(hdu->header, PS_LIST_TAIL, keyword, PS_META_REPLACE, "Fringe Amplitude error", solution->coeffErr->data.F32[i + 1]);
        psFree (keyword);
# endif

    }
    if (expTime != 1.0) {
        psBinaryOp(sumFringe, sumFringe, "*", psScalarAlloc(expTime, PS_TYPE_F32));
    }

    // Apply the correction to all readouts
    psArray *readouts = science->readouts; // Array of science readouts
    for (int i = 0; i < readouts->n; i++) {
        pmReadout *readout = readouts->data[i]; // Readout of interest
        if (!readout || !readout->data_exists) {
            continue;
        }

        // subtract fringe and update mask if fringe value is NAN
        for (int iy = 0; iy < readout->image->numRows; iy++) {
            for (int ix = 0; ix < readout->image->numCols; ix++) {
                readout->image->data.F32[iy][ix] -= sumFringe->data.F32[iy][ix];
                if (!isfinite(sumFringe->data.F32[iy][ix]) && readout->mask) {
                    readout->mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= maskVal;
                }
            }
        }

        // XXX: Make generic, so subregions may be subtracted as well
        // if (!psBinaryOp(readout->image, readout->image, "-", sumFringe)) {
        //     psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to subtract fringe.\n");
        //     return false;
        // }

        // measure residual fringe amplitude. results go to FRINGE.RESIDUALS
        ppImageDetrendFringeMeasure (readout, fringes, true, options);
    }
    psFree(sumFringe);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now, used for reporting
    psString timeString = psTimeToISO(time); // String with time
    psFree(time);
    psStringPrepend(&timeString, "Fringe correction completed at ");
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK,
                     timeString, "");
    psFree(timeString);

    return true;
}

bool ppImageDetrendFringeApply (pmConfig *config, pmChip *chip, const pmFPAview *inputView, const ppImageOptions *options) {

    pmCell *cell = NULL;

    if (!options->doFringe) return true;

    assert (inputView->chip != -1);
    assert (inputView->cell == -1);
    assert (inputView->readout == -1);

    pmFPAview *view = pmFPAviewAlloc(0); // View for local processing
    *view = *inputView;

    // select the reference chip
    pmChip *fringe = pmFPAfileThisChip(config->files, view, "PPIMAGE.FRINGE");
    if (!fringe) {
        psError(PS_ERR_UNKNOWN, false, "missing fringe reference data.\n");
        psFree (view);
        return false;
    }

    // Solve the fringe system
    if (!ppImageDetrendFringeSolve(chip, fringe, false, options)) {
        psError(PS_ERR_UNKNOWN, false, "failed to solve the fringe system.\n");
        psFree (view);
        return false;
    }

    // Go back over the cells to apply the fringe correction
    view->cell = view->readout = -1;
    while ((cell = pmFPAviewNextCell(view, chip->parent, 1)) != NULL) {
        if (!cell->process || !cell->file_exists) {
            continue;
        }

        // Apply the fringe correction
        psTrace("ppImage", 3, "Applying fringe correction...\n");
        pmCell *fringeCell = pmFPAfileThisCell(config->files, view, "PPIMAGE.FRINGE");
        if (!fringeCell) {
            psError(PS_ERR_UNKNOWN, false, "missing fringe reference data.\n");
            psFree (view);
            return false;
        }

        // XXX for now, skip the video cells (cell->readouts->n > 1)
        if (cell->readouts->n > 1) {
          psWarning ("Skipping Video Cell for ppImageDetrendFringeApply");
          continue;
        }

        if (!ppImageDetrendFringeGenerate(cell, fringeCell, options)) {
            psError(PS_ERR_UNKNOWN, false, "failed to apply fringe image.\n");
            psFree (view);
            return false;
        }
    }

    // Solve the residual fringe system
    if (!ppImageDetrendFringeSolve(chip, fringe, true, options)) {
        psError(PS_ERR_UNKNOWN, false, "failed to solve the residual fringe system.\n");
        psFree (view);
        return false;
    }

    // psLogMsg ("ppImage", 5, "apply fringe: %f sec\n", psTimerMark ("apply.fringe"));

    psFree (view);
    return true;
}
