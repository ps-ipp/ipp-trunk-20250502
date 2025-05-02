/** @file psastroLoadRefstars.c
 *
 *  @brief
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.36 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-09 21:25:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"
# define ELIXIR_MODE 1

psArray *psastroLoadRefstars (pmConfig *config, const char *source) {

    int fd;
    bool status;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (NULL, config->recipes, PSASTRO_RECIPE);

    // DVO APIs expect decimal degrees
    float RAmin  = DEG_RAD*psMetadataLookupF32(NULL, recipe, "RA_MIN");
    float RAmax  = DEG_RAD*psMetadataLookupF32(NULL, recipe, "RA_MAX");
    float DECmin = DEG_RAD*psMetadataLookupF32(NULL, recipe, "DEC_MIN");
    float DECmax = DEG_RAD*psMetadataLookupF32(NULL, recipe, "DEC_MAX");

    // extra field fraction to add
    double fieldPadding = psMetadataLookupF32 (&status, recipe, "PSASTRO.FIELD.PADDING");
    PS_ASSERT (status, NULL);

    float dRA = RAmax - RAmin;
    if (dRA * (1. + 2 * fieldPadding) < 180.) {
        RAmin -= dRA * fieldPadding;
        RAmax += dRA * fieldPadding;
    } else {
        // if dRA > 180 getstar has problems. Just search the entire range
        RAmin = 0;
        RAmax = 360;
    }

    float dDEC = DECmax - DECmin;
    DECmin -= dDEC * fieldPadding;
    DECmax += dDEC * fieldPadding;

    // grab the PSASTRO.CATDIR name from the PSASTRO recipe
    char *catdir_recipe = psMetadataLookupStr(&status, recipe, "PSASTRO.CATDIR");
    psAssert (catdir_recipe, "Need a recipe for the catdir!");

    // substitute abstract name with concrete name, if present in PSASTRO.CATDIRS
    psMetadata *catdirs = psMetadataLookupMetadata(&status, config->site, "PSASTRO.CATDIRS"); // List of cameras
    if (!catdirs) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find PSASTRO.CATDIRS in the system configuration.\n");
        return false;
    }

    // the name in the recipe may be one of:
    // (A) the actual directory name
    // (B) a reference to the name in the PSASTRO.CATDIRS folder in site.config
    // (C) a reference to a folder in the PSASTRO.CATDIRS folder in site.config, containing multiple copy locations
    char *catdir_virtual = psMetadataLookupStr(&status, catdirs, catdir_recipe);

    psMetadata *catdir_folder = psMetadataLookupMetadata(&status, catdirs, catdir_recipe);
    if (catdir_folder) {
        // randomly choose one of the entries
        psLogMsg ("psastro", 3, "choosing catdir_folder\n");
        int nEntry = catdir_folder->list->n;

        psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
        double frnd = psRandomUniform(rng);
        int entry = PS_MIN(nEntry - 1, PS_MAX(0, nEntry * frnd));
        psFree(rng);

        psMetadataItem *item = psListGet(catdir_folder->list, entry);
	assert (item);
        if (item->type != PS_DATA_STRING) {
            psError(PSASTRO_ERR_CONFIG, true, "Invalid entry in PSASTRO.CATDIR folder: %s\n", item->name);
            return false;
        }
        catdir_virtual = item->data.str;
    }

    char *catdir = (catdir_virtual == NULL) ? catdir_recipe : catdir_virtual;

    // convert the uri to a real filename (ie, path://foobar)
    psString CATDIR = pmConfigConvertFilename(catdir, config, false, false); // Resolved filename
    PS_ASSERT (CATDIR, NULL);

    psLogMsg ("psastro", 3, "looking up reference objects in %s\n", CATDIR);

    char *getstarCommand = psStringCopy(psMetadataLookupStr(NULL, recipe, "DVO.GETSTAR"));
    PS_ASSERT (getstarCommand, NULL);

    char *outformat = psMetadataLookupStr(NULL, recipe, "DVO.GETSTAR.OUTFORMAT");
    PS_ASSERT (outformat, NULL);

    // we have an exposure with a certain filter and exposure time: choose the maximum
    // reference star density for stars fainter than an equivalent instrumental mag limit
    float minMag;
    float maxRho;
    char *photcode = psastroSetMagLimit (&minMag, &maxRho, config, source);
    PS_ASSERT (photcode, NULL);

    // issue the following command:
    // getstar -region RAmin RAmax DECmin DECmax
    char tempFile[64];
    sprintf (tempFile, "/tmp/psastro.XXXXXX");
    if ((fd = mkstemp (tempFile)) == -1) {
        psError(PSASTRO_ERR_REFSTARS, true, "error creating temp output file\n");
        psFree(CATDIR);
        return NULL;
    }
    close (fd);

    psTimerStart ("psastro");

    // supply a known output format (for CATALOG output) so the code below knows what to read
    if (ELIXIR_MODE) {
        psStringAppend (&getstarCommand, " -D CATMODE mef -D CATFORMAT elixir");
    } else {
        psStringAppend (&getstarCommand, " -D CATMODE mef -D CATFORMAT panstarrs");
    }

    // check for default name (use .ptolemyrc), or use specified CATDIR
    if (strcasecmp(CATDIR, "NONE")) {
        psStringAppend (&getstarCommand, " -D CATDIR %s", CATDIR);
    }
    psFree(CATDIR);

    // supply the max magnitude, the output format, and the photcode
    if (strcasecmp (photcode, "NONE")) {
        psStringAppend (&getstarCommand, " -photcode %s -minmag %f -max-density %f", photcode, minMag, maxRho);
    }
    psStringAppend (&getstarCommand, " -format %s", outformat);

    // add region and output filename
    psStringAppend (&getstarCommand, " -region %f %f %f %f -o %s", RAmin, DECmin, RAmax, DECmax, tempFile);
    psTrace ("psastro", 3, "%s\n", getstarCommand);

    psLogMsg("psastro", PS_LOG_INFO, "getstar command: %s", getstarCommand);

    // XXX use psPipe: catch stderr, stdout, allow for Nsec timeout...
    // use fork to add timeout capability
    status = system (getstarCommand);
    if (status) {
        psError(PSASTRO_ERR_REFSTARS, true, "error loading reference data\n");
        return NULL;
    }
    psFree (getstarCommand);

    psLogMsg ("psastro", 3, "ran getstar : %f sec\n", psTimerMark ("psastro"));

    // the output from getstar is a file with the Average table
    psFits *fits = psFitsOpen (tempFile, "r");

    psTimerStart ("psastro");

    psArray *refstars = NULL;
    if (!strcmp (outformat, "CATALOG")) {
      refstars = psastroReadGetstarCatalog (fits);
    }
    if (!strcmp (outformat, "PS1_DEV_0")) {
      refstars = psastroReadGetstar_PS1_DEV_0 (fits);

      // XXX test
      // FILE *outfile = fopen ("refstars.dat", "w");
      // assert (outfile);
      // for (int nn = 0; nn < refstars->n; nn++) {
      //          pmAstromObj *ref = refstars->data[nn];
      //          fprintf (outfile, "%lf %lf\n", ref->sky->r*PS_DEG_RAD, ref->sky->d*PS_DEG_RAD);
      // }
      // fclose (outfile);
    }
    if (refstars == NULL) {
        psError(PSASTRO_ERR_REFSTARS, true, "error reading reference data\n");
        psFitsClose (fits);
        return NULL;
    }

    // apply a color correction
    {
	// from Tonry 
	// (w-r)_obs = 0.042 + 0.166 (r-i)_obs - 0.398 (r-i)_obs^2,  (r-i)_obs < 0.5
	// (w-r)_obs = 0.268 - 0.435 (r-i)_obs - 0.078 (r-i)_obs^2,  (r-i)_obs > 0.5
	// thus, for (r-i < 0.5):
	// (r - i < 0.5) : w = r + 0.042 + 0.166*(r-i) - 0.398(r-i)^2
	// (r - i > 0.5) : w = r + 0.268 - 0.435 (r-i) - 0.078(r-i)^2

	// apply a color correction
	// XXX this is very GPC1 specific and hard-wired -- be very afraid!
	// select the filter; default to fixed photcode and mag limit otherwise
	pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, source); // we've already done this before
	char *filter = psMetadataLookupStr (&status, input->fpa->concepts, "FPA.FILTERID");
	if (!strcmp(filter, "w")) {
	    for (int i = 0; i < refstars->n; i++) {
		pmAstromObj *ref = refstars->data[i];
		float r = ref->Mag;
		float ri = ref->Color;
		// saturate at some valid range limits
		ri = PS_MAX (PS_MIN(ri, 2.0), -0.2);
		float w = NAN;
		if (ri < 0.5) {
		    w = r + 0.042 + 0.166*(ri) - 0.398*PS_SQR(ri);
		} else {
		    w = r + 0.268 - 0.435*(ri) - 0.078*PS_SQR(ri);
		}
		ref->Mag = w;
	    }
	}
    }

    psLogMsg ("psastro", 3, "loaded %ld reference stars : %f sec\n", refstars->n, psTimerMark ("psastro"));

    psTrace ("psastro", 3, "loaded %ld reference stars from (%10.6f,%10.6f) - (%10.6f,%10.6f)\n",
             refstars->n, RAmin, DECmin, RAmax, DECmax);

    psFitsClose (fits);
    unlink (tempFile);

    // dump or plot the available refstars
    if (psTraceGetLevel("psastro.dump") > 0) {
        psastroDumpRefstars (refstars, "refstars.dat");
    }

    pmAstromVisualPlotRefStars (refstars, recipe);

    if (psTraceGetLevel("psastro.plot") > 0) {
        psastroPlotRefstars (refstars, recipe);
    }

    return refstars;
}

psArray *psastroReadGetstarCatalog (psFits *fits) {

    bool status;

    if (ELIXIR_MODE) {
        psFitsMoveExtName (fits, "DVO_AVERAGE_ELIXIR");
    } else {
        psFitsMoveExtName (fits, "DVO_AVERAGE_PANSTARRS");
    }

    long numSources = psFitsTableSize(fits); // Number of sources in table

    // convert the Average table to the pmAstromObj entries
    psArray *refstars = psArrayAllocEmpty (numSources);
    for (int i = 0; i < numSources; i++) {
        pmAstromObj *ref = pmAstromObjAlloc ();

        psMetadata *row = psFitsReadTableRow(fits, i); // Table row

        // DVO tables are stored in degrees
        if (ELIXIR_MODE) {
            ref->sky->r   = RAD_DEG*psMetadataLookupF32 (&status, row, "RA");
            ref->sky->d   = RAD_DEG*psMetadataLookupF32 (&status, row, "DEC");
            ref->Mag      = 0.001*psMetadataLookupS32 (&status, row, "MAG");  // ELIXIR uses millimags
            ref->Color    = 0.0;
        } else {
            ref->sky->r   = RAD_DEG*psMetadataLookupF64 (&status, row, "RA");
            ref->sky->d   = RAD_DEG*psMetadataLookupF64 (&status, row, "DEC");
            ref->Mag      = psMetadataLookupF32 (&status, row, "MAG"); // PANSTARRS uses mags
            ref->Color    = 0.0;
        }
	ref->magCal   = ref->Mag;

        // XXX VERY temporary hack to avoid M31 bulge
        if ((fabs(ref->sky->r - 0.186438) < 0.002) && (fabs(ref->sky->d - 0.720270) < 0.002)) {
          psFree (ref);
          psFree (row);
          continue;
        }

        psArrayAdd (refstars, 100, ref);
        psFree (ref);
        psFree (row);
    }
    return refstars;
}

psArray *psastroReadGetstar_PS1_DEV_0 (psFits *fits) {

    bool status;

    psFitsMoveExtName (fits, "GETSTAR_PS1_DEV_0");

    long numSources = psFitsTableSize(fits); // Number of sources in table

    // convert the Average table to the pmAstromObj entries
    psArray *refstars = psArrayAllocEmpty (numSources);
    for (int i = 0; i < numSources; i++) {
        pmAstromObj *ref = pmAstromObjAlloc ();

        psMetadata *row = psFitsReadTableRow(fits, i); // Table row

        ref->sky->r   = RAD_DEG*psMetadataLookupF64 (&status, row, "RA");
        ref->sky->d   = RAD_DEG*psMetadataLookupF64 (&status, row, "DEC");
        ref->Mag      = psMetadataLookupF32 (&status, row, "MAG");
        float MagC1   = psMetadataLookupF32 (&status, row, "MAG_C1");
        float MagC2   = psMetadataLookupF32 (&status, row, "MAG_C2");
        if (!isnan(MagC1) && !isnan(MagC2)) {
            ref->Color = MagC1 - MagC2;
        } else {
            // XXX save the color and the slope in the table header?
            ref->Color = 0.0;
        }
	ref->magCal   = ref->Mag;

        // XXX VERY temporary hack to avoid M31 bulge
        if ((fabs(ref->sky->r - 0.186438) < 0.002) && (fabs(ref->sky->d - 0.720270) < 0.002)) {
          psFree (ref);
          psFree (row);
          continue;
        }

        psArrayAdd (refstars, 100, ref);
        psFree (ref);
        psFree (row);
    }
    return refstars;
}

# define ESCAPE(MSG) { \
  psLogMsg ("psastro", PS_LOG_INFO, MSG); \
  goto escape; }

char *psastroSetMagLimit (float *minMag, float *maxRho, pmConfig *config, const char *source) {

    bool status;
    char *photcode;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (NULL, config->recipes, PSASTRO_RECIPE);

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, source);
    if (!input) {
        psLogMsg ("psastro", PS_LOG_DETAIL, "no supplied reference header data");
        photcode = psStringCopy ("NONE");
        return photcode;
    }
    assert (input->fpa);

    *maxRho = psMetadataLookupF32(&status, recipe, "DVO.GETSTAR.MAX.RHO");
    if (!status) {
        psError(PSASTRO_ERR_CONFIG, false, "DVO.GETSTAR.MAX.RHO missing from recipe");
        return NULL;
    }

    // select the filter; default to fixed photcode and mag limit otherwise
    char *filter = psMetadataLookupStr (&status, input->fpa->concepts, "FPA.FILTERID");
    if (!status) ESCAPE ("missing FPA.FILTER in concepts");

    float exptime = psMetadataLookupF32 (&status, input->fpa->concepts, "FPA.EXPOSURE");
    if (!status) ESCAPE ("missing FPA.EXPOSURE in concepts");

    // we need to select the PHOTCODE.DATA folder that matches our filter
    psMetadataItem *item = psMetadataLookup (recipe, "PHOTCODE.DATA");
    if (!item) ESCAPE ("PHOTCODE.DATA folders missing");
    if (item->type != PS_DATA_METADATA_MULTI) ESCAPE ("PHOTCODE.DATA not a multi");

    float minInst = psMetadataLookupF32(&status, recipe, "DVO.GETSTAR.MIN.MAG.INST");
    if (!status) ESCAPE ("missing DVO.GETSTAR.MIN.MAG.INST");

    // if non zero override the zero point in the PHOTCODE.DATA with this value
    float fixedzeropt = psMetadataLookupF32(&status, recipe, "DVO.GETSTAR.FIXED.ZEROPT");

    // PHOTCODE.DATA is a multi of metadata items
    psListIterator *iter = psListIteratorAlloc(item->data.list, PS_LIST_HEAD, false);

    psMetadataItem *refItem = NULL;
    while ((refItem = psListGetAndIncrement (iter))) {
        if (refItem->type != PS_DATA_METADATA) ESCAPE ("PHOTCODE.DATA entry is not a metadata folder");

        char *refFilter = psMetadataLookupStr (&status, refItem->data.md, "FILTER");
        if (!status) {
            psLogMsg ("psastro", PS_LOG_INFO, "a PHOTCODE.DATA recipe folder is missing FILTER");
            continue;
        }

        // does this entry match the current filter?
        if (strcmp (refFilter, filter)) continue;

        psLogMsg ("psastro", PS_LOG_DETAIL, "PHOTCODE.DATA found for filter %s", filter);

        float zeropt = psMetadataLookupF32 (&status, refItem->data.md, "ZEROPT");
        if (!status) {
            psLogMsg ("psastro", PS_LOG_INFO, "a PHOTCODE.DATA recipe folder is missing FILTER");
            continue;
        }
        photcode = psMetadataLookupStr (&status, refItem->data.md, "PHOTCODE");
        if (!status) {
            psLogMsg ("psastro", PS_LOG_INFO, "a PHOTCODE.DATA recipe folder is missing FILTER");
            continue;
        }
        if (fixedzeropt != 0.0) {
            // override the recipe's zero point with the fixed value (used for stacks)
            zeropt = fixedzeropt;
        }

        // convert the minInst to a calibrated minimum magnitude
        *minMag = minInst + 2.5*log10(exptime) + zeropt;

        psFree (iter);
        return photcode;
    }
    psFree (iter);

  escape:
    photcode = psMetadataLookupStr(NULL, recipe, "DVO.GETSTAR.PHOTCODE");
    PS_ASSERT (photcode, NULL);

    // give up and use fixed value
    *minMag = psMetadataLookupF32(NULL, recipe, "DVO.GETSTAR.MIN.MAG");
    return photcode;
}
