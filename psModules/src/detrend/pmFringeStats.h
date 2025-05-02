/* @file pmFringeStats.h
 * @brief Measure fringe statistics, and apply correction
 *
 * @author Eugene Magnier, IfA
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 * Copyright 2004-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FRINGE_STATS
#define PM_FRINGE_STATS

/// @addtogroup detrend Detrend Creation and Application
/// @{

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pmFringeRegions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Fringe measurement regions.
///
/// Fringes are measured within a box of size dX,dY.  A large scale smoothing is performed by subtracting the
/// background within large divisions of the image.  The coordinates of the fringe points and the mask may be
/// NULL, which means that they will be generated when required.
typedef struct
{
    int nRequested;                     // Number of fringe points selected
    int nAccepted;                      // Number of fringe points not masked
    int dX;                             // Median box half-width
    int dY;                             // Median box half-height
    int nX;                             // Number of large-scale smoothing divisions in x (col)
    int nY;                             // Number of large-scale smoothing divisions in y (row)
    psVector *x;                        // Fringe point coordinates (col), or NULL
    psVector *y;                        // Fringe point coordinates (row), or NULL
    psVector *mask;                     // Fringe point on/off mask, or NULL
}
pmFringeRegions;

/// Allocate fringe regions
pmFringeRegions *pmFringeRegionsAlloc (int nPts, ///< Number of fringe points to create
                                       int dX, ///< Half-width of fringe boxes
                                       int dY, ///< Half-height of fringe boxes
                                       int nX, ///< Smoothing scale in x
                                       int nY ///< Smoothing scale in y
                                      );

/// Generate the fringe points
///
/// Fringe points are generated randomly over the image.  No effort is made to avoid masked regions (indeed,
/// the function knows nothing about masks).  If the random number generator is NULL, then a new one will be
/// used.
bool pmFringeRegionsCreatePoints(pmFringeRegions *fringe, ///< Fringe regions to generate
                                 const psImage *image, ///< Image for the regions (defines the size)
                                 psRandom *random ///< Random number generator, or NULL
                                );

/// Write the regions to a FITS file
///
/// The fringe regions are written to the FITS file, with the given extension name.  The header is
/// supplemented with scalar values dX, dY, nX and nY (as PSFRNGDX, PSFRNGDY, PSFRNGNX, PSFRNGNY) from the
/// fringe regions, while the fringe coordinates and mask are written as a FITS table (as x, y, mask).
bool pmFringeRegionsWriteFits(psFits *fits, ///< Output FITS file
                              psMetadata *header, ///< Additional headers to write, or NULL
                              const pmFringeRegions *regions, ///< Regions to write
                              const char *extname ///< Extension name, or NULL
                             );

/// Read the regions from a FITS file
///
/// The fringe regions are read from the FITS file, at the given extension name.  The scalars are retrieved
/// from the header, while the table provides the fringe coordinates and mask.
pmFringeRegions *pmFringeRegionsReadFits(psMetadata *header, ///< Header to read, or NULL
        const psFits *fits, ///< Input FITS file
        const char *extname ///< Extension name, or NULL
                                        );

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pmFringeStats
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Fringe measurements for a particular image
///
/// Measurements of the median and stdev are made at each of the fringe regions.
typedef struct
{
    pmFringeRegions *regions;           ///< Fringe regions
    psVector *f;                        ///< Fringe point median
    psVector *df;                       ///< Fringe point stdev
}
pmFringeStats;

/// Allocate fringe statistics
pmFringeStats *pmFringeStatsAlloc(pmFringeRegions *regions // The fringe regions which will be measured
                                 );

/// Measure the fringe statistics for an image
///
/// Given an input image and fringe regions at which to measure, measures the median and stdev at each of the
/// fringe points.  If the fringe points are undefined, they are generated.
pmFringeStats *pmFringeStatsMeasure(pmFringeRegions *fringe, ///< Fringe regions at which to measure
                                    const pmReadout *readout, ///< Readout for which to measure
                                    psImageMaskType maskVal ///< Mask value for image
                                   );

/// Write the fringe stats for an image to a FITS table
///
/// The fringe measurements are written to the FITS file with the given extension name.  The median and stdev
/// measurements are written as a FITS table (as f and df).
bool pmFringeStatsWriteFits(psFits *fits, ///< FITS file to which to write
                            psMetadata *header, ///< Additional headers to write, or NULL
                            const pmFringeStats *fringe, ///< Fringe statistics to be written
                            const char *extname ///< Extension name for table
                           );

/// Read the fringe stats for an image from a FITS table
///
/// The fringe measurements are read from the FITS file, at the given extension name.  The table provides the
/// median and stdev measurements.  It is assumed that the fringe measurements correspond to the regions
/// provided.
pmFringeStats *pmFringeStatsReadFits(psMetadata *header, ///< Header to read, or NULL
                                     const psFits *fits, ///< FITS file from which to read
                                     const char *extname, ///< Extension name to read
                                     pmFringeRegions *regions ///< Corresponding regions
                                    );

/// Concatenate the fringe stats for several readouts into a single fringe stats.
///
/// Each readout of each chip must be measured separately (so as to avoid any gaps between the cells, as in
/// the case for GPC).  But the fit must be performed with all the readouts belonging to a chip (in order to
/// get a secure measurement of the fringe amplitudes).  To do so, we need to concatenate the fringe
/// measurements for each of the chip components.  This function generates a new pmFringeStats from
/// concatenating those in the array.  The corresponding pmFringeRegions is also generated.
pmFringeStats *pmFringeStatsConcatenate(const psArray *fringes, ///< Array of pmFringeStats for the readouts
                                        const psVector *x0, ///< Offset in x for the readout
                                        const psVector *y0 ///< Offset in y for the readout
                                       );

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Input/output for multiple pmFringeStats
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Convert an array of fringes measurements to a psArray suitable for writing as a FITS table.
///
/// Converts an array of fringe measurements for a cell into the corresponding rows of a FITS
/// table (array of psMetadata).  The array of fringe statistics must all use the same fringe
/// regions (or there is no point in storing them all together).  The header is supplemented
/// with scalar values dX, dY, nX and nY (as PSFRNGDX, PSFRNGDY, PSFRNGNX, PSFRNGNY) from the
/// fringe regions, while the fringe coordinates and mask are written as a FITS table rows (as
/// x, y, mask, f, df; f and df are vectors).  Use psFitsTableWrite to save the resulting rows
/// to disk.
psArray *pmFringesFormatTable(psMetadata *header, const psArray *fringes);


/// Parses an array of fringes measurements from a FITS table.
///
/// The fringes for the cell are read from the FITS table (array of psMetadata rows).  The
/// table provides the region and the (possibly multiple) fringe statistics for that region.
/// The supplied header defines the scalar values dX, dY, nX and nY (as PSFRNGDX, PSFRNGDY,
/// PSFRNGNX, PSFRNGNY)
psArray *pmFringesParseTable(psArray *table, psMetadata *header);

/// Deprecated Function: converts the fringes to a FITS table (array of psMetadata) and saves
/// them on the cell->analysis; scalar values are dX, dY, nX and nY are written to the header
/// (as PSFRNGDX, PSFRNGDY, PSFRNGNX, PSFRNGNY)
bool pmFringesFormat(pmCell *cell,   ///< Cell for which to write
                     psMetadata *header, ///< Header, or NULL
                     const psArray *fringes ///< Array of pmFringeStats, all for the same pmFringeRegion
                    );

/// Deprecated Function: pulls a the header and FITS table (array of psMetadata) representing
/// the fringes from the cell->analysis and converts to fringe measurements
psArray *pmFringesParse(pmCell *cell ///< Cell for which to read fringes
                       );

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pmFringeScale
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// The fringe correction solution
typedef struct
{
    int nFringeFrames;                  ///< Number of fringe frames
    psVector *coeff;                    ///< Fringe coefficients; size = nFringeFrames
    psVector *coeffErr;                 ///< Error in fringe coefficients; size = nFringeFrames
}
pmFringeScale;

/// Measure the scales for the fringe correction
///
/// Given a fringe measurement for a science image, and an array of template fringe measurements, this
/// function measures the contribution of each of the templates to the input.  Rejection is performed on the
/// fringe regions, to weed out stars etc.
pmFringeScale *pmFringeScaleMeasure(pmFringeStats *science, ///< Fringe measurements from science image
                                    psArray *fringes, ///< Array of fringe measurements from templates
                                    float rej, ///< Rejection threshold (in standard deviations)
                                    unsigned int nIter, ///< Maximum number of iterations
                                    float keepFrac ///< Minimum fraction of regions to keep
                                   );

/// Allocate fringe scales
pmFringeScale *pmFringeScaleAlloc(int nFringeFrames ///< Number of fringe frames
                                 );


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fringe correction
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Solve for and apply the fringe correction
///
/// This is a wrapper around each of the fringe correction components to measure the fringe points, solve for
/// the fringe correction, and apply the fringe correction.  The input fringe images are modified (scaled by
/// the solution coefficients in order to correct the science image).  Returns the summed fringe image.
psImage *pmFringeCorrect(pmReadout *in, ///< Input science image
                         pmFringeRegions *fringes, ///< The fringe regions used
                         psArray *fringeImages, ///< Fringe template images to use in correction
                         psArray *fringeStats, ///< Fringe stats (for templates) to use in correction
                         psImageMaskType maskVal, ///< Value to mask for science image
                         float rej,     ///< Rejection threshold, for pmFringeScaleMeasure
                         unsigned int nIter, ///< Maximum number of iterations, for pmFringeScaleMeasure
                         float keepFrac ///< Minimum fraction of regions to keep, for pmFringeScaleMeasure
                        );
/// @}
#endif
