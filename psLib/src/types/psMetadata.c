/** @file  psMetadata.c
 *
 *
 *  @brief Contains metadata structures, enumerations and functions prototypes.
 *
 *  This file defines metadata item, metadata type, metadata flags, metadata containers, and function
 *  prototypes necessary creating psLib metadata APIs
 *
 *  @ingroup Metadata
 *
 *  @author Robert DeSonia, MHPCC
 *  @author Ross Harman, MHPCC
 *
 *  @version $Revision: 1.175 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/******************************************************************************/
/*  INCLUDE FILES                                                             */
/******************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "fitsio.h"
#include "psType.h"
#include "psMemory.h"
#include "psError.h"
#include "psAbort.h"
#include "psList.h"
#include "psHash.h"
#include "psVector.h"
#include "psMetadata.h"
#include "psLookupTable.h"
#include "psString.h"

#include "psAssert.h"
#include "psLogMsg.h"
#include "psTrace.h"

/******************************************************************************/
/*  DEFINE STATEMENTS                                                         */
/******************************************************************************/

#define MAX_STRING_SIZE 1024            // Maximum string size for psMetadatItem

/******************************************************************************/
/*  TYPE DEFINITIONS                                                          */
/******************************************************************************/

// None

/*****************************************************************************/
/*  GLOBAL VARIABLES                                                         */
/*****************************************************************************/

// None

/*****************************************************************************/
/*  FILE STATIC VARIABLES                                                    */
/*****************************************************************************/

static psS32 metadataId = 0;

/*****************************************************************************/
/*  FUNCTION IMPLEMENTATION - LOCAL                                          */
/*****************************************************************************/

static psMetadataItem* makeMetaMulti(psHash* table,
                                     const char* key,
                                     psMetadataItem* existing)
{

    if (existing != NULL && existing->type == PS_DATA_METADATA_MULTI) {
        return existing;
    }

    // move any existing entry into a psList
    psList* newList = psListAlloc(existing);

    psMetadataItem* item = psMetadataItemAlloc(key,
                           PS_DATA_METADATA_MULTI,
                           "",
                           newList);

    if (existing != NULL) {
        psHashRemove(table,key); // take out the old entry
    }

    psHashAdd(table, key, item); // put in the new entry
    psMemDecrRefCounter(item); // get rid of extra reference

    // free local references of newly allocated items.
    psFree(newList);

    return item;
}

static void metadataItemFree(psMetadataItem* metadataItem)
{
    psDataType type;

    type = metadataItem->type;

    //    fprintf(stderr,"%s %s %d\n",metadataItem->name,metadataItem->comment,type);
    psMemDecrRefCounter(metadataItem->name);
    psMemDecrRefCounter(metadataItem->comment);

    if(!PS_DATA_IS_PRIMITIVE(type)) {
        psFree(metadataItem->data.V);
    }
}

static void metadataIteratorFree(psMetadataIterator* iter)
{
    psFree(iter->iter);

    if (iter->regex != NULL) {
        regfree(iter->regex);
        psFree(iter->regex);
    }
}

static void metadataFree(psMetadata* metadata)
{
    psFree(metadata->list);
    psFree(metadata->hash);
}

/*****************************************************************************/
/* FUNCTION IMPLEMENTATION - PUBLIC                                          */
/*****************************************************************************/

psMetadataItem* p_psMetadataItemAlloc(const char *file,
                                      unsigned int lineno,
                                      const char *func,
                                      const char *name,
                                      psDataType type,
                                      const char *comment,
                                      ...)
{
    va_list argPtr;
    psMetadataItem* metadataItem = NULL;

    // Get the variable list parameters to pass to allocation function
    va_start(argPtr, comment);

    // Call metadata item allocation
    metadataItem = p_psMetadataItemAllocV(file, lineno, func, name, type, comment, argPtr);

    // Clean up stack after variable arguement has been used
    va_end(argPtr);

    return metadataItem;
}

bool psMemCheckMetadataItem(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)metadataItemFree );
}


#define METADATAITEM_ALLOC_TYPE(NAME,TYPE,METATYPE) \
psMetadataItem* psMetadataItemAlloc##NAME(const char* name, \
        const char* comment, \
        TYPE value) \
{ \
    return psMetadataItemAlloc(name, METATYPE, comment, value); \
}

METADATAITEM_ALLOC_TYPE(Str,const char*,PS_DATA_STRING)
METADATAITEM_ALLOC_TYPE(F32,psF32,PS_DATA_F32)
METADATAITEM_ALLOC_TYPE(F64,psF64,PS_DATA_F64)
METADATAITEM_ALLOC_TYPE(S8,psS8,PS_DATA_S8)
METADATAITEM_ALLOC_TYPE(S16,psS16,PS_DATA_S16)
METADATAITEM_ALLOC_TYPE(S32,psS32,PS_DATA_S32)
METADATAITEM_ALLOC_TYPE(S64,psS64,PS_DATA_S64)
METADATAITEM_ALLOC_TYPE(U8,psU8,PS_DATA_U8)
METADATAITEM_ALLOC_TYPE(U16,psU16,PS_DATA_U16)
METADATAITEM_ALLOC_TYPE(U32,psU32,PS_DATA_U32)
METADATAITEM_ALLOC_TYPE(U64,psU64,PS_DATA_U64)
METADATAITEM_ALLOC_TYPE(Bool,bool,PS_DATA_BOOL)

psMetadataItem* psMetadataItemAllocPtr(const char *name,
                                       psDataType type,
                                       const char *comment,
                                       psPtr value)
{
    return (psMetadataItemAlloc(name, type, comment, value));
}


