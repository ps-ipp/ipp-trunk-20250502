/** @file  tst_psFits.c
*
*  @brief Contains the tests for psFits.[ch]
*
*
*  @author Robert DeSonia, MHPCC
*
*  @version $Revision: 1.25 $ $Name: not supported by cvs2svn $
*  @date $Date: 2006-07-26 03:37:04 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "psTest.h"
#include "pslib_strict.h"

#define GENIMAGE(img,c,r,TYP, valueFcn) \
img = psImageAlloc(c,r,PS_TYPE_##TYP); \
for (psU32 row=0;row<r;row++) { \
    ps##TYP* imgRow = img->data.TYP[row]; \
    for (psU32 col=0;col<c;col++) { \
        imgRow[col] = (ps##TYP)(valueFcn); \
    } \
}

static bool makeMulti(void);  // implicitly tests psFitsSetExtName
static bool makeTable(void);
const char* multiFilename = "multi.fits";
const char* tableFilename = "table.fits";
const int tableNumRows = 10;


// N.B., the tests to Image read/write was liberally taken from the now
// deprecated psImageReadSection/psImageWriteSection function tests.
static psS32 testImageRead(void);
static psS32 testImageWrite(void);

static psS32 tst_psFitsOpen( void );
static psS32 tst_psFitsMoveExtName( void ); // also tests psFitsGetExtName
static psS32 tst_psFitsMoveExtNum( void );  // also tests psFitsGetExtNum, psFitsGetSize
static psS32 tst_psFitsReadHeader( void );
static psS32 tst_psFitsReadHeaderSet( void );
static psS32 tst_psFitsReadTable( void );
static psS32 tst_psFitsReadTableColumnNum(void);
static psS32 tst_psFitsReadTableColumn(void);
static psS32 tst_psFitsUpdateTable(void);
static psS32 tst_psFitsWriteHeader(void);

testDescription tests[] = {
                              {tst_psFitsOpen, 801, "psFitsOpen", 0, false},
                              {tst_psFitsMoveExtName, 802, "psFitsMoveExtName", 0, false},
                              {tst_psFitsMoveExtName, 802, "psFitsGetExtName", 0, true},
                              {tst_psFitsMoveExtNum, 803, "psFitsMoveExtNum", 0, false},
                              {tst_psFitsMoveExtNum, 803, "psFitsGetExtNum", 0, true},
                              {tst_psFitsMoveExtNum, 803, "psFitsGetSize", 0, true},
                              {tst_psFitsReadHeader, 804, "psFitsReadHeader", 0, false},
                              {tst_psFitsReadHeaderSet,805, "psFitsReadHeaderSet", 0, false},
                              {tst_psFitsReadTable,809, "psFitsReadTable", 0, false},
                              {tst_psFitsReadTableColumnNum,836, "psFitsReadTableColumnNum", 0, false},
                              {tst_psFitsReadTableColumn,839, "psFitsReadTableColumn", 0, false},
                              {tst_psFitsUpdateTable,840, "psFitsUpdateTable", 0, false},
                              {testImageRead,567, "psFitsReadImage", 0, false},
                              {testImageWrite,569, "psFitsWriteImage", 0, false},
                              {tst_psFitsWriteHeader,000,"psFitsWriteHeader",0,false},
                              {NULL}
                          };

psS32 main( psS32 argc, char* argv[] )
{
    psLogSetLevel( PS_LOG_INFO );

    return ( ! runTestSuite( stderr, "psFits", tests, argc, argv ) );
}

bool makeMulti(void)
{
    psFits* fitsFile = psFitsOpen(multiFilename,"w");

    if (fitsFile == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "Could not create 'multi' FITS file.");
        return false;
    }

    psImage* image = psImageAlloc(16,16,PS_TYPE_F32);

    char extname[80];
    for (int lcv = 0; lcv < 8; lcv++) {
        snprintf(extname,80,"ext-%d", lcv);

        psMetadata* header = psMetadataAlloc();

        psMetadataAdd(header,PS_LIST_TAIL, "MYINT",
                      PS_DATA_S32,
                      "psS32 Item", (psS32)lcv);

        psMetadataAdd(header,PS_LIST_TAIL, "MYFLT",
                      PS_DATA_F32,
                      "psF32 Item", (float)(1.0f/(float)(1+lcv)));

        psMetadataAdd(header,PS_LIST_TAIL, "MYDBL",
                      PS_DATA_F64,
                      "psF64 Item", (double)(1.0/(double)(1+lcv)));

        psMetadataAdd(header,PS_LIST_TAIL, "MYBOOL",
                      PS_DATA_BOOL,
                      "psBool Item",
                      (lcv%2 == 0));

        psMetadataAdd(header,PS_LIST_TAIL, "MYSTR",
                      PS_DATA_STRING,
                      "String Item",
                      extname);

        // set the pixels in the image
        psImageInit(image, (float)lcv);
        if (! psFitsWriteImage(fitsFile,header,image,0,extname) ) {
            psError(PS_ERR_UNKNOWN, false,
                    "Could not write image.");
            return false;
        }

        psFree(header);
    }
    psFree(image);
    psFree(fitsFile);

    return true;
}

bool makeTable(void)
{
    psFits* fitsFile = psFitsOpen(tableFilename,"w");

    if (fitsFile == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "Could not create 'table' FITS file.");
        return false;
    }

    // make the PHU an image (per FITS standard, it must be)
    psImage* image = psImageAlloc(16,16,PS_TYPE_F32);

    if (! psFitsWriteImage(fitsFile,NULL,image,1,NULL) ) {
        psError(PS_ERR_UNKNOWN, false,
                "Could not write PHU image.");
        return false;
    }

    psFree(image);

    // build a table structure
    psArray* table = psArrayAlloc(tableNumRows);
    //    table->n = tableNumRows;
    psMetadata* header = NULL;
    for (int row = 0; row < tableNumRows; row++) {
        header = psMetadataAlloc();

        psMetadataAdd(header,PS_LIST_TAIL, "MYINT",
                      PS_DATA_S32,
                      "psS32 Item",
                      (psS32)row);

        psMetadataAdd(header,PS_LIST_TAIL, "MYFLT",
                      PS_DATA_F32,
                      "psF32 Item",
                      (float)(1.0f/(float)(1+row)));

        psMetadataAdd(header,PS_LIST_TAIL, "MYDBL",
                      PS_DATA_F64,
                      "psF64 Item",
                      (double)(1.0/(double)(1+row)));

        psMetadataAdd(header,PS_LIST_TAIL, "MYBOOL",
                      PS_DATA_BOOL,
                      "psBool Item",
                      (row%2 == 0));

        char* str = NULL;
        psStringAppend(&str,"row=%d",row+1);
        psMetadataAdd(header,PS_LIST_TAIL, "MYSTR",
                      PS_DATA_STRING,
                      "psString Item",
                      str);
        psFree(str);

        psVector* vec = psVectorAlloc(5,PS_TYPE_S32);
        for (int x=0; x < 4; x++) {
            vec->data.S32[x] = x*10+row;
            vec->n++;
        }
        psMetadataAdd(header,PS_LIST_TAIL, "MYVEC",
                      PS_DATA_VECTOR,
                      "psVector Item",
                      vec);
        psFree(vec);

        table->data[row] = header;
        table->n++;
    }

    psFitsWriteTable(fitsFile, NULL, table, NULL);

    psFree(table);
    psFree(fitsFile);
    //    return 1;
    return (!psMemCheckLeaks(15,NULL,stderr,false));
}

psS32 tst_psFitsOpen( void )
{

    if (! makeMulti() ) {
        return 1;
    }

    psFits* fitsFile = psFitsOpen(multiFilename,"r");

    if (fitsFile == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen returned NULL on existing file.");
        return 1;
    }

    int extNum = psFitsGetExtNum(fitsFile);
    if (extNum != 0) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen was not queued to the PHU, but to extension #%d.",
                extNum);
        return 2;
    }

    psFitsClose(fitsFile);

    // make sure the file doesn't already exist.
    if (access("new.fits", F_OK) == 0) {
        if (remove
                ("new.fits") != 0) {
            psError(PS_ERR_UNKNOWN, false,
                    "Couldn't delete the new.fits file.");
            return 3;
        }
    }

    fitsFile = psFitsOpen("new.fits","w");

    if (fitsFile == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen returned NULL on w mode.");
        return 4;
    }

    // write something to the file, otherwise CFITSIO will complain on close
    psImage* img = psImageAlloc(16,16,PS_TYPE_F32);
    psFitsWriteImage(fitsFile,NULL,img,1,NULL);

    psFree(fitsFile); // psFree should be equivalent to psFitsClose

    // now, if psFitsOpen actually created the file, I shouldn't error in removing it.
    if (remove
            ("new.fits") != 0) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen seemed to not have created a new file.");
        return 5;
    }

    fitsFile = psFitsOpen("new.fits","w+");

    if (fitsFile == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen returned NULL on w+ mode.");
        return 6;
    }

    // write something to the file, otherwise CFITSIO will complain on close
    psFitsWriteImage(fitsFile,NULL,img,1,NULL);

    psFitsClose(fitsFile);

    // now, if psFitsOpen actually created the file, I shouldn't error in removing it.
    if (remove
            ("new.fits") != 0) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen seemed to not have created a new file.");
        return 7;
    }

    fitsFile = psFitsOpen("new.fits","a");

    if (fitsFile == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen returned NULL on a mode.");
        return 8;
    }

    // write something to the file, otherwise CFITSIO will complain on close
    psFitsWriteImage(fitsFile,NULL,img,1,NULL);

    psFitsClose(fitsFile);

    fitsFile = psFitsOpen("new.fits","a+");

    if (fitsFile == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen returned NULL on a+ mode.");
        return 9;
    }

    psFitsClose(fitsFile);

    // now, if psFitsOpen actually created the file, I shouldn't error in removing it.
    if (remove
            ("new.fits") != 0) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen seemed to not have created a new file.");
        return 10;
    }

    // Attempt to allocate with NULL filename
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    fitsFile = psFitsOpen(NULL,"r");
    if(fitsFile != NULL) {
        psError(PS_ERR_UNKNOWN,true,"psFitsOpen did not return NULL for NULL input");
        return 11;
    }

    // Attempt to use an invalid mode
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    fitsFile = psFitsOpen("new.fits","b+");
    if(fitsFile != NULL) {
        psError(PS_ERR_UNKNOWN,true,"psFitsOpen did not return NULL for NULL input");
        return 12;
    }

    psFree(img);

    return 0;
}

psS32 tst_psFitsMoveExtName( void )
{

    if (! makeMulti() ) {
        return 1;
    }

    psFits* fits = psFitsOpen(multiFilename,"r");

    if (fits == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen returned NULL on existing file.");
        return 1;
    }

    int numHDUs = psFitsGetSize(fits);

    if (numHDUs < 2) {
        psError(PS_ERR_UNKNOWN,true,
                "The 'multi' FITS file does not have multiple HDUs.");
        return 2;
    }

    char extName[80];
    psRegion region = {0,0,0,0};

    for (int lcv = 0; lcv < numHDUs; lcv++) {
        snprintf(extName,80,"ext-%d",lcv);
        // try to move to the named extension.
        if (! psFitsMoveExtName(fits, extName) ) {
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to move to ext-%d.",
                    lcv);
            return 3;
        }

        // check to see if I can retrieve the name back from the psFits object.
        char* nameFromFile = psFitsGetExtName(fits);
        if (strcmp(nameFromFile,extName) != 0) { // hey, it didn't move?
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to retrieve the extension name back ('%s' vs '%s'",
                    nameFromFile, extName);
            return 3;
        }
        psFree(nameFromFile);

        // check that the image is associated to the extension moved, i.e.,
        // did we really move to the proper extension?
        psImage* image = NULL;
        image = psFitsReadImage(fits,region,0);

        if (image == NULL || abs(image->data.F32[0][0] - (float)lcv) > FLT_EPSILON) {
            psError(PS_ERR_UNKNOWN, true,
                    "The image pixel 0,0 of ext-%d was %g, expected %d.",
                    lcv,image->data.F32[0][0],lcv);
            return 4;
        }
        psFree(image);
    }

    for (int lcv = numHDUs-1; lcv >= 0; lcv--) {
        snprintf(extName,80,"ext-%d",lcv);
        // try to move to the named extension.
        if (! psFitsMoveExtName(fits, extName) ) {
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to move to ext-%d.",
                    lcv);
            return 5;
        }

        // check to see if I can retrieve the name back from the psFits object.
        char* nameFromFile = psFitsGetExtName(fits);
        if (strcmp(nameFromFile,extName) != 0) { // hey, it didn't move?
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to retrieve the extension name back ('%s' vs '%s'",
                    nameFromFile, extName);
            return 5;
        }
        psFree(nameFromFile);

        // check that the image is associated to the extension moved, i.e.,
        // did we really move to the proper extension?
        psImage* image = NULL;
        image = psFitsReadImage(fits,region,0);

        if (abs(image->data.F32[0][0] - (float)lcv) > FLT_EPSILON) {
            psError(PS_ERR_UNKNOWN, true,
                    "The image pixel 0,0 of ext-%d was %g, expected %d.",
                    lcv,image->data.F32[0][0],lcv);
            return 6;
        }
        psFree(image);
    }

    // check to see if given a bogus extension name, it errors.
    psLogMsg(__func__,PS_LOG_INFO, "Following should be an error.");
    if (psFitsMoveExtName(fits, "bogus") || psErrorGetStackSize() != 1) {
        psError(PS_ERR_UNKNOWN, false,
                "Moving to non-existant HDU didn't fail.");
        return 7;
    }

    // check to see if given a NULL psFits, it errors.
    psLogMsg(__func__,PS_LOG_INFO, "Following should be an error.");
    if (psFitsMoveExtName(NULL, "bogus") || psErrorGetStackSize() != 1) {
        psError(PS_ERR_UNKNOWN, false,
                "Operation of NULL psFits didn't fail.");
        return 8;
    }

    // check to see if given a NULL extname, it errors.
    psLogMsg(__func__,PS_LOG_INFO, "Following should be an error.");
    if (psFitsMoveExtName(fits, NULL) || psErrorGetStackSize() != 1) {
        psError(PS_ERR_UNKNOWN, false,
                "Operation of NULL extname didn't fail.");
        return 9;
    }

    psFree(fits);

    // Attempt to get ext name from null fits file
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error for NULL fits file");
    if(psFitsGetExtName(NULL) != NULL) {
        psError(PS_ERR_UNKNOWN,true,"Expected NULL return from psFitsGetExtName with NULL fits file");
        return 10;
    }

    return 0;
}

psS32 tst_psFitsMoveExtNum( void )
{

    if (! makeMulti() ) {
        return 1;
    }

    psFits* fits = psFitsOpen(multiFilename,"r");

    if (fits == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen returned NULL on existing file.");
        return 1;
    }

    int numHDUs = psFitsGetSize(fits);

    // as a side test, let's make sure psFitsGetSize can handle NULL.
    psLogMsg(__func__,PS_LOG_INFO,
             "Following should be an error.");
    psErrorClear();
    if (psFitsGetSize(NULL) != 0 || psErrorGetStackSize() != 1) {
        psError(PS_ERR_UNKNOWN,true,
                "The 'multi' FITS file does not have multiple HDUs.");
        return 2;
    }

    if (numHDUs != 8) {
        psError(PS_ERR_UNKNOWN,true,
                "The 'multi' FITS file does not have multiple HDUs.");
        return 2;
    }

    psRegion region = {0,0,0,0};

    // test absolute positioning
    for (int lcv = 0; lcv < numHDUs; lcv++) {
        // try to move to the extension
        if (! psFitsMoveExtNum(fits, lcv, false) ) {
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to move to extension %d.",
                    lcv);
            return 3;
        }

        // check to see if I can retrieve the number back from the psFits object.
        if (psFitsGetExtNum(fits) != lcv) { // hey, it didn't move?
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to retrieve the extension number back (%d vs %d)",
                    psFitsGetExtNum(fits), lcv);
            return 5;
        }

        // check that the image is associated to the extension moved, i.e.,
        // did we really move to the proper extension?
        psImage* image = NULL;
        image = psFitsReadImage(fits,region,0);
        if (image == NULL || abs(image->data.F32[0][0] - (float)lcv) > FLT_EPSILON) {
            psError(PS_ERR_UNKNOWN, true,
                    "The image pixel 0,0 of ext-%d was %g, expected %d.",
                    lcv,image->data.F32[0][0],lcv);
            return 4;
        }
        psFree(image);
    }

    for (int lcv = numHDUs-1; lcv >= 0; lcv--) {
        // try to move to the extension
        if (! psFitsMoveExtNum(fits, lcv, false) ) {
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to move to extension %d.",
                    lcv);
            return 5;
        }

        // check that the image is associated to the extension moved, i.e.,
        // did we really move to the proper extension?
        psImage* image = NULL;
        image = psFitsReadImage(fits,region,0);

        if (abs(image->data.F32[0][0] - (float)lcv) > FLT_EPSILON) {
            psError(PS_ERR_UNKNOWN, true,
                    "The image pixel 0,0 of ext-%d was %g, expected %d.",
                    lcv,image->data.F32[0][0],lcv);
            return 6;
        }
        psFree(image);
    }

    // test relative positioning
    psFitsMoveExtNum(fits,0,false);
    for (int lcv = 1; lcv < numHDUs; lcv++) {
        // try to move to the extension
        if (! psFitsMoveExtNum(fits, 1, true) ) {
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to move to extension %d.",
                    lcv);
            return 13;
        }

        // check to see if I can retrieve the number back from the psFits object.
        if (psFitsGetExtNum(fits) != lcv) { // hey, it didn't move?
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to retrieve the extension number back (%d vs %d)",
                    psFitsGetExtNum(fits), lcv);
            return 13;
        }

        // check that the image is associated to the extension moved, i.e.,
        // did we really move to the proper extension?
        psImage* image = NULL;
        image = psFitsReadImage(fits,region,0);

        if (image == NULL || abs(image->data.F32[0][0] - (float)lcv) > FLT_EPSILON) {
            psError(PS_ERR_UNKNOWN, true,
                    "The image pixel 0,0 of ext-%d was %g, expected %d.",
                    lcv,image->data.F32[0][0],lcv);
            return 14;
        }
        psFree(image);
    }

    for (int lcv = numHDUs-2; lcv >= 0; lcv--) {
        // try to move to the extension
        if (! psFitsMoveExtNum(fits, -1, true) ) {
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to move to extension %d.",
                    lcv);
            return 15;
        }

        // check to see if I can retrieve the number back from the psFits object.
        if (psFitsGetExtNum(fits) != lcv) { // hey, it didn't move?
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to retrieve the extension number back (%d vs %d)",
                    psFitsGetExtNum(fits), lcv);
            return 15;
        }

        // check that the image is associated to the extension moved, i.e.,
        // did we really move to the proper extension?
        psImage* image = NULL;
        image = psFitsReadImage(fits,region,0);

        if (abs(image->data.F32[0][0] - (float)lcv) > FLT_EPSILON) {
            psError(PS_ERR_UNKNOWN, true,
                    "The image pixel 0,0 of ext-%d was %g, expected %d.",
                    lcv,image->data.F32[0][0],lcv);
            return 16;
        }
        psFree(image);
    }

    // check to see if given a negative extension number, it errors.
    psLogMsg(__func__,PS_LOG_INFO, "Following should be an error.");
    psErrorClear();
    if (psFitsMoveExtNum(fits, -1, false) || psErrorGetStackSize() != 1) {
        psError(PS_ERR_UNKNOWN, false,
                "Moving to negative HDU didn't fail.");
        return 21;
    }

    // check to see if relative positioning beyond PHU, it errors.
    psFitsMoveExtNum(fits,0,false);
    psLogMsg(__func__,PS_LOG_INFO, "Following should be an error.");
    psErrorClear();
    if (psFitsMoveExtNum(fits, -1, true) || psErrorGetStackSize() != 1) {
        psError(PS_ERR_UNKNOWN, false,
                "Moving to negative HDU didn't fail.");
        return 22;
    }


    // check to see if given a extension greater than the total #HDUs, it errors.
    psLogMsg(__func__,PS_LOG_INFO, "Following should be an error.");
    psErrorClear();
    if (psFitsMoveExtNum(fits, numHDUs, false) || psErrorGetStackSize() != 1) {
        psError(PS_ERR_UNKNOWN, false,
                "Moving to a HDU beyond the file's contents didn't fail.");
        return 31;
    }

    // check to see if relative positioning beyond PHU, it errors.
    psFitsMoveExtNum(fits,numHDUs-1,false);
    psLogMsg(__func__,PS_LOG_INFO, "Following should be an error.");
    psErrorClear();
    if (psFitsMoveExtNum(fits, 1, true) || psErrorGetStackSize() != 1) {
        psError(PS_ERR_UNKNOWN, false,
                "Moving to negative HDU didn't fail.");
        return 32;
    }

    // check to see if given a NULL psFits, it errors.
    psLogMsg(__func__,PS_LOG_INFO, "Following should be an error.");
    psErrorClear();
    if (psFitsMoveExtNum(NULL, 0, false) || psErrorGetStackSize() != 1) {
        psError(PS_ERR_UNKNOWN, false,
                "Operation of NULL psFits didn't fail.");
        return 40;
    }

    psFitsClose(fits);

    // Attempt to get ext name from null fits file
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error for NULL fits file");
    if(psFitsGetExtNum(NULL) != PS_FITS_TYPE_NONE) {
        psError(PS_ERR_UNKNOWN,true,"Expected NULL return from psFitsGetExtNum with NULL fits file");
        return 10;
    }

    return 0;
}

static psS32 tst_psFitsReadHeader( void )
{
    if (! makeMulti() ) {
        return 1;
    }

    psFits* fits = psFitsOpen(multiFilename,"r");

    if (fits == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen returned NULL on existing file.");
        return 1;
    }

    int numHDUs = psFitsGetSize(fits);

    if (numHDUs < 8) {
        psError(PS_ERR_UNKNOWN,true,
                "The 'multi' FITS file does not have multiple HDUs.");
        return 2;
    }

    char extname[80];
    for (int hdunum = 0; hdunum < numHDUs; hdunum++) {
        snprintf(extname,80,"ext-%d",hdunum);

        psFitsMoveExtNum(fits,hdunum,false);

        psMetadata* header = psFitsReadHeader(NULL,fits);
        if (header == NULL) {
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to read header");
            return 3;
        }

        psMetadata* header2 = psMetadataAlloc();
        header2 = psFitsReadHeader(header2,fits);
        if (header2 == NULL) {
            psError(PS_ERR_UNKNOWN, false,
                    "Failed to read header");
            return 11;
        }

        if (header->list->n < 1 || header->list->n != header2->list->n) {
            psError(PS_ERR_UNKNOWN, true,
                    "Reading the header given a NULL input psMetadata differed "
                    "from giving an existing psMetadata.");
            return 12;
        }

        // check for the extra metadata items
        psS32 intItem = psMetadataLookupS32(NULL,header, "MYINT");
        psF32 fltItem = psMetadataLookupF32(NULL,header, "MYFLT");
        psF64 dblItem = psMetadataLookupF64(NULL,header, "MYDBL");
        psMetadataItem* boolItem = psMetadataLookup(header, "MYBOOL");
        psString strItem = psMetadataLookupStr(NULL, header, "MYSTR");

        if (intItem != hdunum) {
            psError(PS_ERR_UNKNOWN, true,
                    "Failed to retrieve psS32 metadata item from file.");
            return 20;
        }

        if (fabsf(fltItem - 1.0f/(float)(1+hdunum)) > FLT_EPSILON) {
            psError(PS_ERR_UNKNOWN, true,
                    "Failed to retrieve psF32 metadata item from file.  Got %f vs %f",
                    fltItem,1.0f/(float)(1+hdunum));
            return 21;
        }

        if (abs(dblItem - 1.0/(double)(1+hdunum)) > DBL_EPSILON) {
            psError(PS_ERR_UNKNOWN, true,
                    "Failed to retrieve psF64 metadata item from file.  Got %g vs %g",
                    dblItem, 1.0/(double)(1+hdunum));
            return 22;
        }

        if (boolItem == NULL ||
                boolItem->type != PS_DATA_BOOL) {
            psError(PS_ERR_UNKNOWN, true,
                    "Failed to retrieve psBool metadata item from file.");
            return 23;
        }

        if (strItem == NULL || strncmp(strItem,extname,strlen(extname)) != 0) {
            psError(PS_ERR_UNKNOWN, true,
                    "Failed to retrieve string metadata item from file.  Got '%s' vs '%s' (%d)",
                    strItem,extname,strlen(extname));
            return 24;
        }

        psFree(header);
        psFree(header2);
    }

    psLogMsg(__func__,PS_LOG_INFO,"following should be an error (input psFits = NULL)");
    psMetadata* header = psFitsReadHeader(NULL,NULL);

    if (header != NULL || psErrorGetStackSize() != 1) {
        psError(PS_ERR_UNKNOWN, true,
                "psFitsReadHeader didn't error on a NULL psFits.");
        return 30;
    }


    psFree(fits);

    return 0;
}

static psS32 tst_psFitsReadHeaderSet( void )
{
    if (! makeMulti() ) {
        return 1;
    }

    psFits* fits = psFitsOpen(multiFilename,"r");

    if (fits == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen returned NULL on existing file.");
        return 1;
    }

    int numHDUs = psFitsGetSize(fits);

    if (numHDUs < 8) {
        psError(PS_ERR_UNKNOWN,true,
                "The 'multi' FITS file does not have multiple HDUs.");
        return 2;
    }

    // move to the middle
    psFitsMoveExtNum(fits,numHDUs/2, false);

    psMetadata* headerSet = psFitsReadHeaderSet(NULL,fits);

    if (headerSet == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsReadHeaderSet returned NULL unexpectedly.");
        return 3;
    }

    if (psFitsGetExtNum(fits) != numHDUs/2) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsReadHeaderSet changed the CHU.");
        return 4;
    }

    char extname[80];
    for (int i = 0; i < numHDUs; i++) {
        if (i == 0) {
            snprintf(extname, 80, "PHU");
        } else {
            snprintf(extname, 80, "ext-%d", i);
        }

        psMetadata* header = psMetadataLookupPtr(NULL,headerSet, extname);

        if (header == NULL) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsReadHeader returned NULL unexpectedly for HDU#%d.",
                    i);
            return 5;
        }

        psS32 intItem = psMetadataLookupS32(NULL, header, "MYINT");

        if (intItem != i) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsReadHeader for HDU#%d had a MYINT of %d, expected %d.",
                    intItem, i);
            return 6;
        }
    }

    psMetadata* set3 = psFitsReadHeaderSet(NULL,fits);
    if (set3 == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsReadHeaderSet returned NULL unexpectedly.");
        return 11;
    }

    for (int i = 0; i < numHDUs; i++) {
        if (i == 0) {
            snprintf(extname, 80, "PHU");
        } else {
            snprintf(extname, 80, "ext-%d", i);
        }

        psMetadata* header = psMetadataLookupPtr(NULL, set3, extname);

        if (header == NULL) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsReadHeader returned NULL unexpectedly for HDU#%d.",
                    i);
            return 5;
        }

        psS32 intItem = psMetadataLookupS32(NULL, header, "MYINT");

        if (intItem != i) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsReadHeader for HDU#%d had a MYINT of %d, expected %d.",
                    intItem, i);
            return 6;
        }
    }

    set3 = psFitsReadHeaderSet(set3,NULL);
    if (set3 != NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsReadHeaderSet returned non-NULL given a NULL psFits.");
        return 10;
    }

    psFree(headerSet);

    psFitsClose(fits);

    return 0;
}

static psS32 tst_psFitsReadTable( void )
{


    if (! makeTable()) {
        return 111;
    }

    psFits* fits = psFitsOpen(tableFilename,"r");

    if (fits == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen returned NULL on existing file.");
        return 1;
    }

    psFitsMoveExtNum(fits,1,false);

    psArray* table = psFitsReadTable(fits);

    if (table == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsReadTable returned NULL unexpectedly.");
        return 2;
    }

    if (table->n != tableNumRows) {
        psError(PS_ERR_UNKNOWN, false,
                "Expected %d rows, but read %d.",
                tableNumRows, table->n);
        return 3;
    }


    for (int row = 0; row < table->n; row++) {
        psMetadata* rowData = table->data[row];

        psS32 intItem = psMetadataLookupS32(NULL, rowData, "MYINT");
        psF32 fltItem = psMetadataLookupF32(NULL, rowData, "MYFLT");
        psF64 dblItem = psMetadataLookupF64(NULL, rowData, "MYDBL");
        psBool boolItem = psMetadataLookupBool(NULL, rowData, "MYBOOL");
        psString strItem = psMetadataLookupStr(NULL, rowData, "MYSTR");
        psVector* vecItem = psMetadataLookupPtr(NULL, rowData, "MYVEC");

        if (intItem != row) {
            psError(PS_ERR_UNKNOWN, true,
                    "Failed to retrieve psS32 metadata item from file (row=%d).  Got %d vs %d",
                    row, intItem, row);
            return 20;
        }

        if (fabsf(fltItem - 1.0f/(float)(1+row)) > FLT_EPSILON) {
            psError(PS_ERR_UNKNOWN, true,
                    "Failed to retrieve psF32 metadata item from file (row=%d).  Got %f vs %f",
                    row, fltItem,1.0f/(float)(1+row));
            return 21;
        }

        if (abs(dblItem - 1.0/(double)(1+row)) > DBL_EPSILON) {
            psError(PS_ERR_UNKNOWN, true,
                    "Failed to retrieve psF64 metadata item from file (row=%d).  Got %g vs %g",
                    row, dblItem, 1.0/(double)(1+row));
            return 22;
        }

        if ( boolItem != ((row&0x01) == 0)) {
            psError(PS_ERR_UNKNOWN, true,
                    "Failed to retrieve psBool metadata item from file (row=%d). Got %d vs %d",
                    row, boolItem, ((row&0x01) == 0));
            return 23;
        }

        char strValue[16];
        snprintf(strValue,16,"row=%d",row+1);
        if ( strncmp(strItem,strValue,strlen(strValue)) != 0) {
            psError(PS_ERR_UNKNOWN, true,
                    "Failed to retrieve psString metadata item from file (row=%d). Got '%s' vs '%s'",
                    row, strItem, strValue);
            return 24;
        }

        if ( vecItem == NULL ) {
            psError(PS_ERR_UNKNOWN, true,
                    "Failed to retrieve psVector metadata item from file (row=%d).",
                    row);
            return 25;
        }

        if ( vecItem->type.type != PS_DATA_S32 ||
                vecItem->data.S32[0] != row ||
                vecItem->data.S32[3] != row+30 ) {
            psError(PS_ERR_UNKNOWN, true,
                    "Retrieved psVector (row=%d) didn't match expected values [%d,%d,%d,%d] vs [%d,%d,%d,%d].",
                    row,vecItem->data.S32[0], vecItem->data.S32[1],
                    vecItem->data.S32[2], vecItem->data.S32[3],
                    row,row+10,row+20,row+30);
            return 26;
        }

    }

    psFree(table);
    psFree(fits);

    psArray* nullTest = psFitsReadTable(NULL);

    if (nullTest != NULL || psErrorGetStackSize() != 1) {
        psError(PS_ERR_UNKNOWN, true,
                "psFitsReadTable returned non-NULL when given NULL.");
        return 30;
    }

    return 0;
}

static psS32 tst_psFitsReadTableColumnNum( void )
{
    if (! makeTable()) {
        return 111;
    }

    psFits* fits = psFitsOpen(tableFilename,"r");

    if (fits == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen returned NULL on existing file.");
        return 1;
    }

    psFitsMoveExtNum(fits,1,false);

    // read the column data via number
    psVector* colData;
    psElemType type[4] = {PS_TYPE_S32, PS_TYPE_F32, PS_TYPE_F32, PS_TYPE_BOOL};
    psElemType altType[4] = {PS_TYPE_S64, PS_TYPE_F64, PS_TYPE_F64, PS_TYPE_BOOL};
    char* colname[4] = {"MYINT","MYFLT","MYDBL","MYBOOL"};
    psF64 expectedValues[4][10] = {
                                      {0,1,2,3,4,5,6,7,8,9},
                                      {1.0,1.0/2.0,1.0/3.0,1.0/4.0,1.0/5.0,1.0/6.0,1.0/7.0,1.0/8.0,1.0/9.0},
                                      {1.0,1.0/2.0,1.0/3.0,1.0/4.0,1.0/5.0,1.0/6.0,1.0/7.0,1.0/8.0,1.0/9.0},
                                      {1.0,0.0,1.0,0.0,1.0,0.0,1.0,0.0,1.0,0.0}
                                  };

    for (int col = 0; col < 4; col++) {
        colData = psFitsReadTableColumnNum(fits,colname[col]);
        if (colData == NULL) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsReadTableColumnNum returned NULL for col=%d",
                    col);
            return 2;
        }
        if (colData->type.type != type[col] &&
                colData->type.type != altType[col]) {
            char* typeRead;
            char* typeExpected;
            PS_TYPE_NAME(typeRead, colData->type.type);
            PS_TYPE_NAME(typeExpected, type[col]);

            psError(PS_ERR_UNKNOWN, false,
                    "psFitsReadTableColumnNum returned different type, %s vs %s, for col=%d",
                    typeRead, typeExpected, col);
            return 3;
        }
        if (colData->n != tableNumRows) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsReadTableColumnNum returned different number of rows, %d vs %d, for col=%d",
                    colData->n, tableNumRows, col);
            return 4;
        }
        for (int row = 0; row < tableNumRows; row++) {
            if (abs(p_psVectorGetElementF64(colData,row) - expectedValues[col][row]) > FLT_EPSILON) {
                psError(PS_ERR_UNKNOWN, false,
                        "psFitsReadTableColumnNum returned unexpected values (%g vs %g) for col=%d",
                        p_psVectorGetElementF64(colData,row), expectedValues[col][row], col);
                return 5;
            }
        }
        psFree(colData);
    }

    psWarning("Following should be an error.");
    psErrorClear();
    psVector* data = psFitsReadTableColumnNum(NULL,colname[0]);
    psErr* err = psErrorLast();
    if (data != NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsReadTableColumnNum did not return NULL with NULL psFits");
        return 6;
    }
    if (err->code != PS_ERR_BAD_PARAMETER_NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsReadTableColumnNum did not error with NULL psFits");
        return 7;
    }
    psFree(err);

    psWarning("Following should be an error.");
    psErrorClear();
    data = psFitsReadTableColumnNum(fits,"BOGUS");
    err = psErrorLast();
    if (data != NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsReadTableColumnNum did not return NULL with bogus column name.");
        return 8;
    }
    if (err->code != PS_ERR_IO) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsReadTableColumnNum did not error with bogus column name.");
        return 9;
    }
    psFree(err);

    psFree(fits);

    return 0;
}

static psS32 tst_psFitsReadTableColumn( void )
{
    if (! makeTable()) {
        return 111;
    }

    psFits* fits = psFitsOpen(tableFilename,"r");

    if (fits == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen returned NULL on existing file.");
        return 1;
    }

    psFitsMoveExtNum(fits,1,false);

    // read the column data via number
    psArray* colData;
    char* colname[4] = {"MYINT","MYFLT","MYDBL","MYBOOL"};
    psF64 expectedValues[3][10] = {
                                      {0,1,2,3,4,5,6,7,8,9},
                                      {1.0,1.0/2.0,1.0/3.0,1.0/4.0,1.0/5.0,1.0/6.0,1.0/7.0,1.0/8.0,1.0/9.0},
                                      {1.0,1.0/2.0,1.0/3.0,1.0/4.0,1.0/5.0,1.0/6.0,1.0/7.0,1.0/8.0,1.0/9.0}
                                  };
    char* expectedBoolValues[10] = {"T","F","T","F","T","F","T","F","T","F"};
    char* expectedStrValues[10] = {"row=1","row=2","row=3","row=4","row=5","row=6","row=7","row=8","row=9","row=10"};

    for (int col = 0; col < 4; col++) {
        colData = psFitsReadTableColumn(fits,colname[col]);
        if (colData == NULL) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsReadTableColumn returned NULL for col=%d",
                    col);
            return 2;
        }
        if (colData->n != tableNumRows) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsReadTableColumn returned different number of rows, %d vs %d, for col=%d",
                    colData->n, tableNumRows, col);
            return 4;
        }
        if (col < 3) {
            for (int row = 0; row < tableNumRows; row++) {
                if (abs(atof((char*)colData->data[row]) - expectedValues[col][row]) > 0.0001) {
                    psError(PS_ERR_UNKNOWN, false,
                            "psFitsReadTableColumn returned unexpected values (%g vs %g) for col=%d",
                            atof((char*)colData->data[row]), expectedValues[col][row], col);
                    return 5;
                }
            }
        } else if (col == 3) {
            for (int row = 0; row < tableNumRows; row++) {
                if (strncmp(colData->data[row],expectedBoolValues[row],
                            strlen(expectedBoolValues[row])) != 0) {
                    psError(PS_ERR_UNKNOWN, false,
                            "psFitsReadTableColumn returned unexpected values ('%s' vs '%s') for col=%d",
                            (char*)colData->data[row], expectedBoolValues[row], col);
                    return 5;
                }
            }
        } else if (col == 4) {
            for (int row = 0; row < tableNumRows; row++) {
                if (strncmp(colData->data[row],expectedStrValues[row],
                            strlen(expectedStrValues[row])) != 0) {
                    psError(PS_ERR_UNKNOWN, false,
                            "psFitsReadTableColumn returned unexpected values ('%s' vs '%s') for col=%d",
                            (char*)colData->data[row], expectedStrValues[row], col);
                    return 5;
                }
            }
        }

        psFree(colData);
    }

    psWarning("Following should be an error.");
    psErrorClear();
    psArray* data = psFitsReadTableColumn(NULL,"MYINT");
    psErr* err = psErrorLast();
    if (data != NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsReadTableColumn did not return NULL with NULL psFits");
        return 6;
    }
    if (err->code != PS_ERR_BAD_PARAMETER_NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsReadTableColumn did not error with NULL psFits");
        return 7;
    }
    psFree(err);

    psWarning(__func__,"Following should be an error.");
    psErrorClear();
    data = psFitsReadTableColumn(fits,"BOGUS");
    err = psErrorLast();
    if (data != NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsReadTableColumn did not return NULL with col=\"BOGUS\"");
        return 8;
    }
    if (err->code != PS_ERR_IO) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsReadTableColumn did not error with col=\"BOGUS\"");
        return 9;
    }
    psFree(err);

    psFree(fits);

    return 0;
}

static psS32 tst_psFitsUpdateTable( void )
{
    psErr* err;
    char* strValue[] = {
                           "row A",
                           "row B",
                           "row C",
                           "row D",
                           "row E",
                           "row F",
                           "row G",
                           "row H",
                           "row I",
                           "row J",
                           "row K"
                       };


    if (! makeTable()) {
        return 111;
    }

    psFits* fits = psFitsOpen(tableFilename,"rw");

    if (fits == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsOpen returned NULL on existing file.");
        return 2;
    }

    psFitsMoveExtNum(fits,1,false);

    // change the data in the file, going past by one (implicit new row of data)
    for (int row = 0; row < tableNumRows+1; row++) {
        psMetadata* md = psMetadataAlloc();
        psMetadataAddF32(md,PS_LIST_TAIL,"MYFLT", 0,"",(float)row/-10.0);
        psMetadataAddF64(md,PS_LIST_TAIL,"MYDBL", 0,"",(double)row/-100.0);
        psMetadataAddS32(md,PS_LIST_TAIL,"MYINT", 0,"",-row);
        psMetadataAddBool(md,PS_LIST_TAIL,"MYBOOL", 0,"",((row & 1) == 1));
        psMetadataAddStr(md,PS_LIST_TAIL,"MYSTR", 0,"",strValue[row]);

        if (! psFitsUpdateTable(fits,md,row)) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsUpdateTable returned false, but expected true for row=%d.",
                    row);
            return 3;
        }
        psFree(md);
    }

    for (int row = 0; row < tableNumRows+1; row++) {
        psMetadata* md = psFitsReadTableRow(fits, row);
        if (abs(psMetadataLookupF32(NULL,md,"MYFLT") - (float)row/-10.0) > FLT_EPSILON) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsUpdateTable did not change the float value for row=%d.",
                    row);
            return 4;
        }
        if (abs(psMetadataLookupF64(NULL,md,"MYDBL") - (float)row/-100.0) > FLT_EPSILON) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsUpdateTable did not change the double value for row=%d.",
                    row);
            return 5;
        }
        if (psMetadataLookupS32(NULL,md,"MYINT") != -row) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsUpdateTable did not change the integer value for row=%d.",
                    row);
            return 6;
        }
        if (psMetadataLookupBool(NULL,md,"MYBOOL") != ((row &1) == 1)) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsUpdateTable did not change the boolean value for row=%d.",
                    row);
            return 7;
        }
        psString mystr = psMetadataLookupStr(NULL,md,"MYSTR");
        if (strncmp(mystr,strValue[row],
                    strlen(strValue[row])) != 0) {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsUpdateTable did not change the string value for row=%d.",
                    row);
            return 8;
        }
        psFree(md);
    }

    psMetadata* md = psMetadataAlloc();
    psMetadataAddF32(md,PS_LIST_TAIL,"BOGUS", 0,"",-1.0f);
    psWarning("Following should be a warning.");
    psErrorClear();
    if (! psFitsUpdateTable(fits,md,0)) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsUpdateTable did not return false with bogus column data");
        return 10;
    }
    psFree(md);

    md = psMetadataAlloc();
    psMetadataAddF32(md,PS_LIST_TAIL,"MYFLT", 0,"",-1.0f);
    psMetadataAddF64(md,PS_LIST_TAIL,"MYDBL", 0,"",-2.0);
    psMetadataAddS32(md,PS_LIST_TAIL,"MYINT", 0,"",-3);

    psWarning("Following should be an error.");
    psErrorClear();
    if (psFitsUpdateTable(NULL,md,0)) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsUpdateTable did not return false with NULL psFits");
        return 20;
    }
    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsUpdateTable did not error with NULL psFits");
        return 21;
    }
    psFree(err);

    psWarning("Following should be an error.");
    psErrorClear();
    if (psFitsUpdateTable(fits,NULL,0)) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsUpdateTable did not return false with NULL psMetadata");
        return 22;
    }
    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsUpdateTable did not error with NULL psMetadata");
        return 23;
    }
    psFree(err);

    psWarning("Following should be an error.");
    psErrorClear();
    if (psFitsUpdateTable(fits,md,-1)) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsUpdateTable did not return false with row=-1");
        return 24;
    }
    err = psErrorLast();
    if (err->code != PS_ERR_IO) {
        psError(PS_ERR_UNKNOWN, false,
                "psFitsUpdateTable did not error with row=-1");
        return 25;
    }

    psFree(err);
    psFree(md);
    psFitsClose(fits);

    return 0;
}

psS32 testImageRead(void)
{
    psS32 N = 256;
    psS32 M = 128;

    /*
        This function shall open the specified FITS file, read the specified data
        and place the data into a psImage structure. This function shall generate
        an error message and return NULL if any of the input parameters are out of
        range, image file doesn't exist or image is zero or one dimensional.

        Verify the returned psImage structure contains expected values, if the input
        parameter filename specifies an available FITS file with known 2dimensional
        data, input parameters col, row, ncol, nrow, z specify data range with the
        FITS file. Cases should include 1x1, Nx1, 1xN, NxN and MxN sub images and
        total FITS file image. (done in macro)

        Verify the returned psImage structure is equal to the input parameter
        'output', if specified. (done in macro)

        */

    /* generate FITS file to read */

    #define testReadTypeSize(m, n, readM0, readN0, readM, readN, TYP, filename) \
    { \
        psImage* img = NULL; \
        psImage* img2 = NULL; \
        psImage* img3 = NULL; \
        psImage* img4 = NULL; \
        /*        psImagimge* img_ref = NULL; */ \
        \
        GENIMAGE(img,m,n,TYP,row+2*col); \
        img2 = psImageCopy(img2,img,PS_TYPE_##TYP); \
        GENIMAGE(img3,m,n,TYP,row+2*col); \
        psImageClip(img3,32.0,32.0,120.0,120.0); \
        img4 = psImageCopy(img4,img3,PS_TYPE_##TYP); \
        psFits* fits = psFitsOpen(filename, "w"); \
        if (! psFitsWriteImage(fits, NULL, img, 2, NULL)) { \
            psError(PS_ERR_UNKNOWN, true,"Failed to write test image %s",filename); \
            return 1; \
        } \
        if (! psFitsUpdateImage(fits,img3, 0,0, 1)) { \
            psError(PS_ERR_UNKNOWN, true,"Failed to write test image %s",filename); \
            return 2; \
        } \
        if (! psFitsWriteImage(fits,NULL, img3, 2, NULL)) { \
            psError(PS_ERR_UNKNOWN, true,"Failed to write test image %s",filename); \
            return 3; \
        } \
        if (! psFitsUpdateImage(fits,img, 0, 0, 1)) { \
            psError(PS_ERR_UNKNOWN, true,"Failed to write test image %s",filename); \
            return 4; \
        } \
        psFree(img); \
        psFree(fits); \
        img = NULL; \
        psFree(img3); \
        img3 = NULL; \
        fits = psFitsOpen(filename,"r"); \
        psRegion reg = {readM0, readM, readN0, readN}; \
        img = psFitsReadImage(fits, reg, 0); \
        img3 = psFitsReadImage(fits, reg, 1); \
        if (img3 == NULL) { \
            psError(PS_ERR_UNKNOWN, true,"Failed to read test image %s",filename); \
            return 6; \
        } \
        for (psU32 row = readN0; row < readN; row++) { \
            ps##TYP* imgRow = img->data.TYP[row-readN0]; \
            ps##TYP* img2Row = img2->data.TYP[row]; \
            ps##TYP* img3Row = img3->data.TYP[row-readN0]; \
            ps##TYP* img4Row = img4->data.TYP[row]; \
            for (psU32 col = readM0; col < readM; col++) { \
                if (fabsf(imgRow[col-readM0]-img2Row[col]) > FLT_EPSILON) { \
                    psError(PS_ERR_UNKNOWN, true,"Image changed in I/O operation at %d,%d,0 (%.2f vs %.2f) for %s", \
                            col,row,(psF32)imgRow[col-readM0],(psF32)img2Row[col],filename); \
                    return 7; \
                } \
                if (fabsf(img3Row[col-readM0]-img4Row[col]) > FLT_EPSILON) { \
                    psError(PS_ERR_UNKNOWN, true,"Image changed in I/O operation at %d,%d,1 (%.2f vs %.2f) for %s", \
                            col,row,(psF32)img3Row[col-readM0],(psF32)img4Row[col],filename); \
                    return 8; \
                } \
            } \
        } \
        psFree(img); \
        img = NULL; \
        psFree(img3); \
        img3 = NULL; \
        psFitsMoveExtNum(fits,1, false); \
        img3 = psFitsReadImage(fits, reg, 0); \
        img = psFitsReadImage(fits, reg, 1); \
        if (img == NULL) { \
            psError(PS_ERR_UNKNOWN, true,"Failed to read test image %s",filename); \
            return 9; \
        } \
        for (psU32 row = readN0; row < readN; row++) { \
            ps##TYP* imgRow = img->data.TYP[row-readN0]; \
            ps##TYP* img2Row = img2->data.TYP[row]; \
            ps##TYP* img3Row = img3->data.TYP[row-readN0]; \
            ps##TYP* img4Row = img4->data.TYP[row]; \
            for (psU32 col = readM0; col < readM; col++) { \
                if (fabsf(imgRow[col-readM0]-img2Row[col]) > FLT_EPSILON) { \
                    psError(PS_ERR_UNKNOWN, true,"Image changed in I/O operation at %d,%d,0 (%.2f vs %.2f) for %s", \
                            col,row,(psF32)imgRow[col-readM0],(psF32)img2Row[col],filename); \
                    return 10; \
                } \
                if (fabsf(img3Row[col-readM0]-img4Row[col]) > FLT_EPSILON) { \
                    psError(PS_ERR_UNKNOWN, true,"Image changed in I/O operation at %d,%d,1 (%.2f vs %.2f) for %s", \
                            col,row,(psF32)img3Row[col-readM0],(psF32)img4Row[col],filename); \
                    return 11; \
                } \
            } \
        } \
        psFree(img); \
        psFree(img2); \
        psFree(img3); \
        psFree(img4); \
        psFree(fits); \
    }

    #define testReadType(TYP,filename) \
    testReadTypeSize(1,1,0,0,0,0,TYP,"tmpImages/1x1_" filename); \
    testReadTypeSize(M,1,M/4,0,M*3/4,0,TYP,"tmpImages/Mx1_" filename); \
    testReadTypeSize(1,N,0,N/4,0,N*3/4,TYP,"tmpImages/1xN_" filename); \
    testReadTypeSize(M,N,M/4,N/4,M*3/4,N*3/4,TYP,"tmpImages/MxN_" filename);

    mkdir("tmpImages",0777);

    testReadType(U8,"U8.fits");
    testReadType(S8,"S8.fits");   // Not a requirement
    testReadType(S16,"S16.fits");
    testReadType(U16,"U16.fits"); // Not a requirement
    testReadType(S32,"S32.fits");
    testReadType(U32,"U32.fits"); // Not a requirement
    testReadType(F32,"F32.fits");
    testReadType(F64,"F64.fits");

    // Attempt to read from NULL fits object
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message for NULL psFits");
    psRegion region = {
                          0,0,0,0
                      };
    if(psFitsReadImage(NULL,region,0) != NULL) {
        psError(PS_ERR_UNKNOWN,true,"Did not return NULL for NULL psFits");
        return 100;
    }

    return 0;
}

psS32 testImageWrite(void)
{
    psImage* img = NULL;
    psImage* img2 = NULL;
    psS32 m = 64;
    psS32 n = 96;

    /*
    This function shall write the specified section within a psImage structure
    to a FITS file. If the specifiedfile exists, then data should overwrite the
    section to write. If the specified file doesn't exist, it shall be created.
    If an extenstion is specified, then a basic primary header data unit shall
    be created.
    */

    /*
    Verify a FITS file named filename is generated and contains expected
    values, if the input parameter input contains known data values, input
    parameters col, row, ncol, nrow specify a valid data region within psImage
    structure.

    Verify a FITS file named filename is generated and contains a primary
    header data unit with extension with expected values, if the input
    parameter input contains known data values, input parameters col, row,
    ncol, nrow specify a valid data region within psImage structure and
    extname and/or extnum specify an extenstion to write.

    N.B. : these are done in testImageRead tests, see above.
    */

    /*
    Verify a FITS file named filename is overwritten and contains
    expected values, if the input parameter input contains known data values,
    input parameters col, row, ncol, nrow specify a valid data region within
    psImage structure.
    */

    GENIMAGE(img,m,n,F32,0);
    GENIMAGE(img2,m,n,F32,row+2*col);
    mkdir("tmpImages",0777);
    psFits* fits = psFitsOpen("tmpImages/writeTest.fits","w");

    if (! psFitsWriteImage(fits, NULL, img,1, NULL)) {
        psError(PS_ERR_UNKNOWN, true,"Couldn't write writeTest.fits.");
        return 14;
    }
    if (! psFitsUpdateImage(fits, img2, 0, 0, 0)) {
        psError(PS_ERR_UNKNOWN, true,"Couldn't update writeTest.fits.");
        return 15;
    }
    psFree(img);
    psFree(img2);

    // Did it really overwrite the pixel values?  Let's read it in and see.
    psFitsClose(fits);

    psRegion region = {0,0,0,0};
    fits = psFitsOpen("tmpImages/writeTest.fits","r");
    img = NULL;
    img = psFitsReadImage(fits, region, 0);
    if (img == NULL) {
        psError(PS_ERR_UNKNOWN, true,"Could not read in writeTest.fits.");
        return 16;
    }
    for (psU32 row=0;row<n;row++) {
        psF32* imgRow = img->data.F32[row];
        for (psU32 col=0;col<m;col++) {
            if (fabsf(imgRow[col] - (row+2*col)) > FLT_EPSILON) {
                psError(PS_ERR_UNKNOWN, true,"The image values were not overwritten at %d,%d (%.2f vs %.2f)",
                        col,row,imgRow[col],(row+2*col));
                return 17;
            }
        }
    }

    psFree(img);

    /*
    Verify false is returned and program execution is not stopped, if the input image
    is null.
    */
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message because input image is null.");
    if ( psFitsWriteImage(fits,NULL,NULL, 1, NULL) ) {
        psError(PS_ERR_UNKNOWN, true,"psImageWriteSection did not return false when input image is NULL.");
        return 20;
    }

    psFree(fits);

    return 0;
}

psS32 tst_psFitsWriteHeader(void)
{
    if (! makeMulti() ) {
        return 1;
    }

    psMetadata* header   = psMetadataAlloc();
    psFits*     fitsFile = psFitsOpen(multiFilename,"a+");

    // Test psFitsReadWrite generates files from psFitsWriteImage which calls psFitsWriteHeader
    // so these additional tests check for error conditions
    // Attempt call function with NULL metadata
    if(psFitsWriteHeader(fitsFile, NULL)) {
        psError(PS_ERR_UNKNOWN,true,"Did not expect return of true for NULL metadata pointer");
        return 2;
    }

    psFree(fitsFile);

    // Attempt to call function with NULL fits
    if(psFitsWriteHeader(NULL, header)) {
        psError(PS_ERR_UNKNOWN,true,"Did not expect return of true for NULL fits file pointer");
        return 3;
    }
    psFree(header);

    return 0;
}

