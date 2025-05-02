/** @file pswarp.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.15 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-11 23:27:58 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "pswarp.h"

int main (int argc, char **argv)
{
    psTimerStart("pswarp");

    psLibInit(NULL);
    pmModelClassInit();
    psphotInit();

    pmConfig *config = pswarpArguments(argc, argv);
    if (!config) {
        pswarpCleanup(config, NULL);
    }

    pswarpStatsFile *statsFile = pswarpStatsFileOpen (config);

    pswarpVersionPrint();

    // load identify the data sources
    if (!pswarpParseCamera(config)) {
        pswarpCleanup(config, statsFile);
    }

    if (!pswarpOptions(config)) {
        pswarpCleanup(config, statsFile);
    }

    // load the input & output astrometry, find the output overlaps, generate the output pixels
    if (!pswarpDefineLayout(config)) {
        pswarpCleanup(config, statsFile);
    }

    // load input pixels & and warp
    if (!pswarpLoop(config, statsFile->md)) {
        pswarpCleanup(config, statsFile);
    }

    // load input pixels & and warp
    if (!pswarpLoopBackground(config, statsFile->md)) {
	pswarpCleanup(config, statsFile);
    }

    // output and free 
    // NOTE: pswarpCleanup calls exit
    psLogMsg("pswarp", PS_LOG_INFO, "complete pswarp run: %f sec\n", psTimerMark("pswarp"));
    pswarpCleanup(config, statsFile);
}
