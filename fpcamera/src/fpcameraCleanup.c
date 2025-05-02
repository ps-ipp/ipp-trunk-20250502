# include "fpcamera.h"

/* \brief this loop saves the photometry/astrometry data files */
void fpcameraCleanup (pmConfig *config, psMetadata *stats) {

    psMemCheckCorruption (stderr, true);

    psFree (stats);
    psFree (config);
    pmVisualClose ();
    pmVisualCleanup ();

    psTimerStop ();
    psphotVisualClose();

    pmModelClassCleanup ();
    psTimeFinalize ();
    pmConceptsDone ();
    pmConfigDone ();

    pmSourceFitSetDone ();
    psLibFinalize();

    fprintf (stderr, "found %d leaks at %s\n", psMemCheckLeaks (0, NULL, stdout, false), "fpcamera");
    return;
}
