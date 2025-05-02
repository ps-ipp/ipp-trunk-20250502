/* @file pmBias.h
 * @brief Subtract the overscan, bias and dark
 *
 * @author George Gusciora, MHPCC
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-09-09 04:10:14 $
 * Copyright 2004--2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_BIAS_H
#define PM_BIAS_H

/// @addtogroup detrend Detrend Creation and Application
/// @{

/// Subtract the overscan, bias and/or dark
///
/// Subtracts the overscan, as measured from the bias member of the input readout (if options are non-NULL),
/// bias (if non-NULL) and dark (if non-NULL) scaled by the CELL.DARKTIME concept.
bool pmBiasSubtract(pmReadout *in,      ///< Input readout, to be overscan/bias/dark corrected
                    pmReadout *bias, ///< Bias to subtract, or NULL
                    pmReadout *dark, ///< Dark to scale and subtract, or NULL
                    const pmFPAview *view ///< View for readout of interest
                   );

// pmBiasSubtractFrame
// this routine will take as input a readout for the input image and a readout for the bias
// image.  The bias image is subtracted in place from the input image.
bool pmBiasSubtractFrame(pmReadout *in, // Input readout
                         pmReadout *sub, // Readout to be subtracted from input
                         float scale   // Scale to apply before subtracting
    );

/// Thread entry point for bias subtraction
bool pmBiasSubtractScan_Threaded(psThreadJob *job ///< Job to execute
    );

/// Do bias subtraction for a scan
bool pmBiasSubtractScan(
    pmReadout *in,                      ///< Input image to correct
    const pmReadout *sub,               ///< Bias+dark image to subtract
    float scale,                        ///< Scale to apply
    int xOffset, int yOffset,           ///< Offset between input and bias images
    int rowStart, int rowStop           ///< Scan range
    );

/// @}
#endif
