/* @file  pmConceptsStandard.h
 * @brief Private functions for parsing and formatting standard concepts.
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-10-13 21:20:36 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_CONCEPTS_STANDARD_H
#define PM_CONCEPTS_STANDARD_H

/// @addtogroup Concepts Data Abstraction Concepts
/// @{

/// Parse a list of region specifiers on a line
psList *p_pmConceptParseRegions(const char *region ///< Regions, separated by whitespace
    );

/// Parse the CELL.READNOISE concept to do ADU->e correction if required
psMetadataItem *p_pmConceptParse_CELL_READNOISE(
    const psMetadataItem *concept,
    const psMetadataItem *pattern,
    pmConceptSource source,
    const psMetadata *cameraFormat,
    const pmFPA *fpa,
    const pmChip *chip,
    const pmCell *cell
);

/// Format the CELL.READNOISE concept to do e->ADU correction if required
psMetadataItem *p_pmConceptFormat_CELL_READNOISE(
    const psMetadataItem *concept,
    pmConceptSource source,
    const psMetadata *cameraFormat,
    const pmFPA *fpa,
    const pmChip *chip,
    const pmCell *cell
    );

// Parse the TELTEMPS concept : parse a list of the form 'X1 X2 X3 X4 X5 ...' : for now use median
psMetadataItem *p_pmConceptParse_TELTEMPS(
    const psMetadataItem *concept,
    const psMetadataItem *pattern,
    pmConceptSource source,
    const psMetadata *cameraFormat,
    const pmFPA *fpa,
    const pmChip *chip,
    const pmCell *cell
    );

/// Parse the FPA.FILTER concept to apply a lookup table
psMetadataItem *p_pmConceptParse_FPA_FILTER(
    const psMetadataItem *concept, ///< Concept to parse
    const psMetadataItem *pattern, ///< Pattern to use in parsing
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Format the FPA.FILTER concept to (reverse-)apply a lookup table
psMetadataItem *p_pmConceptFormat_FPA_FILTER(
    const psMetadataItem *concept, ///< Concept to format
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Parse the FPA.OBSTYPE concept to apply a lookup table
psMetadataItem *p_pmConceptParse_FPA_OBSTYPE(
    const psMetadataItem *concept, ///< Concept to parse
    const psMetadataItem *pattern, ///< Pattern to use in parsing
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Format the FPA.OBSTYPE concept to (reverse-)apply a lookup table
psMetadataItem *p_pmConceptFormat_FPA_OBSTYPE(
    const psMetadataItem *concept, ///< Concept to format
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Parse the coordinates concepts: FPA.RA and FPA.DEC
psMetadataItem *p_pmConceptParse_FPA_Coords(
    const psMetadataItem *concept, ///< Concept to parse
    const psMetadataItem *pattern, ///< Pattern to use in parsing
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Format the coordinates concepts: FPA.RA and FPA.DEC
psMetadataItem *p_pmConceptFormat_FPA_Coords(
    const psMetadataItem *concept, ///< Concept to format
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Parse the CELL.TRIMSEC concept
psMetadataItem *p_pmConceptParse_CELL_TRIMSEC(
    const psMetadataItem *concept, ///< Concept to parse
    const psMetadataItem *pattern, ///< Pattern to use in parsing
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Parse the CELL.BIASSEC concept
psMetadataItem *p_pmConceptParse_CELL_BIASSEC(
    const psMetadataItem *concept, ///< Concept to parse
    const psMetadataItem *pattern, ///< Pattern to use in parsing
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

// Parse the CHIP.VIDEOCELL concept
psMetadataItem *p_pmConceptParse_VideoCell(
   const psMetadataItem *concept, ///< Concept to parse
   const psMetadataItem *pattern, ///< Pattern to use in parsing
   pmConceptSource source, ///< Source for concept
   const psMetadata *cameraFormat, ///< Camera format definition
   const pmFPA *fpa, ///< FPA for concept, or NULL
   const pmChip *chip, ///< Chip for concept, or NULL
   const pmCell *cell ///< Cell for concept, or NULL
   );

/// Format for the BTOOLAPP conceptn
psMetadataItem *p_pmConceptParse_BTOOLAPP(
    const psMetadataItem *concept, ///< Concept to format
    const psMetadataItem *pattern,
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );
psMetadataItem *p_pmConceptFormat_BTOOLAPP(
    const psMetadataItem *concept, ///< Concept to format
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );


/// Parse the cell binning concepts: CELL.XBIN, CELL.YBIN
psMetadataItem *p_pmConceptParse_CELL_Binning(
    const psMetadataItem *concept, ///< Concept to parse
    const psMetadataItem *pattern, ///< Pattern to use in parsing
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Parse the time system concepts: FPA.TIMESYS and CELL.TIMESYS
psMetadataItem *p_pmConceptParse_TIMESYS(
    const psMetadataItem *concept, ///< Concept to parse
    const psMetadataItem *pattern, ///< Pattern to use in parsing
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Parse the time concepts: FPA.TIME and CELL.TIME
psMetadataItem *p_pmConceptParse_TIME(
    const psMetadataItem *concept, ///< Concept to parse
    const psMetadataItem *pattern, ///< Pattern to use in parsing
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Parse a cell position concept, e.g., CELL.X0
psMetadataItem *p_pmConceptParse_Positions(
    const psMetadataItem *concept, ///< Concept to parse
    const psMetadataItem *pattern, ///< Pattern to use in parsing
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Format the CELL.TRIMSEC concept
psMetadataItem *p_pmConceptFormat_CELL_TRIMSEC(
    const psMetadataItem *concept, ///< Concept to format
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Format the CELL.BIASSEC concept
psMetadataItem *p_pmConceptFormat_CELL_BIASSEC(
    const psMetadataItem *concept, ///< Concept to format
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Format the CELL.XBIN concept
psMetadataItem *p_pmConceptFormat_CELL_XBIN(
    const psMetadataItem *concept, ///< Concept to format
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Format the CELL.YBIN concept
psMetadataItem *p_pmConceptFormat_CELL_YBIN(
    const psMetadataItem *concept, ///< Concept to format
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Format the time system concepts: FPA.TIMESYS and CELL.TIMESYS
psMetadataItem *p_pmConceptFormat_TIMESYS(
    const psMetadataItem *concept, ///< Concept to format
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Format the time concepts: FPA.TIME and CELL.TIME
psMetadataItem *p_pmConceptFormat_TIME(
    const psMetadataItem *concept, ///< Concept to format
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Format a cell position concept, e.g., CELL.X0
psMetadataItem *p_pmConceptFormat_Positions(
    const psMetadataItem *concept, ///< Concept to format
    pmConceptSource source, ///< Source for concept
    const psMetadata *cameraFormat, ///< Camera format definition
    const pmFPA *fpa, ///< FPA for concept, or NULL
    const pmChip *chip, ///< Chip for concept, or NULL
    const pmCell *cell ///< Cell for concept, or NULL
    );

/// Copy the time system only if not already set
psMetadataItem *p_pmConceptCopy_TIMESYS(
    const psMetadataItem *target,
    const psMetadataItem *source,
    const psMetadata *cameraFormat,
    const pmFPA *fpa,
    const pmChip *chip,
    const pmCell *cell
    );

/// @}
#endif
