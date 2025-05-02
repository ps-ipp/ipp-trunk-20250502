/** @file  psMetadata.h
*
*  @brief Contains metadata struuctures, enumerations and functions prototypes
*
*  This file defines metadata item, metadata type, metadata flags, metadata containers, and function
*  prototypes necessary creating psLib metadata APIs
*
*  @author Robert DeSonia, MHPCC
*  @author Ross Harman, MHPCC
*
*  @version $Revision: 1.108 $ $Name: not supported by cvs2svn $
*  @date $Date: 2009-02-04 01:50:48 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/
#ifndef PS_METADATA_H
#define PS_METADATA_H

/// @addtogroup DataContainer Data Containers
/// @{

#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <regex.h>

#include "psHash.h"
#include "psList.h"
#include "psTime.h"
#include "psLookupTable.h"
#include "psMutex.h"
#include "psError.h"

#define PS_DATA_IS_PRIMITIVE(TYPE) \
(TYPE == PS_DATA_S8 || \
 TYPE == PS_DATA_S16 || \
 TYPE == PS_DATA_S32 || \
 TYPE == PS_DATA_S64 || \
 TYPE == PS_DATA_U8 || \
 TYPE == PS_DATA_U16 || \
 TYPE == PS_DATA_U32 || \
 TYPE == PS_DATA_U64 || \
 TYPE == PS_DATA_F32 || \
 TYPE == PS_DATA_F64 || \
 TYPE == PS_DATA_BOOL)

#define PS_DATA_PRIMITIVE_TYPE(DATATYPE) ( \
        (DATATYPE==PS_DATA_S8 || DATATYPE==PS_DATA_S16 || \
         DATATYPE==PS_DATA_S32 || DATATYPE==PS_DATA_S64 || DATATYPE==PS_DATA_U8 || \
         DATATYPE==PS_DATA_U16 || DATATYPE==PS_DATA_U32 || DATATYPE==PS_DATA_U64 || \
         DATATYPE==PS_DATA_F32 || DATATYPE==PS_DATA_F64 || DATATYPE==PS_DATA_BOOL) ? DATATYPE : 0)


/** Option flags for psMetadata functions
 *
 *  Enumeration for the modification of the behaviour in psMetadataAddItem.
 *
 *  @see psMetadataAddItem
 */
typedef enum {
    PS_META_DEFAULT       = 0,          ///< default behaviour (duplicate entry is an error)
    PS_META_REPLACE       = 0x01000000, ///< allow entry to be replaced
    PS_META_NO_REPLACE    = 0x02000000, ///< duplicate entry is silently skipped
    PS_META_DUPLICATE_OK  = 0x04000000, ///< allow duplicate entries
    PS_META_UPDATE_FOLDER = 0x08000000, ///< for a metadata folder, merge contents with existing md
    PS_META_NULL          = 0x10000000, ///< psMetadataItem.data is a NULL value
    PS_META_REQUIRE_ENTRY = 0x20000000, ///< require pre-existing entry with same name
    PS_META_REQUIRE_TYPE  = 0x40000000  ///< require pre-existing entry to have same type
} psMetadataFlags;

#define PS_METADATA_FLAGS_MASK 0xFF000000
#define PS_METADATA_TYPE_MASK 0x00FFFFFF

#define PS_METADATA_ITEM_GET_TYPE(MDITEM) (MDITEM->type & PS_METADATA_TYPE_MASK)

/** Metadata data structure.
 *
 *  Struct for holding metadata items. Metadata items are held in two
 *  containers. The first employs a doubly-linked list to preserve the order
 *  of the metadata. The second container employs a hash table which
 *  allows fast lookup when given a metadata keyword.
 */
typedef struct
{
    psList*  list;                     ///< Metadata in linked-list
    psHash*  hash;                     ///< Metadata in a hash table
    psMutex lock;                       ///< Optional lock for thread safety
}
psMetadata;


/** Metadata iterator
 *
 *  Iterator for metadata.
 */
typedef struct
{
    psListIterator* iter;              ///< iterator for the psMetadata's psList
    regex_t* regex;                    ///< the subsetting regular expression
}
psMetadataIterator;


