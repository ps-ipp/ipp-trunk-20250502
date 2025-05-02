#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>            /* for strn?casecmp */
#include <pslib.h>

#include "pmConfig.h"
#include "pmConfigMask.h"
#include "pmDetrendDB.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAMaskWeight.h"
#include "pmFPAview.h"
#include "pmFPAFlags.h"
#include "pmFPAfile.h"
#include "pmFPACopy.h"
#include "pmFPARead.h"
#include "pmFPAWrite.h"
#include "pmFPAfileFitsIO.h"
#include "pmFPAfileFringeIO.h"

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
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmPSFtry.h"

#include "pmSourceIO.h"
#include "pmPSF_IO.h"

#include "pmKHcorrect.h"
#include "pmAstrometryModel.h"
#include "pmAstrometryRefstars.h"
#include "pmFPA_JPEG.h"
#include "pmSourcePlots.h"
#include "pmFPAConstruct.h"
#include "pmSubtractionIO.h"
#include "pmPatternIO.h"
#include "pmFPAExpNumIO.h"
#include "pmConcepts.h"
#include "pmConfigRun.h"

#include "pmFPAfileIO.h"

void CatchTestFile (pmFPAfile *file, const char *function) {

  int testFile = FALSE;							
  testFile = testFile || !strcmp(file->name, "GDIFF.OUTPUT.SOURCES");	
  testFile = testFile || !strcmp(file->name, "GDIFF.POS1.SOURCES");	
  testFile = testFile || !strcmp(file->name, "GDIFF.POS2.SOURCES");	
  if (FALSE && testFile) {							
    fprintf (stderr, "%s : %d : %d (%s)", file->name, file->state, file->mode, function);
    fprintf (stderr, "\n");
  }
}

// attempt create, read, write, close, or free pmFPAfiles available in files files are
// automatically opened before they are read.  In the case of MEF files, the PHU is
// read when the file is opened and written before the first extension is written.
bool pmFPAfileIOChecks (pmConfig *config, const pmFPAview *view, pmFPAfilePlace place)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(config->files, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    psMetadata *files = config->files;

    // attempt to perform all create, read, write, close operations
    psMetadataItem *item = NULL;
    psMetadataIterator *iter = psMetadataIteratorAlloc (files, PS_LIST_HEAD, NULL);
    while ((item = psMetadataGetAndIncrement (iter)) != NULL) {
        pmFPAfile *file = item->data.V;
        switch (place) {
          case PM_FPA_BEFORE:
            if (!pmFPAfileRead (file, view, config)) {
                psError(PS_ERR_IO, false, "failed READ in FPA_BEFORE block for %s", file->name);
                goto failure;
            }
            if (!pmFPAfileCreate(file, view, config)) {
                psError(PS_ERR_IO, false, "failed CREATE in FPA_BEFORE block for %s", file->name);
                goto failure;
            }
            break;
          case PM_FPA_AFTER:
            if (!pmFPAfileWrite (file, view, config)) {
                psError(PS_ERR_IO, false, "failed WRITE in FPA_AFTER block for %s", file->name);
                goto failure;
            }
            if (!pmFPAfileClose(file, view)) {
                psError(PS_ERR_IO, false, "failed CLOSE in FPA_AFTER block for %s", file->name);
                goto failure;
            }
            break;
          default:
            psAbort("You can't get here");
        }
    }

    // attempt to free data that is no longer needed
    psMetadataIteratorSet (iter, PS_LIST_HEAD);
    while ((item = psMetadataGetAndIncrement (iter)) != NULL) {
        pmFPAfile *file = item->data.V;

        switch (place) {
        case PM_FPA_BEFORE:
            break;
        case PM_FPA_AFTER:
            if (!pmFPAfileFreeData(file, view)) {
                if (!psMetadataRemoveKey(files, file->name)) {
                    psError(PS_ERR_IO, false, "failed to remove %s in FPA_AFTER block", file->name);
                    goto failure;
                }
            }
            break;
        default:
            psAbort("You can't get here");
        }
    }
    psFree (iter);
    return true;

failure:
    psFree (iter);
    return false;
}

// list all defined pmFPAfiles
bool pmFPAfileIOList (pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);
    PS_ASSERT_PTR_NON_NULL(config->files, false);

    psMetadata *files = config->files;

    // attempt to perform all create, read, write, close operations
    psMetadataItem *item = NULL;
    psMetadataIterator *iter = psMetadataIteratorAlloc (files, PS_LIST_HEAD, NULL);
    while ((item = psMetadataGetAndIncrement (iter)) != NULL) {
        pmFPAfile *file = item->data.V;
	
	fprintf (stderr, "%s : %d %d %d : %d %d %d %d\n", file->name, file->type, file->mode, file->state,
		 file->fileLevel, file->dataLevel, file->freeLevel, file->mosaicLevel);
    }
    psFree (iter);
    return true;
}

