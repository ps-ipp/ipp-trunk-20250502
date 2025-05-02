# include "psphotInternal.h"

static void psphotStackOptionsFree (psphotStackOptions *options) {

    if (options == NULL) return;

    // free the psf
    psFree (options->psf);

    // free the array elements
    psFree (options->psfs);
    psFree (options->sourceLists);
    psFree (options->kernels);
    psFree (options->regions);

    // free the vector elements
    psFree (options->inputMask);
    psFree (options->inputSeeing);
    psFree (options->norm);
    psFree (options->matchChi2);
    psFree (options->targetSeeing);

    return;
}

psphotStackOptions *psphotStackOptionsAlloc (int num) {

    psphotStackOptions *options = (psphotStackOptions *) psAlloc(sizeof(psphotStackOptions));
    psMemSetDeallocator(options, (psFreeFunc) psphotStackOptionsFree);

    options->numCols = 0;
    options->numRows = 0;

    options->num = num;
    options->psf = NULL;
    options->convolve = false;
    options->convolveSource = PSPHOT_CNV_SRC_NONE;
    options->targetSeeing = NULL;

    options->psfs        = psArrayAlloc(num);
    options->sourceLists = psArrayAlloc(num); // Individual lists of sources for matching
    options->kernels     = psArrayAlloc(num);
    options->regions     = psArrayAlloc(num);

    options->inputMask   = psVectorAlloc(num, PS_TYPE_VECTOR_MASK); // Mask for inputs
    options->inputSeeing = psVectorAlloc(num, PS_TYPE_F32);
    options->norm        = psVectorAlloc(num, PS_TYPE_F32);
    options->matchChi2   = psVectorAlloc(num, PS_TYPE_F32); // chi^2 for stamps when matching

    psVectorInit(options->inputMask,   0);
    psVectorInit(options->inputSeeing, NAN);
    psVectorInit(options->norm,        NAN);
    psVectorInit(options->matchChi2,   NAN);

    return options;
}

psphotStackConvolveSource psphotStackConvolveSourceFromString (const char *string) {

    if (!strcasecmp(string, "AUTO")) return PSPHOT_CNV_SRC_AUTO;
    if (!strcasecmp(string, "CNV"))  return PSPHOT_CNV_SRC_CNV;
    if (!strcasecmp(string, "RAW"))  return PSPHOT_CNV_SRC_RAW;
    return PSPHOT_CNV_SRC_NONE;
}

pmFPAfile *psphotStackGetConvolveSource (pmConfig *config, psphotStackOptions *options, int index) {

    // which image do we want to convolve?  RAW, CNV, AUTO?
    // find the currently selected readout
    pmFPAfile *fileRaw = pmFPAfileSelectSingle(config->files, "PSPHOT.STACK.INPUT.RAW", index); // File of interest
    pmFPAfile *fileCnv = pmFPAfileSelectSingle(config->files, "PSPHOT.STACK.INPUT.CNV", index); // File of interest

    pmFPAfile *fileSrc = NULL;

    switch (options->convolveSource) {
      case PSPHOT_CNV_SRC_AUTO:
        fileSrc = fileCnv ? fileCnv : fileRaw;
        break;

      case PSPHOT_CNV_SRC_RAW:
        fileSrc = fileRaw;
        break;

      case PSPHOT_CNV_SRC_CNV:
        fileSrc = fileCnv;
        break;

      default:
        psAbort("impossible case");
    }
    if (!fileSrc) {
        psError(PSPHOT_ERR_CONFIG, true, "desired convolution source is missing (cnv : %p, raw : %p)", fileCnv, fileRaw);
    }

    return fileSrc;
}
