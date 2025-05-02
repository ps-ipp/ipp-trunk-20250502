
#ifndef PP_STATS_H
#define PP_STATS_H

#define PPSTATS_RECIPE "PPSTATS"
#define PPSTATS_MD_RECIPE "PPSTATS_METADATA"

typedef struct {
    // Inputs
    psFits *fits;                       // Input file handle
    pmFPA *fpa;                         // FPA to analyse
    pmFPAview *view;                    // View to analyse
    // Stuff to output
    psStats *stats;                     // pixel Statistics to calculate
    bool doStats;                       // Do pixel statistics?
    bool fileLevel;                     // Output file level?
    bool showFormat;                    // Output file format?
    bool showCamera;                    // Output camera name?
    pmFPAview *fileView;                // View to analyse

    psList *headers;                    // Headers to read
    psList *concepts;                   // Concepts to read
    psList *analysis;                   // Analysis entries to read
    psList *summary;                    // Summary statistics to calculate
    // Options for input data
    bool doFirstReadout3D;              // for 3D data, use the first readout?
    float sample;                       // Fraction of cell to sample for statistics
    psImageMaskType maskVal;            // Mask value for images
    psList *chips;                      // Chips to look at
    psList *cells;                      // Cells to look at
} ppStatsData;

typedef struct {
    char *keyword;
    psDataType type;
    char *statistic;
    char *flag;
    psMetadataItem *value;
    psVector *vector;
} ppStatsEntry;

// Allocator
ppStatsData *ppStatsDataAlloc(void);

/// Perform the ppStats steps on the given FPA (optionally for specified view)
psMetadata *ppStatsFPA(psMetadata *out,
                       pmFPA *fpa,         // FPA for which to get statistics
                       pmFPAview *view,    // View for analysis
                       psImageMaskType maskVal, // Value to mask
                       pmConfig *config    // Configuration
    );

psExit ppStatsChip(psMetadata *fpaResults, // Metadata holding the fpa results
                   pmChip *chip,     // Chip for which to get statistics
                   psFits *fits,     // FITS file handle
                   pmFPAview *view,  // View for analysis
                   ppStatsData *data,// The data
                   pmConfig *config // Configuration
    );

psExit ppStatsCell(psMetadata *chipResults, // Metadata holding the chip results
                   pmCell *cell,     // Cell for which to get statistics
                   psFits *fits,     // FITS file handle
                   pmFPAview *view,  // View for analysis
                   ppStatsData *data,// The data
                   pmConfig *config  // Configuration
    );

/// Supplement the statistics with the fringe solution
bool ppStatsFringe(psMetadata *stats,     ///< Statistics metadata to supplement
                   const pmChip *chip,    ///< The chip containing the solution
                   const char *root,      ///< Name of output entry
                   const char *fringeName ///< Name of the solution in the chip->analysis
    );


// measure only the pixel-related statistics (stats, summary)
psMetadata *ppStatsPixels(psMetadata *out,
                          pmFPA *fpa,         // FPA for which to get statistics
                          pmFPAview *view,    // View for analysis
                          psImageMaskType maskVal, // Value to mask
                          pmConfig *config    // Configuration
    );

// measure only the non-pixel-related statistics (headers, concepts)
psMetadata *ppStatsMetadata(psMetadata *out,
                            pmFPA *fpa,         // FPA for which to get statistics
                            pmFPAview *view,    // View for analysis
                            psImageMaskType maskVal, // Value to mask
                            pmConfig *config    // Configuration
    );

/// Loop over the input image and do all the hard work
psMetadata *ppStatsLoop(psExit *result,
                        ppStatsData *data, // The data
                        pmConfig *config // Configuration
    );

/// Set up the options and input/output files
ppStatsData *ppStatsSetupFromArgs(int *argc, char *argv[], // Command-line arguments
                                  pmConfig *config // Configuration
    );

bool ppStatsSetupFromRecipe(ppStatsData *data, // Data for running ppStats
                            pmConfig *config // Configuration
    );


/// Return short version information
psString ppStatsVersion(void);

/// Return source information
psString ppStatsSource(void);

/// Return long version information
psString ppStatsVersionLong(void);

/// Populate header with version information
bool ppStatsVersionHeader(
    psMetadata *header                  ///< Header to populate
    );


void p_ppStatsGetMetadata(psMetadata *target, // Target for metadata
                          psMetadata *source, // Source for metadata
                          psList *list    // List containing keywords
    );

void p_ppStatsGetAnalysis(psMetadata *target, // Output Target for metadata
                          psList *headers,    // List containing desired keywords
                          psMetadata *source, // Input Source for metadata
                          psList *list        // List containing analysis blocks
    );

bool p_ppStatsDoThis(psList *toDoList,    // List of things to do
                     const char *this     // The name of "this"
    );

void p_ppStatsAddToHierarchy(psMetadata *source, // Source to add
                             psMetadata *target, // Target to which to add
                             const char *name, // Name of source
                             const char *comment // Comment for source
    );

psExit ppStatsReadout(psMetadata *cellResults, // Metadata holding the chip results
                      pmReadout *readout,       // Cell for which to get statistics
                      int nReadout,     // readout number
                      ppStatsData *data,        // The data
                      const pmConfig *config // Configuration
    );

// used by ppStatsFromMetadata:
psDataType psDataTypeFromString(char *typename);
ppStatsEntry *ppStatsEntryAlloc(void);

psArray *ppStatsFromMetadataEntries (psMetadata *recipe);
bool ppStatsFromMetadataParse (psMetadata *input, psArray *entries);
bool ppStatsFromMetadataStats (psArray *entries);
bool ppStatsFromMetadataPrint (psArray *entries, char *filename);

#endif