psMetadataItem* p_psMetadataItemAllocV(const char *file,
                                     unsigned int lineno,
                                     const char *func,
                                     const char *name,
                                     psDataType type,
                                     const char *comment,
                                     va_list argPtr)
{
    PS_ASSERT_STRING_NON_EMPTY(name, NULL);
    PS_ASSERT_INT_POSITIVE(type, NULL);

    // Allocate metadata item
    psMetadataItem *metadataItem = (psMetadataItem*) p_psAlloc(file, lineno, func, sizeof(psMetadataItem));
    metadataItem->data.V = NULL;

    // Set deallocator
    psMemSetDeallocator(metadataItem, (psFreeFunc) metadataItemFree);

    // XXX Does it make sense allocate a NULL string here?  It seems like this
    // is just a waste of memory but something, somewhere, is probably
    // dependant on this behavior so I'm leaving it in place for the time
    // being. -JH

    // Allocate and set metadata item comment
    if (comment == NULL) {
        // Per SDRS, null isn't allowed, must use "" instead
        metadataItem->comment = p_psStringCopy(file, lineno, func, "");
    } else {
        metadataItem->comment = p_psStringCopy(file, lineno, func, comment);
    }

    // Set metadata item unique id
    *(psS32 *)(&metadataItem->id) = ++metadataId;

    // Set metadata item type
    metadataItem->type = type & PS_METADATA_TYPE_MASK;

    // Allocate and set metadata item name
    metadataItem->name = p_psStringCopy(file, lineno, func, name);

    // Set metadata item value
    switch(metadataItem->type) {
    case PS_DATA_BOOL:
        metadataItem->data.B = (bool)va_arg(argPtr, psS32);
        break;
    case PS_DATA_S8:
        metadataItem->data.S8 = (psS8)va_arg(argPtr, psS32);
        break;
    case PS_DATA_S16:
        metadataItem->data.S16 = (psS16)va_arg(argPtr, psS32);
        break;
    case PS_DATA_S32:
        metadataItem->data.S32 = (psS32)va_arg(argPtr, psS32);
        break;
    case PS_DATA_S64:
        metadataItem->data.S64 = (psS64)va_arg(argPtr, psS64);
        break;
    case PS_DATA_U8:
        metadataItem->data.U8 = (psU8)va_arg(argPtr, psU32);
        break;
    case PS_DATA_U16:
        metadataItem->data.U16 = (psU16)va_arg(argPtr, psU32);
        break;
    case PS_DATA_U32:
        metadataItem->data.U32 = (psU32)va_arg(argPtr, psU32);
        break;
    case PS_DATA_U64:
        metadataItem->data.U64 = (psU64)va_arg(argPtr, psU64);
        break;
    case PS_DATA_F32:
        metadataItem->data.F32 = (psF32)va_arg(argPtr, psF64);
        break;
    case PS_DATA_F64:
        metadataItem->data.F64 = (psF64)va_arg(argPtr, psF64);
        break;
    case PS_DATA_STRING: {
        // Copy input strings, so they can be messed with
        char *string = va_arg(argPtr, char*);
        metadataItem->data.str = (string ? p_psStringCopy(file, lineno, func, string) : NULL);
        break;
    }
    case     PS_DATA_ARRAY:                     // psArray
    case     PS_DATA_BITS:                      // psBits
    case     PS_DATA_CUBE:                      // psCube
    case     PS_DATA_FITS:                      // psFits
    case     PS_DATA_HASH:                      // psHash
    case     PS_DATA_HISTOGRAM:                 // psHistogram
    case     PS_DATA_IMAGE:                     // psImage
    case     PS_DATA_KERNEL:                    // psKernel
    case     PS_DATA_LIST:                      // psList
    case     PS_DATA_LOOKUPTABLE:               // psLookupTable
    case     PS_DATA_METADATA:                  // psMetadata
    case     PS_DATA_METADATAITEM:              // psMetadataItem
    case     PS_DATA_MINIMIZATION:              // psMinimization
    case     PS_DATA_PIXELS:                    // psPixels
    case     PS_DATA_PLANE:                     // psPlane
    case     PS_DATA_PLANEDISTORT:              // psPlaneDistort
    case     PS_DATA_PLANETRANSFORM:            // psPlaneTransform
    case     PS_DATA_POLYNOMIAL1D:              // psPolynomial1D
    case     PS_DATA_POLYNOMIAL2D:              // psPolynomial2D
    case     PS_DATA_POLYNOMIAL3D:              // psPolynomial3D
    case     PS_DATA_POLYNOMIAL4D:              // psPolynomial4D
    case     PS_DATA_PROJECTION:                // psProjection
    case     PS_DATA_REGION:                  // psRegion
    case     PS_DATA_SCALAR:                    // psScalar
    case     PS_DATA_SPHERE:                    // psSphere
    case     PS_DATA_SPHEREROT:                 // psSphereTransform
    case     PS_DATA_SPLINE1D:                  // psSpline1D
    case     PS_DATA_STATS:                     // psStats
    case     PS_DATA_TIME:                      // psTime
    case     PS_DATA_VECTOR:                    // psVector
    case     PS_DATA_UNKNOWN:                   // "other"
    case     PS_DATA_METADATA_MULTI:
        // Copy of input data not performed due to variability of data types
        metadataItem->data.V = p_psMemIncrRefCounter(file, lineno, func, va_arg(argPtr, psPtr));
        break;
    default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Specified psDataType, %d, is not supported."), type);
        psFree(metadataItem);
        metadataItem = NULL;
        break;
    }

    return metadataItem;
}

psMetadata* p_psMetadataAlloc(const char *file,
                              unsigned int lineno,
                              const char *func)
{
    psList* list = NULL;
    psHash* hash = NULL;
    psMetadata* metadata = NULL;

    // Allocate metadata
    metadata = (psMetadata*) p_psAlloc(file, lineno, func, sizeof(psMetadata));
    // Set deallocator
    psMemSetDeallocator(metadata, (psFreeFunc) metadataFree);

    // Allocate metadata's internal containers
    list = (psList*) psListAlloc(NULL);
    hash = (psHash*) psHashAlloc(10);

    metadata->list = list;
    metadata->hash = hash;

    return metadata;
}


bool psMemCheckMetadata(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)metadataFree );
}


