# include "psphotInternal.h"

// allocate readout and images for PSPHOT.STACK.INPUT.CNV
// This is called only if we are skiping the psf matching
bool psphotStackAllocateOutput (const pmConfig *config, pmFPAview *view, psMetadata *recipe) {
    bool status = false;

    bool useRaw = psMetadataLookupBool (&status, recipe, "PSPHOT.STACK.USE.RAW");
    char *fileruleSrc = useRaw ? "PSPHOT.STACK.INPUT.RAW" : "PSPHOT.STACK.INPUT.CNV";

    int num = psphotFileruleCount(config, fileruleSrc);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        // find hte source readout
        pmFPAfile *fileSrc = pmFPAfileSelectSingle(config->files, fileruleSrc, i);
        psAssert (fileSrc, "missing file?");
        pmReadout *readoutSrc = pmFPAviewThisReadout(view, fileSrc->fpa);
        psAssert (readoutSrc, "missing readout?");

        pmFPAfile *fileOut = pmFPAfileSelectSingle(config->files, "PSPHOT.STACK.OUTPUT.IMAGE", i);
        psAssert (fileOut, "missing output file?");
        pmReadout *readoutOut = pmFPAviewThisReadout(view, fileOut->fpa);
        if (readoutOut == NULL) {
            readoutOut = pmFPAGenerateReadout(config, view, "PSPHOT.STACK.OUTPUT.IMAGE", fileSrc->fpa, NULL, i);
            psAssert (readoutOut, "missing readout?");
        }
    }

    return true;
}
