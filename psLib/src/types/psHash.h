/** @file  psHash.h
 *
 *  @brief Contains support for basic hashing functions.
 *
 *  This file will hold the prototypes for defining a hash table with arbitrary
 *  data types, allocating/deallocating that has table, adding and removing
 *  data from that hash table, and listing all keys defined in the hash table.
 *
 *  @author Robert Lupton, Princeton University
 *  @author Robert DeSonia, MHPCC
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.25 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-08-14 03:18:41 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_HASH_H
#define PS_HASH_H

/// @addtogroup DataContainer Data Containers
/// @{

#include "psList.h"
#include "psMutex.h"

/** A bucket that holds an item of data. */
typedef struct psHashBucket
{
    char *key;                         ///< The key for this item of data.
    psPtr data;                        ///< The data itself.
    struct psHashBucket* next;         ///< The list of other possible keys.
}
psHashBucket;


//typedef struct HashTable psHash; ///< Opaque type for a hash table

/** The hash-table itself. */
typedef struct
{
    long n;                            ///< Number of buckets in hash table.
    psHashBucket* *buckets;            ///< The bucket data.
    psMutex lock;                       ///< Optional lock for thread safety.
}
psHash;

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr
 *  datatype.
 *
 *  @return bool:       True if the pointer matches a psHash structure, false otherwise.
 */
bool psMemCheckHash(
    psPtr ptr                          ///< the pointer whose type to check
);


/// Allocate hash buckets in table.
#ifdef DOXYGEN
psHash* psHashAlloc(
    long nalloc                        ///< The number of buckets to allocate.
);
#else // ifdef DOXYGEN
psHash* p_psHashAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    long nalloc                        ///< The number of buckets to allocate.
) PS_ATTR_MALLOC;
#define psHashAlloc(nalloc) \
      p_psHashAlloc(__FILE__, __LINE__, __func__, nalloc)
#endif // ifdef DOXYGEN



/// Insert entry into table.
bool psHashAdd(
    psHash* hash,                      ///< The table to insert in.
    const char *key,                   ///< The key to use.
    psPtr data                         ///< The data to insert.
);


/// Lookup key in table.
psPtr psHashLookup(
    const psHash* hash,                ///< The table to lookup key in.
    const char *key                    ///< The key to lookup.
);


/// Remove key from table.
bool psHashRemove(
    psHash* hash,                      ///< The table to lookup key in.
    const char *key                    ///< The key to lookup.
);


/// List all keys in table.
psList* psHashKeyList(
    const psHash* hash                 ///< The table to list keys from..
);


/** Create a psArray from a psHash contents.
 *
 *  @return psArray*       A new psArray with duplicate contents of the input psHash
 */
psArray* psHashToArray(
    const psHash* hash                 ///< The table to convert to psArray.
);


#define PS_ASSERT_HASH_NON_NULL(NAME, RVAL) \
if (!(NAME) || !(NAME)->buckets || (NAME)->n < 0) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: Hash %s or one of its components is NULL.", \
            #NAME); \
    return RVAL; \
}

/// @} End of DataContainer Functions
#endif // #ifndef PS_HASH_H
