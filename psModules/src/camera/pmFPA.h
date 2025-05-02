/* @file pmFPA.h
 * @brief Defines the focal plane hierarchy, along with functions for interacting with it
 *
 * @author George Gusciora, MHPCC
 * @author Paul Price, IfA
 * @author Eugene Magnier, IfA
 *
 * @version $Revision: 1.26 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-06 02:31:24 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_H
#define PM_FPA_H

#include <pslib.h>
#include <pmHDU.h>

/// @addtogroup Camera Camera Layout
/// @{

// Return chip position, given FPA position; calculations are all done in pixel units
#define PM_FPA_TO_CHIP(pos, chip0, chipParity) \
    (((pos) - (chip0))*(chipParity))

// Return cell position, given chip position; calculations are all done in pixel units
#define PM_CHIP_TO_CELL(pos, cell0, cellParity, binning) \
    (((pos) - (cell0))*(cellParity)/(binning))

// Return chip position, given a cell position; calculations are all done in pixel units
#define PM_CELL_TO_CHIP(pos, cell0, cellParity, binning) \
    ((pos)*(binning)*(cellParity) + (cell0))

// Return FPA position, given a chip position; calculations are all done in pixel units
#define PM_CHIP_TO_FPA(pos, chip0, chipParity) \
    ((pos)*(chipParity) + (chip0))

/// Focal plane array (the entirety of the camera)
///
/// The FPA is the top-level camera structure, and consists of one or more chips.  It also contains the
/// concepts metadata appropriate to this level, a summary of analysis tasks that have been performed, the
/// camera configuration information, any HDU that corresponds to this level for the file of interest, and
/// astrometric transformations.  The astrometric transformations encode how to transform from the tangent
/// plane to the sky, and back.
typedef struct {
    // Astrometric transformations
    psPlaneTransform *fromTPA;  ///< Transformation from tangent plane to focal plane, or NULL
    psPlaneTransform *toTPA;  ///< Transformation from focal plane to tangent plane, or NULL
    psProjection *toSky;         ///< Projection from tangent plane to sky, or NULL
    bool wcsCDkeys;
    // Information
    psMetadata *concepts;               ///< FPA-level concepts
    unsigned int conceptsRead;          ///< Which concepts have been read; see pmConceptsSource
    psMetadata *analysis;               ///< FPA-level analysis metadata
    const psMetadata *camera;           ///< Camera configuration
    psArray *chips;                     ///< The component chips
    pmHDU *hdu;                         ///< FITS header data unit of interest, or NULL
} pmFPA;

/// A chip (contiguous detector element)
///
/// The chip is the mid-level camera structure, being part of an FPA, and consisting of one or more cells
/// (e.g., a CCD).  It also contains the concepts metadata appropriate to this level, a summary of analysis
/// tasks that have been performed, status flags, any HDU that corresponds to this level for the file of
/// interest, and astrometric transformations.  The astrometric transformations provide transforms between the
/// chip and FPA coordinates and back.
typedef struct {
    // Astrometric transformations
    psPlaneTransform *toFPA;            ///< Transformation from chip to FPA coordinates, or NULL
    psPlaneTransform *fromFPA;          ///< Transformation from FPA to chip coordinates, or NULL
    // Information
    psMetadata *concepts;               ///< Chip-level concepts
    unsigned int conceptsRead;          ///< Which concepts have been read; see pmConceptsSource
    psMetadata *analysis;               ///< Chip-level analysis metadata
    psArray *cells;                     ///< The component cells
    pmFPA *parent;                      ///< Parent FPA
    bool process;                       ///< Do we bother about reading and working with this chip?
    bool file_exists;                   ///< Does the file for this chip exist (read case only)?
    bool data_exists;                   ///< Does the data for this chip exist (read case only)?
    pmHDU *hdu;                         ///< FITS header data unit of interest,
} pmChip;

/// A cell (smallest logical unit)
///
/// A cell is the lowest-level camera structure, being part of a chip (e.g., an amplifier).  It may consist of
/// one or more readouts, which are individual reads of the cell.  It also contains the concepts metadata
/// appropriate to this level, the cell configuration information (for convenience) from the camera
/// configuration, a summary of analysis tasks that have been performed, status flags, and any HDU that
/// corresponds to this level for the file of interest

/** Cell data structure
 *
 *  A cell consists of one or more readouts.  It also contains a pointer to the
 *  cell's metadata, and its parent chip.  On the astrometry side, it also
 *  contains coordinate transforms from the cell to chip, from the cell to
 *  focal-plane, as well as a "quick and dirty" tranform from the cell to
 *  sky coordinates.
 *
 */