psMetadataItem *p_psMetadataItemCopy(const char *file,
                                     unsigned int lineno,
                                     const char *func,
                                     const psMetadataItem *in)
{
    PS_ASSERT_METADATA_ITEM_NON_NULL(in,NULL);

    psMetadataItem *newItem = NULL;     // New metadata item, to be returned

    #define PS_METADATA_ITEM_COPY_CASE(NAME,TYPE) \
case PS_DATA_##NAME: \
    newItem = p_psMetadataItemAlloc(file, lineno, func, in->name, PS_DATA_##NAME, in->comment, in->data.TYPE); \
    break; \

    switch (in->type) {
        // Simple types
        PS_METADATA_ITEM_COPY_CASE(BOOL,B);
        PS_METADATA_ITEM_COPY_CASE(S8,S8);
        PS_METADATA_ITEM_COPY_CASE(S16,S16);
        PS_METADATA_ITEM_COPY_CASE(S32,S32);
        PS_METADATA_ITEM_COPY_CASE(S64,S64);
        PS_METADATA_ITEM_COPY_CASE(U8,U8);
        PS_METADATA_ITEM_COPY_CASE(U16,U16);
        PS_METADATA_ITEM_COPY_CASE(U32,U32);
        PS_METADATA_ITEM_COPY_CASE(U64,U64);
        PS_METADATA_ITEM_COPY_CASE(F32,F32);
        PS_METADATA_ITEM_COPY_CASE(F64,F64);
        PS_METADATA_ITEM_COPY_CASE(STRING,V); // This will copy the string, not point at it.
    case PS_DATA_VECTOR: {
            PS_ASSERT_PTR_NON_NULL(in->data.V, NULL);
            psVector *vecCopy = psVectorCopy(NULL, (psVector*)(in->data.V),
                                             ((psVector*)(in->data.V))->type.type);
            newItem = p_psMetadataItemAlloc(file, lineno, func, in->name, PS_DATA_VECTOR, in->comment, vecCopy);
            psFree(vecCopy);    // Drop reference
            break;
        }
    case PS_DATA_TIME: {
            psTime *timeCopy = NULL;
            // pass through NULL values
            if (in->data.V) {
                timeCopy = psTimeCopy( (psTime*)(in->data.V) );
                if (timeCopy == NULL) {
                    psError(PS_ERR_BAD_PARAMETER_NULL, false,
                            "Error copying time.  Time skipped.\n");
                }
            }
            newItem = p_psMetadataItemAlloc(file, lineno, func, in->name, PS_DATA_TIME, in->comment, timeCopy);
            if (timeCopy) {
                psFree(timeCopy);   // Drop reference
            }
            break;
        }
    case PS_DATA_METADATA: {
            // Metadata: copy the next level and stuff that in too
            psMetadata *metadata = psMetadataCopy(NULL, in->data.md);
            newItem = p_psMetadataItemAlloc(file, lineno, func, in->name, PS_DATA_METADATA, in->comment, metadata);
            psFree(metadata);       // Drop reference
            break;
        }
    case PS_DATA_REGION: {
            psRegion *region = in->data.V; // The region
            psRegion *new = psRegionAlloc(region->x0, region->x1, region->y0, region->y1); // Copy of the region
            newItem = p_psMetadataItemAlloc(file, lineno, func, in->name, PS_DATA_REGION, in->comment, new);
            psFree(new);                  // Drop reference
            break;
        }
    default:
        // Other kinds of pointers
      // XXX EAM : why was this a warning??
      // psWarning("Copying a pointer in the metadata item: %s (%x)\n", in->name, in->type);
        newItem = p_psMetadataItemAlloc(file, lineno, func, in->name, in->type, in->comment, in->data.V);
        break;
    }

    return newItem;
}

psMetadata *p_psMetadataCopy(const char *file,
                             unsigned int lineno,
                             const char *func,
                             psMetadata *out,
                             const psMetadata *in)
{
    PS_ASSERT_METADATA_NON_NULL(in, NULL);

    // if calling function passes the same psMetadata for in and out, we are done
    // otherwise, we get stuck in an infinite loop
    if (out == in) return out;

    bool errorOccurred = false;
    bool outAlloced = false;
    if (out ==  NULL) {
        out = p_psMetadataAlloc(file, lineno, func);
        outAlloced = true;
    }
    psMetadataIterator *iter = psMetadataIteratorAlloc(in, PS_LIST_HEAD, NULL);
    psMetadataItem *inItem = NULL;
    while ((inItem = psMetadataGetAndIncrement(iter))) {
        // Need to look for MULTI, which won't be picked up using the iterator.
        psMetadataItem *multiCheckItem = psMetadataLookup(in, inItem->name);
        unsigned int flag = PS_META_REPLACE; // Flag to indicate MULTI; otherwise, replace
        if (multiCheckItem->type == PS_DATA_METADATA_MULTI) {
            psTrace("psLib.types", 10, "MULTI: %s (%s)\n", inItem->name, inItem->comment);
            flag = PS_META_DUPLICATE_OK;
        }
        psTrace("psLib.types", 5, "Copying %s (%s)...\n", inItem->name, inItem->comment);

        // Copy the item and add it on
        psMetadataItem *newItem = psMetadataItemCopy(inItem); // Copied item
        if (!psMetadataAddItem(out, newItem, PS_LIST_TAIL, flag)) {
            psError(PS_ERR_UNKNOWN, false, "Error copying %s (%s) in the metadata\n",
                    inItem->name, inItem->comment);
            if (outAlloced) {
                psFree(out);
                psFree(newItem);
                psFree(iter);
                return NULL;
            } else {
                errorOccurred = true;
            }
        }
        psFree(newItem);                // Drop reference
    }
    psFree(iter);

    if (errorOccurred) {
        return NULL;
    }

    return out;
}

// this function copies the input metadata to the output, raising an error
// if any entries in 'in' a) do not exist in 'out' or b) have a different type
bool p_psMetadataUpdate(const char *file,
                        unsigned int lineno,
                        const char *func,
                        psMetadata *out,
                        const psMetadata *in)
{
    PS_ASSERT_METADATA_NON_NULL(in, NULL);
    PS_ASSERT_METADATA_NON_NULL(out, NULL);

    bool result = true;

    // we loop over the metadata container by item, not by name.  this pushes us directly into
    // the elements of the MULTI items; we miss the containers.  We need to check the
    // (possible) MULTI containers to see how to set the flags.  If we encounter a MULTI which
    // is requesting REPLACE, then the first item on the input list from that MULTI should be
    // added with the REPLACE flag turned on; the rest should have the REPLACE flag turned off.
    // save a list of the MULTI entries which has REPLACE turned off?

    psArray *multiItems = psArrayAllocEmpty (128);

    psMetadataIterator *iter = psMetadataIteratorAlloc(in, PS_LIST_HEAD, NULL);
    psMetadataItem *inItem = NULL;
    while ((inItem = psMetadataGetAndIncrement(iter))) {

        // it is not possible to have RESET && UPDATE
        // is it possible to have !RESET && !UPDATE?
        // are these mutually exclusive conditions?
        // input MULTI & RESET  : replace an existing MULTI or ITEM of same name
        // input MULTI & UPDATE : supplement an existing MULTI or ITEM of same name
        // in both cases the name must exist in 'out'

        // Need to look for MULTI, which won't be picked up using the iterator.
        psMetadataItem *multiCheckItem = psMetadataLookup(in, inItem->name);

        // default operation for the new metadata item (replace existing entry, require existence & type)
        unsigned int flag = PS_META_REPLACE | PS_META_REQUIRE_ENTRY | PS_META_REQUIRE_TYPE;

        // for MULTI items, the mode is carried on the multi container
        if (PS_METADATA_ITEM_GET_TYPE(multiCheckItem) == PS_DATA_METADATA_MULTI) {
            psTrace("psLib.types", 10, "MULTI: %s (%s)\n", inItem->name, inItem->comment);
            if (multiCheckItem->type & PS_META_UPDATE_FOLDER) {
                psTrace("psLib.types", 10, "supplement MULTI with new entries\n");
                flag = PS_META_DUPLICATE_OK | PS_META_REQUIRE_ENTRY | PS_META_REQUIRE_TYPE;
            } else {
                multiCheckItem->type |= PS_META_UPDATE_FOLDER;
                // save this multi so we can reset their flags below
                psArrayAdd (multiItems, 128, multiCheckItem);
            }
        }
        psTrace("psLib.types", 5, "Copying %s (%s)...\n", inItem->name, inItem->comment);

        // for METADATA folders, do something different?
        if (PS_METADATA_ITEM_GET_TYPE(inItem) == PS_DATA_METADATA) {
            if (inItem->type & PS_META_UPDATE_FOLDER) {
                flag = PS_META_UPDATE_FOLDER | PS_META_REQUIRE_ENTRY | PS_META_REQUIRE_TYPE;
            }
        }

        // Copy the item and add it on.  report all errors so we get a listing
        psMetadataItem *newItem = psMetadataItemCopy(inItem); // Copied item
        if (!psMetadataAddItem(out, newItem, PS_LIST_TAIL, flag)) {
            psError(PS_ERR_UNKNOWN, false, "Error copying %s\n", inItem->name);
            result = false;
        }
        psFree(newItem);                // Drop reference
    }
    psFree(iter);

    // remove the UPDATE_FOLDER flag from the following MULTI items
    for (int i = 0; i < multiItems->n; i++) {
        psMetadataItem *item = multiItems->data[i];
        item->type &= ~PS_META_UPDATE_FOLDER;
    }
    psFree (multiItems);

    if (!result) {
        psError(PS_ERR_UNKNOWN, false, "failed to update metadata\n");
    }

    return result;
}

// this function copies the input metadata to the output, supplementing existing metadata
// folders with the contents from corresponding folders in the output
bool p_psMetadataOverlay(const char *file,
                         unsigned int lineno,
                         const char *func,
                         psMetadata *out,
                         const psMetadata *in)
{
    PS_ASSERT_METADATA_NON_NULL(in, NULL);
    PS_ASSERT_METADATA_NON_NULL(out, NULL);

    bool result = true;

    psMetadataIterator *iter = psMetadataIteratorAlloc(in, PS_LIST_HEAD, NULL);
    psMetadataItem *inItem = NULL;
    while ((inItem = psMetadataGetAndIncrement(iter))) {
        // Need to look for MULTI, which won't be picked up using the iterator.
        psMetadataItem *multiCheckItem = psMetadataLookup(in, inItem->name);
        unsigned int flag = PS_META_REPLACE; // Flag to indicate MULTI; otherwise, replace
        if (multiCheckItem->type == PS_DATA_METADATA_MULTI) {
            psTrace("psLib.types", 10, "MULTI: %s (%s)\n", inItem->name, inItem->comment);
            flag = PS_META_DUPLICATE_OK;
        }
        psTrace("psLib.types", 5, "Copying %s (%s)...\n", inItem->name, inItem->comment);

        // if this is a metadata, and it has a corresponding match, overlay them
        if (inItem->type == PS_DATA_METADATA) {
            bool status;
            psMetadata *outFolder = psMetadataLookupMetadata (&status, out, inItem->name);
            if (outFolder) {
                if (!psMetadataOverlay (outFolder, inItem->data.md)) {
                    fprintf (stderr, "Error overlaying metadata folder\n");
                    result = false;
                }
                continue;
            }
        }

        // Copy the item and add it on.  report all errors so we get a listing
        psMetadataItem *newItem = psMetadataItemCopy(inItem); // Copied item
        if (!psMetadataAddItem(out, newItem, PS_LIST_TAIL, flag)) {
            fprintf (stderr, "Error copying %s\n", inItem->name);
            result = false;
        }
        psFree(newItem);                // Drop reference
    }
    psFree(iter);

    if (!result) {
        psError(PS_ERR_UNKNOWN, false, "failed to update metadata\n");
    }
    return result;
}

// may need to extend this to change the keyname in the copy
bool psMetadataItemSupplement(bool *status,
                              psMetadata *out,
                              const psMetadata *in,
                              const char *key)
{
    PS_ASSERT_METADATA_NON_NULL(in, false);
    PS_ASSERT_METADATA_NON_NULL(out, false);
    PS_ASSERT_STRING_NON_EMPTY(key, false);

    psMetadataItem *item = psMetadataLookup(in, key);
    if (!item) {
        if (status) {
            *status = false;
        } else {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Could not find '%s' in metadata.\n", key);
        }
        return false;
    }
    if (!psMetadataAddItem(out, item, PS_LIST_TAIL, PS_META_REPLACE) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Could not add %s to metadata.\n", key);
        return false;
    }

    return true;
}

// XXX is it sensible that item is a 'const' here?
bool psMetadataAddItem(psMetadata *md,
                       const psMetadataItem *item,
                       int location,
                       psS32 flags)
{
    PS_ASSERT_METADATA_NON_NULL(md,false);
    PS_ASSERT_METADATA_ITEM_NON_NULL(item,false);

    psHash *mdTable = md->hash;
    psList *mdList = md->list;
    char *key = item->name;

    // See if key is already in table
    psMetadataItem *existingEntry = psHashLookup(mdTable, key);

    // this block handles cases for the MULTI items
    if (item->type == PS_DATA_METADATA_MULTI) {
        // the incoming entry is PS_DATA_METADATA_MULTI

        // Shouldn't have a second reference to the same MULTI in a single Metadata!
        // XXX not sure I understand this case
        if (item == existingEntry) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Cannot have 2 references to the same MULTI in a single Metadata!");
            return false;
        }

        if ((flags & PS_META_REPLACE) && psMetadataLookup(md, key)) {
            // drop the existing entry or entries
            if (!psMetadataRemoveKey(md, key)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to remove existing item to replace");
                return false;
            }
        } else {
            // elevate the existing hash entry to be PS_DATA_METADATA_MULTI
            existingEntry = makeMetaMulti(mdTable,key,existingEntry);
        }

        // add all the items in the incoming entry to metadata
        psList* list = item->data.list;
        if (list != NULL) {
            psListIterator* iter = psListIteratorAlloc(list,PS_LIST_HEAD,true);
            psMetadataItem* listItem;
            while ((listItem=(psMetadataItem*)psListGetAndIncrement(iter)) != NULL) {
                psMetadataAddItem(md,listItem,location,flags);
            }
            psFree(iter);
        }

        return true; // all done.
    }

    // special block for items which are METADATA folders
    if (existingEntry && (item->type == PS_DATA_METADATA)) {
        // REPLACE and UPDATE must be mutually exclusive
        psAssert ((!((flags & PS_META_REPLACE) && (flags & PS_META_UPDATE_FOLDER))), "cannot have both REPLACE and UPDATE");

        # if (0)
        // handle the case of replace below
        if (flags & PS_META_REPLACE) {
            // drop the existing entry (skip if we are replacing with same pointer)
            // XXX what if existingEntry is a MULTI?  drop all?
            // XXX this segment does not check for matched types
            if (item != existingEntry) {
                psMetadataRemoveKey(md, key);
            }
            existingEntry = NULL;
        }
        # endif

        if (flags & PS_META_UPDATE_FOLDER) {
            if (existingEntry->type != PS_DATA_METADATA) {
                psError(PS_ERR_UNKNOWN, false, "invalid to request UPDATE for metadata which matches another type");
                return false;
            }

            // merge the existing entry : this completes the insert
            if (!psMetadataCopy ((psMetadata *)existingEntry->data.V, (psMetadata *)item->data.V)) {
                psError(PS_ERR_UNKNOWN, false, "failed to copy new metadata on existing");
                return false;
            }
            return true;
        }
    }

    // how the item is added to the hash depends on prior existence, flags, etc.
    // XXX i think these cases are overloaded - are all combinations possible?
    if (existingEntry) { // prior existence

        // duplicate entries allowed - add another entry.
        if ((existingEntry->type == PS_DATA_METADATA_MULTI) || (flags & PS_META_DUPLICATE_OK)) {

            // make sure the existing entry is PS_DATA_METADATA_MULTI
            existingEntry = makeMetaMulti(mdTable,key,existingEntry);

            // add item to the existing hash's list of duplicate entries
            if (!psListAdd(existingEntry->data.list, PS_LIST_TAIL, (psMetadataItem *) item) ) {
                psError(PS_ERR_UNKNOWN, false, _("Failed to add metadata item, %s, to metadata collection list."), key);
                return false;
            }
            // add to the metadata list of entries
            if (!psListAdd(mdList, location, (psMetadataItem *) item)) {
                psError(PS_ERR_UNKNOWN, false, _("Failed to add metadata item, %s, to metadata collection list."), key);
                return false;
            }
            return true;
        }

        // replace entry instead of creating a duplicate entry.
        if (flags & PS_META_REPLACE) {

            if ((flags & PS_META_REQUIRE_TYPE) && (existingEntry->type != item->type)) {
                psError (PS_ERR_UNKNOWN, true, _("Existing item %s does not match type (%x) for item requiring matching type (%x)"), key, existingEntry->type, item->type);
                return false;
            }
            if (item != existingEntry) {
                // Replacing an item with itself can lead to memory corruption
                // when you blow away what you're trying to add
                psMetadataRemoveKey(md, key);
                // add to the metadata has of entries
                if (!psHashAdd(mdTable, key, (psMetadataItem *) item)) {
                    psError(PS_ERR_UNKNOWN, false, _("Failed to add metadata item, %s, to metadata collection list."), key);
                    return false;
                }
                // add to the metadata list of entries
                if (!psListAdd(mdList, location, (psMetadataItem *) item)) {
                    psError(PS_ERR_UNKNOWN, false, _("Failed to add metadata item, %s, to metadata collection list."), key);
                    return false;
                }
                return true;
            } else {
              // adding myself back to the list; this is a NOP
              return true;
            }
        }

        // if specified, keep the existing entry
        if (flags & PS_META_NO_REPLACE) {
            return true;
        }

        // default is to error on duplicate entry.
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Duplicate metadata item name: %s is not allowed.  Use a psMetadataFlags option to allow such action."), item->name);

        return false;
    }

    // OK, this is a new item.
    if (flags & PS_META_REQUIRE_ENTRY) {
        psError (PS_ERR_UNKNOWN, true, _("No matching item found for item requiring existing entry (%s)"), key);
        return false;
    }

    // Node doesn't exist - Add new metadata item to metadata collection's hash
    psHashAdd(mdTable, key, (psMetadataItem *) item);

    // Create a multi, if requested
    if (flags & PS_META_DUPLICATE_OK) {
        makeMetaMulti(mdTable, key, (psMetadataItem *) item); // Casting away const!
    }

    if (!psListAdd(mdList, location, (psPtr)item)) {
        psError(PS_ERR_UNKNOWN, false, _("Failed to add metadata item, %s, to metadata collection list."), key);
        return false;
    }

    return true;
}

