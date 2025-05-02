#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "ppImage.h"

void ppImageCleanup (pmConfig *config, ppImageOptions *options)
{
    if (psErrorCodeLast() != PS_ERR_NONE) {
        pmFPAfileFreeSetStrict(false);
    }

    // Free memory used by ppImage
    psFree(options);
    psFree(config);

    // Free memory used by psModules
    pmSourceFitSetDone ();
    pmConceptsDone();
    pmConfigDone();
    pmModelClassCleanup();
    pmVisualCleanup ();

    // Free memory used by psLib
    psLibFinalize();

    // psMemBlock **memblocks;
    // int Nleaks = psMemCheckLeaks (0, &memblocks, stderr, false);
    // fprintf (stderr, "Found %d leaks at %s\n", Nleaks, "ppImage");

    // fprintf(stderr, "Found %d leaks at %s\n", psMemCheckLeaks (0, NULL, NULL, false), "ppImage");
    psLogMsg("ppImage", PS_LOG_INFO, "Memory leaks: %d\n", psMemCheckLeaks(0, NULL, stdout, false));

    return;
}