typedef struct {
    psMetadata *concepts;               ///< Cell-level concepts
    unsigned int conceptsRead;          ///< Which concepts have been read; see pmConceptsSource
    psMetadata *config;                 ///< Cell configuration information (from CELLS in the camera config)
    psMetadata *analysis;               ///< Cell-level analysis metadata
    psArray *readouts;                  ///< The component readouts
    pmChip *parent;                     ///< Parent chip
    bool process;                       ///< Do we bother about reading and working with this cell?
    bool file_exists;                   ///< Does the file for this cell exist (read case only)?
    bool data_exists;                   ///< Does the data for this cell exist (read case only)?
    pmHDU *hdu;                         ///< FITS header data unit of interest
} pmCell;

/// A readout (individual read of a cell)
///
/// A readout corresponds to an individual read of a cell (e.g., a single image as part of a video sequence,
/// or one of multiple coadds).  It contains the actual pixels used in analysis (along with mask and variance
/// maps).  When reading from a FITS file, the images are subimages (from CELL.TRIMSEC) of the pixels read
/// from the appropriate HDU (at the FPA, chip or cell level).  The readout also contains a list of bias
/// sections (prescans or overscans, or otherwise), a summary of analysis tasks that have been performed,
/// status flags, and the offsets used for reading a FITS file incrementally.
typedef struct {
    int col0;                           ///< Column offset; non-zero if reading in columns incrementally
    int row0;                           ///< Row offset; non-zero if reading in rows incrementally
    psImage *image;                     ///< Imaging area of readout (corresponds to CELL.TRIMSEC region)
    psImage *mask;                      ///< Mask of input image (corresponds to CELL.TRIMSEC region)
    psImage *variance;                  ///< Variance of input image (corresponds to CELL.TRIMSEC region)
    psKernel *covariance;               ///< Covariance pseudo-matrix (covariance factors for single pixel)
    psList *bias;                       ///< List of bias (prescan/overscan) images
    psMetadata *analysis;               ///< Readout-level analysis metadata
    pmCell *parent;                     ///< Parent cell
    bool process;                       ///< Do we bother about reading and working with this readout?
    bool file_exists;                   ///< Does the file for this readout exist (read case only)?
    bool data_exists;                   ///< Does the data for this readout exist (read case only)?
    int thisImageScan;                  ///< start scan for next/current read of image
    int lastImageScan;                  ///< start scan of the last read of image
    int thisMaskScan;                   ///< start scan for next/current read of mask
    int lastMaskScan;                   ///< start scan of the last read of mask
    int thisVarianceScan;               ///< start scan for next/current read of variance
    int lastVarianceScan;               ///< start scan of the last read of variance
    bool forceScan;                     ///< Force pmFPARead to obey the above commanded this and last scans.
} pmReadout;

/// Free all readouts within a cell
void pmCellFreeReadouts(pmCell *cell);    ///< Cell for which to free readouts

/// Free all cells within a chip
void pmChipFreeCells(pmChip *chip);       ///< Chip for which to free cells

/// Free all data within a readout
void pmReadoutFreeData(pmReadout *readout); ///< Readout for which to free data

/// Free all data within a cell (all readouts as well as metadata)
void pmCellFreeData(pmCell *cell);        ///< Cell for which to free data

/// Free all data within a chip (all cells as well as metadata)
void pmChipFreeData(pmChip *chip);        ///< Chip for which to free data

/// Free all data within an FPA (all chips as well as metadata)
void pmFPAFreeData(pmFPA *fpa);           ///< FPA for which to free data

/// Allocate a readout associated with a cell
pmReadout *pmReadoutAlloc(pmCell *cell);  ///< Parent cell, or NULL
bool psMemCheckReadout(psPtr ptr);

