/* @file  pmConceptsRead.h
 * @brief Reading concepts from a variety of sources.
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-06-17 22:16:38 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_CONCEPTS_READ_H
#define PM_CONCEPTS_READ_H

#include <pslib.h>
#include <pmFPA.h>
#include <pmConfig.h>
#include <pmConcepts.h>

/// Read concepts from the camera format file's CELLS.
///
/// Examines the CELLS metadata in the camera format file
/// for the current type of cell, and sucks in the concepts defined there.
/// This is a useful way of defining concepts that vary depending on the
/// type of the cell.
bool p_pmConceptsReadFromCells(psMetadata *target, ///< Place into which to read the concepts
                               const psMetadata *specs, ///< The concept specifications
                               const pmCell *cell ///< The cell
                              );

/// Read a single concept from the DEFAULTS in the camera format
///
/// The returned item is NOT parsed, but any interpolation for DEPEND is done.
psMetadataItem *p_pmConceptsReadSingleFromDefaults(
    const char *name,                   ///< Name of concept
    const psMetadata *defaults,         ///< DEFAULTS specifications
    const pmFPA *fpa,                   ///< The FPA
    const pmChip *chip,                 ///< The chip, or NULL
    const pmCell *cell                  ///< The cell, or NULL
    );

/// Read concepts from the DEFAULTS in the camera format file.
///
/// Examines the DEFAULTS metadata in the camera format file
/// for concepts in the specs, and imports them into the target.
bool p_pmConceptsReadFromDefaults(psMetadata *target, // Place into which to read the concepts
                                  const psMetadata *specs, // The concept specifications
                                  const pmFPA *fpa, // The FPA
                                  const pmChip *chip, // The chip
                                  const pmCell *cell // The cell
                                 );

/// Read concepts from the header TRANSLATION in the camera format file.
///
/// Examines the TRANSLATION metadata in the camera format file
/// for concepts in the specs, and imports them into the target.
bool p_pmConceptsReadFromHeader(psMetadata *target, // Place into which to read the concepts
                                const psMetadata *specs, // The concept specifications
                                const pmFPA *fpa, // The FPA
                                const pmChip *chip, // The chip
                                const pmCell *cell  // The cell
                               );

/// Read a single concept from the DATABASE specification in the camera format
///
/// The returned item is NOT parsed, but any interpolation for DEPEND is done.
psMetadataItem *p_pmConceptsReadSingleFromDatabase(
    const char *name,                   ///< Name of concept
    const psMetadata *database,         ///< DATABASE specification
    pmConfig *config,                   ///< Configuration
    const pmFPA *fpa,                   ///< The FPA
    const pmChip *chip,                 ///< The chip, or NULL
    const pmCell *cell                  ///< The cell, or NULL
    );

/// Read concepts from the header DATABASE in the camera format file.
///
/// Examines the DATABASE metadata in the camera format file
/// for concepts in the specs, and imports them into the target.
bool p_pmConceptsReadFromDatabase(psMetadata *target, // Place into which to read the concepts
                                  const psMetadata *specs, // The concept specifications
                                  const pmFPA *fpa, // The FPA
                                  const pmChip *chip, // The chip
                                  const pmCell *cell,  // The cell
                                  pmConfig *config // Configuration
                                 );

/// Read the concepts for the given set of fpa, chip, cell
///
/// Attempts to read as many concepts as possible from the specified source for the specified FPA, chip and
/// cell.  That is, it will read chip- and cell-level concepts in addition to fpa-level concepts, if the chip
/// and cell are provided.
bool pmConceptsRead(pmFPA *fpa,         ///< FPA for which to read concepts
                    pmChip *chip,       ///< Chip for which to read concepts, or NULL
                    pmCell *cell,       ///< Cell for which to read concepts, or NULL
                    pmConceptSource source, ///< The source of the concepts to read
                    pmConfig *config    ///< Configuration
                   );

/// Read concepts for an FPA; optionally, read concepts at all lower levels.
///
/// Once concepts should be available for reading at the FPA-level, this function attempts to read the
/// concepts from the specified source.  It also allows concepts to be read at lower levels by iterating over
/// the components.
bool pmConceptsReadFPA(pmFPA *fpa,      ///< FPA for which to read concepts
                       pmConceptSource source, ///< Source for concepts
                       bool propagateDown, ///< Propagate to lower levels?
                       pmConfig *config         ///< Configuration
                      );

/// Read concepts for a chip; optionally, read concepts at the FPA and cell levels.
///
/// Once concepts should be available for reading at the FPA-level, this function attempts to read the
/// concepts from the specified source.  It also allows concepts to be read at the fpa level (through the
/// parent), and the cell level by iterating over the components.
bool pmConceptsReadChip(pmChip *chip,   ///< Chip for which to read concepts
                        pmConceptSource source, ///< Source for concepts
                        bool propagateUp, ///< Propagate to higher levels?
                        bool propagateDown, ///< Propagate to lower levels?
                        pmConfig *config        ///< Configuration
                       );

/// Read concepts for a cell; optionally, read concepts for the parents.
///
/// Once concepts should be available for reading at the FPA-level, this function attempts to read the
/// concepts from the specified source.  It also allows concepts to be read at upper levels through the
/// parents (note, it would not read concepts for all chips, but only the parent of this cell).
bool pmConceptsReadCell(pmCell *cell,   ///< Cell for which to read concepts
                        pmConceptSource source, ///< Source for concepts
                        bool propagateUp, ///< Propagate to higher levels?
                        pmConfig *config        ///< Configuration
                       );

/// @}
#endif
