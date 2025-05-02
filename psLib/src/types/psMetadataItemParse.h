/** @file  psMetadataItemParse.h
*
*  @brief Parse metadata items
*
*  @author EAM, PAP, JH, RHL, 
*
*  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
*  @date $Date: 2007-02-06 21:36:09 $
*
*  Copyright 2004-2005 IfA, University of Hawaii
*/

#ifndef PS_METADATA_ITEM_PARSE_H
#define PS_METADATA_ITEM_PARSE_H

/// @addtogroup DataContainer Data Containers
/// @{

#include "psType.h"
#include "psMetadata.h"

// Parse a psMetadataItem as a particular type
bool psMetadataItemParseBool(const psMetadataItem *item);
psF32 psMetadataItemParseF32(const psMetadataItem *item);
psF64 psMetadataItemParseF64(const psMetadataItem *item);
psS8  psMetadataItemParseS8(const psMetadataItem *item);
psS16 psMetadataItemParseS16(const psMetadataItem *item);
psS32 psMetadataItemParseS32(const psMetadataItem *item);
psU8  psMetadataItemParseU8(const psMetadataItem *item);
psU16 psMetadataItemParseU16(const psMetadataItem *item);
psU32 psMetadataItemParseU32(const psMetadataItem *item);
psString psMetadataItemParseString(const psMetadataItem *item);

/// @}
#endif