// read the file, if necessary and possible
bool pmFPAfileRead(pmFPAfile *file, const pmFPAview *view, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    // skip the following states
    if (file->state & PM_FPA_STATE_INACTIVE) {
        psTrace("psModules.camera", 6, "skip read for %s, file is inactive", file->name);
        return true;
    }

    // an active internal file should not be sent here (should not be left on config->files)
    PS_ASSERT(file->mode != PM_FPA_MODE_INTERNAL, false);

    if (file->mode != PM_FPA_MODE_READ) {
        psTrace("psModules.camera", 6, "skip read for %s, mode is not READ", file->name);
        return true;
    }

    // get the current level
    pmFPALevel level = pmFPAviewLevel (view);

    // do we need to read this file? defer until we read the correct level
    if (level != file->dataLevel) {
        psTrace("psModules.camera", 6, "skip reading of %s at this level %s: dataLevel is %s",
                file->name, pmFPALevelToName(level), pmFPALevelToName(file->dataLevel));
        return true;
    }

    // do we need to open this file?
    if (level >= file->fileLevel) {
        // we are allowed to open a file at a level which is not the fileLevel, but we need to
        // supply a view at the fileLevel for the file lookup functions below
        pmFPAview *fileView = pmFPAviewForLevel (file->fileLevel, view);
        if (!pmFPAfileOpen (file, fileView, config)) {
            psError(PS_ERR_IO, false, "failed to open file %s (%s)", file->filename, file->name);
            psFree (fileView);
            return false;
        }
        psFree (fileView);
    }

    // We need to read it --- double-check it's open!
    if (file->state == PM_FPA_STATE_CLOSED) {
        psError(PS_ERR_IO, false, "failed to open file %s when attempting to read", file->name);
        return false;
    }

    if (!pmConfigRunFileAddRead(config, file)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to add file to run-time information");
        return false;
    }

    // select a reading method
    bool status = true;
    switch (file->type) {
      case PM_FPA_FILE_IMAGE:
        status = pmFPAviewReadFitsImage(view, file, config);
        break;
      case PM_FPA_FILE_MASK:
        status = pmFPAviewReadFitsMask(view, file, config);
        break;
      case PM_FPA_FILE_VARIANCE:
        status = pmFPAviewReadFitsVariance(view, file, config);
        break;
      case PM_FPA_FILE_HEADER:
        status = pmFPAviewReadFitsHeaderSet(view, file, config);
        break;
      case PM_FPA_FILE_DARK:
        status = pmFPAviewReadFitsDark(view, file, config);
        break;
      case PM_FPA_FILE_FRINGE:
        status = pmFPAviewReadFitsImage(view, file, config);
        if (status) {
            if (!pmFPAviewReadFringes(view, file)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to read fringe data from %s.\n", file->filename);
                return false;
            }
        }
        break;
      case PM_FPA_FILE_SUBKERNEL:
        status = pmSubtractionReadKernels(view, file, config);
        break;
      case PM_FPA_FILE_PATTERN:
        status = pmPatternRead(view, file, config);
        break;
      case PM_FPA_FILE_PATTERN_ROW_AMP:
        status = pmPatternRowAmpRead(view, file, config);
        break;
      case PM_FPA_FILE_PATTERN_DEAD_CELLS:
        status = pmPatternDeadCellsRead(view, file, config);
        break;
      case PM_FPA_FILE_SX:
      case PM_FPA_FILE_RAW:
      case PM_FPA_FILE_OBJ:
      case PM_FPA_FILE_CMP:
      case PM_FPA_FILE_CMF:
      case PM_FPA_FILE_CFF:
      case PM_FPA_FILE_WCS:
      case PM_FPA_FILE_SRCTEXT:
        status = pmFPAviewReadObjects (view, file, config);
        break;
      case PM_FPA_FILE_PSF:
        status = pmPSFmodelReadForView (view, file, config);
        break;
      case PM_FPA_FILE_ASTROM_MODEL:
        status = pmAstromModelReadForView (view, file, config);
        break;
      case PM_FPA_FILE_KH_CORRECT:
        status = pmKHcorrectReadForView (view, file, config);
        break;
      case PM_FPA_FILE_EXPNUM:
        status = pmExpNumRead(view, file, config);
        break;
      case PM_FPA_FILE_ASTROM_REFSTARS:
      case PM_FPA_FILE_JPEG:
      case PM_FPA_FILE_KAPA:
      case PM_FPA_FILE_LINEARITY:
      case PM_FPA_FILE_NEWNONLIN:
        break;
      default:
        psError(PS_ERR_IO, true, "warning: type mismatch; saw type %d (%s)", file->type, file->name);
        return false;
    }
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "failed to read %s (%s)\n", file->filename, file->name);
        return false;
    }
    psTrace ("psModules.camera", 5, "read %s (%s) (%d:%d:%d)\n", file->filename, file->name, view->chip, view->cell, view->readout);
    return true;
}

