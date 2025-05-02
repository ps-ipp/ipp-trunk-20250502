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
*  @version $Revision: 1.7 $  $Name: not supported by cvs2svn $
*  @date  $Date: 2005-10-13 00:50:41 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*
*/

#include <string.h>
#include "config.h"
#include "pslib_strict.h"
#include "psTest.h"
#include <fcntl.h>
#include <unistd.h>


static psS32 testXMLInput00(void);
static psS32 testXMLInput01(void);
static psS32 testXMLInput02(void);
static psS32 testXMLConvert00(void);
static psS32 testXMLConvert01(void);
static psS32 testXMLOutput00(void);
static psS32 testXMLOutput01(void);
static psS32 testXMLOutput02(void);
static psS32 testXMLWrongInput(void);

testDescription tests[] = {
                              {testXMLInput00, 0, "Verify XML file parse", 0, false},
                              {testXMLInput01, 1, "Verify XML memory parse", 0, false},
                              {testXMLInput02, 2, "Verify XML file descriptor parse", 0, false},
                              {testXMLConvert00, 3, "Verify XML to Metadata convert", 0, false},
                              {testXMLConvert01, 4, "Verify Metadata to XML convert", 0, false},
                              {testXMLOutput00, 5, "Verify XML file write", 0, false},
                              {testXMLOutput01, 6, "Verify XML memory write", 0, false},
                              {testXMLOutput02, 7, "Verify XML file descriptor write", 0, false},
                              {testXMLWrongInput, 8, "Verify Incorrect XML Data", 0, false},
                              {NULL}
                          };

//static void writeMetadata(psMetadata* metadata, char* indentStr);

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

psS32 main( psS32 argc, char* argv[] )
{
    psLogSetLevel( PS_LOG_INFO );

    if( !runTestSuite(stderr,"psXML",tests,argc,argv)) {
        return 1;
    }

    return 0;
}

//Parses an XML file into memory.  Stores as a psXMLDoc//
psS32 testXMLInput00(void)
{
    //psXMLParseFile should return a psXMLDoc* of testFile1//
    psXMLDoc *newXML = NULL;
    newXML = psXMLParseFile(testFile1);
    if (newXML == NULL ) {
        fprintf(stderr, "Failed test point #1 xmlDocPtr is NULL \n");
    }
    psFree(newXML);
    return 0;
}

//Parses an XML file from memory.  Stores as a psXMLDoc//
psS32 testXMLInput01(void)
{
    psXMLDoc *newXML = NULL;
    newXML = psXMLParseFile(testFile1);
    char buffer[2048];
    if (!psXMLDocToMem(newXML, buffer)) {
        fprintf(stderr, "Failed test point #2 psXMLDocToMem not Working \n");
    }
    psXMLDoc *newDoc = NULL;
    newDoc = psXMLParseMem(buffer, strlen(buffer) );
    if (newDoc == NULL) {
        fprintf(stderr, "Failed test point #2 xmlDocPtr is NULL \n");
    }
    psFree(newDoc);
    psFree(newXML);
    return 0;
}

//Parses an XML file from a file descriptor.  Stores as a psXMLDoc//
psS32 testXMLInput02(void)
{
    psXMLDoc *newXML = NULL;
    newXML = psXMLParseFile(testFile1);
    if (newXML == NULL ) {
        fprintf(stderr, "Failed test point #3 xmlDocPtr is NULL \n");
    }
    int fd = creat("psTest5.xml", 0666);

    if ( !psXMLDocToFD(newXML, fd) ) {
        fprintf(stderr, "Failed test point #3 psXMLDocToFD not Working \n");
    }

    psXMLDoc *newDoc = NULL;
    fd = open("psTest5.xml", O_RDWR, 0);
    newDoc = psXMLParseFD(fd);
    if ( newDoc == NULL ) {
        fprintf(stderr, "Failed test point #3 xmlDocPtr is NULL \n");
    }

    close(fd);
    psFree(newXML);
    psFree(newDoc);
    return 0;
}

