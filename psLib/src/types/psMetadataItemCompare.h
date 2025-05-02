/** @file  psMetadataItemCompare.h
 *
 *  @brief Compares Metadata Items
 *
 *  This file defines functions to compare psMetadataItem's.
 *
 *  @author IFA
 *
 *  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-10-31 00:32:19 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_METADATA_ITEM_COMPARE_H
#define PS_METADATA_ITEM_COMPARE_H

#include "psMetadata.h"

/// @addtogroup DataContainer Data Containers
/// @{

/// Operations for comparing a metadata item
typedef enum {
    PS_METADATA_ITEM_COMPARE_OP_NONE,   // No operation specified
    PS_METADATA_ITEM_COMPARE_OP_EQ,     // Equality
    PS_METADATA_ITEM_COMPARE_OP_LT,     // Less than
    PS_METADATA_ITEM_COMPARE_OP_LE,     // Less than or equal
    PS_METADATA_ITEM_COMPARE_OP_GT,     // Greater than
    PS_METADATA_ITEM_COMPARE_OP_GE,     // Greater than or equal
    PS_METADATA_ITEM_COMPARE_OP_NE      // Not equal
} psMetadataItemCompareOp;

/// Get the comparison operation from a metadata item
psMetadataItemCompareOp psMetadataItemCompareOperation(const psMetadataItem *item // Item of interest
    );


/** Compares two psMetadataItems.
 *
 *  @return bool:       True if compare matches template, otherwise false.
 */
bool psMetadataItemCompare(
    const psMetadataItem *compare,     ///< Item to compare to the template
    const psMetadataItem *template     ///< The template
)
;

/// @}
#endif