bool psMetadataAdd(psMetadata *md,
                   long location,
                   const char *name,
                   int format,
                   const char *comment,...)
{
    PS_ASSERT_METADATA_NON_NULL(md, false);

    va_list argPtr;
    va_start(argPtr, comment);
    bool result = psMetadataAddV(md,location,name,format,comment,argPtr);
    va_end(argPtr);

    return result;
}

bool psMetadataAddV(psMetadata *md,
                    long location,
                    const char *name,
                    int format,
                    const char *comment,
                    va_list list)
{
    PS_ASSERT_METADATA_NON_NULL(md,false);

    psMetadataItem* metadataItem = psMetadataItemAllocV(name, format & PS_METADATA_TYPE_MASK, comment, list);

    if (!psMetadataAddItem(md, metadataItem, location, format & PS_METADATA_FLAGS_MASK)) {
        psError(PS_ERR_UNKNOWN,false,_("Failed to add metadata item to metadata collection list."));
        psFree(metadataItem);
        return false;
    }
    // Decrement reference count, since the metadata item is now in metadata collection and no longer needed
    psFree(metadataItem);

    return true;
}

#define METADATA_ADD_TYPE(NAME,TYPE,METATYPE) \
bool psMetadataAdd##NAME(psMetadata* md, long where, const char* name, \
                         int format, const char* comment, TYPE value) { \
    return psMetadataAdd(md, where, name, format | METATYPE, comment, value); \
}

