#ifndef PM_READOUT_STACK_H
#define PM_READOUT_STACK_H

#include "pmHDU.h"
#include "pmFPA.h"

#define PM_READOUT_STACK_ANALYSIS_COUNT "STACK.COUNT" // Name for count image in analysis metadata
#define PM_READOUT_STACK_ANALYSIS_SIGMA "STACK.SIGMA" // Name for sigma image in analysis metadata

/// Update an output readout (for a stack) with the correct col0,row0 and the image size
bool pmReadoutUpdateSize(pmReadout *readout, ///< Readout which to update
                         int minCols, int minRows, ///< Minimum coordinates
                         int numCols, int numRows, ///< Size of images
                         bool mask,     ///< Worry about the mask?
                         bool weight,   ///< Worry about the weight?
                         psImageMaskType blank ///< Mask value to give to blank pixels
    );

/// Determine how large an output image is needed to combine the input readouts
bool pmReadoutStackValidate(int *minInputColsPtr, int *maxInputColsPtr, ///< Min and max size in x
                            int *minInputRowsPtr, int *maxInputRowsPtr, ///< Min and max size in y
                            int *numColsPtr, int *numRowsPtr, ///< Size of image
                            const psArray *inputs ///< Array of pmReadouts
    );

psImage *pmReadoutSetAnalysisImage(pmReadout *readout, // Readout containing image
				   const char *name, // Name of image in analysis metadata
				   int numCols, int numRows, // Expected size of image
				   psElemType type, // Expected type of image
				   double init // Initial value
    );

// retrieve the specified image
// XXX not sure why this should call psMemIncrRefCounter
psImage *pmReadoutGetAnalysisImage(pmReadout *readout, // Readout containing image
				   const char *name       // Name of image in analysis metadata
    );


/// Return an image from analysis metadata, produced while stacking
psImage *pmReadoutAnalysisImage(pmReadout *readout, // Readout containing image
                                const char *name, // Name of image in analysis metadata
                                int numCols, int numRows, // Expected size of image
                                psElemType type, // Expected type of image
                                double init // Initial value
    );

// XXX for the moment, use col0, row0, numCols, numRows supplied from the outside
bool pmReadoutStackDefineOutput(pmReadout *readout, int col0, int row0, int numCols, int numRows, bool mask, bool weight, psImageMaskType blank);

bool pmReadoutStackSetOutputSize(int *col0, int *row0, int *numCols, int *numRows, const psArray *inputs);

#endif
