/*
 * @file pmAstrometryVisual.h
 * @author Chris Beaumont, IfA
 * @brief A set of functions to display visual diagnostics from psastro
 * Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_ASTROM_VISUAL_H
#define PM_ASTROM_VISUAL_H


/** A fit to the logN / logS curve for a set of stars
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


/** Close plotting windows at the end of a run
 * @return true for success */
bool pmAstromVisualClose(void);


/**
 * Plot the offset between every pair of reference and raw source locations. The peak of this
 * distribution nominally gives the offset, scale difference, and rotation of the two catalogs.
 * Overplots the location of this peak as determined by pmAstromGridAngle, as well as some profiles
 * along horizontal and vertical cuts through this peak.
 */
bool pmAstromVisualPlotGridMatch (const psArray *raw, ///< raw stars
                                  const psArray *ref, ///< reference stars
                                  psImage *gridNP,    ///< a 2D histogram of raw-ref star distances
                                  double offsetX,     ///< The X location (FP coordinates) of the peak of gridNP
                                  double offsetY,     ///< the Y location (FP coordinates) of the peak of gridNP
                                  double maxOffpix,   ///< The half-width of gridNP in FP coordinates
                                  double Scale,       ///< The pixel size of gridNP in histogram-bin-coordinates
                                  double Offset       ///< The (x,y) location (histogram-bin coordinates) of the FP point (0,0) in gridNP
                                  );


bool pmAstromVisualPlotGridMatchOverlay (const psArray *raw,
					 const psArray *ref,
					 const psPlane offset);

/**
 * Plot the refinements made within pmAstromGridTweak.
 * After pmAstromGridMatch finds the best rotaion/scale/offset between raw and reference stars
 * within a coarse grid of rotations/scales, pmAstromGridTweak computes a higher precision
 * estimate of the offset. It computes the 2 point correlation function between raw and ref
 * stars along horizontal and vertical cuts through the first-guess offset. It finds the peak
 * of these two profiles and adjusts the offset accordingly. This procedure plots the profiles.
 */
bool pmAstromVisualPlotTweak (psVector *xHist, ///< Smoothed Horizontal cut through the histogram
                              psVector *yHist, ///< Smoothed Vertical cut throug the histogram
                              int xBin,        ///< X Bin index of the histogram peak
                              int yBin         ///< Y bin index of the histogram peak
                              );


/**
 * Plot the two luminosity functions created within psastroRefStarSubset
 * The luminosity functions are used to select a subset of reference stars,
 * so we plot the cutoff that defines this subset
 */
bool pmAstromVisualPlotLuminosityFunction (psVector *lnMag,   ///< Log(n) for each magnitude bin
                                          psVector *Mag,     ///< magnitude bins
                                          pmLumFunc *lumFunc,///< Fit to the reference star luminosity function
                                          pmLumFunc *rawFunc ///< Fit to the raw star luminoisty function
                                           );


/**
 * Plot raw stars as determined from first pass astrometry fit
 * Called within psastroAstromGeuss
 */
bool pmAstromVisualPlotRawStars (psArray *rawstars, ///< Stars detected in the fpa
                                 pmFPA *fpa,  ///< structure describing the focal plane array
                                 pmChip *chip,  ///< structure describing the chip
                                 psMetadata *recipe ///< the recipe used in psastro
                                 );


/**
 * plot the location of references stars over the entire fpa
 * invoked during psastroChooseRefStars
 */
bool pmAstromVisualPlotRefStars (psArray *refstars, psMetadata *recipe);


/**
 * Plot the stars in a region, and indicate which stars are part of 'clumps'
 * These stars are flagged during astrometric fitting, since dense regions are
 * harder to cross-match than sparse ones. Called during psastroRemoveClumps.
 */
bool pmAstromVisualPlotRemoveClumps (psArray *input, ///< Array containing the field stars
                                    psImage *count, ///< A 2D histogram of the field star distribution
                                    int scale,      ///< The pixel size of the histogram
                                    float limit     ///< The minimum numuber of stars in a bin flagged as a clump
                                     );

/**
 * Plots the chip corners in the FP before and after chips with inconsistent solutions have been fixed.
 * Invoked during psastroFixChips
 */
bool pmAstromVisualPlotFixChips (pmFPAfile *input, ///< focal plane array file
                                 psVector *xOld, ///< old X location of chip cornerss
                                 psVector *yOld ///< old Y location of chip corners
                                 );


/**
 * Assess the goodness of fit for a signle chip by
 * plotting the fit residuals
 * invoked during psastroOneChipFit
 */
bool pmAstromVisualPlotOneChipFit (psArray *rawstars, ///< stars detected in the image
                                  psArray *refstars, ///< reference stars over the same region
                                  psArray *match,    ///< contains which rawstars match to which refstars
                                  psMetadata *recipe ///< data reduction recipe
                                   );

/**
 *  Plots the fpa chip corners projected on to the tangential plane before and after
 *  the astrometry solution has been applied. In psastroAstromGuessCheck, the old corners
 *  are then fit to the new corners to get a sense at how far off the initial WCS info was
 *  in offset, rotation, and scale. This procedure also plots the residuals of the fit from
 *  old to new coordinates
 */
bool pmAstromVisualPlotAstromGuessCheck (psVector *cornerPo, ///< P coordinates of chip corners before fitting
                                        psVector *cornerQo, ///< Q coordinates of chip corners before fitting
                                        psVector *cornerPn, ///< P coordinates of chip corners after fitting
                                        psVector *cornerQn, ///< Q coordinates of chip corners after fitting
                                        psVector *cornerPd, ///< P coordinate residuals of fit from old to new coordinates
                                        psVector *cornerQd  ///< Q coordinate residuals of fit from old to new coordinates
                                         );


/**
 *   plot the residuals between raw stars and ref stars after
 *   fitting in psastroMosaicOneChip
 */
bool pmAstromVisualPlotMosaicOneChip (psArray *rawstars, psArray *refstars, psArray *match, psMetadata *recipe) ;


/**
 * Plots the pixel scales of the fpa before they are
 * equalized in psastroMosaicCommonScale
 */
bool pmAstromVisualPlotCommonScale (pmFPA *fpa,         ///< the fpa
                                   psVector *oldScale  ///< the old pixel scale of each chip in the fpa
                                    );


/** pmAstromVisualPlotMosaicMatches
 * Plot the matches between raw and reference stars during pmAstromVisualMosaicSetMatch
 */
bool pmAstromVisualPlotMosaicMatches (psArray *rawstars, psArray *refstars, psArray *match, int iteration, psMetadata *recipe);


#endif
