# include "fpcamera.h"

# define ESCAPE(ERROR,...) { p_psError(__FILE__,__LINE__,__func__,ERROR,false,__VA_ARGS__); psFree (statsMDC); psFree(view); psFree(resolved); return false; }

bool fpcameraMetadataStats (pmConfig *config, psMetadata *stats) {

    bool status;
    char *statsMDC = NULL;
    pmFPAview *view = NULL;
    psString resolved = NULL;

    char *filename = psMetadataLookupStr (&status, config->arguments, "STATS");
    if (!filename) return true; // stats not requested

    // Extract statistics from the output fpa
    pmFPAfile *output = psMetadataLookupPtr(&status, config->files, "FPCAMERA.OUTPUT");
    if (!output) ESCAPE (PS_ERR_UNEXPECTED_NULL, "Unable to find output file (FPCAMERA.OUTPUT).");

    // extract stats for the complete fpa
    view = pmFPAviewAlloc(0);

    // Entries in the FPCAMERA.HEADER in the FPA don't get seen by ppStats
    psMetadata *header = psMetadataLookupMetadata(&status, output->fpa->analysis, "FPCAMERA.HEADER");
    if (status && header) {
        psMetadataItemSupplement(&status, stats, header, "ZPT_OBS"); // XXX not yet calculated
        psMetadataItemSupplement(&status, stats, header, "ZPT_ERR");
    }

    if (!ppStatsMetadata(stats, output->fpa, view, 0, config)) ESCAPE (PS_ERR_UNEXPECTED_NULL, "Unable to generate stats for image.");

    // if we did not request any specific stats, the structure is empty
    if (stats && stats->list->n == 0) {
        psWarning ("stats output specified, but no requested stats entries in headers");
        return true;
    }

    // convert the stats MDC to a string
    statsMDC = psMetadataConfigFormat(stats);
    if (!statsMDC || strlen(statsMDC) == 0) ESCAPE(PS_ERR_IO, "Unable to serialize stats metadata.\n");

    // convert to a real UNIX filename
    resolved = pmConfigConvertFilename(filename, config, true, false); // Resolved filename
    if (!resolved) ESCAPE(psErrorCodeLast(), "Unable to resolve statistics filename: %s", filename);

    FILE *statsFile = fopen (resolved, "w");
    if (!statsFile) ESCAPE (PS_ERR_IO, "Unable to open statistics file %s for writing.\n", resolved);

    // write the stats MDC to a file
    if (fprintf(statsFile, "%s", statsMDC) != strlen(statsMDC)) ESCAPE (PS_ERR_IO, "Unable to write statistics file %s", resolved);
    if (fclose(statsFile) == EOF) ESCAPE (PS_ERR_IO, "Unable to write statistics file %s", resolved);

    pmConfigRunFilenameAddWrite(config, "STATS", filename);

    psFree(resolved);
    psFree(statsMDC);
    psFree(view);

    return true;
}
