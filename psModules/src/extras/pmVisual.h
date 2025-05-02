/* @file pmVisual.h
 * @brief functions to create visual diagnostics with the help of 'kapa'
 * @author Chris Beaumont, IfA
 *
 * Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_VISUAL_H
#define PM_VISUAL_H

#if (HAVE_KAPA)


/** Globally enable or disable plotting
 * @param value - true to enable plotting
 * @return true
 */
bool pmVisualSetVisual(bool value);


/** Check whether plotting is enabled
 * @retrun true if plots should be generated
 */
bool pmVisualIsVisual(void);


/** Destroy plotting windows at the end of a run
 * @return true for success */
bool pmVisualClose(void);


/** Open, name, and resize a window for plotting.
 * @param kapid an identifier for this window. Initialize to -1. Its value will be updated.
 * @param name What to name the window. Seems not to like spaces.
 * @return true for successful completion
*/
bool pmVisualInitWindow (int *kapid, char *name);


/** Initialize a graph, set the appropriate section and font.
 * @param kapa the index of the kapa window to plot to
 * @param section the section to use
 * @graphdata to use
 * @return true for successful completion
 */
bool pmVisualInitGraph (int kapa, KapaSection *section, Graphdata *graphdata);


/** Ask the user how to proceed.
 * At the user's request, this will disable diagnostic plotting.
 * @param plotFlag, set to false if this plot should be disabled in the future
 */
bool pmVisualAskUser(bool *plotFlag);


/** Ask the user how to proceed.
 * At the user's request, this will disable diagnostic plotting.
 * @param plotFlag, set to false if this plot should be disabled in the future
 * @param dumpData, set to true if user requests a data dump
 */
bool pmVisualAskUserOrDump(bool *plotFlag, bool *dumpData);


/** Scale and display an image.
 * @param kapaFD the index of the Kapa window to draw to
 * @param inImage the image to display
 * @param name the image label
 * @param which channel to draw to (zero works)
 * @param clip set to true to sigma clip the image when scaling
 * @return true for successful completion */
bool pmVisualScaleImage(int kapaFD, psImage *inImage,
                        const char *name, int channel, bool clip);

bool pmVisualRangeImage (int kapaFD, psImage *inImage, const char *name, int channel, float min, float max);

/** Calculate statistics on an image.
 *  This can handle non-finite input pixels
 *  @param image to calculate statistics for
 *  @param mean  stores the calculated mean
 *  @param stdev stores the calculated standard deviation
 *  @param min stores the calculated minimum
 *  @param max stores the calculated maximum
 *  @return true for successful completion */
bool pmVisualImStats(psImage *image, double *mean,
                     double *stdev, double *min, double *max);


/** Create a PS_TYPE_F32 image out of a PS_TYPE_F32 or PS_TYPE_F64 image.
 * This is needed when displaying PS_TYPE_F64 images, which will not work wtih Kii.
 * @param image to convert to type PS_TYPE_F32
 * @return a copy of the image of type PS_TYPE_F32, or NULL upon failure */
psImage* pmVisualImageToFloat(psImage *image);


/** Create a scaled vector of plot point sizes from an unscaled, raw vector.
 *  Used for input into pmVisualTriplePlot
 * @param zVec the raw data
 * @param zScale the scaled vector of plot point sizes
 * @return true for successful completion
 */
bool pmVisualCreateScaleVec (psVector *zVec, psVector *zScale, bool increasing);


/** Use x and y data to determine appropriate values for a Graphdata structure.
 * This procedure sets the max and min keywords of a Graphdata structure to encompass
 * the data coordinates given in xVec and yVec. Optionally, it will try to ignore outliers.
 * @param graphdata structure to set up
 * @param xVec X coordinates of data points
 * @param yVec Y coordinates of data points
 * @clip  set to true to attempt to clip out outliers
 * @return true for successful completion
 */
bool pmVisualScaleGraphdata(Graphdata *graphdata, psVector *xVec,
                            psVector *yVec, bool clip);


/** Create a scatter plot form a set of (x,y) positions, whose plot points
 *  are scaled by the size of a z vector
 * @param kapid the index of the kapa window to plot to
 * @param graphdata describing the plotting window
 * @param xVec x positions
 * @param yVec y positions
 * @param zVec plot point sizes
 * @param increasing whether the points in x,y,zvec are sorted
 */
bool pmVisualTriplePlot (int kapid, Graphdata *graphdata, psVector *xVec,
                         psVector *yVec, psVector *zVec, bool increasing);

/** The same as pmVisualTriplePlot, but draws new points without erasing the old plot window
 * @param kapid the index of the kapa window to plot to
 * @param xVec x positions
 * @param yVec y positions
 * @param zVec plot point sizes
 * @param increasing whether the x,y,zvectors are listed in increasing order
 */
bool pmVisualTripleOverplot (int kapid, Graphdata *graphdata, psVector *xVec,
                             psVector *yVec, psVector *zVec, bool increasing);

#else

// kapa-specific data types are changed to void
bool pmVisualSetVisual(bool value);
bool pmVisualIsVisual(void);
bool pmVisualClose(void);
bool pmVisualInitWindow (int *kapid, char *name);
bool pmVisualInitGraph (int kapa, void *section, void *graphdata);
bool pmVisualAskUser(bool *plotFlag);
bool pmVisualAskUserOrDump(bool *plotFlag, bool *dumpData);
bool pmVisualScaleImage(int kapaFD, psImage *inImage,
                        const char *name, int channel, bool clip);
bool pmVisualImStats(psImage *image, double *mean,
                     double *stdev, double *min, double *max);
psImage* pmVisualImageToFloat(psImage *image);
bool pmVisualCreateScaleVec (psVector *zVec, psVector *zScale, bool increasing);
bool pmVisualResidPlot (psArray *rawstars, psArray *refstars, psArray *match,
                        psMetadata *recipe, char *title, int *kapa, int *kapa2);
bool pmVisualScaleGraphdata(void *graphdata, psVector *xVec,
                            psVector *yVec, bool clip);
bool pmVisualTriplePlot (int kapid, void *graphdata, psVector *xVec,
                         psVector *yVec, psVector *zVec, bool increasing);
bool pmVisualTripleOverplot (int kapid, void *graphdata, psVector *xVec,
                             psVector *yVec, psVector *zVec, bool increasing);

#endif //HAVE_KAPA

#endif //ndef PM_VISUAL_H