/** Metadata item data structure.
 *
 * Struct for maintaining metadata items of varying types. It also contains
 * information about the item name, flags, comments, and other items with the same name.
 */
typedef struct
{
    const psS32 id;                    ///< Unique ID for metadata item.
    psString name;                     ///< Name of metadata item.
    psDataType type;                   ///< Type of metadata item.
    union {
        bool B;                      ///< boolean data
        psS8 S8;                       ///< Signed 8-bit integer data.
        psS16 S16;                     ///< Signed 16-bit integer data.
        psS32 S32;                     ///< Signed 32-bit integer data.
        psS64 S64;                     ///< Signed 64-bit integer data.
        psU8 U8;                       ///< Unsigned 8-bit integer data.
        psU16 U16;                     ///< Unsigned 16-bit integer data.
        psU32 U32;                     ///< Unsigned 32-bit integer data.
        psU64 U64;                     ///< Unsigned 64-bit integer data.
        psF32 F32;                     ///< Single-precision float data.
        psF64 F64;                     ///< Double-precision float data.
        psList *list;                  ///< List data.
        psMetadata *md;                ///< Metadata data.
        psString str;                  ///< string data
        psPtr V;                       ///< Pointer to other type of data.
    } data;                            ///< Union for data types.
    psString comment;                  ///< Optional comment ("", not NULL).
}
psMetadataItem;


/** Create a metadata item.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct. The name argument specifies the name to use for this item, and
 *  may include sprintf formatting codes. The format entry specifies both
 *  the metadata type and optional flags and is created by bit-wise or of the
 *  appropriate type and flag. The comment argument is a fixed string used to
 *  comment the metadata item. The arguments to the name formatting codes and
 *  the metadata itself are passed as arguments following the comment string.
 *  The data must be a pointer for any of the elements stored in data.void.
 *  The argument list must be interpreted appropriately by the va_list
 *  operators in the function specified size and type.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
#ifdef DOXYGEN
psMetadataItem* psMetadataItemAlloc(
    const char *name,                  ///< Name of metadata item.
    psDataType type,                   ///< Type of metadata item.
    const char *comment,               ///< Comment for metadata item.
    ...                                ///< Arguments for name formatting and metadata item data.
);
#else // ifdef DOXYGEN
psMetadataItem* p_psMetadataItemAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    const char *name,                  ///< Name of metadata item.
    psDataType type,                   ///< Type of metadata item.
    const char *comment,               ///< Comment for metadata item.
    ...                                ///< Arguments for name formatting and metadata item data.
) PS_ATTR_MALLOC;
#define psMetadataItemAlloc(name, type, ...) \
      p_psMetadataItemAlloc(__FILE__, __LINE__, __func__, name, type, __VA_ARGS__)
#endif // ifdef DOXYGEN


/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psMetadataItem structure, false otherwise.
 */
bool psMemCheckMetadataItem(
    psPtr ptr                          ///< the pointer whose type to check
);

// XXX Although all of the psMetadataItemAlloc[type] prototypes are allocators,
// I've decided not to modify them all to pass in file, lineno, func.
// Hopefully these prototypes will become macros or generated by macros some day
// so this can be done automatically.  -JH