// create the data elements (headers, images) appropriate for this view
bool pmFPAfileCreate (pmFPAfile *file, const pmFPAview *view, const pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    // these are not error conditions; these are state tests
    if (file->state & PM_FPA_STATE_INACTIVE) {
        psTrace("psModules.camera", 6, "skip create for inactive file %s", file->name);
        return true;
    }

    // an active internal file should not be returned to here
    PS_ASSERT(file->mode != PM_FPA_MODE_INTERNAL, false);

    if (file->mode != PM_FPA_MODE_WRITE) {
        psTrace("psModules.camera", 6, "skip create for non-write file %s", file->name);
        return true;
    }

    // get the current level
    pmFPALevel level = pmFPAviewLevel (view);

    // don't create the file if the src (FPA) is not defined
    if (file->src == NULL) {
        psTrace("psModules.camera", 6, "skip create for FPA without src FPA for %s", file->name);
        return true;
    }

    // do we need to write this file?
    if (level != file->fileLevel || file->mosaicLevel != PM_FPA_LEVEL_NONE) {
        psTrace("psModules.camera", 6, "skip creation of %s at this level %s: fileLevel is %s",
                file->name, pmFPALevelToName(level), pmFPALevelToName(file->fileLevel));
        return true;
    }

    switch (file->type) {
      case PM_FPA_FILE_IMAGE:
      case PM_FPA_FILE_MASK:
      case PM_FPA_FILE_VARIANCE:
      case PM_FPA_FILE_FRINGE:
      case PM_FPA_FILE_DARK:
      case PM_FPA_FILE_PATTERN:
      case PM_FPA_FILE_EXPNUM:
        {
            // create FPA structure component based on view
            psMetadata *format = file->format; // Camera format configuration
            if (!format) {
                format = config->format;
            }

            pmFPAAddSourceFromView(file->fpa, view, format);
            psTrace ("psModules.camera", 5, "created fpa data elements for %s (%s) (%d:%d:%d)\n",
                     file->name, file->name, view->chip, view->cell, view->readout);
            break;
        }
      case PM_FPA_FILE_HEADER:
        psAbort ("Create not defined for HEADER");
        break;
      case PM_FPA_FILE_SUBKERNEL:
      case PM_FPA_FILE_SX:
      case PM_FPA_FILE_RAW:
      case PM_FPA_FILE_OBJ:
      case PM_FPA_FILE_CMP:
      case PM_FPA_FILE_CMF:
      case PM_FPA_FILE_CFF:
      case PM_FPA_FILE_WCS:
      case PM_FPA_FILE_PSF:
      case PM_FPA_FILE_ASTROM_MODEL:
      case PM_FPA_FILE_ASTROM_REFSTARS:
      case PM_FPA_FILE_KH_CORRECT:
      case PM_FPA_FILE_PATTERN_ROW_AMP:
      case PM_FPA_FILE_PATTERN_DEAD_CELLS:
      case PM_FPA_FILE_JPEG:
      case PM_FPA_FILE_KAPA:
        break;

      default:
        psError(PS_ERR_IO, true, "Unsupported type for %s: %d", file->name, file->type);
        return false;
    }
    return true;
}

