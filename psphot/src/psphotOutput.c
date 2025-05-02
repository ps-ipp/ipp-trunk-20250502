# include "psphotInternal.h"

// convert filerule to filerule.NUM and look up in the config->arguments metadata
int psphotFileruleCount(const pmConfig *config, const char *filerule) {

    bool status = false;

    psString name = NULL;
    psStringAppend(&name, "%s.NUM", filerule);
    int num = psMetadataLookupS32 (&status, config->arguments, name);
    if (!status) {
      // we only explicitly define (filerule.NUM) if we have more than 1
      num = 1;
    }
    psFree (name);
    return num;
}

// convert filerule to filerule.NUM and look up in the config->arguments metadata
bool psphotFileruleCountSet(const pmConfig *config, const char *filerule, int num) {

    psString name = NULL;
    psStringAppend(&name, "%s.NUM", filerule);

    bool status = psMetadataAddS32(config->arguments, PS_LIST_TAIL, name, PS_META_REPLACE, "", num);
    psFree (name);

    return status;
}

pmReadout *psphotSelectBackground (pmConfig *config, const pmFPAview *view, int index) {

    pmFPAfile *file = pmFPAfileSelectSingle (config->files, psphotGetFilerule("PSPHOT.BACKMDL"), index);
    if (!file) return NULL;

    pmReadout *background = READOUT_OR_INTERNAL(view, file);
    return background;
}

pmReadout *psphotSelectBackgroundStdev (pmConfig *config, const pmFPAview *view, int index) {

    pmFPAfile *file = pmFPAfileSelectSingle (config->files, psphotGetFilerule("PSPHOT.BACKMDL.STDEV"), index);
    if (!file) return NULL;

    pmReadout *background = READOUT_OR_INTERNAL(view, file);
    return background;
}

// dump source stats for psf stars
bool psphotDumpStats (psArray *sources, char *stage) {

    char filename[64];
    snprintf (filename, 64, "psf.%s.dat", stage);
    FILE *f = fopen (filename, "w");
    for (int i = 0; i < sources->n; i++) {
	pmSource *source = sources->data[i];
	if (!(source->mode & PM_SOURCE_MODE_PSFSTAR)) continue;

	pmModel *model = source->modelPSF;
	if (!model) continue;

	// int xc = source->peak->x - source->pixels->col0;
	// int yc = source->peak->y - source->pixels->row0;
	// float mcore = source->modelFlux ? source->modelFlux->data.F32[yc][xc] : NAN;
	// float mpeak = model ? model->params->data.F32[PM_PAR_I0] : NAN;
	// bool subtracted = source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED;
	// fprintf (stderr, "%d %d : %d : %f %f : %f %f\n", source->peak->x, source->peak->y, subtracted, source->peak->rawFlux, source->pixels->data.F32[yc][xc], mcore, mpeak); 

	fprintf (f, "%6.1f %6.1f : %6.1f %6.1f : %8.3f %8.3f %8.3f : %f : %f %f %f : %f\n",
		 source->peak->xf, source->peak->yf, 
		 model->params->data.F32[PM_PAR_XPOS], model->params->data.F32[PM_PAR_YPOS], 
		 source->psfMag, source->apMag, source->psfMagErr,
		 model->params->data.F32[PM_PAR_I0], 
		 model->params->data.F32[PM_PAR_SXX], model->params->data.F32[PM_PAR_SXY], model->params->data.F32[PM_PAR_SYY], 
		 model->params->data.F32[PM_PAR_7]);
    }
    fclose (f);
    return true;
}

int psphotSaveImage (psMetadata *header, psImage *image, char *filename) {

    psFits *fits = psFitsOpen (filename, "w");
    psFitsWriteImage (fits, NULL, image, 0, NULL);
    psFitsClose (fits);
    return (TRUE);
}

bool psphotDumpMoments (psMetadata *recipe, psArray *sources) {

    bool status;

    // optional dump of all rough source data
    char *output = psMetadataLookupStr (&status, recipe, "MOMENTS_OUTPUT_FILE");
    if (!output) return false;
    if (output[0] == 0) return false;
    if (!strcasecmp (output, "NONE")) return false;

    pmMomentsWriteText (sources, output);
    return true;
}

