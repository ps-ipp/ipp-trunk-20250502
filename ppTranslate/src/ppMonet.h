#ifndef PP_MOPS_H
#define PP_MOPS_H

#include <pslib.h>

#define EXT_TYPE ".psf"                 // Extension type to consider

// Configuration data
typedef struct {
    psString input;                     // Input filenames
    psString exp_name;                  // Exposure name
    psS64 exp_id;                       // Exposure identifier
    psS64 chip_id;                      // Chip stage identifier
    psS64 cam_id;                       // Camera stage identifier
    float zp, zpErr;                    // Magnitude zero point and error
    float rmsAstrom;                    // Astrometric solution RMS
    psString output;                    // Output filename
} ppMonetArguments;

/// Parse arguments
ppMonetArguments *ppMonetArgumentsParse(int argc, char *argv[]);

#endif
