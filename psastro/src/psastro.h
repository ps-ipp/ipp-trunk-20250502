/** @file psastro.h
 *
 *  @brief This file defines the library functions available to external
 *  programs.
 *
 *  It must be included by programs which are compiled against
 *  psphot functions.
 *
 *  @ingroup psastro
 *
 *  @author IfA
 *  @version $Revision: 1.49 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-09 21:25:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# ifndef PSASTRO_H
# define PSASTRO_H

# include "psastroErrorCodes.h"

/// @addtogroup psastro
/// @{

# define PSASTRO_RECIPE "PSASTRO" ///< Name of the recipe to use

# define psMemCopy(A)(psMemIncrRefCounter((A)))
# define DEG_RAD 57.295779513082322
# define RAD_DEG  0.017453292519943
# define SIGN(X)  (((X) == 0) ? 0 : ((fabs((double)(X))) / (X)))

#if 0
/**
 * this structure represents a fit to the logN / logS curve for a set of stars
 * logN = offset + slope * logS
 */
typedef struct {
    double mMin;                        ///< minimum magnitude bin with data
    double mMax;                        ///< maximum magnitude bin with data
    double offset;                      ///< fitted line offset
    double slope;                       ///< fitted line slope
    double mPeak;                       ///< mag of peak bin
    int nPeak;                          ///< # of stars in peak bin
    int sPeak;                          ///< sum of stars to peak bin
} pmLumFunc;
#endif

/**
 * this structure defines the parameters to describe a ghost
 */
typedef struct {
    psPlane *srcFP;			///< location in FPA coords of the source star
    psPlane *FP;			///< location in FPA coords of the ghost center
    psPlane *chip;			///< location in chip coords of the ghost center
    double Mag;				///< instrumental magnitude of source star
    psEllipseAxes inner;		///< inner elliptical annulus boundary
    psEllipseAxes outer;		///< outer elliptical annulus boundary
} psastroGhost;

psastroGhost     *psastroGhostAlloc (void);
bool              psastroLoadGhosts (pmConfig *config);

bool              psastroDataSave (pmConfig *config, psMetadata *stats);
bool              psastroDefineFiles (pmConfig *config, pmFPAfile *input);
bool              psastroDefineFile (pmConfig *config, pmFPA *input, char *filerule, char *argname, pmFPAfileType fileType, pmDetrendType detrendType);
bool              psastroAnalysis (pmConfig *config, psMetadata *stats);

bool              psastroConvertFPA (pmConfig *config, pmFPA *fpa, psMetadata *recipe);
bool              psastroConvertReadout (pmConfig *config, pmFPAview *view, pmReadout *readout, psMetadata *recipe);
bool              psastroCorrectKH (pmConfig *config, pmFPAview *view, pmReadout *readout, psMetadata *recipe, psArray *inStars);

psArray          *pmSourceToAstromObj (psArray *sources, float MagOffset);
bool              psastroAstromGuess (int *nStars, pmConfig *config);
bool              psastroAstromGuessCheck (pmConfig *config);
bool              psastroMaskUpdates (pmConfig *config, psMetadata *stats);

bool              psastroLoadGlints (pmConfig *config);
bool              psastroChooseGlintStars (pmConfig *config, psArray *refs, const char *source);

bool              psastroLoadCrosstalk (pmConfig *config);

psArray          *psastroLoadRefstars (pmConfig *config, const char *source);
bool              psastroChipAstrom (pmConfig *config);
bool              psastroOneChip (pmFPA *fpa, pmChip *chip, psArray *refset, psArray *rawset, psMetadata *recipe, psMetadata *updates);
bool              psastroOneChipGrid (pmFPA *fpa, pmChip *chip, psArray *refset, psArray *rawset, psMetadata *recipe, psMetadata *updates);
bool              psastroOneChipFit (pmFPA *fpa, pmChip *chip, pmReadout *readout, psArray *refset, psArray *rawset, psMetadata *recipe, psMetadata *updates);
bool              psastroChooseRefstars (pmConfig *config, psArray *refs, const char *source, bool saveExistingMatchedRefs);
bool              psastroRefstarSubset (pmReadout *readout);
pmLumFunc        *psastroLuminosityFunction (psArray *stars, pmLumFunc *rawFunc);
bool              psastroLuminosityFunctionPlot(psVector *lnMag, psVector *Mag, pmLumFunc *lumFunc, pmLumFunc *rawFunc);
psArray          *psastroRemoveClumps (psArray *input, int scale);
psArray          *psastroRemoveClumpsIterate (psArray *input, int scale, int nIter);
bool              psastroRemoveClumpsRawstars (pmConfig *config);