/** Create a metadata item with specified string data.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataItemAllocStr(
    const char* name,                  ///< Name of metadata item.
    const char* comment,               ///< Comment for metadata item.
    const char* value                  ///< the value of the metadata item.
);


/** Create a metadata item with specified psF32 data.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataItemAllocF32(
    const char* name,                  ///< Name of metadata item.
    const char* comment,               ///< Comment for metadata item.
    psF32 value                        ///< the value of the metadata item.
);

/** Create a metadata item with specified psF64 data.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataItemAllocF64(
    const char* name,                  ///< Name of metadata item.
    const char* comment,               ///< Comment for metadata item.
    psF64 value                        ///< the value of the metadata item.
);


/** Create a metadata item with specified psS8 data.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataItemAllocS8(
    const char* name,                  ///< Name of metadata item.
    const char* comment,               ///< Comment for metadata item.
    psS8 value                         ///< the value of the metadata item.
);


/** Create a metadata item with specified psS16 data.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataItemAllocS16(
    const char* name,                  ///< Name of metadata item.
    const char* comment,               ///< Comment for metadata item.
    psS16 value                        ///< the value of the metadata item.
);


/** Create a metadata item with specified psS32 data.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataItemAllocS32(
    const char* name,                  ///< Name of metadata item.
    const char* comment,               ///< Comment for metadata item.
    psS32 value                        ///< the value of the metadata item.
);


/** Create a metadata item with specified psS64 data.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataItemAllocS64(
    const char* name,                  ///< Name of metadata item.
    const char* comment,               ///< Comment for metadata item.
    psS64 value                        ///< the value of the metadata item.
);


/** Create a metadata item with specified psU8 data.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataItemAllocU8(
    const char* name,                  ///< Name of metadata item.
    const char* comment,               ///< Comment for metadata item.
    psU8 value                         ///< the value of the metadata item.
);


/** Create a metadata item with specified psU16 data.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataItemAllocU16(
    const char* name,                  ///< Name of metadata item.
    const char* comment,               ///< Comment for metadata item.
    psU16 value                        ///< the value of the metadata item.
);


/** Create a metadata item with specified psU32 data.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataItemAllocU32(
    const char* name,                  ///< Name of metadata item.
    const char* comment,               ///< Comment for metadata item.
    psU32 value                        ///< the value of the metadata item.
);


/** Create a metadata item with specified psU64 data.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataItemAllocU64(
    const char* name,                  ///< Name of metadata item.
    const char* comment,               ///< Comment for metadata item.
    psU64 value                        ///< the value of the metadata item.
);


/** Create a metadata item with specified bool data.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataItemAllocBool(
    const char* name,                  ///< Name of metadata item.
    const char* comment,               ///< Comment for metadata item.
    bool value                         ///< the value of the metadata item.
);


/** Create a metadata item with specified psPtr data.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataItemAllocPtr(
    const char* name,                  ///< Name of metadata item.
    psDataType type,                   ///< Data type of metadata item.
    const char* comment,               ///< Comment for metadata item.
    psPtr value                        ///< the value of the metadata item.
);


/** Create a metadata item with va_list.
 *
 *  Returns a fill psMetadataItem ready for insertion into the psMetadata
 *  struct. The name argument specifies the name to use for this item, and
 *  may include sprintf formatting codes. The format entry specifies both
 *  the metadata type and optional flags and is created by bit-wise or of the
 *  appropriate type and flag. The comment argument is a fixed string used to
 *  comment the metadata item. The arguments to the name formatting codes and
 *  the metadata itself are passed as arguments following the comment string.
 *  The data must be a pointer for any of the elements stored in data.void.
 *  The argument list must be interpreted appropriately by the va_list
 *  operators in the function specified size and type.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
#ifndef SWIG
#ifdef DOXYGEN
psMetadataItem* psMetadataItemAllocV(
    const char *name,                  ///< Name of metadata item.
    psDataType type,                   ///< Type of metadata item.
    const char *comment,               ///< Comment for metadata item.
    va_list list                       ///< Arguments for name formatting and metadata item data.
);
#else // ifdef DOXYGEN
psMetadataItem* p_psMetadataItemAllocV(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    const char *name,                  ///< Name of metadata item.
    psDataType type,                   ///< Type of metadata item.
    const char *comment,               ///< Comment for metadata item.
    va_list list                       ///< Arguments for name formatting and metadata item data.
);
#define psMetadataItemAllocV(name, type, comment, list) \
      p_psMetadataItemAllocV(__FILE__, __LINE__, __func__, name, type,comment, list)
#endif // ifdef DOXYGEN
#endif // #ifndef SWIG


/** Create a metadata collection.
 *
 *  Returns an empty metadata container with fully allocated internal metadata
 *  containers.
 *
 *  @return psMetadata* : Pointer metadata.
 */
#ifdef DOXYGEN
psMetadata* psMetadataAlloc(void);
#else // ifdef DOXYGEN
psMetadata* p_psMetadataAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func                    ///< Function name of caller
) PS_ATTR_MALLOC;
#define psMetadataAlloc() \
      p_psMetadataAlloc(__FILE__, __LINE__, __func__)
#endif // ifdef DOXYGEN