METADATA_ADD_TYPE(Bool,bool,PS_DATA_BOOL)
METADATA_ADD_TYPE(S8,psS8,PS_DATA_S8)
METADATA_ADD_TYPE(S16,psS16,PS_DATA_S16)
METADATA_ADD_TYPE(S32,psS32,PS_DATA_S32)
METADATA_ADD_TYPE(S64,psS64,PS_DATA_S64)
METADATA_ADD_TYPE(U8,psU8,PS_DATA_U8)
METADATA_ADD_TYPE(U16,psU16,PS_DATA_U16)
METADATA_ADD_TYPE(U32,psU32,PS_DATA_U32)
METADATA_ADD_TYPE(U64,psU64,PS_DATA_U64)
METADATA_ADD_TYPE(F32,psF32,PS_DATA_F32)
METADATA_ADD_TYPE(F64,psF64,PS_DATA_F64)

METADATA_ADD_TYPE(Mask,psMaskType,PS_TYPE_MASK)
METADATA_ADD_TYPE(VectorMask,psVectorMaskType,PS_TYPE_VECTOR_MASK)
METADATA_ADD_TYPE(ImageMask,psImageMaskType,PS_TYPE_IMAGE_MASK)

METADATA_ADD_TYPE(List,psList*,PS_DATA_LIST)
METADATA_ADD_TYPE(Str,const char*,PS_DATA_STRING)
METADATA_ADD_TYPE(Vector,psVector*,PS_DATA_VECTOR)
METADATA_ADD_TYPE(Image,psImage*,PS_DATA_IMAGE)
METADATA_ADD_TYPE(Hash,psHash*,PS_DATA_HASH)
METADATA_ADD_TYPE(LookupTable,psLookupTable*,PS_DATA_LOOKUPTABLE)
METADATA_ADD_TYPE(Metadata,psMetadata*,PS_DATA_METADATA)
METADATA_ADD_TYPE(Array,psArray*,PS_DATA_ARRAY)
METADATA_ADD_TYPE(Time,psTime*,PS_DATA_TIME)
METADATA_ADD_TYPE(Unknown,psPtr,PS_DATA_UNKNOWN)

