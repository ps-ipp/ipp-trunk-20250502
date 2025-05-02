/** @file psphotStatsFile.c
 *
 *  @brief functions for managing the stats file (or return NULL, or exit with an error)
 *  @ingroup psphot
 *
 *  @author IfA
 *  @version $Revision: 1.38 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-13 21:54:32 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "psphotStandAlone.h"

static psphotStatsFile *theStatsFile = NULL;

static void psphotStatsFileFree (psphotStatsFile *statsFile) {
    psFree (statsFile->name);
    psFree (statsFile->md);
    theStatsFile = NULL;
}

psphotStatsFile *psphotStatsFileAlloc () {

    psphotStatsFile *statsFile = psAlloc(sizeof(psphotStatsFile));
    psMemSetDeallocator(statsFile, (psFreeFunc)psphotStatsFileFree);

    statsFile->f = NULL;
    statsFile->name = NULL;
    statsFile->md = NULL;

    return statsFile;
}

// Open the statistics file
psphotStatsFile *psphotStatsFileOpen (pmConfig *config) {

    bool mdok;

    char *name = psMetadataLookupStr(&mdok, config->arguments, "STATS"); ///< Filename for statistics

    if (!name) return NULL;

    psphotStatsFile *statsFile = psphotStatsFileAlloc ();
    statsFile->name = psMemIncrRefCounter(name);

    if (!mdok) return statsFile; // XXX this is probably a config error, but treat as ok for now
    if (!name) return statsFile;
    if (strlen(name) == 0) return statsFile;

    psString resolved = pmConfigConvertFilename(name, config, true, true);

    statsFile->f = fopen(resolved, "w");
    if (!statsFile->f) {
	psError(PS_ERR_IO, true, "Unable to open statistics file %s for writing.\n", resolved);
	psFree(resolved);
    }
    psFree(resolved);

    statsFile->md = psMetadataAlloc();
    psMetadataAddS32(statsFile->md, PS_LIST_TAIL, "QUALITY", 0, "No problems", 0);

    // the global copy
    theStatsFile = statsFile;

    return statsFile;
}

bool psphotStatsFileSave (pmConfig *config, psphotStatsFile *statsFile) {

    if (!statsFile) return true;
    if (!statsFile->md) return true;

    // Write out summary statistics
//    psMetadataAddF32(statsFile->md, PS_LIST_TAIL, "DT_WARP", 0, "Time for warp completion", psTimerMark("psphot"));

    // convert the psMetadata to a string block
    const char *statsMDC = psMetadataConfigFormat(statsFile->md);
    if (!statsMDC) {
	psError(psErrorCodeLast(), false, "Unable to get statistics file.");
	return false;
    }

    if (fprintf(statsFile->f, "%s", statsMDC) != strlen(statsMDC)) {
	psError(PSPHOT_ERR_IO, true, "Unable to write statistics file.");
	return false;
    }
    psFree(statsMDC);

    if (fclose(statsFile->f) == EOF) {
	psError(PSPHOT_ERR_IO, true, "Unable to close statistics file.");
	statsFile->f = NULL;
	return false;
    }
    statsFile->f = NULL;

    pmConfigRunFilenameAddWrite(config, "STATS", statsFile->name);
    psFree(statsFile);

    return true;
}

psphotStatsFile *psphotStatsFileGet () {
    return theStatsFile;
}

void psphotStatsFileSetQuality (int quality) {
    psphotStatsFile *statsFile = psphotStatsFileGet();
    if (!statsFile) return;

    psMetadataAddS32(statsFile->md, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE, "quality value", quality);
}