bool pmFPAfileWrite(pmFPAfile *file, const pmFPAview *view, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    if (file->state & PM_FPA_STATE_INACTIVE) {
        psTrace("psModules.camera", 6, "skip write for %s, file is inactive", file->name);
        return true;
    }

    // an ACTIVE internal file should not be sent here
    PS_ASSERT(file->mode != PM_FPA_MODE_INTERNAL, false);

    if (file->mode != PM_FPA_MODE_WRITE) {
        psTrace("psModules.camera", 6, "skip write for %s, mode is not WRITE", file->name);
        return true;
    }

    if (!file->save) {
        psTrace("psModules.camera", 6, "skip write for %s, save is FALSE", file->name);
        return true;
    }

    // get the current level
    pmFPALevel level = pmFPAviewLevel (view);

    // the effective dataLevel (for mosaics, we have preserved the original dataLevel)
    pmFPALevel dataLevel = file->dataLevel;
    if (file->mosaicLevel != PM_FPA_LEVEL_NONE && file->mosaicLevel < dataLevel) {
        dataLevel = file->mosaicLevel;
    }

    // do we need to write this file?
    if (level != dataLevel) {
        psTrace("psModules.camera", 6, "skip writing of %s at this level %s: dataLevel is %s",
                file->name, pmFPALevelToName(level), pmFPALevelToName(dataLevel));
        return true;
    }

    // do we have data to write at this level?
    if (!pmFPAviewCheckDataStatus (file->fpa, view)) {
        psTrace("psModules.camera", 6, "skip write for %s, no data for this entry", file->name);
        return true;
    }

    // note that for CMF and PSF, the test above is not sufficient to determine if there
    // is actually any data to write out.  this is because these types of output files
    // have their data stored on the readout->analysis metadata structure of another
    // (existing) fpa
    if (file->type == PM_FPA_FILE_CMF) {
	if (!pmFPAviewCheckDataStatusForSources (view, file)) {
	    psTrace("psModules.camera", 6, "skip write for %s, no data for this entry", file->name);
	    return true;
	}
    }
    if (file->type == PM_FPA_FILE_PSF) {
      if (!pmPSFmodelCheckDataStatusForView (view, file)) {
        psTrace("psModules.camera", 6, "skip write for %s, no data for this entry", file->name);
        return true;
      }
    }
    if (file->type == PM_FPA_FILE_ASTROM_MODEL) {
      if (!pmAstromModelCheckDataStatusForView (view, file)) {
        psTrace("psModules.camera", 6, "skip write for %s, no data for this entry", file->name);
        return true;
      }
    }
    if (file->type == PM_FPA_FILE_ASTROM_REFSTARS) {
      if (!pmAstromRefstarsCheckDataStatusForView (view, file)) {
        psTrace("psModules.camera", 6, "skip write for %s, no data for this entry", file->name);
        return true;
      }
    }
    if (file->type == PM_FPA_FILE_KH_CORRECT) {
      psTrace("psModules.camera", 6, "skip write for %s, no write function defined", file->name);
      return true;
    }
    if (file->type == PM_FPA_FILE_PATTERN_ROW_AMP) {
      psTrace("psModules.camera", 6, "skip write for %s, no write function defined", file->name);
      return true;
    }
    if (file->type == PM_FPA_FILE_PATTERN_DEAD_CELLS) {
      psTrace("psModules.camera", 6, "skip write for %s, no write function defined", file->name);
      return true;
    }

    // open the file if not yet opened
    // XXX do we need to test mosaicLevel?
    if (level >= file->fileLevel) {
        // we are allowed to open a file at a level which is not the fileLevel, but
        // we need to supply view at the fileLevel for the file lookup functions below
        pmFPAview *fileView = pmFPAviewForLevel (file->fileLevel, view);
        if (!pmFPAfileOpen (file, fileView, config)) {
            psError(PS_ERR_IO, false, "failed to open %s (%s)", file->filename, file->name);
            psFree (fileView);
            return false;
        }

        // do we need to write out a PHU?
        if (!pmFPAfileWritePHU(file, fileView, config)) {
            psError(PS_ERR_IO, false, "failed to write phu for %s (%s)", file->filename, file->name);
            return false;
        }

        psFree (fileView);
    }

    if (file->compression) {
        psTrace("psModules.camera", 7, "Setting compression for %s (%s)\n", file->filename, file->name);
        if (!psFitsCompressionApply(file->fits, file->compression)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to set compression options for %s (%s) (%d:%d:%d)\n",
                    file->filename, file->name, view->chip, view->cell, view->readout);
            return false;
        }
    }

    if (!pmConfigRunFileAddWrite(config, file)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to add file to run-time information");
        return false;
    }

    // IMPORTANT: If adding a FITS-based file, make sure your write function uses an FPA produced by
    // pmFPAfileSuitableFPA.  This ensures the HDUs are at the correct level for your output format, and sets
    // the headers correctly.

    // select a writing method
    bool status = true;
    switch (file->type) {
      case PM_FPA_FILE_IMAGE:
        status = pmFPAviewWriteFitsImage(view, file, config);
        break;
      case PM_FPA_FILE_MASK:
        status = pmFPAviewWriteFitsMask(view, file, config);
        break;
      case PM_FPA_FILE_VARIANCE:
        status = pmFPAviewWriteFitsVariance(view, file, config);
        break;
      case PM_FPA_FILE_HEADER:
        psAbort ("no HEADER write functions defined");
        break;
      case PM_FPA_FILE_DARK:
        status = pmFPAviewWriteFitsDark(view, file, config);
        break;
      case PM_FPA_FILE_FRINGE:
        status = pmFPAviewWriteFitsImage (view, file, config);
        if (status) {
            if (!pmFPAviewWriteFringes(view, file, config)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to write fringe data from %s.\n", file->filename);
                return false;
            }
        }
        break;
      case PM_FPA_FILE_SUBKERNEL:
        status = pmSubtractionWriteKernels(view, file, config);
        break;
      case PM_FPA_FILE_PATTERN:
        status = pmPatternWrite(view, file, config);
        break;
      case PM_FPA_FILE_SX:
      case PM_FPA_FILE_RAW:
      case PM_FPA_FILE_OBJ:
      case PM_FPA_FILE_CMP:
      case PM_FPA_FILE_CMF:
      case PM_FPA_FILE_CFF:
        status = pmFPAviewWriteObjects (view, file, config);
        break;

      case PM_FPA_FILE_PSF:
        status = pmPSFmodelWriteForView (view, file, config);
        break;

      case PM_FPA_FILE_ASTROM_MODEL:
        status = pmAstromModelWriteForView (view, file, config);
        break;

      case PM_FPA_FILE_ASTROM_REFSTARS:
        status = pmAstromRefstarsWriteForView (view, file, config);
        break;

      case PM_FPA_FILE_KH_CORRECT:
        psError(PS_ERR_IO, true, "cannot write type KH.CORRECT (%s)", file->name);
        break;

      case PM_FPA_FILE_PATTERN_ROW_AMP:
        psError(PS_ERR_IO, true, "cannot write type PATTERN.ROW.AMP (%s)", file->name);
        break;

      case PM_FPA_FILE_PATTERN_DEAD_CELLS:
        psError(PS_ERR_IO, true, "cannot write type PATTERN.DEAD.CELLS (%s)", file->name);
        break;

      case PM_FPA_FILE_JPEG:
        status = pmFPAviewWriteJPEG (view, file, config);
        break;

      case PM_FPA_FILE_KAPA:
        status = pmFPAviewWriteSourcePlot (view, file, config);
        break;

      case PM_FPA_FILE_EXPNUM:
        // when ppStack output's EXPNUM file it uses a file rule where the file type is MASK
        psError(PS_ERR_IO, true, "cannot write type EXPNUM (%s)", file->name);
        return false;

      case PM_FPA_FILE_WCS:
        psError(PS_ERR_IO, true, "cannot write type WCS (%s)", file->name);
        return false;

      default:
        psError(PS_ERR_IO, true, "warning: type mismatch; saw type %d (%s)", file->type, file->name);
        return false;
    }
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "failed to write %s (%s)\n", file->filename, file->name);
        return false;
    }
    psTrace ("psModules.camera", 5, "wrote %s (%s) (%d:%d:%d)\n", file->filename, file->name, view->chip, view->cell, view->readout);
    return true;
}