bool              psastroChipFailureHeader (psMetadata *updates);


// utility functions:
bool              psastroUpdateChipToFPA (pmFPA *fpa, pmChip *chip);
bool              psastroWriteStars (char *filename, psArray *sources);
bool              psastroWriteTransform (psPlaneTransform *map);

// mosaic fitting functions
bool              psastroMosaicDistortion (pmFPA *fpa, psMetadata *recipe, int pass);
psPlaneTransform *psastroMosaicFitRotAndScale (pmFPA *fpa);
bool              psastroMosaicApplyRotAndScale (pmFPA *fpa, psPlaneTransform *TPtoFP);
bool              psastroMosaicDistortionFromGradients (pmFPA *fpa, psMetadata *recipe);
bool              psastroMosaicCorrectDistortion (pmFPA *fpa, psPlaneTransform *TPtoFP);
bool              psastroMosaicCommonScale (pmFPA *fpa, psMetadata *recipe);
bool              psastroMosaicAstrom (pmConfig *config, psMetadata *stats);
bool              psastroMosaicChipAstrom (pmFPA *fpa, psMetadata *stats, psMetadata *recipe, int iteration);
bool              psastroMosaicSetMatch (pmFPA *fpa, psMetadata *recipe, int iteration);
bool              psastroMosaicSetAstrom (pmFPA *fpa);
bool              psastroMosaicHeaders (pmConfig *config);
bool              psastroMosaicRescaleChips (pmFPA *fpa);
bool              psastroMosaicOneChip (pmChip *chip, pmReadout *readout, psMetadata *recipe, psMetadata *updates, int iteration);

// Return version strings.
psString          psastroVersion(void);
psString          psastroSource(void);
psString          psastroVersionLong(void);
bool              psastroVersionHeader(psMetadata *header);
bool              psastroVersionHeaderFull(psMetadata *header);
void              psastroVersionPrint(void);

// demo plots
bool              psastroPlotRawstars (psArray *rawstars, pmFPA *fpa, pmChip *chip, psMetadata *recipe);
bool              psastroPlotRefstars (psArray *refstars, psMetadata *recipe);
bool              psastroPlotOneChipFit (psArray *rawstars, psArray *refstars, psArray *match, psMetadata *recipe);

bool              psastroDumpRawstars (psArray *rawstars, pmFPA *fpa, pmChip *chip);
bool              psastroDumpRefstars (psArray *refstars, char *filename);
bool              psastroDumpMatches (pmFPA *fpa, char *filename);
bool              psastroDumpStars (psArray *stars, char *filename);
bool              psastroDumpMatchedStars (char *filename, psArray *rawstars, psArray *refstars, psArray *match);
bool              psastroDumpGradients (psArray *gradients, char *filename);

bool              psastroMosaicSetAstrom_tmp (pmFPA *fpa);
int               psastroSortByMag (const void *a, const void *b);

bool              psastroFixChips (pmConfig *config, psMetadata *recipe);
bool              psastroFixChipsTest (pmConfig *config, psMetadata *recipe);
bool              psastroUseModel (pmConfig *config, psMetadata *recipe);
bool              psastroDumpCorners (char *filenameU, char *filenameD, pmFPA *fpa);

char             *psastroSetMagLimit (float *minMag, float *maxRho, pmConfig *config, const char *source);

psArray          *psastroReadGetstarCatalog (psFits *fits);
psArray          *psastroReadGetstar_PS1_DEV_0 (psFits *fits);

