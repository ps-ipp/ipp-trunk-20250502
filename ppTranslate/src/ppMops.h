#ifndef PP_MOPS_H
#define PP_MOPS_H

#include <pslib.h>
#include <pmSourceMasks.h>

#define IN_EXTNAME "SkyChip.psf"        // Extension name for data in input
// #define OBSERVATORY_CODE "F51"          // IAU Observatory Code
#define OUT_EXTNAME "MOPS_TRANSIENT_DETECTIONS" // Extension name for data in output
#define SOURCE_MASK (PM_SOURCE_MODE_FAIL | PM_SOURCE_MODE_BADPSF | PM_SOURCE_MODE_SATURATED | \
                     PM_SOURCE_MODE_CR_LIMIT | PM_SOURCE_MODE_SKY_FAILURE | PM_SOURCE_MODE_DEFECT) // Flags to exclude
//#define SOURCE_MASK2 (PM_SOURCE_MODE2_DIFF_WITH_DOUBLE | PM_SOURCE_MODE2_ON_SPIKE | PM_SOURCE_MODE2_ON_STARCORE | PM_SOURCE_MODE2_ON_BURNTOOL | PM_SOURCE_MODE2_ON_CONVPOOR) // Flags2 to exclude
#define SOURCE_MASK2 (PM_SOURCE_MODE2_DEFAULT) //Flags2 to exclude
//#define SOURCE_MASK2 (PM_SOURCE_MODE2_SATSTAR_PROFILE) //Flags2 to exclude

#define PS1_DV_FORMAT "PS1_DV%d"

// Configuration data
typedef struct {
  psArray *input;                     // Input filenames
  psString exp_name;                  // Exposure name
  psS64 exp_id;                       // Exposure identifier
  psS64 chip_id;                      // Chip stage identifier
  psS64 cam_id;                       // Camera stage identifier
  psS64 fake_id;                      // Fake stage identifier
  psS64 warp_id;                      // Warp stage identifier
  psS64 diff_id;                      // Diff stage identifier
  psString camera;                    // Camera name
  psString obscode;                   // observatory code (derived from camera)
  bool positive;                      // Sense of subtraction, T=positive, F=negative
  float zp, zpErr;                    // Magnitude zero point and error
  float rmsAstrom;                    // Astrometric solution RMS
  psString output;                    // Output filename
  psU16 version;                      // Version (for parameters)
  psString comment;                   // Comment associated with the first argument of the diff
  psString obsMode;                   // Observation mode
  psString difftype;                  // WW (Warp-Warp Diff) or WS (Warp-Stack Diff) or SW (Stack-Warp Diff)
  float sky;                          // Exposure avg sky background
  psString shutoutc;                  // Camera exposure shutter open (UTC) from DB 
} ppMopsArguments;

/// Parse arguments
ppMopsArguments *ppMopsArgumentsParse(int argc, char *argv[]);

typedef struct {
  psString component;                 // skycell_id for these detections
  psString raBoresight, decBoresight; // RA,Dec of telescope boresight
  psString filter;                    // Filter for exposure
  float airmass;                      // Airmass of exposure
  float exptime;                      // Exposure time
  double posangle;                    // Position angle
  double alt, az;                     // Telescope altitude and azimuth
  double mjd;                         // Modified Julian Date
  float seeing;                       // Seeing of exposure
  int   naxis1, naxis2;               // size of the image
  long num;                           // Number of detections
  long numGood;                       // Number of "good" detections
  psS64 diffSkyfileId;                // unique id for input skyfile
  psMetadata *table;                  // Columns from the input file (SkyChip.psf extension)
  psVector *x, *y;                    // Image coordinates
  psVector *ra, *dec;                 // Sky coordinates
  psVector *raErr, *decErr;           // Error in sky coordinates
  psVector *raExt, *decExt;           // Sky coordinates for RA_EXT and DEC_EXT
  psVector *raExtErr, *decExtErr;     // Error in sky coordinates for RA_EXT and DEC_EXT
  psVector *mask;                     // Mask for detections
  float platescale; 		      // Plate scale at centroid
  psString fpashutoutc;		      // FPA shutoutc 
  psString fpashutcutc;               // FPA shutcutc 
  psString fpashmdoutc;               // FPA shmdoutc 
  psString fpashmdcutc;               // FPA shmdcutc 
  psString refcat;                    // Reference catalog used
} ppMopsDetections;

ppMopsDetections *ppMopsDetectionsAlloc(void);

/// Copy a detection
bool ppMopsDetectionsCopySingle(ppMopsDetections *target, const ppMopsDetections *source, long index);

/// Purge the detections list of masked detections
bool ppMopsDetectionsPurge(ppMopsDetections *detections);

/// Read detections
psArray *ppMopsRead(ppMopsArguments *args);

/// Merge detections
// ppMopsDetections *ppMopsMerge(const psArray *detections);
bool ppMopsPurgeDuplicates(const psArray *detections);

/// Write detections
bool ppMopsWrite(const psArray *detections, const ppMopsArguments *args);

/// Get the version contained in EXTTYPE of the "SkyChip.psf" table:
/// @returns 1 if EXTTYPE of "SkyChip.psf" is PS1_DV1
/// @returns 2 if EXTTYPE of "SkyChip.psf" is PS1_DV2
/// @returns 3 if EXTTYPE of "SkyChip.psf" is PS1_DV3
/// @returns 0 otherwise
int ppMopsGetSkyChipPsfVersion(const psFits* fits);

#endif
