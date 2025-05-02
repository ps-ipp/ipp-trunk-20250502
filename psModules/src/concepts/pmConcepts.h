/* @file pmConcepts.h
 * @brief Top-level functions for defining, registering, reading and writing concepts
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.19 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-06-30 00:53:45 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_CONCEPTS_H
#define PM_CONCEPTS_H

#include <pslib.h>
#include <pmFPALevel.h>
#include <pmFPA.h>
#include <pmConfig.h>

/// @addtogroup Concepts Data Abstraction Concepts
/// @{

/// Source for concepts when reading and writing.
///
/// Since some sources become available at different times from others, we need to provide some specificity to
/// reading and writing concepts (or we're forced to wait until everything's available, which we don't want to
/// do).  Concepts may be read from or written to multiple sources at once by OR-ing them.
typedef enum {
    PM_CONCEPT_SOURCE_NONE     = 0x00,  ///< No concepts
    PM_CONCEPT_SOURCE_BLANK    = 0x01,  ///< Blank concepts defined, but not read
    PM_CONCEPT_SOURCE_CELLS    = 0x02,  ///< Concept comes from the camera information
    PM_CONCEPT_SOURCE_DEFAULTS = 0x04,  ///< Concept comes from defaults
    PM_CONCEPT_SOURCE_PHU      = 0x08,  ///< Concept comes from PHU
    PM_CONCEPT_SOURCE_HEADER   = 0x10,  ///< Concept comes from FITS header
    PM_CONCEPT_SOURCE_DATABASE = 0x20,  ///< Concept comes from database
    PM_CONCEPT_SOURCE_ALL      = 0xfe   ///< All concepts (exclude BLANK)
} pmConceptSource;

/// Function to call to parse a concept once it has been read
typedef psMetadataItem* (*pmConceptParseFunc)(
    const psMetadataItem *concept, ///< Concept to parse
    const psMetadataItem *pattern, ///< Pattern for parsing
    pmConceptSource source, ///< Source of concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Function to call to format a concept for writing
typedef psMetadataItem* (*pmConceptFormatFunc)(
    const psMetadataItem *concept, ///< Concept to format
    pmConceptSource source, ///< Source of concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Function to call to copy a concept
typedef psMetadataItem* (*pmConceptCopyFunc)(
    const psMetadataItem *target,       ///< Target concept
    const psMetadataItem *source,       ///< Source concept
    const psMetadata *cameraFormat,     ///< Camera format definition
    const pmFPA *fpa,                   ///< Source FPA for concept, or NULL
    const pmChip *chip,                 ///< Source chip for concept, or NULL
    const pmCell *cell                  ///< Source cell for concept, or NULL
    );


/// A "concept" specification
///
/// Defines the name, default comment, blank value, and functions to parse (after reading) and format (before
/// writing) the concept.
typedef struct {
    psMetadataItem *blank;              ///< Blank value of concept; also contains the name and comment
    pmConceptParseFunc parse;           ///< Function to call to read the concept, or NULL
    pmConceptFormatFunc format;         ///< Function to call to write the concept, or NULL
    pmConceptCopyFunc copy;             ///< Function to call to copy the concept, or NULL
    bool required;                      ///< Is concept required (throw an error on problems)?
}
pmConceptSpec;

/// Allocator for pmConceptSpec
pmConceptSpec *pmConceptSpecAlloc(psMetadataItem *blank, ///< Blank value; contains the name
                                  pmConceptParseFunc parse, ///< Function to call to parse the concept/NULL
                                  pmConceptFormatFunc format, ///< Function to call to format the concept/NULL
                                  pmConceptCopyFunc copy, ///< Function to call to copy the concept, or NULL
                                  bool required ///< Is concept required?
                                 );

/// Get whether a particular concept is required
bool pmConceptGetRequired(const char *name, ///< Name of concept
                          pmFPALevel level ///< Level at which concept resides
    );

/// Set whether a particular concept is required
bool pmConceptSetRequired(const char *name, ///< Name of concept
                          pmFPALevel level, ///< Level at which concept resides
                          bool required ///< Whether concept is required or not
    );

/// Register a new concept for parsing and formatting
///
/// Defines a new concept, based on the blank value (with name and default comment), and functions to parse
/// and format the concept.  The new concept is registered at the specified level (FPA, chip or cell).  If the
/// parse function is NULL, then a default parse function is used, which performs minimal parsing.  Similarly
/// for the format function.
bool pmConceptRegister(psMetadataItem *blank, ///< Blank value; contains the name and default comment
                       pmConceptParseFunc parse, ///< Function to call to parse the concept, or NULL
                       pmConceptFormatFunc format, ///< Function to call to format the concept, or NULL
                       pmConceptCopyFunc copy, ///< Function to call to copy the concept, or NULL
                       bool required,   ///< Is concept required?
                       pmFPALevel level ///< Level at which to store concept in the FPA hierarchy
                      );

/// Get the specifications for defined concepts of a particular level
psMetadata *pmConceptsSpecs(pmFPALevel level);


/// Set the concepts within the FPA to the blank value
bool pmConceptsBlankFPA(pmFPA *fpa      ///< FPA for which to set blank concepts
                       );

/// Set the concepts within the chip to the blank value
bool pmConceptsBlankChip(pmChip *chip   ///< FPA for which to set blank concepts
                        );

/// Set the concepts within the cell to the blank value
bool pmConceptsBlankCell(pmCell *cell   ///< Cell for which to set blank concepts
                        );

/// Initialise the concepts system.
///
/// Register the standard concepts, so that concepts may be read and written.  This function is called
/// automatically the first time the concepts functions are used.
bool pmConceptsInit(void);

/// Signifies that the user is done with the concepts system.
///
/// Frees the registered concepts so there is no memory leak when the user checks "persistent" memory.
void pmConceptsDone(void);

/// Interpolate a concept name to the actual value
///
/// Concepts enclosed within braces {}, are replaced with the value of the concept
psString pmConceptsInterpolate(const char *input, ///< Input string
                               const pmFPA *fpa, ///< FPA with concept values, or NULL
                               const pmChip *chip, ///< Chip with concept values, or NULL
                               const pmCell *cell ///< Cell with concept values, or NULL
    );

/// Look up a dependency menu to get a concept's value
///
/// Returns a psMetadataItem with the concept value
psMetadataItem *p_pmConceptsDepend(const char *name, ///< Name of concept for which to get dependent value
                                   const psMetadata *menu, ///< Menu in which to look up key
                                   const psMetadata *source, ///< Source metadata with CONCEPT.DEPEND
                                   const pmFPA *fpa, ///< FPA for dependency
                                   const pmChip *chip, ///< Chip for dependency
                                   const pmCell *cell ///< Cell for dependency
    );

// some utility functions:
int pmConceptsChipNumberFromName (pmFPA *fpa, char *name);
pmChip *pmConceptsChipFromName (pmFPA *fpa, char *name);

/// @}
#endif
