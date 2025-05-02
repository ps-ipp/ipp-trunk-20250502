#include <stdio.h>
#include <pslib.h>
#include <string.h>

#include "pmErrorCodes.h"
#include "pmHDU.h"
#include "pmHDUUtils.h"
#include "pmFPA.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmFPAfileFitsIO.h"
#include "pmConceptsRead.h"

#include "pmSubtractionTypes.h"
#include "pmSubtraction.h"
#include "pmSubtractionKernels.h"
#include "pmSubtractionMatch.h"
#include "pmSubtractionAnalysis.h"

#include "pmSubtractionIO.h"
#include "pmStackVisual.h"

#define ARRAY_BUFFER 16                 // Number to add to array at a time

// Names of FITS table columns
#define NAME_XMIN "XMIN"                // Region of applicability: minimum x value
#define NAME_XMAX "XMAX"                // Region of applicability: maximum x value
#define NAME_YMIN "YMIN"                // Region of applicability: minimum y value
#define NAME_YMAX "YMAX"                // Region of applicability: maximum y value
#define NAME_KERNEL "KERNEL"            // Kernel description
#define NAME_TYPE "TYPE"                // Kernel type
#define NAME_SIZE "SIZE"                // Kernel half-size
#define NAME_INNER "INNER"              // Size of inner region (only applicable for some kernel types)
#define NAME_SPATIAL "SPATIAL_ORDER"    // Order of spatial polynomial
#define NAME_BG "BG_ORDER"              // Order of background polynomial
#define NAME_MODE "MODE"                // Matching mode
#define NAME_COLS "COLUMNS"             // Number of columns
#define NAME_ROWS "ROWS"                // Number of rows
#define NAME_SOL1 "SOLUTION_1"          // Solution for convolving image 1
#define NAME_SOL2 "SOLUTION_2"          // Solution for convolving image 2
#define NAME_MEAN "MEAN"                // Mean of chi^2 from stamps
#define NAME_RMS  "RMS"                 // RMS of chi^2 from stamps
#define NAME_NUMSTAMPS "NUMSTAMPS"      // Number of good stamps

