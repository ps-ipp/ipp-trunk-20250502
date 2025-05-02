/** @file pswarpStatsFile.c
 *
 *  @brief functions for managing the stats file (or return NULL, or exit with an error)
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.38 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-13 21:54:32 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "pswarp.h"

static void pswarpStatsFileFree (pswarpStatsFile *statsFile) {
    psFree (statsFile->name);
    psFree (statsFile->md);
}

pswarpStatsFile *pswarpStatsFileAlloc () {

    pswarpStatsFile *statsFile = psAlloc(sizeof(pswarpStatsFile));
    psMemSetDeallocator(statsFile, (psFreeFunc)pswarpStatsFileFree);

    statsFile->f = NULL;
    statsFile->name = NULL;
    statsFile->md = NULL;

    return statsFile;
}

// Open the statistics file
pswarpStatsFile *pswarpStatsFileOpen (pmConfig *config) {

    bool mdok;

    char *name = psMetadataLookupStr(&mdok, config->arguments, "STATS"); ///< Filename for statistics

    pswarpStatsFile *statsFile = pswarpStatsFileAlloc ();
    statsFile->name = psMemIncrRefCounter(name);

    if (!mdok) return statsFile; // XXX this is probably a config error, but treat as ok for now
    if (!name) return statsFile;
    if (strlen(name) == 0) return statsFile;

    psString resolved = pmConfigConvertFilename(name, config, true, true);

    statsFile->f = fopen(resolved, "w");
    if (!statsFile->f) {
	psError(PS_ERR_IO, true, "Unable to open statistics file %s for writing.\n", resolved);
	psFree(resolved);
        pswarpCleanup(config, statsFile);
    }
    psFree(resolved);

    statsFile->md = psMetadataAlloc();
    psMetadataAddS32(statsFile->md, PS_LIST_TAIL, "QUALITY", 0, "No problems", 0);

    return statsFile;
}

bool pswarpStatsFileSave (pmConfig *config, pswarpStatsFile *statsFile) {

    if (!statsFile) return true;
    
    if (!statsFile->md) {
      psFree (statsFile);
      return true;
    }

    // Write out summary statistics
    psMetadataAddF32(statsFile->md, PS_LIST_TAIL, "DT_WARP", 0, "Time for warp completion", psTimerMark("pswarp"));

    // convert the psMetadata to a string block
    const char *statsMDC = psMetadataConfigFormat(statsFile->md);
    if (!statsMDC) {
	psError(psErrorCodeLast(), false, "Unable to get statistics file.");
	return false;
    }

    if (fprintf(statsFile->f, "%s", statsMDC) != strlen(statsMDC)) {
	psError(PSWARP_ERR_IO, true, "Unable to write statistics file.");
	return false;
    }
    psFree(statsMDC);

    if (fclose(statsFile->f) == EOF) {
	psError(PSWARP_ERR_IO, true, "Unable to close statistics file.");
	statsFile->f = NULL;
	return false;
    }
    statsFile->f = NULL;

    pmConfigRunFilenameAddWrite(config, "STATS", statsFile->name);
    psFree(statsFile);

    return true;
}