/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr
 *  datatype.
 *
 *  @return bool:       True if the pointer matches a psMetadata structure, false otherwise.
 */
bool psMemCheckMetadata(
    psPtr ptr                          ///< the pointer whose type to check
);


/** Creates a new copy of a psMetadataItem.
 *
 * @return psMetadataItem*: the copy of the psMetadataItem
 */
#ifdef DOXYGEN
psMetadataItem *psMetadataItemCopy(
    const psMetadataItem *in            ///< metadata item to be copied
);
#else // ifdef DOXYGEN
psMetadataItem *p_psMetadataItemCopy(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    const psMetadataItem *in            ///< metadata item to be copied
);
#define psMetadataItemCopy(in) \
      p_psMetadataItemCopy(__FILE__, __LINE__, __func__, in)
#endif // ifdef DOXYGEN


/** Create a copy of an existing psMetadata collection.
 *
 *  Creates a new copy of all the psMetadataItems in the psMetadata collection,
 *  in, and returns them in out, or creates a new container if out is NULL.  If
 *  an error occurs, NULL is returned but out may still have been updated if it
 *  is non-NULL.
 *
 *  @return psMetadata*:        the copy of the psMetadata container.
 */
#ifdef DOXYGEN
psMetadata *psMetadataCopy(
    psMetadata *out,                   ///< output Metadata container for copying.
    const psMetadata *in               ///< Metadata collection to be copied.
);
#else // ifdef DOXYGEN
psMetadata *p_psMetadataCopy(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psMetadata *out,                    ///< output Metadata container for copying.
    const psMetadata *in                ///< Metadata collection to be copied.
);
#define psMetadataCopy(out, in) \
      p_psMetadataCopy(__FILE__, __LINE__, __func__, out, in)
#endif // ifdef DOXYGEN


/** Updates an existing psMetadata collection with elements from a metadata collection
 *
 *  Creates a new copy of all the psMetadataItems in the psMetadata collection 'in' and places
 *  them in out.  all items in 'in' must already be in 'out' and be of the same type.
 *
 *  @return psMetadata*:        the copy of the psMetadata container.
 */
#ifdef DOXYGEN
bool psMetadataUpdate(
    psMetadata *out,                   ///< output Metadata container for copying.
    const psMetadata *in               ///< Metadata collection to be copied.
    );
#else // ifdef DOXYGEN
bool p_psMetadataUpdate(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psMetadata *out,                    ///< output Metadata container for copying.
    const psMetadata *in                ///< Metadata collection to be copied.
    );
#define psMetadataUpdate(out, in) \
      p_psMetadataUpdate(__FILE__, __LINE__, __func__, out, in)
#endif // ifdef DOXYGEN


/** Overlays an existing psMetadata collection with elements from a metadata collection
 *
 *  Creates a new copy of all the psMetadataItems in the psMetadata collection 'in' and places
 *  them in out.  matching metadata structures in 'out' are supplemented with corresponding
 *  entries from 'in'
 *
 *  @return psMetadata*:        the copy of the psMetadata container.
 */
#ifdef DOXYGEN
bool psMetadataOverlay(
    psMetadata *out,                   ///< output Metadata container for copying.
    const psMetadata *in               ///< Metadata collection to be copied.
    );
#else // ifdef DOXYGEN
bool p_psMetadataOverlay(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psMetadata *out,                    ///< output Metadata container for copying.
    const psMetadata *in                ///< Metadata collection to be copied.
    );
#define psMetadataOverlay(out, in) \
      p_psMetadataOverlay(__FILE__, __LINE__, __func__, out, in)
#endif // ifdef DOXYGEN


/** Supplements a metadata with an item from another metadata.
 *
 *  Supplements the output metadata with the metadata item of the specified name from the input metadata.  If
 *  out is NULL, a new container is created.
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool psMetadataItemSupplement(
    bool *status,			///< if supplied, returns true/false if key is found (suppresses the error)
    psMetadata *out,                   ///< output Metadata container for copying.
    const psMetadata *in,              ///< Metadata collection from which to copy.
    const char *key                    ///< key to identify the metadata item for copying.
);


/** Add existing metadata item to metadata collection.
 *
 *  Add a metadata item that has already been created to the metadata
 *  collection.
 *
 * Note: that this function accepts it's "value" as a stdarg.  This means that
 * the type of the value is not coerced by the prototype.  You need to be
 * careful to cast 64-bit integer values as smaller types will not be promoted.
 * This *includes* constant values as they are typically a 32-bit type.
 *
 *  @return bool: True for success, false for failure.
 */