bool pmFPAfileClose (pmFPAfile *file, const pmFPAview *view)
{
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    // skip the following states
    if (file->state & PM_FPA_STATE_INACTIVE) {
        psTrace("psModules.camera", 6, "skip close for %s, files is inactive", file->name);
        return true;
    }

    if (file->state == PM_FPA_STATE_CLOSED) {
        psTrace("psModules.camera", 6, "skip close for %s, files is closed", file->name);
        return true;
    }

    // an active internal file should not be sent here (should not be left on config->files)
    PS_ASSERT(file->mode != PM_FPA_MODE_INTERNAL, false);

    // is current level == open level?
    pmFPALevel level = pmFPAviewLevel (view);
    if (file->fileLevel != level) {
        psTrace("psModules.camera", 6, "skip closing of %s at this level %s: fileLevel is %s",
                file->name, pmFPALevelToName(level), pmFPALevelToName(file->fileLevel));
        return true;
    }

    // check if we are actually open
    bool status = true;
    switch (file->type) {
        // check the FITS types
      case PM_FPA_FILE_IMAGE:
      case PM_FPA_FILE_MASK:
      case PM_FPA_FILE_VARIANCE:
      case PM_FPA_FILE_HEADER:
      case PM_FPA_FILE_FRINGE:
      case PM_FPA_FILE_DARK:
      case PM_FPA_FILE_SUBKERNEL:
      case PM_FPA_FILE_PATTERN:
      case PM_FPA_FILE_CMF:
      case PM_FPA_FILE_CFF:
      case PM_FPA_FILE_WCS:
      case PM_FPA_FILE_PSF:
      case PM_FPA_FILE_ASTROM_MODEL:
      case PM_FPA_FILE_ASTROM_REFSTARS:
      case PM_FPA_FILE_KH_CORRECT:
      case PM_FPA_FILE_PATTERN_ROW_AMP:
      case PM_FPA_FILE_PATTERN_DEAD_CELLS:
      case PM_FPA_FILE_LINEARITY:
      case PM_FPA_FILE_NEWNONLIN:
      case PM_FPA_FILE_EXPNUM:
        psTrace ("psModules.camera", 5, "closing %s (%s) (%d:%d:%d)\n", file->filename, file->name, view->chip, view->cell, view->readout);
        status = psFitsClose (file->fits);
        file->fits = NULL;
        file->header = NULL;
        file->state = PM_FPA_STATE_CLOSED;
        file->wrote_phu = false;
        break;

        // ignore the TEXT types
      case PM_FPA_FILE_SX:
      case PM_FPA_FILE_RAW:
      case PM_FPA_FILE_OBJ:
      case PM_FPA_FILE_CMP:
      case PM_FPA_FILE_JPEG:
      case PM_FPA_FILE_KAPA:

        break;

      default:
        psError(PS_ERR_IO, true, "type mismatch: %d (%s)", file->type, file->name);
        return false;
    }
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "failed to close %s (%s) (%d:%d:%d)\n", file->filename, file->name, view->chip, view->cell, view->readout);
        return false;
    }
    return true;
}

bool pmFPAfileFreeData(pmFPAfile *file, const pmFPAview *view)
{
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    if (file->state & PM_FPA_STATE_INACTIVE) {
        psTrace("psModules.camera", 6, "skip free for %s, files is inactive", file->name);
        return true;
    }

    // an active internal file should not be sent here (should not be left on config->files)
    PS_ASSERT(file->mode != PM_FPA_MODE_INTERNAL, false);

    // get the current level
    pmFPALevel level = pmFPAviewLevel (view);

    // do we need to free this file?
    if (level != file->freeLevel) {
        psTrace("psModules.camera", 6, "skip free of %s at this level %s: freeLevel is %s",
                file->name, pmFPALevelToName(level), pmFPALevelToName(file->freeLevel));
        return true;
    }

    bool status = true;
    switch (file->type) {
      case PM_FPA_FILE_IMAGE:
      case PM_FPA_FILE_MASK:
      case PM_FPA_FILE_VARIANCE:
      case PM_FPA_FILE_HEADER:
      case PM_FPA_FILE_FRINGE:
      case PM_FPA_FILE_DARK:
	//
        status = pmFPAviewFreeData(view, file);
        break;
      case PM_FPA_FILE_SUBKERNEL:
      case PM_FPA_FILE_PATTERN:
      case PM_FPA_FILE_SX:
      case PM_FPA_FILE_RAW:
      case PM_FPA_FILE_OBJ:
      case PM_FPA_FILE_CMP:
      case PM_FPA_FILE_CMF:
      case PM_FPA_FILE_CFF:
      case PM_FPA_FILE_WCS:
      case PM_FPA_FILE_PSF:
      case PM_FPA_FILE_ASTROM_MODEL:
      case PM_FPA_FILE_ASTROM_REFSTARS:
      case PM_FPA_FILE_KH_CORRECT:
      case PM_FPA_FILE_PATTERN_ROW_AMP:
      case PM_FPA_FILE_PATTERN_DEAD_CELLS:
      case PM_FPA_FILE_EXPNUM:
        psTrace ("psModules.camera", 6, "NOT freeing %s (%s) : save for further analysis\n", file->filename, file->name);
        return true;
      case PM_FPA_FILE_JPEG:
      case PM_FPA_FILE_KAPA:
      case PM_FPA_FILE_LINEARITY:
      case PM_FPA_FILE_NEWNONLIN:
        psTrace ("psModules.camera", 5, "nothing to free for %s (%s)\n", file->filename, file->name);
        return true;
      default:
        psError(PS_ERR_IO, true, "warning: type mismatch; saw type %d", file->type);
        return false;
    }
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "failed to read %s (%s)\n", file->filename, file->name);
        return false;
    }
    psTrace ("psModules.camera", 5, "freed %s (%s) (%d:%d:%d)\n", file->filename, file->name, view->chip, view->cell, view->readout);
    return true;
}

