#ifndef PM_CONCEPTS_COPY_H
#define PM_CONCEPTS_COPY_H

#include <pmFPA.h>

/// Copy all the concepts within an FPA to another FPA
///
/// Iterates over all components of the FPA, and copies the concepts metadata from the source to the target.
bool pmFPACopyConcepts(pmFPA *target,   ///< The target FPA
                       const pmFPA *source    ///< The source FPA
                      );

/// Copy the concepts within an FPA to another FPA; optionally recurse to lower levels
bool pmConceptsCopyFPA(pmFPA *target,   ///< Target FPA
                       const pmFPA *source, ///< Source FPA
                       bool chips,      ///< Recurse to chips level?
                       bool cells       ///< Recurse to cells level?
    );

/// Copy the concepts within a chip to another chip; optionally recurse to lower level
bool pmConceptsCopyChip(pmChip *target, ///< Target chip
                        const pmChip *source, ///< Source chip
                        bool cells      ///< Recurse to cells level?
    );

/// Copy the concepts within a cell to another cell
bool pmConceptsCopyCell(pmCell *target, ///< Target cell
                        const pmCell *source ///< Source cell
    );

#endif