bool psMetadataAddItem(
    psMetadata*  md,                   ///< Metadata collection to insert metadata item.
    const psMetadataItem* item,        ///< Metadata item to be added.
    int location,                      ///< Index number, PS_LIST_HEAD, or PS_LIST_TAIL
    psS32 flags                        ///< Options flag mask, see psMetadataFlags enum
);


/** Create and add a metadata item to metadata collection.
 *
 * Creates a new metadata item add to the metadata collection.
 *
 * Note: that this function accepts it's "value" as a stdarg.  This means that
 * the type of the value is not coerced by the prototype.  You need to be
 * careful to cast 64-bit integer values as smaller types will not be promoted.
 * This *includes* constant values as they are typically a 32-bit type.
 *
 * @return bool: True for success, false for failure.
 */
bool psMetadataAdd(
    psMetadata* md,                    ///< Metadata collection to insert metadata item.
    long location,                     ///< Index number, PS_LIST_HEAD, or PS_LIST_TAIL
    const char *name,                  ///< Name of metadata item.
    int format,                        ///< psDataType of metadata item & options (psMetadataFlags)
    const char *comment,               ///< Comment for metadata item.
    ...                                ///< Arguments for name formatting and metadata item data.
);


/** Create and add a metadata item to metadata collection.
 *
 * Creates a new metadata item add to the metadata collection.
 *
 * @return bool: True for success, false for failure.
 */
#ifndef SWIG
bool psMetadataAddV(
    psMetadata* md,                    ///< Metadata collection to insert metadata item.
    long location,                     ///< Index number, PS_LIST_HEAD, or PS_LIST_TAIL
    const char *name,                  ///< Name of metadata item.
    int format,                        ///< psDataType of metadata item & options (psMetadataFlags)
    const char *comment,               ///< Comment for metadata item.
    va_list list                       ///< Arguments for name formatting and metadata item data.
);
#endif // #ifndef SWIG


#define PS_METADATA_ADD_TYPE_DECL(NAME, TYPE) \
bool psMetadataAdd##NAME( \
    psMetadata* md,                    /* Metadata collection to insert metadata item */ \
    long location,                     /* Index number, PS_LIST_HEAD, or PS_LIST_TAIL */ \
    const char* name,                  /* Name of metadata item */ \
    int format,                        /* psMetadataFlag options/flags */ \
    const char* comment,               /* Comment for metadata item */ \
    TYPE value                         /* Value for metadata item data */ \
)

PS_METADATA_ADD_TYPE_DECL(Bool, psBool);
PS_METADATA_ADD_TYPE_DECL(S8,   psS8);
PS_METADATA_ADD_TYPE_DECL(S16,  psS16);
PS_METADATA_ADD_TYPE_DECL(S32,  psS32);
PS_METADATA_ADD_TYPE_DECL(S64,  psS64);
PS_METADATA_ADD_TYPE_DECL(U8,   psU8);
PS_METADATA_ADD_TYPE_DECL(U16,  psU16);
PS_METADATA_ADD_TYPE_DECL(U32,  psU32);
PS_METADATA_ADD_TYPE_DECL(U64,  psU64);
PS_METADATA_ADD_TYPE_DECL(F32,  psF32);
PS_METADATA_ADD_TYPE_DECL(F64,  psF64);

PS_METADATA_ADD_TYPE_DECL(Mask, psMaskType);
PS_METADATA_ADD_TYPE_DECL(VectorMask, psVectorMaskType);
PS_METADATA_ADD_TYPE_DECL(ImageMask, psImageMaskType);