bool psphotDumpSource (pmSource *source, char *name) {

    FILE *f = fopen (name, "w");
    if (f == NULL) psAbort("can't open file");

    for (int i = 0; i < source->pixels->numRows; i++) {
        for (int j = 0; j < source->pixels->numCols; j++) {
            // skip masked points
            if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[i][j]) {
                continue;
            }
            // skip zero-variance points
            if (source->variance->data.F32[i][j] == 0) {
                continue;
            }

            fprintf (f, "%d %d %f %f %d\n",
                     (j + source->pixels->col0),
                     (i + source->pixels->row0),
                     source->pixels->data.F32[i][j],
                     1.0 / source->variance->data.F32[i][j],
                     source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[i][j]);
        }
    }
    fclose (f);
    return true;
}

bool psphotCleanInputsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index);
bool psphotCleanInputs (pmConfig *config, const pmFPAview *view, const char *filerule) {

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotCleanInputsReadout (config, view, filerule, i)) {
	  psError (PSPHOT_ERR_CONFIG, false, "failed to clean inputs for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

bool psphotCleanInputsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index) {

    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    PS_ASSERT (file, false);

    pmReadout  *readout = pmFPAviewThisReadout (view, file->fpa);

    // XXX anything else to remove?
    if (psMetadataLookup(readout->analysis, "PSPHOT.DETECTIONS")) {
	psMetadataRemoveKey(readout->analysis, "PSPHOT.DETECTIONS");
    }

    return true;
}

bool psphotAddPhotcode (pmConfig *config, const pmFPAview *view, const char *filerule) {

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotAddPhotcodeReadout (config, view, filerule, i)) {
	  psError (PSPHOT_ERR_CONFIG, false, "failed to add photcode to %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

bool psphotAddPhotcodeReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index) {

    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    PS_ASSERT (file, false);

    pmReadout  *readout = pmFPAviewThisReadout (view, file->fpa);

    // determine PHOTCODE from fpa & view, overwrite in readout->analysis
    char *photcode = pmConceptsPhotcodeForView (file, view);
    PS_ASSERT (photcode, false);

    psMetadataAddStr (readout->analysis, PS_LIST_TAIL, "PHOTCODE", PS_META_REPLACE, "photcode from FPA concepts", photcode);
    psLogMsg ("psphot", 3, "PHOTCODE is %s", photcode);

    psFree (photcode);
    return true;
}

bool psphotSetHeaderNstars (psMetadata *header, psArray *sources) {

    int nSrc = 0;
    int nCR  = 0;
    int nEXT = 0;
    int nForced = 0;
    int nDetections = sources != NULL ? sources->n : 0;

    // count the number of sources which will be written and other sub-types
    for (int i = 0; (sources != NULL) && (i < sources->n); i++) {
        pmSource *source = (pmSource *) sources->data[i];
        if (source->mode2 & PM_SOURCE_MODE2_MATCHED) {
            nForced ++;
        }
        pmModel *model = pmSourceGetModel (NULL, source);
        if (model == NULL)
            continue;
        if (source->mode & PM_SOURCE_MODE_EXT_LIMIT) {
            nEXT ++;
        }
        if (source->mode & PM_SOURCE_MODE_CR_LIMIT) {
            nCR ++;
        }
        nSrc ++;
    }

    // XXX: This loop cauess nSrc to be twice as large as it should be. This is kept
    // for compatability
    for (int i = 0; (sources != NULL) && (i < sources->n); i++) {
        pmSource *source = (pmSource *) sources->data[i];
        pmModel *model = pmSourceGetModel (NULL, source);
        if (model == NULL)
            continue;
        nSrc ++;
    }

    psMetadataAddS32 (header, PS_LIST_TAIL, "NSTARS",   PS_META_REPLACE, "Number of sources", nSrc);
    psMetadataAddS32 (header, PS_LIST_TAIL, "NDET",     PS_META_REPLACE, "Number of detections", nDetections);
    psMetadataAddS32 (header, PS_LIST_TAIL, "NDET_EXT", PS_META_REPLACE, "Number of extended sources", nEXT);
    psMetadataAddS32 (header, PS_LIST_TAIL, "NDET_CR",  PS_META_REPLACE, "Number of cosmic rays", nCR);
    psMetadataAddS32 (header, PS_LIST_TAIL, "NDET_FRC", PS_META_REPLACE, "Number of Forced detections", nForced);
    return true;
}

// these values are saved in an output header stub - they are added to either the PHU header
// (CMP) or the MEF table header (CMF).  before the header is created, each readout has these
// values stored on readout->analysis
psMetadata *psphotDefineHeader (psMetadata *analysis) {

    bool status = true;

    psMetadata *header = psMetadataAlloc ();

    // write necessary information to output header
    psMetadataItemSupplement (&status, header, analysis, "ZERO_PT");
    psMetadataItemSupplement (&status, header, analysis, "PHOTCODE");

    psMetadataItemSupplement (&status, header, analysis, "APMIFIT");
    psMetadataItemSupplement (&status, header, analysis, "DAPMIFIT");
    psMetadataItemSupplement (&status, header, analysis, "NAPMIFIT");

    // PSF model parameters (shape values for image center)
    psMetadataItemSupplement (&status, header, analysis, "NPSFSTAR");
    psMetadataItemSupplement (&status, header, analysis, "APLOSS");
    psMetadataItemSupplement (&status, header, analysis, "APREFOFF");

    psMetadataItemSupplement (&status, header, analysis, "FWHM_MAJ");
    psMetadataItemSupplement (&status, header, analysis, "FW_MJ_SG");
    psMetadataItemSupplement (&status, header, analysis, "FW_MJ_LQ");
    psMetadataItemSupplement (&status, header, analysis, "FW_MJ_UQ");

    psMetadataItemSupplement (&status, header, analysis, "FWHM_MIN");
    psMetadataItemSupplement (&status, header, analysis, "FW_MN_SG");
    psMetadataItemSupplement (&status, header, analysis, "FW_MN_LQ");
    psMetadataItemSupplement (&status, header, analysis, "FW_MN_UQ");

    psMetadataItemSupplement (&status, header, analysis, "ANGLE");

    psMetadataItemSupplement (&status, header, analysis, "PSFMODEL");
    psMetadataItemSupplement (&status, header, analysis, "PSF_OK");

    // Image Quality measurements
    psMetadataItemSupplement (&status, header, analysis, "IQ_NSTAR");

    psMetadataItemSupplement (&status, header, analysis, "IQ_FW1");
    psMetadataItemSupplement (&status, header, analysis, "IQ_FW1_E");
    psMetadataItemSupplement (&status, header, analysis, "IQ_FW2");
    psMetadataItemSupplement (&status, header, analysis, "IQ_FW2_E");

    psMetadataItemSupplement (&status, header, analysis, "IQ_M2");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M2_ER");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M2_LQ");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M2_UQ");

    psMetadataItemSupplement (&status, header, analysis, "IQ_M2C");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M2C_E");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M2C_L");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M2C_U");

    psMetadataItemSupplement (&status, header, analysis, "IQ_M2S");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M2S_E");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M2S_L");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M2S_U");

    psMetadataItemSupplement (&status, header, analysis, "IQ_M3");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M3_ER");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M3_LQ");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M3_UQ");

    psMetadataItemSupplement (&status, header, analysis, "IQ_M4");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M4_ER");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M4_LQ");
    psMetadataItemSupplement (&status, header, analysis, "IQ_M4_UQ");

    // XXX these need to be defined from elsewhere
    psMetadataAdd (header, PS_LIST_TAIL, "FSATUR",   PS_DATA_F32 | PS_META_REPLACE, "SATURATION MAG",      0.0);
    psMetadataAdd (header, PS_LIST_TAIL, "FLIMIT",   PS_DATA_F32 | PS_META_REPLACE, "COMPLETENESS MAG",    0.0);
    psMetadataItemSupplement (&status, header, analysis, "NSTARS");

    psMetadataItemSupplement (&status, header, analysis, "NDET");
    psMetadataItemSupplement (&status, header, analysis, "NDET_EXT");
    psMetadataItemSupplement (&status, header, analysis, "NDET_CR");
    psMetadataItemSupplement (&status, header, analysis, "NDET_FRC");

    psMetadataItemSupplement (&status, header, analysis, "PSPHOT.PSF.APRESID");
    psMetadataItemSupplement (&status, header, analysis, "PSPHOT.PSF.APRESID.SYSERR");
    psMetadataItemSupplement (&status, header, analysis, "PSPHOT.CR.MAX.SIZE");
    psMetadataItemSupplement (&status, header, analysis, "PSPHOT.CR.MAX.MAG");

    psMetadataItemSupplement (&status, header, analysis, "EFFECTIVE_AREA");
    psMetadataItemSupplement (&status, header, analysis, "SIGNIFICANCE_SCALE_FACTOR");

    // sky background model statistics
    psMetadataItemSupplement (&status, header, analysis, "MSKY_MN");
    psMetadataItemSupplement (&status, header, analysis, "MSKY_SIG");
    psMetadataItemSupplement (&status, header, analysis, "MSKY_MIN");
    psMetadataItemSupplement (&status, header, analysis, "MSKY_MAX");
    psMetadataItemSupplement (&status, header, analysis, "MSKY_NX");
    psMetadataItemSupplement (&status, header, analysis, "MSKY_NY");

    psMetadataItemSupplement (&status, header, analysis, "MSKY_DEV");
    psMetadataItemSupplement (&status, header, analysis, "MSKY_DQ");

    psMetadataItemSupplement (&status, header, analysis, "DETEFF.MAGREF");

    psMetadataAddF32 (header, PS_LIST_TAIL, "DT_PHOT", PS_META_REPLACE, "elapsed psphot time", psTimerMark ("psphotReadout"));

    return header;
}

