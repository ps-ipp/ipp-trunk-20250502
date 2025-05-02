/* @file  pmConceptsPhotcode.h
 * @brief Generate a photcode from the concepts
 *
 * @author Eugene Magnier, IfA
 *
 * @version $Revision: 1.10 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-08-12 03:27:14 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_CONCEPTS_PHOTCODE_H
#define PM_CONCEPTS_PHOTCODE_H

/// @addtogroup Concepts Data Abstraction Concepts
/// @{

/// Return the photcode based on the PHOTCODE.RULE in the camera configuration
///
/// A photometry code ("photcode") is a string that represents the combination of filter and detector (chip).
/// This functions generates a photcode for a particular chip within the FPA, based on the PHOTCODE.RULE in
/// the camera configuration.  Interpolation using the usual syntax (e.g., "{CHIP.NAME}") is permitted.
psString pmConceptsPhotcodeForView(pmFPAfile *file, const pmFPAview *view);

/// @}
# endif