PS_METADATA_ADD_TYPE_DECL(List, psList*);
PS_METADATA_ADD_TYPE_DECL(Str, const char*);
PS_METADATA_ADD_TYPE_DECL(Vector, psVector*);
PS_METADATA_ADD_TYPE_DECL(Array, psArray*);
PS_METADATA_ADD_TYPE_DECL(Image, psImage*);
PS_METADATA_ADD_TYPE_DECL(Time, psTime*);
PS_METADATA_ADD_TYPE_DECL(Hash, psHash*);
PS_METADATA_ADD_TYPE_DECL(LookupTable, psLookupTable*);
PS_METADATA_ADD_TYPE_DECL(Metadata, psMetadata*);
PS_METADATA_ADD_TYPE_DECL(Unknown, psPtr);

bool psMetadataAddPtr(
    psMetadata* md,                    ///< Metadata collection to insert metadata item
    long location,                     ///< Index number, PS_LIST_HEAD, or PS_LIST_TAIL
    const char* name,                  ///< Name of metadata item
    psDataType type,                   ///< psDataType for metadata item
    const char* comment,               ///< Comment for metadata item
    psPtr value                        ///< Unknown for metadata item data
);

#undef PS_METADATA_ADD_TYPE_DECL

/** Removes an item from metadata by key name.
 *
 *  @return bool:  True for success, false for failure.
 */
bool psMetadataRemoveKey(
    psMetadata *md,                    ///< Metadata collection to remove metadata item.
    const char *key                    ///< Name of metadata key.
);


/** Removes an item from metadata by index number.
 *
 *  @return bool:  True for success, false for failure.
 */
bool psMetadataRemoveIndex(
    psMetadata *md,                    ///< Metadata collection to remove metadata item.
    long location                       ///< Index number, PS_LIST_HEAD, or PS_LIST_TAIL
);


/** Find an item in the metadata collection based on key name.
 *
 *  Items may be found in the metadata by providing a key. If the key is
 *  non-unique, the first item is returned. If the item is not found, null is
 *  returned.
 *
 * @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataLookup(
    const psMetadata * md,             ///< Metadata collection to lookup metadata item.
    const char * key                   ///< Name of metadata key.
);


#define PS_METADATA_LOOKUP_TYPE_DECL(NAME, TYPE) \
TYPE psMetadataLookup##NAME( \
    bool *status,                      /* Status of lookup */ \
    const psMetadata *md,              /* Metadata collection to lookup metadata item */ \
    const char *key                    /* Name of metadata key */ \
)

PS_METADATA_LOOKUP_TYPE_DECL(Bool, psBool);
PS_METADATA_LOOKUP_TYPE_DECL(S8,   psS8);
PS_METADATA_LOOKUP_TYPE_DECL(S16,  psS16);
PS_METADATA_LOOKUP_TYPE_DECL(S32,  psS32);
PS_METADATA_LOOKUP_TYPE_DECL(S64,  psS64);
PS_METADATA_LOOKUP_TYPE_DECL(U8,   psU8);
PS_METADATA_LOOKUP_TYPE_DECL(U16,  psU16);
PS_METADATA_LOOKUP_TYPE_DECL(U32,  psU32);
PS_METADATA_LOOKUP_TYPE_DECL(U64,  psU64);
PS_METADATA_LOOKUP_TYPE_DECL(F32,  psF32);
PS_METADATA_LOOKUP_TYPE_DECL(F64,  psF64);

PS_METADATA_LOOKUP_TYPE_DECL(Mask, psMaskType);
PS_METADATA_LOOKUP_TYPE_DECL(VectorMask, psVectorMaskType);
PS_METADATA_LOOKUP_TYPE_DECL(ImageMask, psImageMaskType);

PS_METADATA_LOOKUP_TYPE_DECL(Ptr, psPtr);
PS_METADATA_LOOKUP_TYPE_DECL(Str, psString);
PS_METADATA_LOOKUP_TYPE_DECL(Vector, psVector*);
PS_METADATA_LOOKUP_TYPE_DECL(Metadata, psMetadata*);
PS_METADATA_LOOKUP_TYPE_DECL(Time, psTime*);

#undef PS_METADATA_LOOKUP_TYPE_DECL

/** Find an item in the metadata collection based on list index.
 *
 *  Items may be found in the metadata by their entry position in the list
 *  container.
 *
 *  @return psMetadataItem* : Pointer metadata item.
 */