//Converts an existing psXMLDoc into psMetadata//
psS32 testXMLConvert00(void)
{
    psXMLDoc *newXML = NULL;
    newXML = psXMLParseFile(testFile1);
    psMetadata *metaData = NULL;
    metaData = psXMLDocToMetadata(newXML);
    if (metaData == NULL) {
        fprintf(stderr, "Failed test point #4 - Convert to Metadata \n");
    } else {
        printMetadata(metaData);
    }
    psFree(newXML);
    psFree(metaData);
    return 0;
}

//Converts existing psMetadata into a psXMLDoc//
psS32 testXMLConvert01(void)
{
    psXMLDoc *newXML = NULL;
    newXML = psXMLParseFile(testFile1);
    psMetadata *metaData = NULL;
    metaData = psXMLDocToMetadata(newXML);
    if (metaData == NULL) {
        fprintf(stderr, "Failed test point #5 - Convert from Metadata \n");
    }
    psXMLDoc *XML2 = NULL;
    XML2 = psMetadataToXMLDoc(metaData);
    if( !psXMLDocToFile(XML2, "psTest2.xml") ) {
        fprintf(stderr, "Failed test point #5 - Write to File \n");
    }
    psFree(newXML);
    psFree(XML2);
    psFree(metaData);
    return 0;
}

//Writes a psXMLDoc to File//
psS32 testXMLOutput00(void)
{
    psXMLDoc *newXML = NULL;
    newXML = psXMLParseFile(testFile1);
    if( !psXMLDocToFile(newXML, "psTest.xml") ) {
        fprintf(stderr, "Failed test point #6 - Write to File \n");
    }
    psFree(newXML);
    return 0;
}

//Writes a psXMLDoc to Memory//
psS32 testXMLOutput01(void)
{
    psXMLDoc *newXML = NULL;
    newXML = psXMLParseFile(testFile1);
    char buffer[2048];
    FILE *file;
    file = fopen("psTest3.xml", "w");
    if( !psXMLDocToMem(newXML, buffer) ) {
        fprintf(stderr, "Failed test point #7 - Write to Memory \n");
    }
    fprintf(file, "%s", buffer);
    fclose(file);
    psFree(newXML);
    return 0;
}

//Writes a psXMLDoc to a file descriptor//
psS32 testXMLOutput02(void)
{
    psXMLDoc *newXML = NULL;
    newXML = psXMLParseFile(testFile1);
    int fd = creat("psTest4.xml", 0666);

    if( !psXMLDocToFD(newXML, fd) ) {
        fprintf(stderr, "Failed test point #8 - Write to file descriptor \n");
    }
    close(fd);
    psFree(newXML);
    return 0;
}

psS32 testXMLWrongInput(void)
{
    psXMLDoc *newXML = NULL;
    newXML = psXMLParseFile(testFile2);
    if (newXML == NULL ) {
        fprintf(stderr, "Failed to parse xml file.  Incorrect data\n");
    }
    psMetadata *metaData = NULL;
    metaData = psXMLDocToMetadata(newXML);
    if (metaData == NULL) {
        fprintf(stderr, "Incorrect xml data for Metadata Conversion \n");
    }
    psFree(newXML);
    psFree(metaData);
    psXMLDoc *newXML2 = NULL;
    psMetadata *metaData2 = NULL;
    newXML2 = psXMLParseFile(testFile3);
    if (newXML2 == NULL ) {
        fprintf(stderr, "Failed to parse xml file.  Incorrect data\n");
    }
    metaData2 = psXMLDocToMetadata(newXML2);
    if (metaData2 == NULL) {
        fprintf(stderr, "Incorrect xml data for Metadata Conversion \n");
    }
    psFree(newXML2);
    psFree(metaData2);
    psXMLDoc *newXML3 = NULL;
    psMetadata *metaData3 = NULL;
    newXML3 = psXMLParseFile(testFile4);
    if (newXML3 == NULL ) {
        fprintf(stderr, "Failed to parse xml file.  Incorrect data\n");
    }
    metaData3 = psXMLDocToMetadata(newXML3);
    if (metaData3 == NULL) {
        fprintf(stderr, "Incorrect xml data for Metadata Conversion \n");
    }
    psFree(newXML3);
    psFree(metaData3);
    return 0;
}
