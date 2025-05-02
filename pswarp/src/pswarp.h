/** @file pswarp.h
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.18 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 03:10:36 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>  // for strcasecmp
#include <unistd.h>   // for unlink
#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>
#include <ppStats.h>

#define THREADED 1

#include "pswarpErrorCodes.h"
#define PSWARP_RECIPE  "PSWARP" ///< Name of the recipe to use
#define PSASTRO_RECIPE "PSASTRO" ///< Name of the recipe to use

#define PSWARP_ANALYSIS_GOODPIX     "PSWARP.GOODPIX" ///< Name for number of good pixels in analysis metadata
#define PSWARP_ANALYSIS_COVARIANCES "PSWARP.COVARIANCES" ///< Name for covariance matrices on analysis MD
#define PSWARP_ANALYSIS_JACOBIAN    "PSWARP.JACOBIAN" ///< Name for Jacobian on analysis MD
#define PSWARP_ANALYSIS_BACKMAPS    "PSWARP.BACKMAPS" ///< Name for array of backwarps maps on analysis MD
#define PSWARP_ANALYSIS_CHIPNAMES   "PSWARP.CHIPNAMES" ///< Name for array of input chipnames on analysis MD
#define PSWARP_ANALYSIS_CHIPREGIONS "PSWARP.CHIPREGIONS" ///< Name for array of input chipnames on analysis MD

/**
 * a single pswarpMap converts coordinates from one image to a second image
 * the linear model is only valid over a limited range of pixels
 */
typedef struct {
    double Xo, Xx, Xy;
    double Yo, Yx, Yy;
    int xo;
    int yo;
} pswarpMap;

/* the pswarpMapGrid carries a collection of pswarpMag structures representing
 * the local value of the pswarpMap at different locations in the image.
 */
typedef struct {
    pswarpMap ***maps;
    int nXpts, nYpts;                   ///< number of x,y samples in the grid
    int nXpix, nYpix;                   ///< x,y spacing in src image pixels of grid samples
    double xMin,  yMin;                 ///< coordinate of first grid sample
} pswarpMapGrid;

typedef struct {
    /** values which are common to all tilesa */
    pmReadout *input;
    pmReadout *output;
    pswarpMapGrid *grid;
    psImageInterpolation *interp;
    psImage *region;

    /** values which are needed to control the background model warping. */
    bool background_warping;
    double offset_x;
    double offset_y;
  
    /** input values for this tile */
    int gridX;
    int gridY;

    /** output values for this tile */
    long goodPixels;                    ///< Number of good pixels
    int xMin, xMax, yMin, yMax;         ///< Bounds of tile
    psKernel *covariance;               ///< Covariance matrix
    double jacobian;                    ///< (Square root of) local Jacobian
} pswarpTransformTileArgs;

typedef struct {
    FILE *f;				// File stream for statistics
    char *name;				// Filename for statistics
    psMetadata *md;			// Container for statistics
} pswarpStatsFile;

pswarpTransformTileArgs *pswarpTransformTileArgsAlloc(void);
bool pswarpTransformTile (pswarpTransformTileArgs *args);

pmConfig *pswarpArguments (int argc, char **argv);
bool pswarpOptions(pmConfig *config);
bool pswarpSetMaskBits (pmConfig *config);
bool pswarpParseCamera (pmConfig *config);
bool pswarpDefine (pmConfig *config);
bool pswarpDefineBackground (pmConfig *config);
bool pswarpLoop (pmConfig *config, psMetadata *stats);
bool pswarpLoopSkycell (pmConfig *config, psMetadata *stats);
bool pswarpLoopBackground (pmConfig *config, psMetadata *stats);
psExit pswarpExitCode(psExit exitValue);
bool pswarpTransformReadout (pmReadout *output, pmReadout *input, pmConfig *config, bool backgroundWarp);
bool pswarpTransformSources(pmReadout *output, pmReadout *input, pmConfig *config);

bool pswarpMatchRange (int *minX, int *minY, int *maxX, int *maxY, pmReadout *dest, pmReadout *src);

pswarpMap *pswarpMapAlloc (void);
pswarpMapGrid *pswarpMapGridAlloc (int Nx, int Ny);

pswarpMapGrid *pswarpMapGridFromImage (pmReadout *dest, pmReadout *src, int nXpix, int nYpix);
bool pswarpMapGridSetGrid (pswarpMapGrid *grid, int ix, int iy, int *gridX, int *gridY);
bool pswarpMapGridCoordRange (pswarpMapGrid *grid, int gridX, int gridY, psPlane *min, psPlane *max);
int pswarpMapGridNextGrid_X (pswarpMapGrid *grid, int gridX);
int pswarpMapGridNextGrid_Y (pswarpMapGrid *grid, int gridY);
double pswarpMapGridMaxError (pswarpMapGrid *grid);
bool pswarpMapApply (double *outX, double *outY, pswarpMap *map, double inX, double inY);
bool pswarpMapSetLocalModel (pswarpMap *map, pmReadout *dest, pmReadout *src, int ix, int iy);

