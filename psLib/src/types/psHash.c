
/** @file  psHash.c
*
*  @brief Contains support for basic hashing functions.
*
*  This file will hold the functions for defining a hash table with arbitrary
*  data types, allocating/deallocating that hash table, adding and removing
*  data from that hash table, and listing all keys defined in the hash table.
*
*  @author Robert Lupton, Princeton University
*  @author Robert DeSonia, MHPCC
*  @author GLG, MHPCC
*
*  @version $Revision: 1.43 $ $Name: not supported by cvs2svn $
*  @date $Date: 2008-04-17 23:43:03 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "psAbort.h"
#include "psHash.h"
#include "psMemory.h"
#include "psString.h"
#include "psTrace.h"
#include "psError.h"
#include "psAssert.h"



static psHashBucket* hashBucketAlloc(const char *key, psPtr data, psHashBucket* next);
static void hashBucketFree(psHashBucket* bucket);
static psPtr doHashWork(psHash* table, const char *key, psPtr data, bool remove);
static void hashFree(psHash* table);

/******************************************************************************
psHashKeyList(table): this function creates a linked list with an entry in
that list for every key in the hash table.
Inputs:
    table: a hash table
Return;
    The linked list
 *****************************************************************************/
psList* psHashKeyList(const psHash* hash)
{
    PS_ASSERT_HASH_NON_NULL(hash, NULL);

    psList* myLinkList = NULL;  // The output data structure
    psHashBucket* ptr = NULL;   // Used to step thru linked list.

    // Create the linked list
    myLinkList = psListAlloc(NULL);

    // Loop through every bucket in the hash table.  If that bucket is not
    // NULL, then add the bucket's key to the linked list.
    for (long i = 0; i < hash->n; i++) {
        if (hash->buckets[i] != NULL) {
            // Since a bucket contains a linked list of keys/data, we must
            // step trough each key in that linked list:

            ptr = hash->buckets[i];
            while (ptr != NULL) {
                psListAdd(myLinkList, PS_LIST_HEAD, ptr->key);
                ptr = ptr->next;
            }
        }
    }

    // Return the linked list
    return (myLinkList);
}

/******************************************************************************
hashBucketAlloc(key, data, next): This procedure creates a new hash bucket
with the specified key, data, and next.
Inputs:
    key:  the new bucket's key pointer
    data: the new bucket's data pointer
    next: the new bucket's key pointer
Return:
    the new hash bucket.
 *****************************************************************************/
static psHashBucket* hashBucketAlloc(const char *key,
                                     psPtr data,
                                     psHashBucket* next)
{
    psAssert(key && strlen(key) > 0, "impossible");

    // Allocate memory for the new hash bucket.
    psHashBucket* bucket = psAlloc(sizeof(psHashBucket));

    psMemSetDeallocator(bucket, (psFreeFunc) hashBucketFree);

    // Initialize the bucket.
    bucket->key = psStringCopy(key);

    //XXX:  Since this function is static and only called by doHashWork,
    //data can never be NULL here.
    //    if (data == NULL) {
    // NOTE: Should we flag a warning message?
    //        bucket->data = NULL;
    //    } else {
    bucket->data = psMemIncrRefCounter(data);
    //    }

    bucket->next = next;

    return bucket;
}

/******************************************************************************
hashBucketFree(bucket): This procedure deallocates the specified
hash bucket.
Inputs:
    bucket: the hash bucket to be freed.
Return:
    NONE
 *****************************************************************************/
static void hashBucketFree(psHashBucket* bucket)
{
    psFree(bucket->key);
    psFree(bucket->data);
}

/******************************************************************************
psHashAlloc(n): this procedure creates a new hash table with the
specified number of buckets.
Inputs:
    n: initial number of buckets
Return:
    The new hash table.
 *****************************************************************************/