#define EXTNAME_KERNEL "SUBTRACTION_KERNEL"    // Extension name for kernel
#define EXTNAME_IMAGE  "SUBTRACTION_KERNEL_IMAGE"    // Extension name for image

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmReadoutWriteSubtractionKernels(pmReadout *ro, psFits *fits)
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    // Extract the regions and solutions used in the image matching
    psArray *regions = psArrayAllocEmpty(ARRAY_BUFFER); // Array of regions
    {
        psString regex = NULL;          // Regular expression
        psStringAppend(&regex, "^%s$", PM_SUBTRACTION_ANALYSIS_REGION);
        psMetadataIterator *iter = psMetadataIteratorAlloc(ro->analysis, PS_LIST_HEAD, regex); // Iterator
        psFree(regex);
        psMetadataItem *item = NULL;// Item from iteration
        while ((item = psMetadataGetAndIncrement(iter))) {
            assert(item->type == PS_DATA_REGION);
            regions = psArrayAdd(regions, ARRAY_BUFFER, item->data.V);
        }
        psFree(iter);
    }
    if (regions->n == 0) {
        // We wrote everything we could find
        psFree(regions);
        return true;
    }

    psArray *kernels = psArrayAllocEmpty(ARRAY_BUFFER); // Array of kernels
    {
        psString regex = NULL;          // Regular expression
        psStringAppend(&regex, "^%s$", PM_SUBTRACTION_ANALYSIS_KERNEL);
        psMetadataIterator *iter = psMetadataIteratorAlloc(ro->analysis, PS_LIST_HEAD, regex); // Iterator
        psFree(regex);
        psMetadataItem *item = NULL;// Item from iteration
        while ((item = psMetadataGetAndIncrement(iter))) {
            assert(item->type == PS_DATA_UNKNOWN);
            pmSubtractionKernels *kernel = item->data.V; // Kernel used in subtraction
            kernels = psArrayAdd(kernels, ARRAY_BUFFER, kernel);
        }
        psFree(iter);
    }

    if (regions->n != kernels->n) {
        psError(PM_ERR_PROG, true, "Number of regions (%ld) and kernels (%ld) don't match.\n",
                regions->n, kernels->n);
        psFree(regions);
        psFree(kernels);
        return false;
    }

    // Format for writing a table
    int num = regions->n;              // Number of regions and kernels
    psArray *rows = psArrayAlloc(num); // Array of FITS table rows
    for (int i = 0; i < num; i++) {
        psMetadata *row = rows->data[i] = psMetadataAlloc(); // Row of interest

        psRegion *region = regions->data[i]; // Region of interest
        psMetadataAddS32(row, PS_LIST_TAIL, NAME_XMIN,  0, "Applicability minimum x", region->x0);
        psMetadataAddS32(row, PS_LIST_TAIL, NAME_XMAX,  0, "Applicability maximum x", region->x1);
        psMetadataAddS32(row, PS_LIST_TAIL, NAME_YMIN,  0, "Applicability minimum y", region->y0);
        psMetadataAddS32(row, PS_LIST_TAIL, NAME_YMAX,  0, "Applicability maximum y", region->y1);

        pmSubtractionKernels *kernel = kernels->data[i]; // Kernel
        psMetadataAddStr(row, PS_LIST_TAIL, NAME_KERNEL, 0, "Kernel description", kernel->description);

#if 0
        psMetadataAddS32(row, PS_LIST_TAIL, NAME_TYPE,  0, "Kernel type (enum)", kernel->type);
        psMetadataAddS32(row, PS_LIST_TAIL, NAME_SIZE,  0, "Kernel half-size", kernel->size);
        psMetadataAddS32(row, PS_LIST_TAIL, NAME_INNER, 0, "Size of inner region", kernel->inner);
        psMetadataAddS32(row, PS_LIST_TAIL, NAME_SPATIAL, 0, "Polynomial order for spatial variations",
                         kernel->spatialOrder);
#endif

        psMetadataAddS32(row, PS_LIST_TAIL, NAME_BG,  0, "Polynomial order for background fitting",
                         kernel->bgOrder);
        psMetadataAddS32(row, PS_LIST_TAIL, NAME_MODE,  0, "Matching mode (enum)", kernel->mode);
        if (kernel->mode == PM_SUBTRACTION_MODE_1 || kernel->mode == PM_SUBTRACTION_MODE_2) {
            psMetadataAddVector(row, PS_LIST_TAIL, NAME_SOL1, 0, "Solution vector 1", kernel->solution1);
        }
        if (kernel->mode == PM_SUBTRACTION_MODE_DUAL) {
            psMetadataAddVector(row, PS_LIST_TAIL, NAME_SOL1, 0, "Solution vector 1", kernel->solution1);
            psMetadataAddVector(row, PS_LIST_TAIL, NAME_SOL2, 0, "Solution vector 2", kernel->solution2);
        }
        psMetadataAddF32(row, PS_LIST_TAIL, NAME_MEAN,  0, "Mean of chi^2 from stamps", kernel->mean);
        psMetadataAddF32(row, PS_LIST_TAIL, NAME_RMS,  0, "RMS of chi^2 from stamps", kernel->rms);
        psMetadataAddS32(row, PS_LIST_TAIL, NAME_NUMSTAMPS,  0, "Number of good stamps", kernel->numStamps);
    }
    psFree(regions);
    psFree(kernels);

    psMetadata *header = psMetadataAlloc(); // Header for FITS file

    pmCell *cell = ro->parent;          // Cell of interest
    if (cell) {
        pmChip *chip = cell->parent;    // Chip of interest
        pmFPA *fpa = chip->parent;      // FPA of interest
        pmHDU *hdu = pmHDUGetHighest(fpa, chip, cell); // HDU for readout
        if (hdu) {
            header = psMetadataCopy(header, hdu->header);
        }
    }

    // CVS tags, used to identify the version of this file (in case incompatibilities are introduced)
    psString cvsFile = psStringCopy("$RCSfile: pmSubtractionIO.c,v $");
    psString cvsRev  = psStringCopy("$Revision: 1.9.18.1 $");
    psString cvsDate = psStringCopy("$Date: 2009-02-19 17:59:50 $");
    psStringSubstitute(&cvsFile, NULL, "RCSfile: ");
    psStringSubstitute(&cvsRev,  NULL, "Revision: ");
    psStringSubstitute(&cvsDate, NULL, "Date: ");

    psString version = NULL;            // Version information, for header
    psStringAppend(&version, "%s %s %s", cvsFile, cvsRev, cvsDate);
    psFree(cvsFile);
    psFree(cvsRev);
    psFree(cvsDate);
    psStringSubstitute(&version, NULL, "$");
    psMetadataAddStr(header, PS_LIST_TAIL, "PSVERSION", 0, "S/W version", version);
    psFree(version);

    if (!psFitsWriteTable(fits, header, rows, EXTNAME_KERNEL)) {
        psError(psErrorCodeLast(), false, "Unable to write subtraction kernel to FITS table.");
        psFree(header);
        psFree(rows);
        return false;
    }

    psImage *image = psMetadataLookupPtr(NULL, ro->analysis, PM_SUBTRACTION_ANALYSIS_KERNEL_IMAGE); // Image
    pmStackVisualPlotTestImage(image, "Subtraction_kernels.fits");

    if (image && !psFitsWriteImage(fits, header, image, 0, EXTNAME_IMAGE)) {
        psError(psErrorCodeLast(), false, "Unable to write subtraction kernel image.");
        psFree(header);
        psFree(rows);
        return false;
    }

    psFree(header);
    psFree(rows);

    return true;
}