// XXX add args as needed
bool psphotDumpPSFStars (pmReadout *readout, pmPSFtry *try, float radius, psImageMaskType maskVal, psImageMaskType markVal) {

    psphotSaveImage (NULL, readout->image,  "rawstars.fits");

    for (int i = 0; i < try->sources->n; i++) {
        // masked for: bad model fit, outlier in parameters
        if (try->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PSFTRY_MASK_ALL)
            continue;

        pmSource *source = try->sources->data[i];
        float x = source->modelPSF->params->data.F32[PM_PAR_XPOS];
        float y = source->modelPSF->params->data.F32[PM_PAR_YPOS];

        // set the mask and subtract the PSF model
        // XXX should we be using maskObj? should we be unsetting the mask?
        // use pmModelSub because modelFlux has not been generated
        assert (source->maskObj);
        psImageKeepCircle (source->maskObj, x, y, radius, "OR", markVal);
        pmModelSub (source->pixels, source->maskObj, source->modelPSF, PM_MODEL_OP_FULL, maskVal);
        psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal));
    }

    FILE *f = fopen ("shapes.dat", "w");
    for (int i = 0; i < try->sources->n; i++) {
        psF32 inPar[10];  // must be psF32 to pmPSF_FitToModel

        // masked for: bad model fit, outlier in parameters
        if (try->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PSFTRY_MASK_ALL) continue;

        pmSource *source = try->sources->data[i];
        psF32 *outPar = source->modelEXT->params->data.F32;

	bool useReff = pmModelUseReff (source->modelEXT->type);

        psEllipseAxes axes1;
	pmModelParamsToAxes (&axes1, outPar[PM_PAR_SXX], outPar[PM_PAR_SXY], outPar[PM_PAR_SYY], useReff);

        psEllipsePol pol = pmPSF_ModelToFit (outPar, useReff);
        inPar[PM_PAR_E0] = pol.e0;
        inPar[PM_PAR_E1] = pol.e1;
        inPar[PM_PAR_E2] = pol.e2;
        pmPSF_FitToModel (inPar, 0.1, useReff);

        psEllipseAxes axes2 = psEllipsePolToAxes(pol, 0.1);

        psEllipsePol pol2 = psEllipseAxesToPol (axes1);

        fprintf (f, "%3d  %7.2f %7.2f  %7.4f %7.4f %7.4f  --  %7.4f %7.4f %7.4f  :  %7.4f %7.4f %7.4f  --  %7.4f %7.4f %7.4f : %7.4f %7.4f %6.1f : %7.4f %7.4f %6.1f\n",
                 i, outPar[PM_PAR_XPOS], outPar[PM_PAR_YPOS],
                 outPar[PM_PAR_SXX], outPar[PM_PAR_SXY], outPar[PM_PAR_SYY],
                 pol.e0, pol.e1, pol.e2,
                 pol2.e0, pol2.e1, pol2.e2,
                 inPar[PM_PAR_SXX], inPar[PM_PAR_SXY], inPar[PM_PAR_SYY],
                 axes1.major, axes1.minor, axes1.theta*PM_DEG_RAD,
                 axes2.major, axes2.minor, axes2.theta*PM_DEG_RAD
            );
    }
    fclose (f);

    psphotSaveImage (NULL, readout->image,  "psfstars.fits");
    pmSourcesWritePSFs (try->sources, "psfstars.dat");
    pmSourcesWriteEXTs (try->sources, "extstars.dat", false);
    // XXX need alternative output function
    // psMetadata *psfData = pmPSFtoMetadata (NULL, try->psf);
    // psMetadataConfigWrite (psfData, "psfmodel.dat", NULL);
    psLogMsg ("psphot.choosePSF", PS_LOG_INFO, "wrote out psf-subtracted image, psf data, exiting\n");

    return true;
}

