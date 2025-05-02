/** @file psastroConvert.c
 *
 *  @brief
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.25 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"
// leak free 2006.04.27

// To select good stars, I want them to have a PSFmodel fit (PM_SOURCE_MODE_PSFMODEL & PM_SOURCE_MODE_FITTED) and exlcude things that are CR, EXT, FAIL, POOR 
// Stars are allowed to be saturated, as long as the PSF model fits
# define PHOT_SOURCE_MASK (PM_SOURCE_MODE_FAIL | PM_SOURCE_MODE_BLEND | PM_SOURCE_MODE_BADPSF | \
                           PM_SOURCE_MODE_DEFECT | PM_SOURCE_MODE_CR_LIMIT | PM_SOURCE_MODE_EXT_LIMIT | \
                           PM_SOURCE_MODE_POOR ) // Mask to apply to sources for rejection

static psArray *chooseStars(psArray *inStars, char *listName, psArray *sources, psVector *index, int nMax, float iMagMin, float iMagMax, pmSourceMode skip);

bool psastroConvertFPA (pmConfig *config, pmFPA *fpa, psMetadata *recipe) {

    pmChip *chip;
    pmCell *cell;
    pmReadout *readout;
    pmFPAview *view = pmFPAviewAlloc (0);

    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }

        while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
                if (! readout->data_exists) { continue; }

                if (!psastroConvertReadout (config, view, readout, recipe)) {
		  psError(PSASTRO_ERR_CONFIG, true, "problem converting readout\n");
		  return false;
		}
            }
        }
    }
    psFree (view);
    return true;
}

// pass/apply the WCS information?
bool psastroConvertReadout (pmConfig *config, pmFPAview *view, pmReadout *readout, psMetadata *recipe) {

    bool status;

    // XXX I want to make this optional
    float MagOffset = 0.0;
    if (1) {
      // select the input data sources
      pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
      if (!input) {
	psError(PSASTRO_ERR_CONFIG, true, "failed to find PSASTRO.INPUT\n");
	return false;
      }
      pmFPA *fpa = input->fpa;

      float zeropt, exptime;

      // really error-out here?  or just skip?
      if (!psastroZeroPointFromRecipe (&zeropt, &exptime, NULL, NULL, fpa, recipe)) {
        psLogMsg ("psastro", PS_LOG_INFO, "failed to load zeropt data from recipe");
	zeropt = 0.0;
	exptime = 1.0;
      }

      // recipe values are given in instrumental magnitudes
      // use the zero point and exposure time to convert to apparent mags: M_ap = M_inst + C_0 + 2.5*log(exptime)
      MagOffset = zeropt + 2.5*log10(exptime);
    }

    // PSPHOT.SOURCES carries the pmSource objects (from psphot analysis or loaded externally)
    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    // convert the pmSource objects into pmAstromObj objects (drop !STAR and SATSTAR?)
    psArray *inStars = pmSourceToAstromObj (sources, MagOffset);

    // apply Koppenhoefer correction if needed
    if (!psastroCorrectKH (config, view, readout, recipe, inStars)){
      psError(PSASTRO_ERR_CONFIG, true, "failed to correct Koppenhoefer Effect\n");
      return false;
    }

    // sort in ascending magnitude order
    // psArraySort (inStars, psastroSortByMag);
    // psVector *index = psArraySortIndex (sources, pmSourceSortByFlux);
    psVector *index = psArraySortIndex (NULL, inStars, psastroSortByMag);

    // XXX need to exit gracefully is inStars->n is 0 (or 1?)

    // we are going to select the brighter Nmax subset for astrometry
    int nMax = psMetadataLookupS32 (&status, recipe, "PSASTRO.MAX.NRAW");
    if (!status || !nMax) nMax = inStars->n;

    // we are going to select the brighter Nmax subset for astrometry
    float iMagMax = psMetadataLookupF32 (&status, recipe, "PSASTRO.MAX.INST.MAG.RAW");
    float iMagMin = psMetadataLookupF32 (&status, recipe, "PSASTRO.MIN.INST.MAG.RAW");

    // we are going to select the brighter Nmax subset for astrometry
    pmSourceMode skip = PM_SOURCE_MODE_DEFAULT;
    char *ignoreList = psMetadataLookupStr (&status, recipe, "PSASTRO.IGNORE");
    if (ignoreList != NULL) {
      psArray *list = psStringSplitArray (ignoreList, ",", false);
      for (int i = 0; i < list->n; i++) {
        pmSourceMode mode = pmSourceModeFromString (list->data[i]);
        if (mode == PM_SOURCE_MODE_DEFAULT) {
          psWarning ("unknown source mode in PSASTRO.IGNORE, skipping");
          continue;
        }
        skip |= mode;
      }
      psFree (list);
    }

    psArray *rawStars = chooseStars(inStars, "", sources, index, PS_MIN(nMax, inStars->n), iMagMin, iMagMax, skip);
    psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSASTRO.RAWSTARS", PS_DATA_ARRAY, "astrometry objects", rawStars);

    bool gridSearch = psMetadataLookupBool (&status, recipe, "PSASTRO.GRID.SEARCH");
    if (gridSearch) {
        // See if different magnitude limits have been specified for grid search. If so, create a separate list of stars to use.
        float iGridMagMax = psMetadataLookupF32 (&status, recipe, "PSASTRO.GRID.MAX.INST.MAG.RAW");
        float iGridMagMin = psMetadataLookupF32 (&status, recipe, "PSASTRO.GRID.MIN.INST.MAG.RAW");
        int   nMaxGrid = psMetadataLookupS32 (&status, recipe, "PSASTRO.GRID.NRAW.MAX");
    
        // XXX Should we check PSASTRO.GRID.NRAW.MAX != PSASTRO.MAX.NRAW as well? It usually is smaller so that would cause
        // us to always create a separate list. So I won't check.

        if ((iGridMagMax != iMagMax) || (iGridMagMin != iMagMin)) {
            psArray *gridStars = chooseStars(inStars, "grid search ", sources, index, PS_MIN(nMaxGrid, inStars->n), iGridMagMin, iGridMagMax, skip);
            psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSASTRO.GRID.RAWSTARS", PS_DATA_ARRAY, "astrometry objects for grid search", gridStars);
            psFree(gridStars);
        }
    }

    //also select a sample of stars which are allowed to be saturated and bright, as long as they are well fit by a PSF model
    int j = 0;
    psArray *calStars = psArrayAlloc(inStars->n);

    for (int i = 0; (i < inStars->n) && (j < calStars->n); i++) {
        int n = index->data.S32[i];
        pmSource *source = sources->data[n];

        // we only want to use stars which are not bad. Use the source->mode to check for the rejection mask while keeping fitted PSFmodel
        if (source->mode & PHOT_SOURCE_MASK || !isfinite(source->psfMag)) {
            continue;
        }

        //Ensure the source has a fitted PSF model
        if (!(source->mode & PM_SOURCE_MODE_PSFMODEL) || !(source->mode & PM_SOURCE_MODE_FITTED) ) {
            continue;
        }

        calStars->data[j] = psMemIncrRefCounter (inStars->data[n]);
        j++;
    }
    calStars->n = j;
    psMetadataAdd (readout->analysis, PS_LIST_TAIL, "PSASTRO.CALSTARS", PS_DATA_ARRAY, "astrometry objects for masking", calStars);
    psFree (calStars);

    psFree (index);
    psFree (inStars);
    psFree (rawStars);

    return true;
}


psArray * chooseStars(psArray *inStars, char *listName, psArray *sources, psVector *index, int nMax, float iMagMin, float iMagMax, pmSourceMode skip) {
    // choose the first nMax sources
    int j = 0;
    psArray *rawStars = psArrayAlloc(nMax);

    float mMin = +100.0;
    float mMax = -100.0;
    int nModeSkip = 0;
    int nFaintSkip = 0;
    int nBrightSkip = 0;
    int nInfSkip = 0;
    int nCTESkip = 0;

    for (int i = 0; (i < inStars->n) && (j < rawStars->n); i++) {
        int n = index->data.S32[i];
        pmSource *source = sources->data[n];

        psTrace ("psastro", 6, "mag: %f +/- %f, mode: %x, skip: %x\n", source->psfMag, source->psfMagErr, source->mode, skip);

        if (source->mode & skip) {
          nModeSkip ++;
          continue;
        }

        if ((iMagMax != 0.0) && (source->psfMag > iMagMax)) {
          nFaintSkip ++;
          continue;
        }
        if ((iMagMin != 0.0) && (source->psfMag < iMagMin)) {
          nBrightSkip ++;
          continue;
        }
        if (!isfinite(source->psfMag)) {
          nInfSkip ++;
          continue;
        }
	//Also kick out stars that touch the CTE region
        if (source->mode2 & PM_SOURCE_MODE2_ON_CTE) {
            nCTESkip ++;
            continue;
        }
	
        mMin = PS_MIN (mMin, source->psfMag);
        mMax = PS_MAX (mMax, source->psfMag);
        rawStars->data[j] = psMemIncrRefCounter (inStars->data[n]);
        j++;
    }
    rawStars->n = j;

    psLogMsg ("psastro", 4, "loaded %ld %ssources, using %ld of %ld good sources (inst mag: %f to %f)\n", sources->n, listName, rawStars->n, inStars->n, mMin, mMax);
    psLogMsg ("psastro", 4, "skip reasons: mode: %d, faint: %d, bright: %d, inf: %d, CTE: %d\n", nModeSkip, nFaintSkip, nBrightSkip, nInfSkip,nCTESkip);

    return rawStars;
}

// select a magnitude range?
psArray *pmSourceToAstromObj (psArray *sources, float MagOffset) {

    psArray *objects = psArrayAllocEmpty (sources->n);

    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];

        // only accept the PSF sources?
        // XXX drop SATSTAR?
        // if (source->type != PM_SOURCE_TYPE_STAR) continue;

        pmModel *model = source->modelPSF;
        if (model == NULL) continue;

        psF32 *PAR = model->params->data.F32;
        psF32 *dPAR = model->dparams->data.F32;

	// Note from EAM: I measured the KH correction from the nightly science database
	// The KH effect was active until 2011/05/11 when the camera voltages were modified
	// Between 2011/09/06 and 2013/04/30, there was a bug in pmPSF_ModelToAxes with the 
	// result that the reported values were too large by a factor of sqrt(2).  Fortunately,
	// this did not affect the data in the nightly science DVO used to measure the
	// effect (all affected data was processed BEFORE the bug was introduced), and it
	// does not affect any of the PV2 or PV3 data processed AFTER the bug was fixed.
	psEllipseAxes axes = pmPSF_ModelToAxes (PAR, model->class->useReff);

        pmAstromObj *obj = pmAstromObjAlloc ();

        // is the source magnitude calibrated in any sense?
        obj->pix->x    = PAR[PM_PAR_XPOS];
        obj->pix->y    = PAR[PM_PAR_YPOS];
        obj->pix->xErr = dPAR[PM_PAR_XPOS];
        obj->pix->yErr = dPAR[PM_PAR_YPOS];
        obj->Mag = source->psfMag;
        obj->dMag = source->psfMagErr;
        obj->SBinst = source->psfMag + 5.0*log10(axes.major);
        obj->magCal = obj->Mag + MagOffset;

        // XXX do we have the information giving the readout and cell offset?
        // for the moment, assume chip == cell == readout
        *obj->cell = *obj->pix;
        *obj->chip = *obj->cell;

        psArrayAdd (objects, 100, obj);
        psFree (obj);
    }
    return objects;
}

/**
 * sort by Mag (ascending)
 */
