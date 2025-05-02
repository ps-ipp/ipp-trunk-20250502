/*  @file pmConfigMask.h
 *  @brief Mask configuration functions
 *
 *  @author Paul Price, IfA
 *
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *  Copyright 2007 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_CONFIG_MASK_H
#define PM_CONFIG_MASK_H

#include <pslib.h>
#include <pmConfig.h>

#define PM_MASKS_RECIPE "MASKS"

/// @addtogroup Config Configuration System
/// @{

// pmConfigMaskSetInMetadata examines named mask values and set the bits for maskValue and
// markValue.  Ensures that the below-named mask values are set, and calculates the mask value
// to catch all of the mask values marked as 'bad'.  Supplies the fallback name if the primary
// name is not found, or the default values if the fallback name is not found.
bool pmConfigMaskSetInMetadata(psImageMaskType *outMaskValue, // Value of MASK.VALUE, returned
                               psImageMaskType *outMarkValue, // Value of MARK.VALUE, returned
                               psMetadata *source  // Source of mask bits
  );


// Get a mask value by name(s)
psImageMaskType pmConfigMaskGetFromMetadata(psMetadata *source, // Source of masks
                                            const char *masks // Mask values to get
  );


// lookup an image mask value by name from a psMetadata, without requiring the entry to
// be of type psImageMaskType, but verifying that it will fit in psImageMaskType
psImageMaskType psMetadataLookupImageMaskFromGeneric (bool *status, const psMetadata *md, const char *name);

// Remove from the header keywords starting with the provided string
int pmConfigMaskRemoveHeaderKeywords(psMetadata *header, // Header from which to remove keywords
                                     const char *start // Remove keywords that start with this string
  );

/// Return a mask value given a list of symbolic names
///
/// The mask values are derived from the MASKS recipe
psImageMaskType pmConfigMaskGet(const char *masks, ///< List of symbolic names, space/comma delimited
                           const pmConfig *config ///< Configuration
    );

bool pmConfigMaskSet(const pmConfig *config, const char *maskName, psImageMaskType maskValue);

// replace the named masks in the recipe with values in the header:
// replace only the names in the header in the recipe
bool pmConfigMaskReadHeader(pmConfig *config, const psMetadata *header);

// write the named mask bits to the header
bool pmConfigMaskWriteHeader(const pmConfig *config, psMetadata *header);

bool pmConfigMaskSetBits(psImageMaskType *outMaskValue, psImageMaskType *outMarkValue, const pmConfig *config);

#endif
