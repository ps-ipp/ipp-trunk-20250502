/** @file ppArith.h
 *
 *  @brief
 *
 *  @ingroup ppArith
 *
 *  @author IfA
 *  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-01 21:40:52 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */


#ifndef PP_ARITH_H
#define PP_ARITH_H

/// @addtogroup ppArith
/// @{
#define PPARITH_RECIPE "PPARITH"            ///< Name of the recipe to use

/**
 * Parse the arguments
 */
bool ppArithArguments(int argc, char *argv[], ///< Command-line arguments
                      pmConfig *config    ///< Configuration
    );

/**
 * Parse the camera input
 */
bool ppArithCamera(pmConfig *config       ///< Configuration
    );

/**
 * Loop over the FPA hierarchy
 */
bool ppArithLoop(pmConfig *config         ///< Configuration
    );

/**
 * Perform arithmetic on the readout
 */
bool ppArithReadout(pmReadout *output,  ///< Output readout
                    const pmReadout *input1, ///< Input readout
                    const pmReadout *input2, ///< Input readout
                    float const2,            ///< Constant to apply
                    const pmConfig *config, ///< Configuration
                    const pmFPAview *view ///< View of readout on which to operate
    );

/**
 * Put the program version information into header
 */
bool ppArithVersionHeader(psMetadata *header ///< Header to populate
    );

/// Print version information
void ppArithVersionPrint(void);

///@}

#endif