int psastroSortByMag (const void *a, const void *b)
{
    const pmAstromObj *A = a;
    const pmAstromObj *B = b;

    psF32 mA = (isfinite(A->Mag)) ? A->Mag : FLT_MAX;
    psF32 mB = (isfinite(B->Mag)) ? B->Mag : FLT_MAX;

    psF32 diff = mA - mB;
    if (diff > +FLT_EPSILON) return (+1);
    if (diff < -FLT_EPSILON) return (-1);
    return (0);
}

static float pltScale[] = {
+0.0,
-0.255347177386,
-0.255585625172,
-0.255733260512,
-0.255747352242,
-0.255611001253,
-0.255311933756,
 +0.0,
 +0.0,
 +0.0,
-0.255296201766,
-0.255688636720,
-0.256161080003,
-0.256464074850,
-0.256324860930,
-0.256147363186,
-0.255734743476,
-0.255422729909,
 +0.0,
 +0.0,
-0.255508323163,
-0.256142899513,
-0.255493895173,
-0.256997376740,
-0.256935591102,
-0.256644588679,
-0.256123321533,
-0.255515242517,
 +0.0,
 +0.0,
-0.255633033872,
-0.256356916189,
-0.256929253668,
-0.257250442505,
-0.257260403216,
-0.256962024927,
-0.256387320280,
-0.255681164920,
 +0.0,
 +0.0,
-0.255671992302,
-0.256371312678,
-0.256936249495,
-0.257237892330,
-0.257248004675,
-0.256911942035,
-0.256365648448,
-0.255632205397,
 +0.0,
 +0.0,
-0.255455072403,
-0.256080205411,
-0.256570428520,
-0.256908982217,
-0.256899240971,
-0.256607972383,
-0.256107793808,
-0.255482232273,
 +0.0,
 +0.0,
-0.255237901688,
-0.255669494152,
-0.256101093233,
-0.256372770309,
-0.256373448133,
-0.256118197262,
-0.255689391643,
-0.255279636681,
 +0.0,
 +0.0,
 +0.0,
-0.255286350489,
-0.255517200172,
-0.255689459741,
-0.255668706447,
-0.255539349556,
-0.255363308907,
};

