/** @file psastroMetaDataStats.c
 *
 *  @brief
 *
 *  @ingroup psastro
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

bool psastroMetadataStats (pmConfig *config, psMetadata *stats) {

    bool status;

    char *filename = psMetadataLookupStr (&status, config->arguments, "STATS");
    if (!filename) return true; // stats not requested

    // Extract statistics from the last output fpa
    pmFPAfile *output = psMetadataLookupPtr(&status, config->files, "PSASTRO.OUTPUT");

    if (!output) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find output file (PSASTRO.OUTPUT).");
        return false;
    }

    // extract stats for the complete fpa
    pmFPAview *view = pmFPAviewAlloc(0);

    // Entries in the PSASTRO.HEADER in the FPA don't get seen by ppStats
    psMetadata *header = psMetadataLookupMetadata(&status, output->fpa->analysis, "PSASTRO.HEADER");
    if (status && header) {
        psMetadataItemSupplement(&status, stats, header, "ZPT_OBS");
        psMetadataItemSupplement(&status, stats, header, "ZPT_ERR");
        psMetadataItemSupplement(&status, stats, header, "CERROR");
        psMetadataItemSupplement(&status, stats, header, "CERSTD");
        psMetadataItemSupplement(&status, stats, header, "NASTRO");
        psMetadataItemSupplement(&status, stats, header, "AST_R0");
        psMetadataItemSupplement(&status, stats, header, "AST_D0");
        psMetadataItemSupplement(&status, stats, header, "AST_T0");
        psMetadataItemSupplement(&status, stats, header, "AST_S0");
        psMetadataItemSupplement(&status, stats, header, "AST_RS");
        psMetadataItemSupplement(&status, stats, header, "AST_DS");
    }

    if (!ppStatsMetadata(stats, output->fpa, view, 0, config)) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate stats for image.");
        psFree(view);
        return false;
    }

    psFree(view);

    // if we did not request any specific stats, the structure is empty
    if (stats && stats->list->n == 0) {
        psWarning ("stats output specified, but no requested stats entries in headers");
        return true;
    }

    // convert the stats MDC to a string
    char *statsMDC = psMetadataConfigFormat(stats);
    if (!statsMDC || strlen(statsMDC) == 0) {
        psError(PS_ERR_IO, false, "Unable to serialize stats metadata.\n");
        return false;
    }

    // convert to a real UNIX filename
    psString resolved = pmConfigConvertFilename(filename, config, true, false); // Resolved filename
    if (!resolved) {
        psError(psErrorCodeLast(), false, "Unable to resolve statistics filename: %s", filename);
        psFree(statsMDC);
        return false;
    }
    FILE *statsFile = fopen (resolved, "w");
    if (!statsFile) {
        psError(PS_ERR_IO, true, "Unable to open statistics file %s for writing.\n", resolved);
        psFree(statsMDC);
        psFree(resolved);
        return false;
    }

    // write the stats MDC to a file
    if (fprintf(statsFile, "%s", statsMDC) != strlen(statsMDC)) {
        psError(PS_ERR_IO, false, "Unable to write statistics file %s", resolved);
        psFree(statsMDC);
        psFree(resolved);
        return false;
    }
    psFree(statsMDC);
    if (fclose(statsFile) == EOF) {
        psError(PS_ERR_IO, false, "Unable to write statistics file %s", resolved);
        psFree(resolved);
        return false;
    }

    psFree(resolved);

    pmConfigRunFilenameAddWrite(config, "STATS", filename);

    return true;
}