static bool pmCellWriteSubtractionKernels(pmCell *cell, const pmFPAview *view,
                                          pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(cell->readouts, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    pmFPAview *thisView = pmFPAviewAlloc(view->nRows); // Copy of input view
    *thisView = *view;

    for (int i = 0; i < cell->readouts->n; i++) {
        pmReadout *readout = cell->readouts->data[i];
        thisView->readout = i;
        if (!pmReadoutWriteSubtractionKernels(readout, file->fits)) {
            psError(psErrorCodeLast(), false, "Failed to write %dth readout", i);
            psFree(thisView);
            return false;
        }
    }
    psFree(thisView);
    return true;
}

static bool pmChipWriteSubtractionKernels(pmChip *chip, const pmFPAview *view,
                                          pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(chip->cells, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    pmFPAview *thisView = pmFPAviewAlloc(view->nRows); // Copy of input view
    *thisView = *view;

    for (int i = 0; i < chip->cells->n; i++) {
        pmCell *cell = chip->cells->data[i];
        thisView->cell = i;
        if (!pmCellWriteSubtractionKernels(cell, thisView, file, config)) {
            psError(psErrorCodeLast(), false, "Failed to write %dth cell", i);
            psFree(thisView);
            return false;
        }
    }
    psFree(thisView);
    return true;
}

static bool pmFPAWriteSubtractionKernels(pmFPA *fpa, const pmFPAview *view,
                                         pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(fpa->chips, false);

    pmFPAview *thisView = pmFPAviewAlloc(view->nRows); // Copy of input view
    *thisView = *view;

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        thisView->chip = i;
        if (!pmChipWriteSubtractionKernels(chip, thisView, file, config)) {
            psError(psErrorCodeLast(), false, "Failed to write %dth chip", i);
            psFree(thisView);
            return false;
        }
    }
    psFree(thisView);
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool pmReadoutReadSubtractionKernels(pmReadout *ro, psFits *fits)
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    if (!psFitsMoveExtName(fits, EXTNAME_KERNEL)) {
        psError(psErrorCodeLast(), false, "Unable to move to subtraction kernel table.");
        return false;
    }

    psArray *table = psFitsReadTable(fits); // Table of interest
    if (!table) {
        psError(psErrorCodeLast(), false, "Unable to read FITS table");
        return false;
    }

    // Look up a column value for a row
#define TABLE_LOOKUP(TYPE, SUFFIX, TARGET, NAME) \
    TYPE TARGET; \
    { \
        bool mdok; \
        TARGET = psMetadataLookup##SUFFIX(&mdok, row, NAME); \
        if (!mdok) { \
            psError(PM_ERR_PROG, false, "Unable to find column %s in subtraction kernel table.", NAME); \
            psFree(table); \
            return false; \
        } \
    }

    for (int i = 0; i < table->n; i++) {
        psMetadata *row = table->data[i]; // Table row

        TABLE_LOOKUP(int, S32, xMin, NAME_XMIN);
        TABLE_LOOKUP(int, S32, xMax, NAME_XMAX);
        TABLE_LOOKUP(int, S32, yMin, NAME_YMIN);
        TABLE_LOOKUP(int, S32, yMax, NAME_YMAX);

        psRegion *region = psRegionAlloc(xMin, xMax, yMin, yMax); // Region of applicability
        psMetadataAddPtr(ro->analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_REGION, PS_DATA_REGION,
                         "Subtraction region", region);
        psFree(region);

        TABLE_LOOKUP(const char *, Str, description, NAME_KERNEL);
        TABLE_LOOKUP(pmSubtractionMode, S32, mode,    NAME_MODE);

#if 0
        TABLE_LOOKUP(int, S32, size,    NAME_SIZE);
        TABLE_LOOKUP(int, S32, inner,   NAME_INNER);
        TABLE_LOOKUP(int, S32, spatial, NAME_SPATIAL);
#endif

        TABLE_LOOKUP(int, S32, bg,      NAME_BG);

        TABLE_LOOKUP(float, F32, mean,      NAME_MEAN);
        TABLE_LOOKUP(float, F32, rms,       NAME_RMS);
        TABLE_LOOKUP(int,   S32, numStamps, NAME_NUMSTAMPS);

        pmSubtractionKernels *kernels = pmSubtractionKernelsFromDescription(description, bg, *region, mode);
        kernels->mean = mean;
        kernels->rms = rms;
        kernels->numStamps = numStamps;

        bool mdok;                      // Status of MD lookup
        if (mode == PM_SUBTRACTION_MODE_1 || mode == PM_SUBTRACTION_MODE_2) {
            kernels->solution1 = psMemIncrRefCounter(psMetadataLookupPtr(&mdok, row, NAME_SOL1));
            if (!mdok) {
                psError(PM_ERR_PROG, false, "Unable to find column %s in subtraction kernel table.",
                        NAME_SOL1);
                psFree(kernels);
                psFree(table);
                return false;
            }
        }
        if (mode == PM_SUBTRACTION_MODE_DUAL) {
            kernels->solution1 = psMemIncrRefCounter(psMetadataLookupPtr(&mdok, row, NAME_SOL1));
            if (!mdok) {
                psError(PM_ERR_PROG, false, "Unable to find column %s in subtraction kernel table.",
                        NAME_SOL1);
                psFree(kernels);
                psFree(table);
                return false;
            }
            kernels->solution2 = psMemIncrRefCounter(psMetadataLookupPtr(&mdok, row, NAME_SOL2));
            if (!mdok) {
                psError(PM_ERR_PROG, false, "Unable to find column %s in subtraction kernel table.",
                        NAME_SOL2);
                psFree(kernels);
                psFree(table);
                return false;
            }
        }

        psMetadataAddPtr(ro->analysis, PS_LIST_TAIL, PM_SUBTRACTION_ANALYSIS_KERNEL, PS_DATA_UNKNOWN,
                         "Subtraction kernels", kernels);
        psFree(kernels);
    }
    psFree(table);
    return true;
}

static bool pmCellReadSubtractionKernels(pmCell *cell, const pmFPAview *view,
                                         pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(cell->readouts, false);

    pmFPAview *thisView = pmFPAviewAlloc(view->nRows); // Copy of input view
    *thisView = *view;

    // Create a readout if none exists
    if (!cell->readouts || cell->readouts->n == 0) {
        pmReadout *readout = pmReadoutAlloc(cell); // New readout
        psFree(readout);                // Drop reference
    }

    cell->data_exists = false;
    for (int i = 0; i < cell->readouts->n; i++) {
        pmReadout *readout = cell->readouts->data[i];
        thisView->readout = i;
        if (!pmReadoutReadSubtractionKernels(readout, file->fits)) {
            psError(psErrorCodeLast(), false, "Unable to read subtraction kernels from cell");
            psFree(thisView);
            return false;
        }
        if (!readout->data_exists) {
            continue;
        }

        // load in the concept information for this cell
        if (!pmConceptsReadCell(cell, PM_CONCEPT_SOURCE_HEADER, true, NULL)) {
            psErrorClear();
            psWarning("Difficulty reading concepts for cell; attempting to proceed.");
        }
        cell->data_exists = true;
    }
    psFree(thisView);

    return true;
}

static bool pmChipReadSubtractionKernels(pmChip *chip, const pmFPAview *view,
                                         pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(chip->cells, false);

    pmFPAview *thisView = pmFPAviewAlloc(view->nRows); // Copy of input view
    *thisView = *view;

    chip->data_exists = false;
    for (int i = 0; i < chip->cells->n; i++) {
        pmCell *cell = chip->cells->data[i];
        thisView->cell = i;
        if (!pmCellReadSubtractionKernels(cell, thisView, file, config)) {
            psError(psErrorCodeLast(), false, "Unable to read subtraction kernels from cell");
            psFree(thisView);
            return false;
        }
         if (!cell->data_exists) {
            continue;
        }
        chip->data_exists = true;
    }
    psFree(thisView);

    if (!pmConceptsReadChip(chip, PM_CONCEPT_SOURCE_HEADER, true, true, NULL)) {
        psError(psErrorCodeLast(), false, "Failed to read concepts for chip.\n");
        return false;
    }

    return true;
}

static bool pmFPAReadSubtractionKernels(pmFPA *fpa, const pmFPAview *view,
                                        pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa->chips, false);

    pmFPAview *thisView = pmFPAviewAlloc(view->nRows); // Copy of input view
    *thisView = *view;

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        thisView->chip = i;
        if (!pmChipReadSubtractionKernels(chip, thisView, file, config)) {
            psError(psErrorCodeLast(), false, "Unable to read subtraction kernels from chip");
            psFree(thisView);
            return false;
        }
    }
    psFree(thisView);

    if (!pmConceptsReadFPA(fpa, PM_CONCEPT_SOURCE_HEADER, true, NULL)) {
        psError(psErrorCodeLast(), false, "Failed to read concepts for fpa.\n");
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmSubtractionWriteKernels(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // Suitable FPA for writing

    if (view->chip == -1) {
        if (!pmFPAWriteSubtractionKernels(fpa, view, file, config)) {
            psError(psErrorCodeLast(), false, "Failed to write subtraction kernels from fpa");
            psFree(fpa);
            return false;
        }
        psFree(fpa);
        return true;
    }

    if (view->chip >= fpa->chips->n) {
        psError(PM_ERR_PROG, true, "Writing chip == %d (>= chips->n == %ld)", view->chip, fpa->chips->n);
        psFree(fpa);
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        if (!pmChipWriteSubtractionKernels(chip, view, file, config)) {
            psError(psErrorCodeLast(), false, "Failed to write objects from chip");
            psFree(fpa);
            return false;
        }
        psFree(fpa);
        return true;
    }

    if (view->cell >= chip->cells->n) {
        psError(PM_ERR_PROG, true, "Writing cell == %d (>= cells->n == %ld)",
                view->cell, chip->cells->n);
        psFree(fpa);
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        if (!pmCellWriteSubtractionKernels(cell, view, file, config)) {
            psError(psErrorCodeLast(), false, "Failed to write objects from cell");
            psFree(fpa);
            return false;
        }
        psFree(fpa);
        return true;
    }

    if (view->readout >= cell->readouts->n) {
        psError(PM_ERR_PROG, true, "Writing readout == %d (>= readouts->n == %ld)",
                view->readout, cell->readouts->n);
        psFree(fpa);
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    if (!pmReadoutWriteSubtractionKernels(readout, file->fits)) {
        psError(psErrorCodeLast(), false, "Failed to write objects from readout %d", view->readout);
        psFree(fpa);
        return false;
    }

    psFree(fpa);
    return true;
}

bool pmSubtractionWritePHU(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    if (file->wrote_phu) {
        return true;
    }

    // find the FPA phu
    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // Suitable FPA for writing
    pmHDU *phu = psMemIncrRefCounter(pmFPAviewThisPHU(view, fpa));
    psFree(fpa);

    // if there is no PHU, this is a single header+image (extension-less) file. This could be the case for an
    // input SPLIT set of files being written out as a MEF.  if there is a PHU, write it out as a 'blank'
    psMetadata *outhead = psMetadataAlloc();
    if (phu) {
        psMetadataCopy (outhead, phu->header);
    }
    psFree(phu);

    pmConfigConformHeader(outhead, file->format);

    psFitsWriteBlank(file->fits, outhead, "");
    file->wrote_phu = true;

    psTrace("pmFPAfile", 5, "wrote phu %s (type: %d)\n", file->filename, file->type);
    psFree(outhead);

    return true;
}

bool pmSubtractionReadKernels(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        pmFPAReadSubtractionKernels(fpa, view, file, config);
        return true;
    }

    if (view->chip >= fpa->chips->n) {
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        pmChipReadSubtractionKernels(chip, view, file, config);
        return true;
    }

    if (view->cell >= chip->cells->n) {
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        pmCellReadSubtractionKernels(cell, view, file, config);
        return true;
    }

    if (view->readout >= cell->readouts->n) {
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    return pmReadoutReadSubtractionKernels(readout, file->fits);
}