bool              psastroAstromGuessSetChip (pmFPA *fpa, pmChip *chip, const pmFPAview *view, double pixelScale, bool bilevelAstrometry);
bool              psastroAstromGuessSetFPA (pmFPA *fpa, bool *bilevelAstrometry);
bool              psastroMetadataStats (pmConfig *config, psMetadata *stats);

bool 		  psastroZeroPoint (pmConfig *config);
psVector         *psastroZeroPointReadoutAccum(psVector *dMag, pmReadout *readout, float exptime, float refminMag, float refmaxMag);
bool              psastroZeroPointAnalysis (psMetadata *header, psVector *dMag, float zeropt, psMetadata *recipe);
bool 		  psastroZeroPointFromRecipe (float *zeropt, float *exptime, float *ghostMaxMag, double *glintMaxMag, pmFPA *fpa, psMetadata *recipe);
bool              psastroZeroPointRefMagLimitFromRecipe (float *refminMag, float *refmaxMag, pmFPA *fpa, psMetadata *recipe);

psStats          *psastroStatsPercentile (psVector *myVector, psMetadata *recipe);
float             psastroStatsPercentileValue (psHistogram *histogram, psHistogram *cumulative, psVector *myVector, float flimit);

// masking functions
pmCell           *pmCellInChip (pmChip *chip, float x, float y);
bool 		  pmCellCoordsForChip (float *xCell, float *yCell, pmCell *cell, float xChip, float yChip);
bool 		  pmChipCoordsForCell (float *xChip, float *yChip, pmCell *cell, float xCell, float yCell);
		  
bool 		  psastroMaskCircle (psImage *mask, psImageMaskType value, float x0, float y0, float dX, float dY);
bool 		  psastroMaskEllipse (psImage *mask, psImageMaskType value, float x0, float y0, psEllipseAxes axes);
bool 		  psastroMaskEllipticalAnnulus (psImage *mask, psImageMaskType value, float x0, float y0, psEllipseAxes eInner, psEllipseAxes eOuter);
		  
bool 		  psastroMaskBox (psImage *mask, psImageMaskType value, float x0, float y0, float dL, float dW, float theta);
void 		  psastroMaskLine (psImage *mask, psImageMaskType value, double x1, double y1, double x2, double y2, int dW);
void 		  psastroMaskLineBresen (psImage *mask, psImageMaskType value, int X1, int Y1, int X2, int Y2, int dW, int swapcoords);
void 		  psastroMaskRectangle (psImage *mask, psImageMaskType value, int x0, int y0, int x1, int y1);

pmChip           *psastroFindChip (double *xChip, double *yChip, pmFPA *fpa, double xFPA, double yFPA);
bool 		  psastroChipBounds (pmFPA *fpa);
bool              psastroFindChipInXrange (pmFPA *fpa, int nChip, double xFPA, double yFPA);
bool              psastroFindChipInYrange (pmFPA *fpa, int nChip, double xFPA, double yFPA);
bool 		  psastroFindChipYedges (double *yFPAs, double *yFPAe, pmFPA *fpa, int nChip);
bool 		  psastroFindChipXedges (double *yFPAs, double *yFPAe, pmFPA *fpa, int nChip);
bool 		  psastroFPAtoChip (double *xChip, double *yChip, pmFPA *fpa, int nChip, double xFPA, double yFPA);

//bool              psastroMaskStats(pmConfig *config, psMetadata *stats);

// psastroExtract functions
bool 		  psastroExtractAnalysis (pmConfig *config);
bool 		  psastroExtractStars (pmConfig *config);
bool 		  psastroExtractGhosts (pmConfig *config);

psImage          *psastroExtractStar (psImage *input, double x, double y, double dX, double dY);
bool 		  psastroExtractParseCamera (pmConfig *config);
bool 		  psastroExtractDataLoad (pmConfig *config);
pmConfig         *psastroExtractArguments (int argc, char **argv);
bool              psastroExtractFreeChipBounds(void);

bool              psastroGalaxyShapeErrors (psMetadata *recipe, pmReadout *readout);

///@}
# endif /* PSASTRO_H */
