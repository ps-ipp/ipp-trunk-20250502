#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

int main(int argc, char **argv) {

    psLibInit(NULL);

    psTimerStart(TIMER_TOTAL);

    // Parse the configuration and arguments
    // Open the input image(s)
    // Determine camera, format from header if not already defined
    // Construct camera in preparation for reading
    pmConfig *config = ppFocusArguments(argc, argv);
    if (config == NULL) {
        psErrorStackPrint(stderr, " ");
        exit(1);
    }

    // we search the argument data for the named fileset (argname)
    psArray *infiles = psMetadataLookupPtr(NULL, config->arguments, "INPUT");
    if (!infiles) {
        psTrace("pmFPAfile", 5, "Failed to find INPUT in argument list");
        exit(1);
    }

    // allocate vectors for analysis
    psVector *focus = psVectorAllocEmpty(infiles->n, PS_TYPE_F32);
    psVector *fwhm = psVectorAllocEmpty(infiles->n, PS_TYPE_F32);

    for (int i = 0; i < infiles->n; i++) {

        // define recipe options
        // define the active I/O files
        ppImageOptions *options = ppFocusParseCamera(config, i);
        if (options == NULL) {
            psErrorStackPrint(stderr, " ");
            exit(1);
        }

        // Image Arithmetic Loop
        // XXX ppFocus REQUIRES photom: for it to be true?
        //
        if (!ppImageLoop(config, options)) {
            psErrorStackPrint(stderr, " ");
            exit(1);
        }

        // determine FWHM at reference location in image
        // (also removes PPIMAGE.INPUT from config->files)
        ppFocusGetFWHM (config, focus, fwhm);

        ppFocusDropCamera (config);
        psFree (options);
    }

    ppFocusFitFWHM (config, focus, fwhm);

    psLogMsg ("ppFocus", 3, "complete ppFocus run: %f sec\n", psTimerMark (TIMER_TOTAL));

    // Cleaning up
    psFree (focus);
    psFree (fwhm);
    ppImageCleanup(config, NULL);
    return EXIT_SUCCESS;
}

// ppFocus is a lot like ppImage, but with a few important differences:
// - the input list is a set of independent images (not multiple files for a single image)
// - each pass to ppImageLoop performs the analysis on a different pmFPAfile
// - after each ppImageLoop, grap the input pmFPAfile and extract the FWHM stats