psString pmFPAfileName(const pmFPAfile *file, const pmFPAview *view, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(file, NULL);
    PS_ASSERT_PTR_NON_NULL(view, NULL);
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psString filename = pmFPAfileNameFromRule(file->filerule, file, view); // Filename, based on rule
    if (!filename) {
        psError(PS_ERR_IO, true, "Cannot determine file name from rule");
        return false;
    }

    // indirect filenames: these come from a list on the command line or elsewhere
    if (!strcasecmp(filename, "@FILES")) {
        psString filesrc = pmFPAfileNameFromRule(file->filesrc, file, view); // Source of file name
        if (!filesrc) {
            psError(PS_ERR_IO, false, "error converting filesrc to name %s", file->filesrc);
            return false;
        }
        psFree(filename);
        filename = psMemIncrRefCounter(psMetadataLookupStr(NULL, file->names, filesrc));
        if (!filename) {
            psError(PS_ERR_IO, false, "filename lookup error (@FILES) for %s : %s", file->filesrc, filesrc);
            psFree(filesrc);
            return false;
        }
        psFree(filesrc);
    }

    // get name from detrend database
    // file->detrend->detID contains the desired -det_id detID -iteration iter string
    if (!strcasecmp(filename, "@DETDB")) {
        if (!file->detrend) {
            psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find information about selected detrend.");
            return false;
        }
        psMetadata *menu = psMetadataLookupMetadata(NULL, file->camera, "CLASSID"); // Menu of class IDs
        if (!menu) {
            psError(PS_ERR_IO, false, "Unable to find CLASSID metadata in camera configuration");
            return false;
        }
        const char *rule = psMetadataLookupStr(NULL, menu, file->detrend->level); // Rule for class_id
        if (!rule || strlen(rule) == 0) {
            psError(PS_ERR_IO, false, "Unable to find %s in CLASSID in camera configuration",
                    file->detrend->level);
            return false;
        }
        psString classId = pmFPAfileNameFromRule(rule, file, view); // The class identifier, for pmDetrendFile
        if (!classId) {
            psError(PS_ERR_IO, false, "error converting CLASSID rule to name: %s\n", rule);
            return false;
        }

        psTrace("psModules.camera", 6, "looking for detrend (%s, %s)\n", file->detrend->detID, classId);
        psFree(filename);
        filename = pmDetrendFile(file->detrend->detID, classId, config);
        if (!filename) {
            psError(PS_ERR_IO, false, "failed to find a valid detrend image for detID %s : classID %s",
                    file->detrend->detID, classId);
            psFree(classId);
            return false;
        }

        psTrace("psModules.camera", 6, "got detrend file %s", filename);
        psFree(classId);
    }

    return filename;
}

