/** @file ppSubExtras.c
 *
 *  @brief
 *
 *  @ingroup ppSub
 *
 *  @author IfA
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 01:37:17 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "ppSub.h"

// This file contains minor functions used in ppSub.  some of these should be pushed into
// psModules or psLib.

/**
 * Copy every instance of a single keyword from one metadata to another
 */
bool psMetadataCopySingle(psMetadata *target, psMetadata *source, const char *name)
{
    PS_ASSERT_METADATA_NON_NULL(target, false);
    PS_ASSERT_METADATA_NON_NULL(source, false);
    PS_ASSERT_STRING_NON_EMPTY(name, false);

    psString regex = NULL;      // Regular expression
    psStringAppend(&regex, "^%s$", name);
    psMetadataIterator *iter = psMetadataIteratorAlloc(source, PS_LIST_HEAD, regex); // Iterator
    psFree(regex);
    psMetadataItem *item;       // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter))) {
        psMetadataAddItem(target, item, PS_LIST_TAIL, PS_META_DUPLICATE_OK);
    }
    psFree(iter);

    return true;
}
