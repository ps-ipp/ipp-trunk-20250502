/* @file  pmConceptsWrite.h
 * @brief Writing concepts to a variety of sources.
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-06-17 22:16:38 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_CONCEPTS_WRITE_H
#define PM_CONCEPTS_WRITE_H

#include <pslib.h>
#include <pmFPA.h>
#include <pmConfig.h>
#include <pmConcepts.h>

/// @addtogroup Concepts Data Abstraction Concepts
/// @{

/// "Write" concept to (actually, check against) the camera format file's CELLS.
///
/// Examines the CELLS metadata in the camera format file for the current type of cell, and checks that the
/// concept as defined there match the ones defined in the cell.  A warning is produced if the concept does
/// not match.
bool p_pmConceptWriteToCells(const pmCell *cell, ///< The cell
                             const pmConceptSpec *spec, ///< Concept specification
                             const psMetadataItem *conceptItem, ///< Concept to write
                             const psMetadata *format ///< Camera format, or NULL
    );

/// "Write" concept to (actually, check against) the camera format file's DEFAULTS.
///
/// Examines the DEFAULTS metadata in the camera format file, and checks that the concept as defined there
/// match the one defined in the cell.  A warning is produced if the concept does not match.
bool p_pmConceptWriteToDefaults(const pmFPA *fpa, ///< The FPA
                                const pmChip *chip, ///< The chip
                                const pmCell *cell, ///< The cell
                                const pmConceptSpec *spec, ///< Concept specification
                                const psMetadataItem *conceptItem,      ///< Concept to write
                                const psMetadata *format, ///< Camera format, or NULL
                                const psMetadata *defaults ///< DEFAULTS configuration, or NULL
    );

/// "Write" concept to (actually, add to, pending a later write) the FITS header.
///
/// Examines the FITS header TRANSLATION metadata in the camera format file, and writes concept to the
/// appropriate FITS header(s) in the HDU, in preparation for a future write of the HDU.
bool p_pmConceptWriteToHeader(const pmFPA *fpa, ///< The FPA
                              const pmChip *chip, ///< The chip
                              const pmCell *cell, ///< The cell
                              const pmConceptSpec *spec, ///< Concept specification
                              const psMetadataItem *conceptItem, ///< Concept to write
                              const psMetadata *format, ///< Camera format, or NULL
                              const psMetadata *translation ///< TRANSLATION configuration, or NULL
                              );

/// Write concept to the database.
///
/// Examines the DATABASE metadata in the camera format file, and writes (actually, check against)
/// concept to the database.
bool p_pmConceptWriteToDatabase(const pmFPA *fpa, ///< The FPA
                                const pmChip *chip, ///< The chip
                                const pmCell *cell, ///< The cell
                                pmConfig *config, ///< Configuration
                                const pmConceptSpec *spec, ///< Concept specification
                                const psMetadataItem *conceptItem, ///< Concept to write
                                const psMetadata *format, ///< Camera format, or NULL
                                const psMetadata *database ///< DATABASE configuration, or NULL
                                );


bool pmConceptWriteSingle(const pmFPA *fpa, ///< The FPA
                          const pmChip *chip, ///< The chip
                          const pmCell *cell, ///< The cell
                          pmConfig *config, ///< Configuration
                          const psMetadataItem *concept ///< Concept to write
    );


/// Write concepts for an FPA; optionally, write concepts at all lower levels.
///
/// This function writes all concepts for the FPA to the specified "source".  It also allows concepts to be
/// written for all lower levels by iterating over the components.
bool pmConceptsWriteFPA(const pmFPA *fpa,     ///< FPA for which to write concepts
                        bool propagateDown, ///< Propagate to lower levels?
                        pmConfig *config        ///< Configuration
                       );

/// Write concepts for a chip; optionally, write concepts at the FPA and cell levels.
///
/// This function writes all concepts for the chip to the specified "source".  It also allows concepts to be
/// written for the FPA, and the cell level by iterating over the components.
bool pmConceptsWriteChip(const pmChip *chip,  ///< Chip for which to write concepts
                         bool propagateUp,///< Propagate to higher levels?
                         bool propagateDown, ///< Propagate to lower levels?
                         pmConfig *config       ///< Configuration
                        );

/// Write concepts for a cell; optionally, write concepts for the parents.
///
/// This function writes all concepts for the chip to the specified "source".  It also allows concepts to be
/// written for the upper levels through the parents (note, it would not write concepts for all chips, but
/// only the parent of this cell).
bool pmConceptsWriteCell(const pmCell *cell,  ///< FPA for which to write concepts
                         bool propagateUp, ///< Propagate to higher levels?
                         pmConfig *config ///< Configuration
                        );

/// @}
#endif