bool pswarpDefineSkycell (pmFPAfile **outFile, pmConfig **outConfig, pmConfig *config,
                          const char *filename, const char *argname);

/// Get the range of lit pixels
bool pswarpPixelsLit(const pmReadout *readout, ///< Readout to inspect
                     psMetadata *stats, ///< Statistics to update with the result
                     const pmConfig *config ///< Configuration
    );

bool pswarpMaskStats(const pmReadout *readout,
		     psMetadata *stats,
		     const pmConfig *config);


/**
 * define threads for this program
 */
bool pswarpSetThreads (void);

/// Return software version
psString pswarpVersion(void);

/// Return software souce
psString pswarpSource(void);

/// Return long software version information
psString pswarpVersionLong(void);

/// Populate header with version information
bool pswarpVersionHeader(
    psMetadata *header                  ///< Header to populate
    );

/// Print version information
void pswarpVersionPrint(void);

/// Activate a list of files
///
/// File list must be NULL-terminated
void pswarpFileActivation(pmConfig *config, // Configuration
                          char **files, // Files to turn on/off
                          bool state   // Activation state
    );


// Run down the FPA hierarchy, checking files
bool pswarpIOChecksBefore(pmConfig *config // Configuration
    );

// Run up the FPA hierarchy, checking files
bool pswarpIOChecksAfter(pmConfig *config // Configuration
    );

pswarpStatsFile *pswarpStatsFileAlloc ();
pswarpStatsFile *pswarpStatsFileOpen (pmConfig *config);
bool pswarpStatsFileSave (pmConfig *config, pswarpStatsFile *statsFile);

// cleanup memory and exit
void pswarpCleanup (pmConfig *config, pswarpStatsFile *statsFile) PS_ATTR_NORETURN;

// load the astrometry header info and generate the tranformations 
bool pswarpDefineLayout (pmConfig *config);

// XXX function for testing
bool pswarpDumpOutput (pmConfig *config);

// structure to describe approximate bounds for each fpa element (eg, chip)
// P,Q are a locally linear projection
typedef struct {
    psVector *Pmin; 
    psVector *Pmax;
    psVector *Qmin;
    psVector *Qmax;
} pswarpBounds;

bool pswarpFindOverlap (pmFPA *input, pmFPA *output, pswarpBounds *src, pswarpBounds *tgt);
psProjection *pswarpLocalFrame (pmFPA *fpa);
pswarpBounds *pswarpMakeBounds (pmFPA *fpa, psProjection *frame);
bool pswarpBoundsAppend(pswarpBounds *bounds, float Pmin, float Pmax, float Qmin, float Qmax);
pswarpBounds *pswarpBoundsAlloc();

bool pswarpLoadAstrometry (pmFPAfile *target, pmFPAfile *astrom, pmConfig *config);

bool pswarpTransformToTarget (pmFPA *output, pmReadout *input, pmConfig *config, bool backgroundWarp);
bool pswarpMakePSF (pmConfig *config, pmFPAfile *output, psMetadata *stats);
bool pswarpUpdateStatistics (pmFPA *output, psMetadata *stats, pmFPA *input, pmFPA *astrom, pmConfig *config);
bool pswarpUpdateMetadata (pmFPA *output, pmFPA *skycell, pmFPA *input, pmFPA *astrom, pmConfig *config, bool fullImage);

// XXX functions in pswarpParseCamera

bool AddStringAsArray (psMetadata *md, char *string, char *name);

pmFPAfile *pswarpDefineInputFile(pmConfig *config,// Configuration
				 pmFPAfile *bind,    // File to which to bind, or NULL
				 char *filerule,     // Name of file rule
				 char *argname,      // Argument name
				 pmFPAfileType fileType // Type of file
  );

bool pswarpParseSingleInput (pmConfig *config);
bool pswarpParseMultiInput (pmConfig *config, psMetadata *fileListMD);

bool pswarpModifyChipAstrom (pmConfig *config, pmFPAview *view, pmChip *chip, pmFPAfile *astrom, bool bilevelAstrometry, double xBin, double yBin);
bool pswarpGetInputScales (double *xBin, double *yBin, pmConfig *config, pmFPAview *view, pmChip *chip);

// ppSubMaskSetInMetadata examines named mask values and set the bits for maskValue and
// markValue.  Ensures that the below-named mask values are set, and calculates the mask value
// to catch all of the mask values marked as 'bad'.  Supplies the fallback name if the primary
// name is not found, or the default values if the fallback name is not found.
bool pswarpMaskSetInMetadata(psImageMaskType *outMaskValue, // Value of MASK.VALUE, returned
                               psImageMaskType *outMarkValue, // Value of MARK.VALUE, returned
                               psMetadata *source  // Source of mask bits
  );