bool psMetadataAddPtr(psMetadata* md, long where, const char* name,
                      psDataType type, const char* comment, psPtr value)
{
    return psMetadataAdd(md, where, name, type, comment, value);
}



// Remove by key name
bool psMetadataRemoveKey(psMetadata *md,
                         const char *key)
{
    PS_ASSERT_METADATA_NON_NULL(md, false);
    PS_ASSERT_STRING_NON_EMPTY(key, false);

    psList* mdList = md->list;
    psHash* mdTable = md->hash;

    psMetadataItem* entry = psHashLookup(mdTable,key);
    if (entry == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                _("Failed to remove metadata item, %s, from metadata table."), key);
        return false;
    }
    if (entry->type == PS_DATA_METADATA_MULTI) {
        psMetadataItem* listItem;
        psListIterator* iter = psListIteratorAlloc(
                                   entry->data.list,
                                   PS_LIST_HEAD,true);
        while ((listItem=psListGetAndIncrement(iter)) != NULL) {
            psListRemoveData(mdList, listItem);
        }
        psFree(iter);
        psHashRemove(mdTable,key);
    } else {
        psListRemoveData(mdList, entry);
        psHashRemove(mdTable, key);
    }

    return true;
}


// Remove by index
bool psMetadataRemoveIndex(psMetadata *md,
                           long location)
{
    PS_ASSERT_METADATA_NON_NULL(md, false);

    psList* mdList = md->list;
    psHash* mdTable = md->hash;

    psMetadataItem* entry = psListGet(mdList, location);
    if (entry == NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Could not find metadata item at index %ld."), location);
        return false;
    }
    const char *key = entry->name;

    if (key == NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Failed to remove metadata item, at index %ld, from metadata list."),
                location);
        return false;
    }

    psMetadataItem* tableItem = psHashLookup(mdTable, key);
    if (tableItem == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                _("Failed to remove metadata item, %s, from metadata table."), key);
        return false;
    }

    if (tableItem->type == PS_DATA_METADATA_MULTI) {
        // multiple entries with same key, remove just the specified one
        psListRemoveData(tableItem->data.list, entry);
    } else {
        //Tested below.  psHashRemove can't return false here.
        psHashRemove(mdTable, key);
    }
    psListRemove(mdList, location);

    return true;
}

// Get entry by index
psMetadataItem *psMetadataGetIndex(psMetadata *md, long location)
{
    PS_ASSERT_METADATA_NON_NULL(md, false);
    return psListGet(md->list, location);
}

psMetadataItem *psMetadataLookup(const psMetadata *md,
                                 const char *key)
{
    PS_ASSERT_METADATA_NON_NULL(md,NULL);
    PS_ASSERT_STRING_NON_EMPTY(key,NULL);

    return (psMetadataItem*)psHashLookup(md->hash, key);
}

void* psMetadataLookupPtr(bool *status,
                          const psMetadata *md,
                          const char *key)
{
    PS_ASSERT_METADATA_NON_NULL(md,NULL);

    if (status) {
        *status = true;
    }

    psMetadataItem *metadataItem = psMetadataLookup(md, key);
    if (metadataItem == NULL) {
        if (status) {
            *status = false;
        }
        return NULL;
    }
    if (metadataItem->type == PS_DATA_METADATA_MULTI) {
        // if multiple keys found, use the first.
        metadataItem = (psMetadataItem*)(metadataItem->data.list->head->data);
        if (status) {
            *status = true;
        }
    }

    if(PS_DATA_IS_PRIMITIVE(metadataItem->type)) {
        if (status) {
            *status = false;
        }
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified psDataType, %d, is not supported."),
                metadataItem->type);
        return NULL;
    } else {
        return metadataItem->data.V;
    }
}

