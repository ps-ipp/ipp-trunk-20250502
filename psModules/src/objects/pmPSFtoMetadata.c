
// create a psMetadata representation (human-readable) of a psf model
psMetadata *pmPSFtoMetadata (psMetadata *metadata, pmPSF *psf)
{

    if (metadata == NULL) {
        metadata = psMetadataAlloc ();
    }

    char *modelName = pmModelClassGetName (psf->type);
    psMetadataAdd (metadata, PS_LIST_TAIL, "PSF_MODEL_NAME", PS_DATA_STRING, "PSF model name", modelName);

    int nPar = pmModelClassParameterCount (psf->type)    ;
    psMetadataAdd (metadata, PS_LIST_TAIL, "PSF_MODEL_NPAR", PS_DATA_S32, "PSF model parameter count", nPar);

    for (int i = 0; i < nPar; i++) {
        psPolynomial2D *poly = psf->params->data[i];
        if (poly == NULL)
            continue;
        psPolynomial2DtoMetadata (metadata, poly, "PSF_PAR%02d", i);
    }

    // XXX fix this
    psWarning ("APTREND is currently missing");
    // psPolynomial4DtoMetadata (metadata, psf->ApTrend, "APTREND");

    psMetadataAdd (metadata, PS_LIST_TAIL, "PSF_AP_RESID", PS_DATA_F32, "aperture residual", psf->ApResid);
    psMetadataAdd (metadata, PS_LIST_TAIL, "PSF_dAP_RESID", PS_DATA_F32, "aperture residual scatter", psf->dApResid);
    psMetadataAdd (metadata, PS_LIST_TAIL, "PSF_SKY_BIAS", PS_DATA_F32, "sky bias level", psf->skyBias);

    psMetadataAdd (metadata, PS_LIST_TAIL, "PSF_CHISQ", PS_DATA_F32, "chi-square for fit", psf->chisq);
    psMetadataAdd (metadata, PS_LIST_TAIL, "PSF_NSTARS", PS_DATA_S32, "number of stars used to measure PSF", psf->nPSFstars);
    psMetadataAdd (metadata, PS_LIST_TAIL, "PSF_POISSON_ERRORS", PS_DATA_BOOL, "Poisson errors for fits", psf->poissonErrors);

    return metadata;
}

// parse a psMetadata representation (human-readable) of a psf model
pmPSF *pmPSFfromMetadata (psMetadata *metadata)
{

    bool status;
    char keyword[80];

    char *modelName = psMetadataLookupPtr (&status, metadata, "PSF_MODEL_NAME");
    pmModelType type = pmModelClassGetType (modelName);

    bool poissonErrors = psMetadataLookupPtr (&status, metadata, "PSF_POISSON_ERRORS");
    if (!status)
        poissonErrors = true;

    // we determine the PSF parameter polynomials from the MD-defined polynomials
    pmPSF *psf = pmPSFAlloc (type, poissonErrors, NULL);

    int nPar = psMetadataLookupS32 (&status, metadata, "PSF_MODEL_NPAR");
    if (nPar != pmModelClassParameterCount (psf->type))
        psAbort("mismatch model par count");

    // un-fitted terms, not in the Metadata, are left NULL
    // XXX add a double-check of the expected number?
    for (int i = 0; i < nPar; i++) {
        sprintf (keyword, "PSF_PAR%02d", i);
        psMetadata *folder = psMetadataLookupPtr (&status, metadata, keyword);
        if (!status)
            continue;
        psPolynomial2D *poly = psPolynomial2DfromMetadata (folder);
        psFree (psf->params->data[i]);
        psf->params->data[i] = poly;
    }

    // load the APTREND data
    // XXX fix this to work with pmTrend2D
    psWarning ("APTREND is not being read");
    # if (0)
    sprintf (keyword, "APTREND");
    psMetadata *folder = psMetadataLookupPtr (&status, metadata, keyword);
    psPolynomial4D *poly = psPolynomial4DfromMetadata (folder);
    psFree (psf->ApTrend);
    psf->ApTrend = poly;
    # endif

    psf->ApResid = psMetadataLookupF32 (&status, metadata, "PSF_AP_RESID");
    psf->dApResid = psMetadataLookupF32 (&status, metadata, "PSF_dAP_RESID");
    psf->skyBias = psMetadataLookupF32 (&status, metadata, "PSF_SKY_BIAS");

    psf->chisq = psMetadataLookupF32 (&status, metadata, "PSF_CHISQ");
    psf->nPSFstars = psMetadataLookupS32 (&status, metadata, "PSF_NSTARS");

    psFree (metadata);
    return (psf);
}