psMetadataItem* psMetadataGet(
    const psMetadata* md,              ///< Metadata collection to retrieve metadata item.
    int location                       ///< Index number, PS_LIST_HEAD, or PS_LIST_TAIL
);


/** Creates a psMetadataIterator to iterate over the specified psMetadata.
 *
 *  Supports the subsetting of the metadata via keyword using regular
 *  expression.  If no regular expression is specified, iteration
 *  over the entire psMetadata is performed.
 *
 *  @return psMetadataIterator*        a new psMetadataIterator, of NULL if error occurred
 */
#ifdef DOXYGEN
psMetadataIterator* psMetadataIteratorAlloc(
    const psMetadata* md,               ///< the psMetadata to iterate with
    long location,                      ///< Index number, PS_LIST_HEAD, or PS_LIST_TAIL
    const char* regex
    ///< A regular expression for subsetting the psMetadata.  If NULL, no
    ///< subsetting is performed.
);
#else // ifdef DOXYGEN
psMetadataIterator* p_psMetadataIteratorAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    const psMetadata* md,               ///< the psMetadata to iterate with
    long location,                      ///< Index number, PS_LIST_HEAD, or PS_LIST_TAIL
    const char* regex
    ///< A regular expression for subsetting the psMetadata.  If NULL, no
    ///< subsetting is performed.
) PS_ATTR_MALLOC;
#define psMetadataIteratorAlloc(md, location, regex) \
      p_psMetadataIteratorAlloc(__FILE__, __LINE__, __func__, md, location, regex)
#endif // ifdef DOXYGEN


/** Set the iterator of the psMetadat to a given position.  If location is
 *  invalid the iterator position is not changed.
 *
 *  @return bool        TRUE if iterator successfully set, otherwise FALSE.
*/
bool psMetadataIteratorSet(
    psMetadataIterator* iterator,      ///< psMetadata iterator
    long location                      ///< index number, PS_LIST_HEAD, or PS_LIST_TAIL
);


/** Position the specified iterator to the next matching item in psMetadata,
 *  given the regular expression of the iterator
 *
 *  @return psPtr       the psMetadataItem at the original iterator position
 *                      or NULL if the iterator went past the end of the list.
 */
psMetadataItem* psMetadataGetAndIncrement(
    psMetadataIterator* iterator       ///< iterator to move
);


/** Position the specified iterator to the previous matching item in psMetadata,
 *  given the regular expression of the iterator
 *
 *  @return psPtr       the psMetadataItem at the original iterator position
 *                      or NULL if the iterator went past the beginning of the
 *                      list.
 */
psMetadataItem* psMetadataGetAndDecrement(
    psMetadataIterator* iterator       ///< iterator to move
);


/// Return a list of keys for a metadata
psList *psMetadataKeys(psMetadata *md   ///< Metadata for which to return keys
                       );

/** Prints metadata collection.
 *
 *  Metadata contents are printed to a valid file descriptor if one exists.  Otherwise,
 *  fd should be NULL and the contents are printed to the screen (stdout).
 *
 *  @return bool:           True if successful, otherwise false.
*/
bool psMetadataPrint(
    FILE *fd,                          ///< File Descriptor or NULL
    const psMetadata *md,              ///< Metadata collection to print.
    int level                          ///< the level of metadata items.
);

/// Assert on metadata with extant hash and list components
#define PS_ASSERT_METADATA_NON_NULL(NAME, RVAL) \
if (!(NAME) || !(NAME)->hash || !(NAME)->list) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: Metadata %s or one of its components is NULL.", \
            #NAME); \
    return RVAL; \
}

/// Assert on metadata item with extant name and type
///
/// The data contained within the metadata item is permitted to be NULL.
#define PS_ASSERT_METADATA_ITEM_NON_NULL(NAME, RVAL) \
if (!(NAME) || !(NAME)->name || !(NAME)->type) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: Metadata item %s or its name or type is NULL.", \
            #NAME); \
    return RVAL; \
}

#define PS_ASSERT_METADATA_ITERATOR_NON_NULL(NAME, RVAL) \
if (!(NAME) || !(NAME)->iter) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: Metadata iterator %s or its component is NULL.", \
            #NAME); \
    return RVAL; \
}

/// @}
#endif // #ifndef PS_METADATA_H
