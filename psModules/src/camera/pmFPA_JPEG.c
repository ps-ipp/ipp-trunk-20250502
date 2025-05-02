/** @file  pmFPA_JPEG.c
 *
 * This file contains functions to write JPEG images.
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.27 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*****************************************************************************/
/* INCLUDE FILES                                                             */
/*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <strings.h>            /* for strn?casecmp */
#include <pslib.h>

#include "pmConfig.h"
#include "pmDetrendDB.h"

#include "pmConfigMask.h"
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmFPA_JPEG.h"

bool pmFPAviewWriteJPEG(const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        pmFPAWriteJPEG (fpa, view, file, config);
        return true;
    }

    if (view->chip >= fpa->chips->n) {
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        pmChipWriteJPEG (chip, view, file, config);
        return true;
    }

    if (view->cell >= chip->cells->n) {
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        pmCellWriteJPEG (cell, view, file, config);
        return true;
    }

    if (view->readout >= cell->readouts->n) {
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    pmReadoutWriteJPEG (readout, view, file, config);
    return true;
}

// read in all chip-level JPEG files for this FPA
bool pmFPAWriteJPEG (pmFPA *fpa, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    for (int i = 0; i < fpa->chips->n; i++) {

        pmChip *chip = fpa->chips->data[i];
        pmChipWriteJPEG (chip, view, file, config);
    }
    return true;
}

// read in all cell-level JPEG files for this chip
bool pmChipWriteJPEG (pmChip *chip, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    for (int i = 0; i < chip->cells->n; i++) {

        pmCell *cell = chip->cells->data[i];
        pmCellWriteJPEG (cell, view, file, config);
    }
    return true;
}

// read in all readout-level JPEG files for this cell
bool pmCellWriteJPEG (pmCell *cell, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    for (int i = 0; i < cell->readouts->n; i++) {

        pmReadout *readout = cell->readouts->data[i];
        pmReadoutWriteJPEG (readout, view, file, config);
    }
    return true;
}

// write JPEG image for readout
// note that this function will try as hard a possible to write out a jpeg.  it will not fail
// just because the values are in a poor range.  it is more convenient to know you are getting
// a jpeg which is weird than to fail to get the file at all.
bool pmReadoutWriteJPEG (pmReadout *readout, const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
{
    bool status;
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    if (file->type != PM_FPA_FILE_JPEG) {
        psError(PS_ERR_UNKNOWN, true, "File is not of type JPEG");
        return false;
    }

    psTrace("psModules.camera", 3, "writing jpeg for readout %p\n", readout);

    psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, "JPEG");
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find JPEG recipe");
        return false;
    }

    psMetadata *options = psMetadataLookupMetadata(&status, recipe, file->name);
    if (!status) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find options for file %s in JPEG recipe", file->name);
        return false;
    }

    // measure the image statistics for scaling
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN);
    stats->nSubsample = 10000;
    psImageMaskType maskVal = pmConfigMaskGet("MASK.VALUE", config); // Value to mask
    float mean = 0, delta = 0;          //
    if (!psImageBackground(stats, NULL, readout->image, readout->mask, maskVal, rng)) {
        psStats *statsAlt = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
        stats->nSubsample = 10000;
        if (!psImageBackground(stats, NULL, readout->image, readout->mask, maskVal, rng)) {
            psLogMsg("psModules.jpeg", PS_LOG_WARN,
                     "Unable to measure statistics for image; writing blank jpeg");
            mean = 0;
            delta = 1;
        } else {
            mean = statsAlt->sampleMean;
            delta = 2.0 * statsAlt->sampleStdev;
        }
        psFree(statsAlt);
    } else {
        mean = stats->robustMedian;
        delta = stats->robustUQ - stats->robustLQ;
    }
    psFree(rng);
    psFree(stats);

    // default options are: no flip in X or Y, scale bar on bottom
    psImageJpegOptions *jpegOptions = psImageJpegOptionsAlloc();

    char *colormapName = psMetadataLookupStr(NULL, options, "COLORMAP"); // Name of colour map
    if (!colormapName) {
        colormapName = "-greyscale";
    }
    psImageJpegColormapSet(jpegOptions, colormapName);

    // set up the scale options
    char *mode = psMetadataLookupStr(NULL, options, "SCALE.MODE"); // Mode for scaling image
    if (!mode) {
        mode = "RANGE";
    }
    float fmin = psMetadataLookupF32(&status, options, "SCALE.MIN"); // Minimum value/faction for scaling
    if (!status) {
        fmin = -3.0;
    }
    float fmax = psMetadataLookupF32(&status, options, "SCALE.MAX"); // Maximum value/faction for scaling
    if (!status) {
        fmax = +6.0;
    }
    if (!strcasecmp(mode, "RANGE")) {
        jpegOptions->min = mean + fmin*delta;
        jpegOptions->max = mean + fmax*delta;
    } else if (!strcasecmp(mode, "FRACTION")) {
        jpegOptions->min = fmin*mean;
        jpegOptions->max = fmax*mean;
    } else if (!strcasecmp(mode, "VALUE")) {
	jpegOptions->min = fmin;
        jpegOptions->max = fmax;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unrecognised scaling mode: %s", mode);
        return false;
    }
    if (!isfinite(jpegOptions->min) || !isfinite(jpegOptions->max)) {
        psLogMsg("psModules.jpeg", PS_LOG_WARN,
                 "The stretch parameters are not both finite --- writing blank jpeg");
        jpegOptions->min = 0;
        jpegOptions->max = 1;
    }

    jpegOptions->showScale = PS_JPEG_SHOWSCALE_NONE;
    jpegOptions->xFlip = false;
    jpegOptions->yFlip = false;
    
    char *userOptions = psMetadataLookupStr(&status, options, "OPTIONS"); // Mode for scaling image
    if (userOptions) {
	// just use strstr : strcasestr is non-standard an not always available.  replace with our own?
	if (strstr(userOptions, "+SB")) {
	    jpegOptions->showScale = PS_JPEG_SHOWSCALE_BOTTOM;
	} 
	if (strstr(userOptions, "-X")) {
	    jpegOptions->xFlip = true;
	} 
	if (strstr(userOptions, "-Y")) {
	    jpegOptions->yFlip = true;
	} 
	// lowercase versions 
	if (strstr(userOptions, "+sb")) {
	    jpegOptions->showScale = PS_JPEG_SHOWSCALE_BOTTOM;
	} 
	if (strstr(userOptions, "-x")) {
	    jpegOptions->xFlip = true;
	} 
	if (strstr(userOptions, "-y")) {
	    jpegOptions->yFlip = true;
	} 
    }

    // NOTE: we can overlay the location of the stars from the readout by passing a bDrawBuffer to this function

    if (!psImageJpeg(jpegOptions, readout->image, NULL, file->filename)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to write JPEG image");
        psFree(jpegOptions);
        return false;
    }

    psFree(jpegOptions);
    return true;
}
