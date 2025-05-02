/* @file  pmConceptsUpdate.h
 * @brief Function to update concepts.
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-30 21:12:56 $
 * Copyright 2005-2007 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_CONCEPTS_UPDATE_H
#define PM_CONCEPTS_UPDATE_H

/// Check for concepts to update.
///
/// Updating concepts is necessary if one concept depends on the value of another.  In that case, a flag
/// (e.g., CONCEPTNAME.UPDATE" in the concepts) can be set, and it can be updated once the required value is
/// known.
bool pmConceptsUpdate(const pmFPA *fpa,       ///< FPA for which to update concepts
                      const pmChip *chip,     ///< Chip for which to update concepts
                      const pmCell *cell      ///< Cell for which to update concepts
    );


#endif