/// Allocate a cell associated with a chip
///
/// The name is used to set CELL.NAME within the concepts.
pmCell *pmCellAlloc(pmChip *chip,       ///< Parent chip, or NULL
                    const char *name);  ///< Name of cell, for CELL.NAME
bool psMemCheckCell(psPtr ptr);


/// Allocate a chip associated with an FPA
///
/// The name is used to set CHIP.NAME within the concepts
pmChip *pmChipAlloc(pmFPA *fpa,         ///< Parent FPA, or NULL
                    const char *name);  ///< Name of chip, for CHIP.NAME
bool psMemCheckChip(psPtr ptr);


/// Allocate an FPA
pmFPA *pmFPAAlloc(const psMetadata *camera, ///< Camera configuration (to store in FPA)
                  const char *cameraName ///< Name of camera (for FPA.CAMERA concept)
    );
bool psMemCheckFPA(psPtr ptr);

/// Check parent links within an FPA
///
/// Iterates through the FPA to verify that the "parent" links in the chip, cell and readout are set
/// correctly.  If there are any incorrect links, they are fixed, and the function returns false.
bool pmFPACheckParents(pmFPA *fpa);     ///< FPA to check


/// Assertions

/// Check that the fundamentals of a readout are set
#define PM_ASSERT_READOUT_NON_NULL(READOUT, RETVAL) { \
    if (!(READOUT) || !(READOUT)->bias || !(READOUT)->analysis) { \
        psError(PS_ERR_UNEXPECTED_NULL, true, "Readout %s or one of its components is NULL.", #READOUT); \
        return RETVAL; \
    } \
    int numCols = 0, numRows = 0; /* Size of readout images */ \
    psImage *image = (READOUT)->image; /* Image pixels */ \
    if (image) { \
        PS_ASSERT_IMAGE_TYPE((READOUT)->image, PS_TYPE_F32, RETVAL); \
        numCols = image->numCols; \
        numRows = image->numRows; \
    } \
    psImage *mask = (READOUT)->mask; /* Mask pixels */ \
    if (mask) { \
        PS_ASSERT_IMAGE_NON_NULL((READOUT)->mask, RETVAL); \
        if ((numCols != 0 || numRows != 0) && (mask->numCols != numCols || mask->numRows != numRows)) { \
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Mask in readout %s has wrong size (%dx%d vs %dx%d)", \
                    #READOUT, mask->numCols, mask->numRows, numCols, numRows); \
            return RETVAL; \
        } else { \
            numCols = mask->numCols; \
            numRows = mask->numRows; \
        } \
    } \
    psImage *variance = (READOUT)->variance; /* Variance map pixels */ \
    if (variance) { \
        PS_ASSERT_IMAGE_NON_NULL((READOUT)->variance, RETVAL); \
        if ((numCols != 0 || numRows != 0) && \
            (variance->numCols != numCols || variance->numRows != numRows)) { \
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, \
                    "Variance in readout %s has wrong size (%dx%d vs %dx%d)", \
                    #READOUT, variance->numCols, variance->numRows, numCols, numRows); \
            return RETVAL; \
        } \
    } \
}

/// Assert that a readout contains an image
#define PM_ASSERT_READOUT_IMAGE(READOUT, RETVAL) \
    PS_ASSERT_IMAGE_NON_NULL((READOUT)->image, RETVAL);

/// Assert that a readout contains a mask
#define PM_ASSERT_READOUT_MASK(READOUT, RETVAL) \
    PS_ASSERT_IMAGE_NON_NULL((READOUT)->mask, RETVAL);

/// Assert that a readout contains a variance map
#define PM_ASSERT_READOUT_VARIANCE(READOUT, RETVAL) \
    PS_ASSERT_IMAGE_NON_NULL((READOUT)->variance, RETVAL);

/// Assert that a readout contains a covariance matrix
#define PM_ASSERT_READOUT_COVARIANCE(READOUT, RETVAL) \
    PS_ASSERT_KERNEL_NON_NULL((READOUT)->covariance, RETVAL);

/// @}
#endif // #ifndef PM_FPA_H