// for now, let's store the detections on the readout->analysis for each readout
bool psphotDumpTest (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    static int npass = 0;
    char filename[64];

    // XXX dump tests are disabled unless this is commented out:
    return true;

    bool status = true;

    int num = psphotFileruleCount(config, filerule);

    snprintf (filename, 64, "testdump.%02d.dat", npass);
    FILE *f = fopen (filename, "w");

    // loop over the available readouts
    for (int i = 0; i < num; i++) {

        // find the currently selected readout
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
        psAssert (file, "missing file?");

        pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
        psAssert (readout, "missing readout?");

        pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
        psAssert (detections, "missing detections?");

        psArray *sources = detections->newSources ? detections->newSources : detections->allSources;
        psAssert (sources, "missing sources?");

	if (detections->newSources) {
	    fprintf (f, "## --- from new sources ---\n");
	} else {
	    fprintf (f, "## --- from all sources ---\n");
	}

	for (int i = 0; i < sources->n; i++) {
	    pmSource *source = sources->data[i];
	    if (!source) continue;

	    pmPeak *peak = source->peak;
	    if (!peak) continue;

	    // XXX only dump a given region
	    // if (peak->xf < 20) continue;
	    // if (peak->yf < 20) continue;
	    // if (peak->xf > 40) continue;
	    // if (peak->yf > 70) continue;

	    float Msum = source->moments ? source->moments->Sum : NAN;
	    float Mx   = source->moments ? source->moments->Mx : NAN;
	    float My   = source->moments ? source->moments->My : NAN;
	    // float Npix = source->moments ? source->moments->nPixels : NAN;
	    float Io = source->modelPSF ? source->modelPSF->params->data.F32[PM_PAR_I0] : NAN;
	    fprintf (f, "%d %f %f  : %f %f : %f %f\n", source->imageID, peak->xf, peak->yf, Mx, My, Msum, Io);
	}
    }
    fclose (f);
    npass ++;

    return true;
}

# if (0)
bool psphotDumpTest (pmConfig *config, const pmFPAview *view, const char *filerule, char *filename) {

    bool status;

    psTimerStart ("psphot.models");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, 0); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->newSources;
    psAssert (sources, "missing sources?");

    FILE *f = NULL;

    f = fopen (filename, "w");
    psAssert(f, "cannot open file");

    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];

        // skip non-astronomical objects (very likely defects)
        if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
        if (source->type == PM_SOURCE_TYPE_SATURATED) continue;
        if (!source->peak) continue;
        if (!source->moments) continue;

        float Io = source->peak->rawFlux;

	fprintf (f, "%f %f : %f %f :: %f\n", 
		 source->moments->Mx, source->moments->My, 
		 source->peak->xf, source->peak->yf, Io);
    }
    fclose (f);
    return true;
}
# endif
