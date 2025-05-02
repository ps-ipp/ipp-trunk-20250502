/**
 *  C Implementation: tap_psHash_all
 *
 * Description:  Tests for psHashAlloc, psHashAdd, psMemCheckHash, psHashLookup,
 *               psHashRemove, psHashKeyList, psHashToArray
 *
 * Author: dRob <David.Robbins@mhpcc.hpc.mil>, (C) 2006
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */

#include <pslib.h>
#include <string.h>

#include "tap.h"
#include "pstap.h"

void testHashCreate(void);
void testHashManip(void);
void testHashConvert(void);

int main(void)
{
    plan_tests(24);

    note("Tests for psHash Functions");

    testHashCreate();
    testHashManip();
    testHashConvert();

    done();
}

void testHashCreate(void)
{
    note("  >>>Test 1:  psHash Creation Fxns");

    psHash *h1 = NULL;
    psHash *h2 = NULL;

    //Return NULL on attempting to allocate a negative size hash
    {
        h1 = psHashAlloc(-2);
        ok( h1 == NULL,
            "psHashAlloc:          return NULL for negative array-size input.");
    }
    //Return properly allocated psHash of 0-length
    {
        h2 = psHashAlloc(0);
        ok( h2 != NULL && psMemCheckHash(h2),//XXX: && h1->buckets == NULL,
            "psHashAlloc:          return properly allocated psHash.");
    }
    //Return properly allocated psHash
    {
        h1 = psHashAlloc(3);
        ok( h1 != NULL && psMemCheckHash(h1) && h1->buckets != NULL,
            "psHashAlloc:          return properly allocated psHash.");
    }
    //Make sure psMemCheckHash works correctly - return false
    if (0) {
        int j = 2;
        ok( !psMemCheckHash(&j),
            "psMemCheckHash:       return false for non-Hash input.");
    }
    //Allocate a hashBucket by inserting a psHash entry - return true
    psMetadataItem *itemBool = psMetadataItemAllocBool("itemBool", "", true);
    {
        ok( psHashAdd(h1, "h1-1", (psPtr)itemBool),
            "psHashAlloc:          return properly allocated psHashBucket item.");
    }
    //Return false for trying to add to a NULL hash input
    {
        ok( !psHashAdd(NULL, "h1-2", (psPtr)itemBool),
            "psHashAlloc:          return false for trying to add to a NULL hash input.");
    }
    //Return false for trying to add data with NULL key
    {
        ok( !psHashAdd(h1, NULL, (psPtr)itemBool),
            "psHashAlloc:          return false for trying to add data with NULL key.");
    }
    //Return false for trying to add NULL data
    {
        ok( !psHashAdd(h1, "h1-2", NULL),
            "psHashAlloc:          return false for trying to add NULL data.");
    }
    //Return false for trying to add data to empty hash
    {
        ok( !psHashAdd(h2, "h1-2", (psPtr)itemBool),
            "psHashAlloc:          return false for trying to add data to empty table.");
    }
    //Return true for trying to add a duplicate hash entry - replaces
    {
        psHashAdd(h1, "h1-2", (psPtr)itemBool);
        psHashAdd(h1, "h1-3", (psPtr)itemBool);
        psHashAdd(h1, "h1-4", (psPtr)itemBool);
        ok( psHashAdd(h1, "h1-3", (psPtr)itemBool),
            "psHashAlloc:         return true when adding duplicate data entry.");
    }


    //Check for Memory leaks
    {
        psFree(itemBool);
        psFree(h1);
        psFree(h2);
        checkMem();
    }
}

