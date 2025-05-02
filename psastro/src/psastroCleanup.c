/** @file psastroCleanup.c
 *
 *  @brief
 *
 *  @ingroup psastro
 *
 *  @author IfA
 *  @version $Revision: 1.8.2.1 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-19 17:59:50 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"

void psastroCleanup (pmConfig *config) {

    psFree (config);
    pmVisualClose ();
    pmVisualCleanup ();

    psTimerStop ();
    psMemCheckCorruption (stderr, true);
    pmModelClassCleanup ();
    psTimeFinalize ();
    pmConceptsDone ();
    pmConfigDone ();
    fprintf (stderr, "found %d leaks at %s\n", psMemCheckLeaks (0, NULL, stdout, false), "psastro");
    // fprintf (stderr, "found %d leaks at %s\n", psMemCheckLeaks (0, NULL, NULL, false), "psastro");

    return;
}