bool psastroCorrectKH (pmConfig *config, pmFPAview *view, pmReadout *readout, psMetadata *recipe, psArray *inStars) {

  bool status;

  psAssert (readout, "missing readout");
  psAssert (readout->parent, "missing cell");
  psAssert (readout->parent->parent, "missing chip");

  // should we stay or should we go?
  bool apply = psMetadataLookupBool (&status, recipe, "KH.CORRECT.APPLY.EXP");
  if (!apply) return true;

  // get the chip ID from the chip name:
  pmChip *chip = readout->parent->parent;
  char *chipName = psMetadataLookupStr (NULL, chip->concepts, "CHIP.NAME");

  // skip stacks
  if (!strcmp(chipName, "SkyChip")) {
    psLogMsg ("psastro.correctKH", PS_LOG_DETAIL, "skipping KH correction: not a gpc1 chip\n");
    return true;
  }

  // only try to address gpc1 chips (should probably check the camera)
  if (strncmp(chipName, "XY", 2)) {
    psLogMsg ("psastro.correctKH", PS_LOG_DETAIL, "skipping KH correction: not a gpc1 chip\n");
    return true;
  }

  psAssert (strlen(chipName) == 4, "error in chip name");
  psAssert (chipName[0] == 'X', "error in chip name");
  psAssert (chipName[1] == 'Y', "error in chip name");
  
  int chipID = atoi (&chipName[2]);

  // XXX hardwired list of chips to correct
  // some notes:
  // XY24 surpisingly does not need correction
  // XY27 has poor astrometry with a weird correction for all mags
  // XY36 surpisingly does not need correction
  // XY67 has poor astrometry with a weird correction for all mags

  // correct this chip? (raise an error for unexpected chipID values)
  switch (chipID) {
    case  1: // do not correct
    case  2: // do not correct
    case  3: // do not correct
    case 10: // do not correct
    case 11: // do not correct
    case 12: // do not correct
    case 13: // do not correct
    case 20: // do not correct
    case 21: // do not correct
    case 22: // do not correct
    case 23: // do not correct
    case 24: // do not correct
    case 27: // do not correct
    case 30: // do not correct
    case 31: // do not correct
    case 32: // do not correct
    case 33: // do not correct
    case 36: // do not correct
    case 44: // do not correct
    case 45: // do not correct
    case 46: // do not correct
    case 47: // do not correct
    case 54: // do not correct
    case 55: // do not correct
    case 56: // do not correct
    case 57: // do not correct
    case 64: // do not correct
    case 65: // do not correct
    case 66: // do not correct
    case 67: // do not correct
    case 74: // do not correct
    case 75: // do not correct
    case 76: // do not correct
      return true;
    case  4: // correct
    case  5: // correct
    case  6: // correct
    case 14: // correct
    case 15: // correct
    case 16: // correct
    case 17: // correct
    case 25: // correct
    case 26: // correct
    case 34: // correct
    case 35: // correct
    case 37: // correct
    case 40: // correct
    case 41: // correct
    case 42: // correct
    case 43: // correct
    case 50: // correct
    case 51: // correct
    case 52: // correct
    case 53: // correct
    case 60: // correct
    case 61: // correct
    case 62: // correct
    case 63: // correct
    case 71: // correct
    case 72: // correct
    case 73: // correct
      break;
    default:
      psAbort ("chipID is invalid");
  }

  // grab the KH correction file
  pmFPAfile *KHfile = psMetadataLookupPtr (NULL, config->files, "PSASTRO.KH.CORRECT");
  if (!KHfile) {
    psError(PM_ERR_CONFIG, false, "KH correction file not found");
    return false;
  }

  // grab the corresponding chip
  pmChip *KHchip = pmFPAviewThisChip (view, KHfile->fpa);
  psAssert (KHchip, "found KH file, but not chip?");

  // grab the correction spline
  KHcorrectData *spline = psMetadataLookupPtr (&status, KHchip->analysis, "KH.CORRECT");
  if (!spline) {
    psError(PM_ERR_CONFIG, false, "KH correction not found");
    return false;
  }

  // the hard-wired array of plate-scales per chip above come from 
  // a single smf.  in there, a negative plate scale means parity is flipped
  // on the sky.
  float myPltScale = fabs(pltScale[chipID]);

  psLogMsg ("psastro.correctKH", PS_LOG_INFO, "applying KH correction to %s (%d)\n", chipName, chipID);

  // apply the correction to the detections
  for (int i = 0; i < inStars->n; i++) {
    pmAstromObj *obj = inStars->data[i];

    float Xraw = obj->pix->x;
    float SBinst = obj->SBinst;

    float dX = KHcorrectApply (spline, SBinst);

    float Xfix = Xraw + dX / myPltScale;

    // note that we carry around pix, cell, chip but the real analysis only operates on chip
    // if in the future we add transformations between chip->cell->pix, then we will need to
    // make these consistent as well.
    obj->pix->x = Xfix;
    obj->cell->x = Xfix;
    obj->chip->x = Xfix;
  }

  return true;

}

