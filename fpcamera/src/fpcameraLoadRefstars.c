# include "fpcamera.h"

# define ESCAPE(ERROR,...) { p_psError(__FILE__,__LINE__,__func__,ERROR,false,__VA_ARGS__); return false; }

/* \brief this loop saves the photometry/astrometry data files */
bool fpcameraLoadRefstars (pmFPAfile *input, pmConfig *config) {

    int fd;
    bool status;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (NULL, config->recipes, FPCAMERA_RECIPE);

    // FPA FOV is determined by fpcameraReadAstrometry & save on fpa->analysis
    float RAmin  = psMetadataLookupF32(NULL, input->fpa->analysis, "RA_MIN");
    float RAmax  = psMetadataLookupF32(NULL, input->fpa->analysis, "RA_MAX");
    float DECmin = psMetadataLookupF32(NULL, input->fpa->analysis, "DEC_MIN");
    float DECmax = psMetadataLookupF32(NULL, input->fpa->analysis, "DEC_MAX");

    // extra field fraction to add
    double fieldPadding = psMetadataLookupF32 (&status, recipe, "FPCAMERA.FIELD.PADDING");
    if (!status) ESCAPE(FPCAMERA_ERR_CONFIG, "missing FPCAMERA.FIELD.PADDING in recipe");

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

    // grab the FPCAMERA.CATDIR name from the FPCAMERA recipe
    char *catdir_recipe = psMetadataLookupStr(&status, recipe, "FPCAMERA.CATDIR");
    if (!catdir_recipe) ESCAPE(FPCAMERA_ERR_CONFIG, "Need a recipe for the catdir!");

    // *** substitute abstract name with concrete name, if present in FPCAMERA.CATDIRS ***

    // the name in the recipe may be one of:
    // (A) the actual directory name
    // (B) a reference to the name in the FPCAMERA.CATDIRS folder in site.config
    // (C) a reference to a folder in the FPCAMERA.CATDIRS folder in site.config, containing multiple copy locations

    // Folder in site.config containing a list of FPCAMERA.CATDIR entries.
    // NOTE: it is allowed that FPCAMERA.CATDIRS not be defined in site.config: then FPCAMERA.CATDIR must be a real path
    psMetadata *catdirs = psMetadataLookupMetadata(&status, config->site, "FPCAMERA.CATDIRS"); // List of cameras

    // can we find a plain string matching catdir_recipe in the catdirs folder?
    char *catdir_virtual = catdirs ? psMetadataLookupStr(&status, catdirs, catdir_recipe) : NULL;

    // OR, can we find a subfolder in the catdirs folder?
    psMetadata *catdir_folder = catdirs ? psMetadataLookupMetadata(&status, catdirs, catdir_recipe) : NULL;
    if (catdir_folder) {
        // randomly choose one of the entries
        psLogMsg ("fpcamera", 3, "choosing catdir_folder\n");
        int nEntry = catdir_folder->list->n;

        psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
        double frnd = psRandomUniform(rng);
        int entry = PS_MIN(nEntry - 1, PS_MAX(0, nEntry * frnd));
        psFree(rng);

        psMetadataItem *item = psListGet(catdir_folder->list, entry);
	psAssert (item, "programming error? no valid entry?");

        if (item->type != PS_DATA_STRING) ESCAPE(FPCAMERA_ERR_CONFIG, "Invalid entry in FPCAMERA.CATDIR folder: %s\n", item->name);
        catdir_virtual = item->data.str;
    }

    // if catdir_virtual is NULL, catdir_recipe must be a file name
    char *catdir = (catdir_virtual == NULL) ? catdir_recipe : catdir_virtual;

    // convert the uri to a real filename (ie, path://foobar)
    psString CATDIR = pmConfigConvertFilename(catdir, config, false, false); // Resolved filename
    if (!CATDIR) ESCAPE(FPCAMERA_ERR_CONFIG, "unable to determine filename for CATDIR %s", catdir);

    psLogMsg ("fpcamera", 3, "looking up reference objects in %s\n", CATDIR);

    char *getstarCommand = psStringCopy(psMetadataLookupStr(NULL, recipe, "DVO.GETSTAR"));
    if (!getstarCommand) ESCAPE(FPCAMERA_ERR_CONFIG, "unable to find DVO.GETSTAR in recipe");

    char *outformat = psMetadataLookupStr(NULL, recipe, "DVO.GETSTAR.OUTFORMAT");
    if (!outformat) ESCAPE(FPCAMERA_ERR_CONFIG, "unable to find DVO.GETSTAR.OUTFORMAT in recipe");

    // issue the following command:
    // getstar -region RAmin RAmax DECmin DECmax
    char tempFile[64];
    sprintf (tempFile, "/tmp/fpcamera.XXXXXX");
    if ((fd = mkstemp (tempFile)) == -1) {
        psFree(CATDIR);
	ESCAPE(FPCAMERA_ERR_IO, "error creating temp output file for getstar");
    }
    close (fd);

    psTimerStart ("fpcamera");

    // supply a known output format (for CATALOG output) so the code below knows what to read
    // XXX I do not think this affects the getstar command (CATFORMAT is used to define the dvo db format)
    // if (ELIXIR_MODE) {
    //     psStringAppend (&getstarCommand, " -D CATMODE mef -D CATFORMAT elixir");
    // } else {
    //     psStringAppend (&getstarCommand, " -D CATMODE mef -D CATFORMAT panstarrs");
    // }

    // Define the full getstar command.  Above, we require CATDIR to be valid and defined at
    // this point.  Also add region and output filename.
    psStringAppend (&getstarCommand, " -D CATDIR %s", CATDIR);
    psStringAppend (&getstarCommand, " -format %s", outformat);
    psStringAppend (&getstarCommand, " -region %f %f %f %f -o %s", RAmin, DECmin, RAmax, DECmax, tempFile);

    psFree(CATDIR);

    psLogMsg("fpcamera", PS_LOG_INFO, "getstar command: %s", getstarCommand);

    // run getstar, result is saved in the temp file
    status = system (getstarCommand);
    if (status) ESCAPE(FPCAMERA_ERR_REFSTARS, "error loading reference data");
    psFree (getstarCommand);

    psLogMsg ("fpcamera", 3, "ran getstar : %f sec\n", psTimerMark ("fpcamera"));

    // the output from getstar is a file with the Average table
    psFits *fits = psFitsOpen (tempFile, "r");

    psTimerStart ("fpcamera");

    psArray *refstars = NULL;
    if (!strcmp (outformat, "PS1_DEV_0")) {
      refstars = fpcameraReadGetstar_PS1_DEV_0 (fits);
    }
    if (!refstars) {
        psFitsClose (fits);
        ESCAPE(FPCAMERA_ERR_REFSTARS, "error reading reference data");
    }
    if (refstars->n == 0) ESCAPE(FPCAMERA_ERR_REFSTARS, "no reference stars found");

    psLogMsg ("fpcamera", 3, "loaded %ld reference stars : %f sec\n", refstars->n, psTimerMark ("fpcamera"));

    psTrace ("fpcamera", 3, "loaded %ld reference stars from (%10.6f,%10.6f) - (%10.6f,%10.6f)\n",
             refstars->n, RAmin, DECmin, RAmax, DECmax);

    psFitsClose (fits);
    unlink (tempFile);

    psMetadataAdd (input->fpa->analysis, PS_LIST_TAIL, "FPCAMERA.REFSTARS", PS_DATA_ARRAY, "reference sources", refstars);
    psFree (refstars);

    return true;
}

// method to read PS1_DEV_0 format
// Note other options can be found in psastroLoadRefstars.c
psArray *fpcameraReadGetstar_PS1_DEV_0 (psFits *fits) {

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