psHash* p_psHashAlloc(const char *file,
                      unsigned int lineno,
                      const char *func,
                      long nalloc)        // initial number of buckets
{
    if (nalloc < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Can't allocate a psHash of negative size.");
        return NULL;
    }

    // Create the new hash table.
    psHash* table = p_psAlloc(file, lineno, func, sizeof(psHash));

    psMemSetDeallocator(table, (psFreeFunc) hashFree);

    // Allocate memory for the buckets.
    table->buckets = psAlloc(nalloc * sizeof(psHashBucket* ));
    table->n = nalloc;

    psTrace("psLib.types", 1, "Creating %ld-element hash table\n", nalloc);

    // Initialize all buckets to NULL.
    for (long i = 0; i < nalloc; i++) {
        table->buckets[i] = NULL;
    }

    // Return the new hash table.
    return table;
}


bool psMemCheckHash(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)hashFree );
}


/******************************************************************************
hashFree(table): This procedure deallocates the specified hash
table.  It loops through each bucket, and calls hashBucketFree() on that
bucket.

Inputs:
    table: a hash table
Return:
    NONE
 *****************************************************************************/
static void hashFree(psHash* table)
{
    // Loop through each bucket in the hash table.  If that bucket is not
    // NULL, then free the bucket via a function call to hashBucketFree();
    for (long i = 0; i < table->n; i++) {
        // A bucket is composed of a linked list of buckets.
        while (table->buckets[i] != NULL) {
            psHashBucket* bucket = table->buckets[i];
            table->buckets[i] = bucket->next;
            psFree(bucket);
        }
    }

    // Free the bucket structure, then the hash table.
    psFree(table->buckets);
}

/******************************************************************************
doHashWork(table, key, data, remove): This is an internal
procedure which does the bulk of the work in using the hash table.  Depending
upon the input parameters, it will either insert a new key/data into the hash
table, retrieve the data for a specified key, or remove a key/data item.  If
we try to insert a key that already exists in the hash table, then we deallocate
the existing data/key item.
Inputs:
    table: a hash table
    key: the key to insert, retrieve, or remove.  Must not be NULL.
    data: the data to insert, if not NULL
    remove: set to non-zero if the key/data should be removed from the table.
Return:
    NONE

NOTE: consider removing this private function and simply putting the code
into the psHashInsert(), psHashLookup(), and psHashRemove().  Why?  Because
there is little common code between those functions.
  *****************************************************************************/
static psPtr doHashWork(psHash* table,
                        const char *key,
                        psPtr data,
                        bool remove
                           )
{
    psAssert(table, "impossible");
    psAssert(table->n >= 0, "impossible");
    psAssert(key && strlen(key) > 0, "impossible");

    if (table->n == 0) {
        return NULL;
    }

    // This hash algorithm is from Sedgewick.  NOTE: must reread to ensure that
    // the size of the hash table is not required to be a prime number.
    char *tmpchar = (char *)key;        // Used in computing the hash function.
    long hash;                          // The hash value
    for (hash = 0; *tmpchar != '\0'; tmpchar++) {
        hash = (64 * hash + *tmpchar) % (table->n);
    }
    psAssert(hash >= 0 && hash < table->n, "impossible");

    // ptr will have the correct hash bucket.
    psHashBucket *ptr = table->buckets[hash];   // Used to retrieve the hash bucket.
    psHashBucket* optr = NULL;          // "original pointer": used to step thru the linked list for a bucket.

    // We know the correct hash bucket, now we need to know what to do.
    // If the data parameter is NULL, then, by definition, this is a retrieve
    // or a remove operation on the hash table.

    if (!data) {
        if (remove) {
            // We search through the linked list for this bucket in
            // the hash table and look for an entry for this key.

            optr = ptr;
            while (ptr != NULL) {
                // Determine if this entry holds the correct key.
                if (strcmp(key, ptr->key) == 0) {
                    // The following lines of code are fairly standard ways
                    // of removing an item from a single-linked list.

                    psPtr data = ptr->data;

                    optr->next = ptr->next;
                    if (ptr == table->buckets[hash]) {
                        table->buckets[hash] = ptr->next;
                    }
                    psFree(ptr);

                    // By definition, the data associated with that key
                    // must be returned, not freed.
                    return data;
                }
                optr = ptr;
                ptr = ptr->next;
            }
            return NULL;                   // not in hash
        }
        else {
            // If we get here, then a retrieve operation is requested.  So,
            // we step trough the linked list at this bucket, and return the
            // data once we find it, or return NULL if we don't.
            while (ptr != NULL) {
                if (strcmp(key, ptr->key) == 0) {
                    return ptr->data;
                }
                ptr = ptr->next;
            }
            return NULL;                   // not in hash
        }
    } else {
        // We get here if this procedure was called with non-NULL data.
        // Therefore, we should insert that data into the hash table.
        // First, we search through the linked list for this bucket in
        // the hash table and look for a duplicate entry for this key.

        while (ptr != NULL) {
            if (strcmp(key, ptr->key) == 0) {
                // We have found this key in the hash table.

                psTrace("psLib.types", 3, "Replacing data for %s\n", key);

                // NOTE: I have changed this behavior from the originally
                // supplied code.  Formerly, if itemFree was NULL, then
                // the new data was not inserted into the hash table.

                psFree(ptr->data);

                ptr->data = psMemIncrRefCounter(data);
                return data;
            }
            ptr = ptr->next;
        }
        // We did not found key in the linked list for this bucket of the hash
        // table.  So, we insert this data at the head of that linked list.

        table->buckets[hash] = hashBucketAlloc(key, data, table->buckets[hash]);
        return data;
    }
}

