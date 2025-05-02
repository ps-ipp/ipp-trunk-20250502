/** @file psastroDefineFiles.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.10 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

bool psastroDefineFiles (pmConfig *config, pmFPAfile *input) {

    // these calls bind the I/O handle to the specified fpa
    bool status;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
	psError(PSASTRO_ERR_CONFIG, false, "Can't find PSASTRO recipe!\n");
	return false;
    }

    pmFPAfile *output = pmFPAfileDefineOutput (config, input->fpa, "PSASTRO.OUTPUT");
    if (!output) {
	psError(PSASTRO_ERR_CONFIG, false, "Failed to build FPA from PSASTRO.INPUT");
	return false;
    }
    output->save = true;

    bool fixChips = psMetadataLookupBool (&status, config->arguments, "PSASTRO.FIX.CHIPS");
    if (!status) {
	fixChips = psMetadataLookupBool (&status, recipe, "PSASTRO.FIX.CHIPS");
    }
    bool useModel = psMetadataLookupBool (&status, config->arguments, "PSASTRO.USE.MODEL");
    if (!status) {
	useModel = psMetadataLookupBool (&status, recipe, "PSASTRO.USE.MODEL");
    }
    if (fixChips || useModel) {
        if (!psastroDefineFile (config, input->fpa, "PSASTRO.MODEL", "ASTROM.MODEL", PM_FPA_FILE_ASTROM_MODEL, PM_DETREND_TYPE_ASTROM)) {
            psError (PS_ERR_IO, false, "Can't find an astrometry model file");
            return false;
        }
    }

    bool KHapply = psMetadataLookupBool (&status, recipe, "KH.CORRECT.APPLY");
    if (KHapply) {
	// Get the time from FPA.TIME
	psTime *time = psMetadataLookupPtr(NULL, input->fpa->concepts, "FPA.TIME");
	if (time->sec == 0 && time->nsec == 0) {
	    psError (PM_ERR_CONFIG, false, "KH.CORRECT.APPLY requested, but date/time of exposure not found");
	    return false;
	}
	// what is the appropriate date range for the correction
	char *KHendDate = psMetadataLookupStr (&status, recipe, "KH.CORRECT.ENDDATE");
	if (!KHendDate) {
	    psError (PM_ERR_CONFIG, false, "KH.CORRECT.APPLY requested, but KH.CORRECT.ENDDATE not in recipe");
	    return false;
	}
	psTime *endtime = psTimeFromString (KHendDate, PS_TIME_TAI);
	if (!endtime) {
	    psError (PM_ERR_CONFIG, false, "KH.CORRECT.APPLY requested, KH.CORRECT.ENDDATE has an invalid date/time");
	    return false;
	}
	if (psTimeDelta (endtime, time) < 0) {
	    KHapply = false;
	    psLogMsg ("psastro", PS_LOG_INFO, "Koppenhoefer correction requested, but exposure after end date, correction skipped");
	} else {
	    psLogMsg ("psastro", PS_LOG_INFO, "Koppenhoefer correction requested, exposure in valid date range, correction may be applied");
	}   
	psFree (endtime);
    }
    if (KHapply) {
        if (!psastroDefineFile (config, input->fpa, "PSASTRO.KH.CORRECT", "KH.CORRECT", PM_FPA_FILE_KH_CORRECT, PM_DETREND_TYPE_KH_CORRECT)) {
            psError (PS_ERR_IO, false, "Can't find a Koppenhoefer Correction file");
            return NULL;
        }
    }
    // the bool above in the recipe says "apply generally for this camera".  here we are asserting that
    // we can and should apply for this exposure XXX change the name?
    psMetadataAddBool (recipe, PS_LIST_TAIL, "KH.CORRECT.APPLY.EXP", PS_META_REPLACE, "", KHapply);

    // this step is optional
    bool REFSTAR_MASK = psMetadataLookupBool (&status, recipe, "REFSTAR_MASK");
    if (REFSTAR_MASK) {

        if (!psastroDefineFile (config, input->fpa, "PSASTRO.REFMASK", "REFMASK", PM_FPA_FILE_MASK, PM_DETREND_TYPE_MASK)) {
            psError (PS_ERR_IO, false, "Can't find an input mask file");
            return NULL;
        }

        if (!psastroDefineFile (config, input->fpa, "PSASTRO.INPUT.MASK", "INPUT.MASK", PM_FPA_FILE_MASK, PM_DETREND_TYPE_MASK)) {
            psError (PS_ERR_IO, false, "Can't find an input mask file");
            return NULL;
        }

	pmFPAfile *inMask = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT.MASK");
	if (!inMask) {
            psError (PS_ERR_IO, false, "Can't find an input mask file");
            return NULL;
        }

	// XXX not yet sure if we need to mosaic or not...
	pmFPAfile *outMask = pmFPAfileDefineOutputFromFile  (config, inMask, "PSASTRO.OUTPUT.MASK");
	// pmFPAfile *outMask = pmFPAfileDefineChipMosaic(config, inMask->fpa, "PSASTRO.OUTPUT.MASK");
	if (!outMask) {
	    psError (PS_ERR_IO, false, "Can't find the astrometry refstars file definition");
	    return NULL;
	}
	if (outMask->type != PM_FPA_FILE_MASK) {
	    psError(PS_ERR_IO, true, "%s is not of type %s", "PSASTRO.OUTPUT.MASK", pmFPAfileStringFromType (PM_FPA_FILE_MASK));
	    return NULL;
	}
	outMask->save = true;
	inMask->freeLevel = outMask->dataLevel;
    }

    bool saveRefstars = psMetadataLookupBool (&status, config->arguments, "PSASTRO.SAVE.REFSTARS");
    if (!status) {
      saveRefstars = psMetadataLookupBool (&status, recipe, "PSASTRO.SAVE.REFSTARS");
    }
    if (saveRefstars) {
	// look for the file in the camera config table
	pmFPAfile *file = pmFPAfileDefineOutputFromFile  (config, input, "PSASTRO.OUT.REFSTARS");
	if (!file) {
	    psError (PS_ERR_IO, false, "Can't find the astrometry refstars file definition");
	    return NULL;
	}
	if (file->type != PM_FPA_FILE_ASTROM_REFSTARS) {
	    psError(PS_ERR_IO, true, "%s is not of type %s", "PSASTRO.OUT.REFSTARS", pmFPAfileStringFromType (PM_FPA_FILE_ASTROM_REFSTARS));
	    return NULL;
	}
	file->save = true;
    }

    bool writeCff = psMetadataLookupBool (&status, recipe, "PSASTRO.SAVE.CFF");
    if (writeCff) {
	pmFPAfile *file = pmFPAfileDefineOutputFromFile  (config, input, "PSPHOT.OUTPUT.CFF");
	if (!file) {
	    psError (PS_ERR_IO, false, "Can't find the output cff file definition");
	    return NULL;
	}
	if (file->type != PM_FPA_FILE_CFF) {
	    psError(PS_ERR_IO, true, "%s is not of type %s", "PSPHOT.OUTPUT.CFF", pmFPAfileStringFromType (PM_FPA_FILE_CFF));
	    return NULL;
	}
	file->save = true;
    }


# if (0)
    // optionally save output plots
    if (!pmFPAfileDefineOutput (config, input->fpa, "SOURCE.PLOT.MOMENTS")) {
	psTrace ("psphot", 3, "Cannot find a rule for SOURCE.PLOT.MOMENTS");
    }
    if (!pmFPAfileDefineOutput (config, input->fpa, "SOURCE.PLOT.PSFMODEL")) {
	psTrace ("psphot", 3, "Cannot find a rule for SOURCE.PLOT.PSFMODEL");
    }
# endif

    return true;
}

bool psastroDefineFile (pmConfig *config, pmFPA *input, char *filerule, char *argname, pmFPAfileType fileType, pmDetrendType detrendType) {

    bool status;
    pmFPAfile *file;

    // look for the file on the argument list
    file = pmFPAfileDefineFromArgs  (&status, config, filerule, argname);
    if (!status) {
	psError (PS_ERR_UNKNOWN, false, "failed to load find definition");
	return false;
    }
    if (file) {
	if (file->type != fileType) {
	    psError(PS_ERR_IO, true, "%s is not of type %s", filerule, pmFPAfileStringFromType (fileType));
	    return false;
	}
	return true;
    }

    // look for the file in the camera config table
    file = pmFPAfileDefineFromConf  (&status, config, filerule);
    if (!status) {
	psError (PS_ERR_UNKNOWN, false, "failed to load find definition");
	return false;
    }
    if (file) {
	if (file->type != fileType) {
	    psError(PS_ERR_IO, true, "%s is not of type %s", filerule, pmFPAfileStringFromType (fileType));
	    return false;
	}
	return true;
    }

    // look for the file to be loaded from the detrend database
    file = pmFPAfileDefineFromDetDB (&status, config, filerule, input, detrendType);
    if (!status) {
	psError (PS_ERR_UNKNOWN, false, "failed to load file definition");
	return false;
    }
    if (file) {
	if (file->type != fileType) {
	    psError(PS_ERR_IO, true, "%s is not of type %s", filerule, pmFPAfileStringFromType (fileType));
	    return false;
	}
	return true;
    }
    return false;
}

