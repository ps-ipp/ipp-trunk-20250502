/** @file  tst_psXML.c
*
*  @brief Test driver for psXML functions
*
*  This test driver contains the following tests for psXML:
*     Test 1 - Parse an XML file
*     Test 2 - Parse an XML memory block
*     Test 3 - Parse an XML file descriptor
*     Test 4 - Convert an XML doc to Metadata
*     Test 5 - Convert Metadata to an XML doc
*     Test 6 - Write an XML doc to file
*     Test 7 - Write an XML doc to memory block
*     Test 8 - Write an XML doc to file descriptor
*
*  @author  Dave Robbins, MHPCC
*
* XXXX: This doesn't even compile
* XXXX: There are no data tests, he simple prints data to STDOUT
*
*  @version $Revision: 1.2 $  $Name: not supported by cvs2svn $
*  @date  $Date: 2007-05-01 00:08:52 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*
*/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

//static void writeMetadata(psMetadata* metadata, char* indentStr);

// XXX: What should SRCDIR be?
#define SRCDIR
const char testFile1[] = SRCDIR "/psTime.xml";
const char testFile2[] = SRCDIR "/psTime2.xml";
const char testFile3[] = SRCDIR "/psTime3.xml";
const char testFile4[] = SRCDIR "/psTime4.xml";
//static void printMetadata(psMetadata *in);
static void printMetadataItem(psMetadataItem *metadataItem, char *spaces);
static void printMetadata(psMetadata *metadata);
static void printMetadataList(psList *metadataItemList, char* spaces);
static void printMetadataTable(psHash *mdTable);

static void printMetadata(psMetadata *metadata)
{
    printf("Contents of metadata list:\n");
    printMetadataList(metadata->list, " ");
    printf("\nContents of metadata table:\n");
    printMetadataTable(metadata->hash);
}

static void printMetadataList(psList *metadataItemList, char* spaces)
{
    psMetadataItem *entryChild = NULL;

    psListIterator* iter = psListIteratorAlloc(metadataItemList, PS_LIST_HEAD, true);

    while ( (entryChild=psListGetAndIncrement(iter)) != NULL) {
        printMetadataItem(entryChild, spaces);
    }

    psFree(iter);
}

static void printMetadataTable(psHash *mdTable)
{
    psS32 i;
    psHashBucket* ptr = NULL;
    for(i=0; i<mdTable->n; i++) {
        ptr = mdTable->buckets[i];
        while (ptr != NULL) {
            printMetadataItem(ptr->data, " ");
            ptr = ptr->next;
        }
    }
}

static void printMetadataItem(psMetadataItem *metadataItem, char *spaces)
{
    int i = 0;
    printf("%sKey Name: %25s  ", spaces, metadataItem->name);
    //    printf("Key mdType: %d  ", (int)metadataItem->type);
    //    printf("Key mdType: 0x%08x  ", metadataItem->type);
    //    if ( metadataItem->type == PS_DATA_S32 )
    //        printf("Key mdType: S32  " );

    switch (metadataItem->type) {
    case PS_DATA_BOOL:
        printf("Key Type:  BOOL     Key Value: %15d  ", metadataItem->data.B);
        break;
    case PS_DATA_S32:
        printf("Key Type:  S32      Key Value: %15d  ", metadataItem->data.S32);
        break;
    case PS_DATA_F32:
        printf("Key Type:  F32      Key Value: %15.3f  ", metadataItem->data.F32);
        break;
    case PS_DATA_F64:
        printf("Key Type:  F64      Key Value: %15.3f  ", metadataItem->data.F64);
        break;
    case PS_DATA_METADATA:
        printf("Key Type:  METADATA    ");
        break;
    default:
        printf("Key type:  psPtr    ");
    }
    if ( !strncmp(metadataItem->name, "psLib.time.Vector.S32", 256) ) {
        printf("Key Values:   ");
        while ( (int)((psVector*)(metadataItem->data.V))->data.S32[i] != 0 ) {
            printf("%d  ", (int)((psVector*)(metadataItem->data.V))->data.S32[i] );
            i++;
        }
        printf("\n");
    } else if ( !strncmp(metadataItem->name, "psLib.time.tables.dir", 256) ) {
        printf("Key Value:   ");
        printf("%s", (char*)metadataItem->data.V );
        printf("\n");
    } else if ( !strncmp(metadataItem->name, "psLib.TIME.Magazine", 256) ) {
        printf("Key Value:   ");
        printf("%ld, ", (long)((psTime*)(metadataItem->data.V))->sec );
        printf("%u, ", ((psTime*)(metadataItem->data.V))->nsec );
        if( ((psTime*)(metadataItem->data.V))->leapsecond )
            printf("TRUE  ");
        else
            printf("FALSE  ");
        if( ((psTime*)(metadataItem->data.V))->type == PS_TIME_UTC )
            printf("PS_TIME_UTC ");
        else if( ((psTime*)(metadataItem->data.V))->type == PS_TIME_TAI )
            printf("PS_TIME_TAI ");
        else if( ((psTime*)(metadataItem->data.V))->type == PS_TIME_UT1 )
            printf("PS_TIME_UT1 ");
        else if( ((psTime*)(metadataItem->data.V))->type == PS_TIME_TT )
            printf("PS_TIME_TT ");
        printf("\n");
    } else
        printf("Key Comment: %s\n", metadataItem->comment);

    //    if(metadataItem->data.V && metadataItem->type==PS_META_MULTI) {
    //    if(metadataItem->data.V) {
    //        printMetadataList(metadataItem->data.V, "    ");
    //    }
}