/******************************************************************************
psHashAdd(table, key, data): this procedure, which is part of
the public API, inserts a new key/data pair into the hash table.
Inputs:
    table: a hash table
    key: the key to use
    data: the data to insert.
Return:
    boolean value defining success or failure
 *****************************************************************************/
bool psHashAdd(psHash* hash,
               const char *key,
               psPtr data)
{
    PS_ASSERT_HASH_NON_NULL(hash, false);
    PS_ASSERT_STRING_NON_EMPTY(key, false);
    PS_ASSERT_PTR_NON_NULL(data, false);

    return (doHashWork(hash, key, data, false) != NULL);
}

/******************************************************************************
psHashLookup(table, key): this procedure, which is part of the public API,
looks up the specified key in the hash table and returns the data associated
with that key.

Inputs:
    table: a hash table
    key: the key to use
Return:
    The data associated with that key.
 *****************************************************************************/
psPtr psHashLookup(const psHash* hash,      // hash to lookup key in
                   const char *key)     // key to lookup
{
    PS_ASSERT_HASH_NON_NULL(hash, NULL);
    PS_ASSERT_STRING_NON_EMPTY(key, NULL);

    return doHashWork((psPtr)hash, key, NULL, false);
}

/******************************************************************************
psHashRemove(table, key): this procedure, which is part of the
public API, removes the specified key from the hash table.
Inputs:
    table: a hash table
    key: the key to remove
Return:
    boolean value defining success or failure
 *****************************************************************************/
bool psHashRemove(psHash* hash,
                  const char *key)
{
    PS_ASSERT_HASH_NON_NULL(hash, false);
    PS_ASSERT_STRING_NON_EMPTY(key, false);

    psPtr data = NULL;
    bool retVal = false;

    data = doHashWork(hash, key, NULL, true);
    if (data != NULL) {
        retVal = true;
    } else {
        retVal = false;
    }

    return retVal;
}

psArray* psHashToArray(const psHash* hash)
{
    PS_ASSERT_HASH_NON_NULL(hash, NULL);

    // first, let's just count the number of data elements to know what size psArray we need to allocate.
    int nElements = 0;
    int nbucket = hash->n;
    // XXX:  If we do psArrayAlloc(0) here and use psArrayAdd(result, 1, tmpBucket->data)
    // we can eliminate the 2nd for loop below.
    for (int i = 0; i < nbucket; i++) {
        psHashBucket* tmpBucket = hash->buckets[i];
        while (tmpBucket != NULL) {
            nElements++;
            tmpBucket = tmpBucket->next;
        }
    }

    psArray* result = psArrayAlloc(nElements);

    // now fill in the array with the hash hash's data
    psPtr* data = result->data;
    for (int i = 0; i < nbucket; i++) {
        psHashBucket* tmpBucket = hash->buckets[i];
        while (tmpBucket != NULL) {
            *(data++) = psMemIncrRefCounter(tmpBucket->data);
            tmpBucket = tmpBucket->next;
        }
    }

    return result;
}
