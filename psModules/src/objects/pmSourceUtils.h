/* @file  pmSourceUtils.h
 *
 * Utility functions for working with pmSources
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-08-24 00:11:02 $
 * Copyright 2007 IfA, University of Hawaii
 */

# ifndef PM_SOURCE_UTILS_H
# define PM_SOURCE_UTILS_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

/** pmSourceModelGuess()
 *
 * Convert available data to an initial guess for the given model. This
 * function allocates a pmModel entry for the pmSource structure based on the
 * provided model selection. The method of defining the model parameter guesses
 * are specified for each model below. The guess values are placed in the model
 * parameters. The function returns TRUE on success or FALSE on failure.
 *
 */
pmModel *pmSourceModelGuess(
    pmSource *source,   ///< The input pmSource
    pmModelType model,   ///< The type of model to be created.
    psImageMaskType maskVal, 
    psImageMaskType markVal
);

pmSource *pmSourceFromModel (
  pmModel *model, 
  pmReadout *readout, 
  float radius,
  pmSourceType type
  );

/// @}
# endif /* PM_SOURCE_UTILS_H */