// open file (if not already opened).
// this function is only called only within pmFPAfileRead or pmFPAfileWrite.
bool pmFPAfileOpen (pmFPAfile *file, const pmFPAview *view, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    bool status;
    char *mode = NULL;
    char *readMode = "r";
    char *writeMode = "w";

    if (file->state & PM_FPA_STATE_INACTIVE) {
        psTrace("psModules.camera", 6, "skip open for %s, files is inactive", file->name);
        return true;
    }

    if (file->state == PM_FPA_STATE_OPEN) {
        return true;
    }

    // an ACTIVE internal file should not be sent here
    PS_ASSERT(file->mode != PM_FPA_MODE_NONE, false);
    PS_ASSERT(file->mode != PM_FPA_MODE_INTERNAL, false);

    if (file->mode == PM_FPA_MODE_READ) {
        mode = readMode;
    }
    if (file->mode == PM_FPA_MODE_WRITE) {
        mode = writeMode;
    }
    if ((file->mode == PM_FPA_MODE_WRITE) && !file->save) {
        psTrace("psModules.camera", 6, "skip open for %s, output file not requested", file->name);
        return true;
    }

    // determine the file name, free a name allocated earlier
    psFree(file->filename);
    file->filename = pmFPAfileName(file, view, config);
    if (!file->filename) {
        psError(PS_ERR_IO, true, "Unable to determine filename for file %s", file->name);
        return false;
    }

    // apply filename mangling rules (file://, path://, neb://)
    bool create = file->mode == PM_FPA_MODE_WRITE ? true : false;
    psString realName = pmConfigConvertFilename(file->filename, config, create, false);
    if (!realName) {
        psError(psErrorCodeLast(), false, "failed to determine real name from template for %s\n",
                file->filename);
        return false;
    }
    psFree(file->origname);
    file->origname = file->filename;
    file->filename = realName;

    switch (file->type) {
        // open the FITS types:
      case PM_FPA_FILE_IMAGE:
      case PM_FPA_FILE_MASK:
      case PM_FPA_FILE_VARIANCE:
      case PM_FPA_FILE_HEADER:
      case PM_FPA_FILE_FRINGE:
      case PM_FPA_FILE_DARK:
      case PM_FPA_FILE_SUBKERNEL:
      case PM_FPA_FILE_PATTERN:
      case PM_FPA_FILE_CMF:
      case PM_FPA_FILE_CFF:
      case PM_FPA_FILE_WCS:
      case PM_FPA_FILE_PSF:
      case PM_FPA_FILE_ASTROM_MODEL:
      case PM_FPA_FILE_ASTROM_REFSTARS:
      case PM_FPA_FILE_KH_CORRECT:
      case PM_FPA_FILE_PATTERN_ROW_AMP:
      case PM_FPA_FILE_PATTERN_DEAD_CELLS:
      case PM_FPA_FILE_LINEARITY:
      case PM_FPA_FILE_NEWNONLIN:
      case PM_FPA_FILE_EXPNUM:
        psTrace ("psModules.camera", 5, "opening %s (%s) (%d:%d:%d)\n",
                 file->filename, file->name, view->chip, view->cell, view->readout);
        file->fits = psFitsOpen (file->filename, mode);
        if (file->fits == NULL) {
            psError(PS_ERR_IO, false, "error opening file %s\n", file->filename);
            return false;
        }
        file->state = PM_FPA_STATE_OPEN;

        file->fits->options = psMemIncrRefCounter(file->options);

        // in most cases, we have already open and read the phu and determined the format.
        // in some cases, (eg DetDB images), we have only just determined the filename.
        // we need to check the file format before we can work with the file
        if (!file->format) {
          psMetadata *phu = psFitsReadHeader (NULL, file->fits);
          if (!phu) {
            psError(PS_ERR_IO, false, "Failed to read file header %s\n", file->filename);
            return false;
          }

          // determine the current format from the header
          // determine camera if not specified already
          // XXX can I actually reach this with camera not specified??
          psMetadata *camera = NULL;
          psString formatName = NULL;
          psString cameraName = NULL;
          file->format = pmConfigCameraFormatFromHeader(&camera, &cameraName, &formatName, config, phu, true);
          if (!file->format) {
            psError(PS_ERR_IO, false, "Failed to read CCD format from %s\n", file->filename);
            psFree(phu);
            return false;
          }
          psFree(phu);

          pmFPA *newFPA = pmFPAConstruct (camera, formatName);
          if (!newFPA) {
              psError(PS_ERR_IO, false, "Failed to construct FPA from %s for %s", file->filename, formatName);
              psFree(camera);
              psFree(formatName);
              return NULL;
          }
          psFree(camera);
          psFree(formatName);
          psFree(cameraName);

          // XXX this is really dangerous...
          psFree (file->fpa);
          file->fpa = newFPA;
        }

        // if needed, set the optional EXTWORD field based on the camera value
        psMetadata *fileMenu = psMetadataLookupMetadata (NULL, file->format, "FILE");
        if (!fileMenu) {
          psError (PS_ERR_IO, true, "FILE METADATA missing from camera format %s\n",
                   config->formatName);
          return false;
        }
        char *extword = psMetadataLookupStr (&status, fileMenu, "EXTWORD");
        if (status) {
          psFitsSetExtnameWord (file->fits, extword);
        }

        // XXX these are probably only needed for WRITE files
        if (file->compression) {
            psTrace("psModules.camera", 7, "Setting compression for %s (%s)\n", file->filename, file->name);
            if (!psFitsCompressionApply(file->fits, file->compression)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to set compression options for %s (%s) (%d:%d:%d)\n",
                        file->filename, file->name, view->chip, view->cell, view->readout);
                return false;
            }
        }

        // In some cases, we need to read the PHU after we've opened the file.  This happens for the images
        // supplied by the detrend database, which are only identified here (pmConfigConvertFilename).
        if (!pmFPAfileReadPHU (file, view, config)) {
            psError (PS_ERR_IO, true, "error reading PHU for %s (%s) (%d:%d:%d)\n",
                     file->filename, file->name, view->chip, view->cell, view->readout);
            return false;
        }
        break;

        // defer opening TEXT types:
      case PM_FPA_FILE_SX:
      case PM_FPA_FILE_OBJ:
      case PM_FPA_FILE_CMP:
      case PM_FPA_FILE_RAW:
      case PM_FPA_FILE_JPEG:
      case PM_FPA_FILE_KAPA:
        psTrace ("psModules.camera", 5, "defer opening %s\n", file->filename);
        break;

      default:
        psError(PS_ERR_IO, true, "type mismatch for %s : %d\n", file->filename, file->type);
        return false;
    }
    return true;
}