void testHashManip(void)
{
    note("  >>>Test 2:  psHash Manipulation Fxns");

    psHash *h1 = NULL;
    h1 = psHashAlloc(2);
    psMetadataItem *itemBool = psMetadataItemAllocBool("itemBool", "", true);
    psMetadataItem *newBool = psMetadataItemAllocBool("newBool", "", true);
    psHashAdd(h1, "h1-1", (psPtr)itemBool);
    psHashAdd(h1, "h1-2", (psPtr)newBool);
    psMetadataItem *data = NULL;

    //psHashLookup Tests
    //Return NULL for NULL hash input
    {
        data = (psMetadataItem*)psHashLookup(NULL, "h1-1");
        ok( data == NULL,
            "psHashLookup:        return NULL for NULL hash input.");
    }
    //Return NULL for NULL key input
    {
        data = (psMetadataItem*)psHashLookup(h1, NULL);
        ok( data == NULL,
            "psHashLookup:        return NULL for NULL key input.");
    }
    //Return NULL for no matching key
    {
        data = (psMetadataItem*)psHashLookup(h1, "h1-3");
        ok( data == NULL,
            "psHashLookup:        return NULL for invalid key input.");
    }
    //Return correct data for valid inputs
    {
        data = (psMetadataItem*)psHashLookup(h1, "h1-1");
        ok( data != NULL && psMemCheckMetadataItem(data) && data->data.B,
            "psHashLookup:        return correct data for valid inputs.");
    }

    //psHashRemove Tests
    //Return false for NULL hash input
    {
        ok( !psHashRemove(NULL, "h1-1"),
            "psHashRemove:        return false for NULL hash input.");
    }
    //Return false for NULL key input
    {
        ok( !psHashRemove(h1, NULL),
            "psHashRemove:        return false for NULL key input.");
    }
    //Return false for no matching key
    {
        ok( !psHashRemove(h1, "h1-3"),
            "psHashRemove:        return false for invalid key input.");
    }
    //Return true for properly removed hash entry
    {
        ok( psHashRemove(h1, "h1-1") && (psHashLookup(h1, "h1-1") == NULL),
            "psHashRemove:        return true for properly removed hash entry.");
    }

    //Check for Memory leaks
    {
        psFree(h1);
        psFree(itemBool);
        psFree(newBool);
        checkMem();
    }
}

void testHashConvert(void)
{
    note("  >>>Test 3:  psHash Conversion/List Fxns");

    psHash *h1 = NULL;
    h1 = psHashAlloc(2);
    psHash *h2 = psHashAlloc(0);
    psMetadataItem *itemBool = psMetadataItemAllocBool("itemBool", "", true);
    psMetadataItem *newBool = psMetadataItemAllocBool("newBool", "", true);
    psHashAdd(h1, "h1-1", (psPtr)itemBool);
    psHashAdd(h1, "h1-2", (psPtr)newBool);
    psList *newList = NULL;
    psArray *newArr = NULL;

    //psHashKeyList Tests
    //Return NULL for NULL hash input
    {
        newList = psHashKeyList(NULL);
        ok( newList == NULL,
            "psHashKeyList:       return NULL for NULL hash input.");
    }
    //Return empty list for empty hash input
    {
        newList = psHashKeyList(h2);
        ok( newList != NULL && newList->n == 0,
            "psHashKeyList:       return empty list for empty hash input.");
        psFree(newList);
        newList = NULL;
    }
    //Return correct list for valid, non-empty hash input
    {
        newList = psHashKeyList(h1);
        ok( newList != NULL && !strncmp((char*)(newList->head->data), "h1-1", 10),
            "psHashKeyList:       return correct list for valid hash input.");
    }

    //psHashToArray
    //Return NULL for NULL hash input
    {
        newArr = psHashToArray(NULL);
        ok( newArr == NULL,
            "psHashToArray:       return NULL for NULL hash input.");
    }
    //Return empty array for empty hash input
    {
        newArr = psHashToArray(h2);
        ok( newArr != NULL && newArr->n == 0,
            "psHashToArray:       return empty array for empty hash input.");
        psFree(newArr);
        newArr = NULL;
    }
    //Return correct list for valid, non-empty hash input
    {
        newArr = psHashToArray(h1);
        psMetadataItem *tempItem = (psMetadataItem*)psArrayGet(newArr, 0);
        if (tempItem->type != PS_DATA_BOOL)
            printf("\n not a bool - %s\n", tempItem->name);
        ok( newArr != NULL,// && !strncmp(tempItem->name, "itemBool", 10),
            "psHashToArray:       return correct array for valid hash input.");
    }

    //Check for Memory leaks
    {
        psFree(h1);
        psFree(h2);
        psFree(newList);
        psFree(newArr);
        psFree(itemBool);
        psFree(newBool);
        checkMem();
    }
}