#define psMetadataLookupNumTYPE(TYPE,NAME) \
ps##TYPE psMetadataLookup##NAME(bool *status, const psMetadata *md, const char *key) \
{ \
    psMetadataItem *metadataItem = NULL; \
    ps##TYPE value = 0; \
    \
    if (status) {  \
        *status = true; \
    } \
    \
    metadataItem = psMetadataLookup(md, key); \
    if(metadataItem == NULL) { \
        if (status) {  \
            *status = false; \
        } \
        return 0; \
    } \
    if (metadataItem->type == PS_DATA_METADATA_MULTI) { \
        /* if multiple keys found, use the first. */ \
        metadataItem = (psMetadataItem*)((metadataItem->data.list)->head); \
    } \
    \
    switch (metadataItem->type) { \
    case PS_DATA_S8: \
        value = (ps##TYPE)metadataItem->data.S8; \
        break; \
    case PS_DATA_S16: \
        value = (ps##TYPE)metadataItem->data.S16; \
        break; \
    case PS_DATA_S32: \
        value = (ps##TYPE)metadataItem->data.S32; \
        break; \
    case PS_DATA_S64: \
        value = (ps##TYPE)metadataItem->data.S64; \
        break; \
    case PS_DATA_U8: \
        value = (ps##TYPE)metadataItem->data.U8; \
        break; \
    case PS_DATA_U16: \
        value = (ps##TYPE)metadataItem->data.U16; \
        break; \
    case PS_DATA_U32: \
        value = (ps##TYPE)metadataItem->data.U32; \
        break; \
    case PS_DATA_U64: \
        value = (ps##TYPE)metadataItem->data.U64; \
        break; \
    case PS_DATA_F32: \
        value = (ps##TYPE)metadataItem->data.F32; \
        break; \
    case PS_DATA_F64: \
        value = (ps##TYPE)metadataItem->data.F64; \
        break; \
    case PS_DATA_BOOL: \
        if (metadataItem->data.B) { \
            value = 1; \
        } \
        break; \
    default: \
        /* if you get to this point, the value is not a number. */ \
        if (status) {  \
            *status = false; \
        }  \
        break; \
    } \
    \
    return value; \
}

psMetadataLookupNumTYPE(F32,F32)
psMetadataLookupNumTYPE(F64,F64)
psMetadataLookupNumTYPE(S8,S8)
psMetadataLookupNumTYPE(S16,S16)
psMetadataLookupNumTYPE(S32,S32)
psMetadataLookupNumTYPE(S64,S64)
psMetadataLookupNumTYPE(U8,U8)
psMetadataLookupNumTYPE(U16,U16)
psMetadataLookupNumTYPE(U32,U32)
psMetadataLookupNumTYPE(U64,U64)
psMetadataLookupNumTYPE(Bool,Bool)

psMetadataLookupNumTYPE(MaskType,Mask)
psMetadataLookupNumTYPE(VectorMaskType,VectorMask)
psMetadataLookupNumTYPE(ImageMaskType,ImageMask)

#define psMetadataLookupPtrTYPE(TYPENAME,NAME,TYPE,VAL) \
TYPENAME psMetadataLookup##NAME(bool *status, const psMetadata *md, const char *key) \
{ \
    PS_ASSERT_METADATA_NON_NULL(md, NULL); \
    PS_ASSERT_STRING_NON_EMPTY(key, NULL); \
    psMetadataItem *item = psMetadataLookup((psMetadata*)md, key); /* The item of interest */ \
    if (!item) { \
        if (status) { \
            *status = false; \
        } else { \
	    psError(PS_ERR_IO, true, "Couldn't find %s in the metadata.\n", key); \
         /* psLogMsg(__func__, PS_LOG_DETAIL, "Couldn't find %s in the metadata.\n", key); */  \
        } \
        return NULL; \
    } \
    \
    if (item->type == PS_DATA_METADATA_MULTI) { \
        /* if multiple keys found, use the first. */ \
        item = item->data.list->head->data; \
    } \
    if (item->type != TYPE) { \
        if (status) { \
            *status = false; \
        } else { \
            psLogMsg(__func__, PS_LOG_DETAIL, "%s isn't of type PS_DATA_META, as expected.\n", key); \
        } \
        return NULL; \
    } \
    \
    if (status) { \
        *status = true; \
    } \
    return item->data.VAL; \
}

psMetadataLookupPtrTYPE(psMetadata*, Metadata, PS_DATA_METADATA, md)
psMetadataLookupPtrTYPE(psString, Str, PS_DATA_STRING, str)
psMetadataLookupPtrTYPE(psTime*, Time, PS_DATA_TIME, V)
psMetadataLookupPtrTYPE(psVector*, Vector, PS_DATA_VECTOR, V)


psMetadataItem* psMetadataGet(const psMetadata *md,
                              int location)
{
    psMetadataItem* entry = NULL;

    PS_ASSERT_METADATA_NON_NULL(md,NULL);

    // XXX remove this as an error
    entry = (psMetadataItem*) psListGet(md->list, location);
    if (entry == NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Could not find metadata item at index %d."), location);
        return NULL;
    }

    return entry;
}

// XXX should md be const?
psMetadataIterator* p_psMetadataIteratorAlloc(const char *file,
                                              unsigned int lineno,
                                              const char *func,
                                              const psMetadata* md,
                                              long location,
                                              const char* regex)
{
    PS_ASSERT_METADATA_NON_NULL(md,NULL);

    psMetadataIterator* newIter = p_psAlloc(file, lineno, func, sizeof(psMetadataIterator));
    psMemSetDeallocator(newIter, (psFreeFunc) metadataIteratorFree);

    newIter->iter = psListIteratorAlloc(md->list, location, false);

    if (regex) {
        newIter->regex = psAlloc(sizeof(regex_t));
        int regRtn = regcomp(newIter->regex, regex, 0);
        if (regRtn != 0) {
	    char errMsg[256]; // fixed buffer length should match in call below
            regerror(regRtn, newIter->regex, errMsg, 256);
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Specified regular expression is invalid.  %s."),
                    errMsg);
            psFree(newIter);
            return NULL;
        }
        if (!psMetadataIteratorSet(newIter, location)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to set location=%ld for metadata iterator.\n", location);
            psFree(newIter);
            return NULL;
        }
    } else {
        newIter->regex = NULL;
    }

    return newIter;
}

bool psMetadataIteratorSet(psMetadataIterator* iterator,
                           long location)
{
    PS_ASSERT_METADATA_ITERATOR_NON_NULL(iterator, NULL);
    psListIterator *listIter = iterator->iter;

    regex_t *regex = iterator->regex;

    // handle trivial case where no regex subsetting is required.
    if (regex == NULL) {
        return psListIteratorSet(listIter, location);
    }

    // If there's a regex, then we need to count into the list.
    // We count ONLY those entries that match the regex

    // Here we count in from the tail
    if (location < 0) {
        psListIteratorSet(listIter, PS_LIST_TAIL);
        psMetadataItem *item;           // Item from iteration
        int match = 0;                  // Match number
        while (listIter->cursor && (item = listIter->cursor->data)) {
            if (regexec(regex, item->name, 0, NULL, 0) == 0) {
                // this key is a match
                match--;
                if (match == location) {
                    break;
                }
            }
            (void)psListGetAndDecrement(listIter);
        }

        // We return "true" even if we didn't find the Nth match, because the iterator is set appropriately
        // (off the front), and subsequent calls to psMetadataGetAnd{In,De}crement will return NULL.
        return true;
    }

    // Here we count in from the head
    psListIteratorSet(listIter, PS_LIST_HEAD);
    psMetadataItem *item;               // Item from iteration
    int match = -1;                     // Match number
    while (listIter->cursor && (item = listIter->cursor->data)) {
        if (regexec(regex, item->name, 0, NULL, 0) == 0) {
            // this key is a match
            match++;
            if (match == location) {
                break;
            }
        }
        (void)psListGetAndIncrement(listIter);
    }
    // We return "true" even if we didn't find the Nth match, because the iterator is set appropriately
    // (off the end), and subsequent calls to psMetadataGetAnd{In,De}crement will return NULL.
    return true;
}

psMetadataItem* psMetadataGetAndIncrement(psMetadataIterator* iterator)
{
    PS_ASSERT_METADATA_ITERATOR_NON_NULL(iterator,NULL);
    psListIterator* listIter = iterator->iter;
    regex_t* regex = iterator->regex;

    // handle trivial case where no regex subsetting is required.
    if (regex == NULL) {
        return (psMetadataItem*)psListGetAndIncrement(listIter);
    }

    // Iterate until we find something matching the regex
    psMetadataItem *newItem;            // New MD item from iteration
    while ((newItem = psListGetAndIncrement(listIter))) {
        if (regexec(regex, newItem->name, 0, NULL, 0) == 0) {
            // this key is a match
            break;
        }
    }
    return newItem;
}

psMetadataItem* psMetadataGetAndDecrement(psMetadataIterator* iterator)
{
    PS_ASSERT_METADATA_ITERATOR_NON_NULL(iterator, NULL);
    psListIterator* listIter = iterator->iter;
    regex_t* regex = iterator->regex;

    // handle trivial case where no regex subsetting is required.
    if (regex == NULL) {
        return (psMetadataItem*)psListGetAndDecrement(listIter);
    }

    // Iterate until we find something matching the regex
    psMetadataItem *newItem;            // New MD item from iteration
    while ((newItem = psListGetAndDecrement(listIter))) {
        if (regexec(regex, newItem->name, 0, NULL, 0) == 0) {
            // this key is a match
            break;
        }
    }
    return newItem;
}





psList *psMetadataKeys(psMetadata *md)
{
    PS_ASSERT_METADATA_NON_NULL(md, NULL);
    return psHashKeyList(md->hash);
}


bool psMetadataPrint(FILE *fd,
                     const psMetadata *md,
                     int level)
{
    PS_ASSERT_METADATA_NON_NULL(md,false);
    if (fd == NULL) {
        fd = stdout;
    } else {
        if ( fprintf(fd, "\n") < 0 ) {
            psError(PS_ERR_IO, true,
                    "Invalid file pointer in psMetadataPrint.  Could not write to fd.\n");
            return false;
        }
    }

    psErrorClear();   // we're going to _append_ errors to the stack
    bool noErrors = true;  // have see seen any errors?

    // Casting away const --- the addition of an iterator should not be considered an invasion of "const".
    psMetadataIterator *iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item = NULL; // Item from metadata
    while ( (item = psMetadataGetAndIncrement(iter)) ) {
        for (int i = 0; i < level; i++) {
            fprintf(fd, "  ");
        }
        fprintf(fd, "%s", item->name);
        if (item->comment && strlen(item->comment) > 0) {
            fprintf(fd, " (%s)", item->comment);
        }
        fprintf(fd, ": ");

        switch (item->type) {
        case PS_DATA_STRING:
            fprintf(fd, "%s\n", item->data.str);
            break;
        case PS_DATA_BOOL:
            if (item->data.B) {
                fprintf(fd, "True\n");
            } else {
                fprintf(fd, "False\n");
            }
            break;
        case PS_DATA_S8:
            fprintf(fd, "%d\n", item->data.S8);
            break;
        case PS_DATA_S16:
            fprintf(fd, "%d\n", item->data.S16);
            break;
        case PS_DATA_S32:
            fprintf(fd, "%d\n", item->data.S32);
            break;
        case PS_DATA_S64:
            fprintf(fd, "%" PRId64 "\n", item->data.S64);
            break;
        case PS_DATA_U8:
            fprintf(fd, "%u\n", item->data.U8);
            break;
        case PS_DATA_U16:
            fprintf(fd, "%u\n", item->data.U16);
            break;
        case PS_DATA_U32:
            fprintf(fd, "%u\n", item->data.U32);
            break;
        case PS_DATA_U64:
            fprintf(fd, "%" PRIu64 "\n", item->data.U64);
            break;
        case PS_DATA_F32:
            fprintf(fd, "%f\n", item->data.F32);
            break;
        case PS_DATA_F64:
            fprintf(fd, "%g\n", item->data.F64);
            break;
        case PS_DATA_METADATA:
            if (!item->data.V) {
                fprintf(fd, "(null)\n");
                break;
            }
            psMetadataPrint(fd, item->data.V, level + 1);
            for (int i = 0; i < level; i++) {
                fprintf(fd, "  ");
            }
            fprintf(fd, "%s  -- END\n", item->name);
            break;
        case PS_DATA_REGION:
            if (!item->data.V) {
                fprintf(fd, "(null)\n");
                break;
            }
            psString region = psRegionToString(*(psRegion*)item->data.V);
            fprintf(fd, "%s\n", region);
            psFree(region);
            break;
        case PS_DATA_LIST:
            if (!item->data.V) {
                fprintf(fd, "(null)\n");
                break;
            }
            fprintf(fd, "<a list of size %ld>\n", ((psList*)item->data.V)->n);
            break;
        case PS_DATA_TIME:
            if (!item->data.V) {
                fprintf(fd, "(null)\n");
                break;
            }
            psString time = psTimeToISO(item->data.V);
            fprintf(fd, "%s\n", time);
            psFree(time);
            break;
        case PS_DATA_VECTOR:
          if (!item->data.V) {
              fprintf(fd, "(null)\n");
              break;
          }
          psVector *vector = item->data.V;
          switch (vector->type.type) {
            case PS_DATA_U8:
              fprintf(fd, "U8  ");
              for (int i = 0; i < vector->n; i++) {
                  fprintf(fd, "%u ", vector->data.U8[i]);
              }
              fprintf(fd, "\n");
              break;
            case PS_DATA_U16:
              fprintf(fd, "U16  ");
              for (int i = 0; i < vector->n; i++) {
                  fprintf(fd, "%u ", vector->data.U16[i]);
              }
              fprintf(fd, "\n");
              break;
            case PS_DATA_U32:
              fprintf(fd, "U32  ");
              for (int i = 0; i < vector->n; i++) {
                  fprintf(fd, "%u ", vector->data.U32[i]);
              }
              fprintf(fd, "\n");
              break;
            case PS_DATA_U64:
              fprintf(fd, "U64  ");
              for (int i = 0; i < vector->n; i++) {
                  fprintf(fd, "%" PRIu64, vector->data.U64[i]);
              }
              fprintf(fd, "\n");
              break;
            case PS_DATA_S8:
              fprintf(fd, "S8  ");
              for (int i = 0; i < vector->n; i++) {
                  fprintf(fd, "%d ", vector->data.S8[i]);
              }
              fprintf(fd, "\n");
              break;
            case PS_DATA_S16:
              fprintf(fd, "S16  ");
              for (int i = 0; i < vector->n; i++) {
                  fprintf(fd, "%d ", vector->data.S16[i]);
              }
              fprintf(fd, "\n");
              break;
            case PS_DATA_S32:
              fprintf(fd, "S32  ");
              for (int i = 0; i < vector->n; i++) {
                  fprintf(fd, "%d ", vector->data.S32[i]);
              }
              fprintf(fd, "\n");
              break;
            case PS_DATA_S64:
              fprintf(fd, "S64  ");
              for (int i = 0; i < vector->n; i++) {
                  fprintf(fd, "%" PRId64, vector->data.S64[i]);
              }
              fprintf(fd, "\n");
              break;
            case PS_DATA_F32:
              fprintf(fd, "F32 ");
              for (int i = 0; i < vector->n; i++) {
                  fprintf(fd, "%f ", vector->data.F32[i]);
              }
              fprintf(fd, "\n");
              break;
            case PS_DATA_F64:
              fprintf(fd, "F64 ");
              for (int i = 0; i < vector->n; i++) {
                  fprintf(fd, "%f ", vector->data.F64[i]);
              }
              fprintf(fd, "\n");
              break;
            default:
              fprintf(fd, "<Unsupported type>\n");
          }
          break;
        default:
          fprintf(fd, "<Unsupported type>\n");
          noErrors = false;
          break;
        }
    }
    psFree(iter);

    return noErrors;
}