// for this file and view, if we need to read a PHU, read it.  return true for any non-error
// condition. this function should be called by pmFPAfileOpen.  this function is only called
// for files for which the PHU is not already loaded (files passed via the db or files after
// the first in a multifile dataset)
bool pmFPAfileReadPHU (pmFPAfile *file, const pmFPAview *view, pmConfig *config)
{
    // required conditions
    if (file->mode != PM_FPA_MODE_READ) return true;
    if (file->state != PM_FPA_STATE_OPEN) psAbort ("pmFPAfileReadPHU called on unopened file");
    if (file->fpa == NULL) psAbort ("pmFPAfileReadPHU called on file without an FPA");

    // check if we need to read a PHU (if not, return true)
    switch (file->fileLevel) {
      case PM_FPA_LEVEL_FPA:
        if (file->fpa->hdu) return true;
        break;
      case PM_FPA_LEVEL_CHIP: {
          pmChip *chip = pmFPAviewThisChip(view, file->fpa);
          if (!chip) psAbort ("inconsistent file/fpa: fileLevel is CHIP, view is FPA");
          if (chip->hdu) return true;
          break;
      }
      case PM_FPA_LEVEL_CELL: {
          pmCell *cell = pmFPAviewThisCell(view, file->fpa);
          if (!cell) psAbort ("inconsistent file/fpa: fileLevel is CELL, view is FPA");
          if (cell->hdu) return true;
          break;
      }
      case PM_FPA_LEVEL_NONE:
        // Might get here immediately after opening a file selected from the detrend database.
        break;
      default:
        psAbort("fileLevel not correctly set");
        break;
    }

    // XXX do we need to advance to the first HDU?
    psMetadata *phu = psFitsReadHeader (NULL, file->fits);
    if (!file->format) {
        // determine the format (camera is already known); do not load the recipe
        file->format = pmConfigCameraFormatFromHeader (NULL, NULL, &file->formatName, config, phu, false);
        if (!file->format) {
            psError(PS_ERR_IO, false, "Failed to read CCD format from %s\n", file->filename);
            psFree(phu);
            return false;
        }
    } else {
        bool valid;
        if (!pmConfigValidateCameraFormat (&valid, file->format, phu)) {
            psError (PS_ERR_UNKNOWN, false, "Error in camera configuration\n");
            psFree (phu);
            return false;
        }
        if (!valid) {
            psError(PS_ERR_IO, false, "file %s is not from the required camera", file->filename);
            psFree (phu);
            return false;
        }
    }
    pmFPAview *thisView = pmFPAAddSourceFromHeader (file->fpa, phu, file->format);
    assert (thisView); // XXX we are having some trouble with input psf files not having the Cell and fpa names matching.
    psFree (thisView);
    psFree (phu);
    // XXX we can check the output view to be sure it corresponds to our current view
    return true;
}

// XXX this function is only called from pmFPAfileWrite
// XXX for each data type, there should be a function which writes the PHU, if needed
bool pmFPAfileWritePHU(pmFPAfile *file, const pmFPAview *view, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    if (file->wrote_phu) return true;

    bool status = true;
    switch (file->type) {
      case PM_FPA_FILE_IMAGE:
      case PM_FPA_FILE_MASK:
      case PM_FPA_FILE_VARIANCE:
      case PM_FPA_FILE_DARK:
      case PM_FPA_FILE_FRINGE:
        status = pmFPAviewFitsWritePHU (view, file, config);
        break;
      case PM_FPA_FILE_SUBKERNEL:
        status = pmSubtractionWritePHU(view, file, config);
        break;
      case PM_FPA_FILE_PATTERN:
        status = pmPatternWritePHU(view, file, config);
        break;
      case PM_FPA_FILE_CMF:
        status = pmSource_CMF_WritePHU (view, file, config);
        break;
      case PM_FPA_FILE_PSF:
        status = pmPSFmodelWritePHU(view, file, config);
        break;
      case PM_FPA_FILE_ASTROM_REFSTARS:
        status = pmAstromRefstarsWritePHU (view, file, config);
        break;
      case PM_FPA_FILE_EXPNUM:
      case PM_FPA_FILE_ASTROM_MODEL:
      case PM_FPA_FILE_KH_CORRECT:
      case PM_FPA_FILE_PATTERN_ROW_AMP:
      case PM_FPA_FILE_PATTERN_DEAD_CELLS:
      case PM_FPA_FILE_SX:
      case PM_FPA_FILE_RAW:
      case PM_FPA_FILE_OBJ:
      case PM_FPA_FILE_CMP:
      case PM_FPA_FILE_WCS:
      case PM_FPA_FILE_CFF:
      case PM_FPA_FILE_JPEG:
      case PM_FPA_FILE_KAPA:
        break;
      default:
        fprintf (stderr, "warning: type mismatch\n");
        return false;
    }
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "failed to write PHU for %s (%s)\n", file->filename, file->name);
        return false;
    }
    // XXX this is also being set in the individual functions.  choose one or the other
    file->wrote_phu = true;
    return true;
}


// set the state of the specified pmFPAfile(s) to active (state == true) or inactive
// if name is NULL, set the state for all pmFPAfiles
bool pmFPAfileActivate(psMetadata *files, bool state, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(files, false);

    // return false if the requested file is not in the list (not an error, but informational)
    psArray *selected = pmFPAfileSelect(files, name);
    if (!selected) {
        return false;
    }
    for (int i = 0; i < selected->n; i++) {
        pmFPAfile *file = selected->data[i]; // File of interest
        if (!file) {
            continue;
        }
        if (state) {
            file->state &= PS_NOT_U8(PM_FPA_STATE_INACTIVE);
        } else {
            file->state |= PM_FPA_STATE_INACTIVE;
        }
    }
    psFree(selected);

    return true;
}


pmFPAfile *pmFPAfileActivateSingle(psMetadata *files, bool state, const char *name, int num)
{
    PS_ASSERT_PTR_NON_NULL(files, NULL);
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);
    PS_ASSERT_INT_NONNEGATIVE(num, NULL);

    pmFPAfile *file = pmFPAfileSelectSingle(files, name, num);
    if (!file) {
        psError(PS_ERR_UNKNOWN, false, "Unable to select instance %d of file %s", num, name);
        return NULL;
    }
    if (state) {
        file->state &= PS_NOT_U8(PM_FPA_STATE_INACTIVE);
    } else {
        file->state |= PM_FPA_STATE_INACTIVE;
    }

    return file;
}