typedef xmlDocPtr psXMLDoc;
psS32 main( psS32 argc, char* argv[] )
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(24);

    //Parses an XML file into memory.  Stores as a psXMLDoc//
    // testXMLInput00
    {
        psMemId id = psMemGetId();
        //psXMLParseFile should return a psXMLDoc* of testFile1//
        psXMLDoc *newXML = NULL;
        newXML = psXMLParseFile(testFile1);
        ok(newXML != NULL, "psXMLParseFile() returned non-NULL");
        psFree(newXML);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    //Parses an XML file from memory.  Stores as a psXMLDoc//
    // testXMLInput01
    {
        psMemId id = psMemGetId();
        psXMLDoc *newXML = NULL;
        newXML = psXMLParseFile(testFile1);
        char buffer[2048];
        ok(psXMLDocToMem(newXML, buffer), "psXMLDocToMem() returned non-NULL");
        psXMLDoc *newDoc = NULL;
        newDoc = psXMLParseMem(buffer, strlen(buffer) );
        ok(newDoc != NULL, "psXMLParseMem() returned non-NULL");
        psFree(newDoc);
        psFree(newXML);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Parses an XML file from a file descriptor.  Stores as a psXMLDoc//
    // testXMLInput02
    {
        psMemId id = psMemGetId();
        psXMLDoc *newXML = NULL;
        newXML = psXMLParseFile(testFile1);
        ok(newXML != NULL, "psXMLParseFile () returned non-NULL");
        int fd = creat("psTest5.xml", 0644);
        ok(psXMLDocToFD(newXML, fd), "psXMLDocToFD() returned TRUE");

        psXMLDoc *newDoc = NULL;
        fd = open("psTest5.xml", O_RDWR, 0);
        newDoc = psXMLParseFD(fd);
        ok(newDoc != NULL, "psXMLParseFD () returned non-NULL");
        close(fd);
        psFree(newXML);
        psFree(newDoc);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Converts an existing psXMLDoc into psMetadata//
    // testXMLConvert00
    {
        psMemId id = psMemGetId();
        psXMLDoc *newXML = NULL;
        newXML = psXMLParseFile(testFile1);
        psMetadata *metaData = NULL;
        metaData = psXMLDocToMetadata(newXML);
        if (metaData == NULL)
        {
            ok(false, "psXMLDocToMetadata() returned non-NULL");
        } else
        {
            ok(true, "psXMLDocToMetadata() returned non-NULL");
            printMetadata(metaData);
        }
        psFree(newXML);
        psFree(metaData);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Converts existing psMetadata into a psXMLDoc//
    // testXMLConvert01
    {
        psMemId id = psMemGetId();
        psXMLDoc *newXML = NULL;
        newXML = psXMLParseFile(testFile1);
        psMetadata *metaData = NULL;
        metaData = psXMLDocToMetadata(newXML);
        ok(metaData != NULL, "psXMLDocToMetadata() returned non-NULL");
        psXMLDoc *XML2 = NULL;
        XML2 = psMetadataToXMLDoc(metaData);
        ok(psXMLDocToFile(XML2, "psTest2.xml"), "psMetadataToXMLDoc() returned TRUE");
        psFree(newXML);
        psFree(XML2);
        psFree(metaData);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Writes a psXMLDoc to File//
    // testXMLOutput00
    {
        psMemId id = psMemGetId();
        psXMLDoc *newXML = NULL;
        newXML = psXMLParseFile(testFile1);
        ok(psXMLDocToFile(newXML, "psTest.xml"), "psXMLParseFile() returned TRUE");
        psFree(newXML);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Writes a psXMLDoc to Memory//
    // testXMLOutput01
    {
        psMemId id = psMemGetId();
        psXMLDoc *newXML = NULL;
        newXML = psXMLParseFile(testFile1);
        char buffer[2048];
        FILE *file;
        file = fopen("psTest3.xml", "w");
        ok(psXMLDocToMem(newXML, buffer), "fopen() successful");
        fprintf(file, "%s", buffer);
        fclose(file);
        psFree(newXML);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //Writes a psXMLDoc to a file descriptor//
    // testXMLOutput02
    {
        psMemId id = psMemGetId();
        psXMLDoc *newXML = NULL;
        newXML = psXMLParseFile(testFile1);
        int fd = creat("psTest4.xml", 0666);
        ok(!psXMLDocToFD(newXML, fd), "psXMLDocToFD() returned TRUE");
        close(fd);
        psFree(newXML);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testXMLWrongInput
    {
        psMemId id = psMemGetId();
        psXMLDoc *newXML = NULL;
        newXML = psXMLParseFile(testFile2);
        ok(newXML != NULL, "psXMLParseFile() returned non-NULL");
        psMetadata *metaData = NULL;
        metaData = psXMLDocToMetadata(newXML);
        ok(metaData != NULL, "psXMLDocToMetadata() returned non-NULL");
        psFree(newXML);
        psFree(metaData);
        psXMLDoc *newXML2 = NULL;
        psMetadata *metaData2 = NULL;
        newXML2 = psXMLParseFile(testFile3);
        ok(newXML2 != NULL, "psXMLParseFile() returned non-NULL");
        metaData2 = psXMLDocToMetadata(newXML2);
        ok(metaData2 != NULL, "psXMLDocToMetadata() returned non-NULL");
        psFree(newXML2);
        psFree(metaData2);
        psXMLDoc *newXML3 = NULL;
        psMetadata *metaData3 = NULL;
        newXML3 = psXMLParseFile(testFile4);
        ok(newXML3 != NULL, "psXMLParseFile() returned non-NULL");
        metaData3 = psXMLDocToMetadata(newXML3);
        ok(metaData3 != NULL, "psXMLDocToMetadata() returned non-NULL");
        psFree(newXML3);
        psFree(metaData3);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
