#ifndef PSPHOT_STATS_FILE_H
#define PSPHOT_STATS_FILE_H


typedef struct {
    FILE *f;                            // File stream for statistics
    char *name;                         // Filename for statistics
    psMetadata *md;                     // Container for statistics
} psphotStatsFile;
    
psphotStatsFile *psphotStatsFileOpen (pmConfig  *config);
psphotStatsFile *psphotStatsFileGet ();
bool            psphotStatsFileSave (pmConfig *config, psphotStatsFile *statsFile);
void            psphotStatsFileSetQuality (int quality);

#endif
