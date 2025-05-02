/** @file  tst_psFits.c
*
*  @brief Contains the tests for psFits.[ch]
*
*
*  @author Robert DeSonia, MHPCC
*
*  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
*  @date $Date: 2007-11-29 01:29:20 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#include <unistd.h>

#define GENIMAGE(img,c,r,TYP, valueFcn) \
img = psImageAlloc(c,r,PS_TYPE_##TYP); \
for (psU32 row=0;row<r;row++) { \
    ps##TYP* imgRow = img->data.TYP[row]; \
    for (psU32 col=0;col<c;col++) { \
        imgRow[col] = (ps##TYP)(valueFcn); \
    } \
}

//static bool makeMulti(void);  // implicitly tests psFitsSetExtName
//static bool makeTable(void);
const char* multiFilename = "multi.fits";
const char* tableFilename = "table.fits";
const int tableNumRows = 10;


// N.B., the tests to Image read/write was liberally taken from the now
// deprecated psImageReadSection/psImageWriteSection function tests.

bool makeMulti(void)
{
    psFits* fitsFile = psFitsOpen(multiFilename,"w");

    if (fitsFile == NULL) {
        diag("Could not create 'multi' FITS file");
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
        if (!psFitsWriteImage(fitsFile,header,image,0,extname)) {
            diag("Could not write image");
        }
        psFree(header);
    }
    psFree(image);
    psFree(fitsFile);

    return true;
}

bool makeTable(void)
{
// XXX: Should we be checking for memory leaks in this function?
//    psMemId id = psMemGetId();
    psFits* fitsFile = psFitsOpen(tableFilename, "w");
    if (fitsFile == NULL) {
        diag("Could not create 'table' FITS file");
        return false;
    }

    // make the PHU an image (per FITS standard, it must be)
    psImage* image = psImageAlloc(16,16,PS_TYPE_F32);

    if (!psFitsWriteImage(fitsFile,NULL,image,1,NULL)) {
        diag("Could not write PHU image");
        return false;
    }

    psFree(image);

    // build a table structure
    psArray* table = psArrayAlloc(tableNumRows);
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
        }
        psMetadataAdd(header,PS_LIST_TAIL, "MYVEC",
                      PS_DATA_VECTOR,
                      "psVector Item",
                      vec);
        psFree(vec);

        table->data[row] = header;
    }

    bool rc = psFitsWriteTable(fitsFile, NULL, table, NULL);

    psFree(table);
    psFree(fitsFile);
    return(rc);
}

psS32 main(psS32 argc, char* argv[])
{
    psLogSetLevel(PS_LOG_INFO);
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(509);

    // tst_psFitsOpen()
    {
        psMemId id = psMemGetId();
        ok(makeMulti(), "Created 'multi' FITS file");
        psFits* fitsFile = psFitsOpen(multiFilename,"r");
        ok(fitsFile != NULL, "psFitsOpen returned non-NULL on existing file");
        int extNum = psFitsGetExtNum(fitsFile);
        ok(extNum == 0, "psFitsOpen was queued to the PHU");
        psFitsClose(fitsFile);

        // make sure the file doesn't already exist.
        // XXX: What is F_OK?
        //        if (access("new.fits", F_OK) == 0) {
        //            if (remove
        //                    ("new.fits") != 0) {
        //                psError(PS_ERR_UNKNOWN, false,
        //                        "Couldn't delete the new.fits file");
        //                return 3;
        //            }
        //        }

        fitsFile = psFitsOpen("new.fits","w");
        ok(fitsFile != NULL, "psFitsOpen returned non-NULL on w mode");

        // write something to the file, otherwise CFITSIO will complain on close
        psImage* img = psImageAlloc(16,16,PS_TYPE_F32);
        psFitsWriteImage(fitsFile,NULL,img,1,NULL);
        // psFree should be equivalent to psFitsClose
        psFree(fitsFile);

        // now, if psFitsOpen actually created the file, I shouldn't error in removing it.
        ok(remove
           ("new.fits") == 0, "psFitsOpen seemed to have created a new file");

        fitsFile = psFitsOpen("new.fits","w+");
        ok(fitsFile != NULL, "psFitsOpen returned non-NULL on w+ mode");

        // write something to the file, otherwise CFITSIO will complain on close
        psFitsWriteImage(fitsFile,NULL,img,1,NULL);
        psFitsClose(fitsFile);

        // now, if psFitsOpen actually created the file, I shouldn't error in removing it.
        ok(remove
           ("new.fits") == 0, "psFitsOpen seemed to have created a new file");

        fitsFile = psFitsOpen("new.fits","a");
        ok(fitsFile != NULL, "psFitsOpen returned non-NULL on a mode");

        // write something to the file, otherwise CFITSIO will complain on close
        psFitsWriteImage(fitsFile,NULL,img,1,NULL);
        psFitsClose(fitsFile);
        fitsFile = psFitsOpen("new.fits","a+");
        ok(fitsFile != NULL, "psFitsOpen returned non-NULL on a+ mode");

        psFitsClose(fitsFile);
        // now, if psFitsOpen actually created the file, I shouldn't error in removing it.
        ok(remove
           ("new.fits") == 0, "psFitsOpen seemed to have created a new file");

        // Attempt to allocate with NULL filename
        // Following should generate an error message
        // XXX: Verify error
        fitsFile = psFitsOpen(NULL,"r");
        ok(fitsFile == NULL, "psFitsOpen returned NULL for NULL input");

        // Attempt to use an unallowed mode
        // Following should generate an error message
        // XXX: Verify error
        fitsFile = psFitsOpen("new.fits","b+");
        ok(fitsFile == NULL, "psFitsOpen returned NULL for NULL input");

        psFree(img);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // tst_psFitsMoveExtName()
    {
        psMemId id = psMemGetId();

        ok(makeMulti(), "Created 'multi' FITS file");
        if (! makeMulti() )
        {
            return 1;
        }

        psFits* fits = psFitsOpen(multiFilename,"r");

        if (fits == NULL)
        {
            psError(PS_ERR_UNKNOWN, false,
                    "psFitsOpen returned NULL on existing file");
            return 1;
        }

        int numHDUs = psFitsGetSize(fits);

        if (numHDUs < 2)
        {
            psError(PS_ERR_UNKNOWN,true,
                    "The 'multi' FITS file does not have multiple HDUs");
            return 2;
        }

        char extName[80];
        psRegion region = {0,0,0,0};

        for (int lcv = 0; lcv < numHDUs; lcv++)
        {
            snprintf(extName,80,"ext-%d",lcv);
            // try to move to the named extension.
            if (! psFitsMoveExtName(fits, extName) ) {
                psError(PS_ERR_UNKNOWN, false,
                        "Failed to move to ext-%d",
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
                        "The image pixel 0,0 of ext-%d was %g, expected %d",
                        lcv,image->data.F32[0][0],lcv);
                return 4;
            }
            psFree(image);
        }

        for (int lcv = numHDUs-1; lcv >= 0; lcv--)
        {
            snprintf(extName,80,"ext-%d",lcv);
            // try to move to the named extension.
            if (! psFitsMoveExtName(fits, extName) ) {
                psError(PS_ERR_UNKNOWN, false,
                        "Failed to move to ext-%d",
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
                        "The image pixel 0,0 of ext-%d was %g, expected %d",
                        lcv,image->data.F32[0][0],lcv);
                return 6;
            }
            psFree(image);
        }

        // check to see if given a bogus extension name, it errors.
        // Following should be an error
        // XXX: Verify error
        if (psFitsMoveExtName(fits, "bogus") || psErrorGetStackSize() != 1)
        {
            psError(PS_ERR_UNKNOWN, false,
                    "Moving to non-existant HDU didn't fail");
            return 7;
        }

        // check to see if given a NULL psFits, it errors.
        // Following should be an error
        // XXX: Verify error
        if (psFitsMoveExtName(NULL, "bogus") || psErrorGetStackSize() != 1)
        {
            psError(PS_ERR_UNKNOWN, false,
                    "Operation of NULL psFits didn't fail");
            return 8;
        }

        // check to see if given a NULL extname, it errors.
        // Following should be an error
        // XXX: Verify error
        if (psFitsMoveExtName(fits, NULL) || psErrorGetStackSize() != 1)
        {
            psError(PS_ERR_UNKNOWN, false,
                    "Operation of NULL extname didn't fail");
            return 9;
        }

        psFree(fits);

        // Attempt to get ext name from null fits file
        // Following should generate an error for NULL fits file
        if(psFitsGetExtName(NULL) != NULL)
        {
            psError(PS_ERR_UNKNOWN,true,"Expected NULL return from psFitsGetExtName with NULL fits file");
            return 10;
        }
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // tst_psFitsMoveExtNum()
    {
        psMemId id = psMemGetId();

        ok(makeMulti(), "Created 'multi' FITS file");
        psFits* fits = psFitsOpen(multiFilename,"r");
        ok(fits != NULL, "psFitsOpen returned non-NULL on existing file");

        int numHDUs = psFitsGetSize(fits);
        // as a side test, let's make sure psFitsGetSize can handle NULL.
        // Following should be an error
        // XXX: Verify error
        psErrorClear();
        ok(psFitsGetSize(NULL) == 0 && psErrorGetStackSize() == 1,
           "The 'multi' FITS file has multiple HDUs");

        ok(numHDUs == 8, "The 'multi' FITS file does not have multiple HDUs");

        psRegion region = {0,0,0,0};
        // test absolute positioning
        for (int lcv = 0; lcv < numHDUs; lcv++)
        {
            // try to move to the extension
            ok(psFitsMoveExtNum(fits, lcv, false), "Moved to extension %d", lcv);

            // check to see if I can retrieve the number back from the psFits object.
            ok(psFitsGetExtNum(fits) == lcv, "Retrieved the extension number back (%d vs %d)",
               psFitsGetExtNum(fits), lcv);

            // check that the image is associated to the extension moved, i.e.,
            // did we really move to the proper extension?
            psImage* image = NULL;
            image = psFitsReadImage(fits,region,0);
            ok(image != NULL && abs(image->data.F32[0][0] - (float)lcv) <= FLT_EPSILON,
               "The image pixel 0,0 of ext-%d was %g, expected %d",
               lcv,image->data.F32[0][0],lcv);
            psFree(image);
        }

        for (int lcv = numHDUs-1; lcv >= 0; lcv--)
        {
            // try to move to the extension
            ok(psFitsMoveExtNum(fits, lcv, false), "Moved to extension %d", lcv);

            // check that the image is associated to the extension moved, i.e.,
            // did we really move to the proper extension?
            psImage* image = NULL;
            image = psFitsReadImage(fits,region,0);

            ok(abs(image->data.F32[0][0] - (float)lcv) <= FLT_EPSILON,
               "The image pixel 0,0 of ext-%d was %g, expected %d",
               lcv, image->data.F32[0][0],lcv);
            psFree(image);
        }

        // test relative positioning
        psFitsMoveExtNum(fits,0,false);
        for (int lcv = 1; lcv < numHDUs; lcv++)
        {
            // try to move to the extension
            ok(psFitsMoveExtNum(fits, 1, true), "Failed to move to extension %d", lcv);

            // check to see if I can retrieve the number back from the psFits object.
            ok(psFitsGetExtNum(fits) == lcv,
               "Failed to retrieve the extension number back (%d vs %d)",
               psFitsGetExtNum(fits), lcv);

            // check that the image is associated to the extension moved, i.e.,
            // did we really move to the proper extension?
            psImage* image = NULL;
            image = psFitsReadImage(fits,region,0);

            ok(image != NULL && abs(image->data.F32[0][0] - (float)lcv) <= FLT_EPSILON,
               "The image pixel 0,0 of ext-%d was %g, expected %d",
               lcv,image->data.F32[0][0],lcv);
            psFree(image);
        }

        for (int lcv = numHDUs-2; lcv >= 0; lcv--)
        {
            // try to move to the extension
            ok(psFitsMoveExtNum(fits, -1, true), "Failed to move to extension %d", lcv);

            // check to see if I can retrieve the number back from the psFits object.
            ok(psFitsGetExtNum(fits) == lcv,
               "Failed to retrieve the extension number back (%d vs %d)",
               psFitsGetExtNum(fits), lcv);

            // check that the image is associated to the extension moved, i.e.,
            // did we really move to the proper extension?
            psImage* image = NULL;
            image = psFitsReadImage(fits,region,0);

            ok(abs(image->data.F32[0][0] - (float)lcv) <= FLT_EPSILON,
               "The image pixel 0,0 of ext-%d was %g, expected %d",
               lcv,image->data.F32[0][0],lcv);
            psFree(image);
        }


        // check to see if given a negative extension number, it errors.
        // Following should be an error
        // XXX: Verify error
        psErrorClear();
        ok(!psFitsMoveExtNum(fits, -1, false) && psErrorGetStackSize() == 1,
           "Moving to negative HDU did fail");

        // check to see if relative positioning beyond PHU, it errors.
        psFitsMoveExtNum(fits,0,false);
        // Following should be an error
        // XXX: Verify error
        psErrorClear();
        ok(!psFitsMoveExtNum(fits, -1, true) && psErrorGetStackSize() == 1,
           "Moving to negative HDU did fail");

        // check to see if given a extension greater than the total #HDUs, it errors.
        // Following should be an error
        // XXX: Verify error
        psErrorClear();
        ok(!psFitsMoveExtNum(fits, numHDUs, false) && psErrorGetStackSize() == 1,
           "Moving to a HDU beyond the file's contents did fail");

        // check to see if relative positioning beyond PHU, it errors.
        psFitsMoveExtNum(fits,numHDUs-1,false);
        // Following should be an error
        // XXX: Verify error
        psErrorClear();
        ok(!psFitsMoveExtNum(fits, 1, true) && psErrorGetStackSize() == 1,
           "Moving to negative HDU did fail");

        // check to see if given a NULL psFits, it errors.
        // Following should be an error
        // XXX: Verify error
        psErrorClear();
        ok(!psFitsMoveExtNum(NULL, 0, false) && psErrorGetStackSize() == 1,
           "Operation of NULL psFits didt fail");
        psFitsClose(fits);

        // Attempt to get ext name from null fits file
        // Following should generate an error for NULL fits file
        // XXX: Verify error
        ok(psFitsGetExtNum(NULL) == PS_FITS_TYPE_NONE,
           "Expected NULL return from psFitsGetExtNum with NULL fits file");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");

    }


    // tst_psFitsReadHeader()
    {
        psMemId id = psMemGetId();
        ok(makeMulti(), "Created 'multi' FITS file");

        psFits* fits = psFitsOpen(multiFilename,"r");
        ok(fits != NULL, "psFitsOpen returned non-NULL on existing file");

        int numHDUs = psFitsGetSize(fits);
        ok(numHDUs >= 8, "The 'multi' FITS file has multiple HDUs");

        char extname[80];
        for (int hdunum = 0; hdunum < numHDUs; hdunum++)
        {
            snprintf(extname,80,"ext-%d",hdunum);
            psFitsMoveExtNum(fits,hdunum,false);

            psMetadata* header = psFitsReadHeader(NULL,fits);
            ok(header != NULL, "Read header");

            psMetadata* header2 = psMetadataAlloc();
            header2 = psFitsReadHeader(header2,fits);
            ok(header2 != NULL, "Read header");

            ok(header->list->n >= 1 && header->list->n == header2->list->n,
               "Reading the header given a NULL input psMetadata did not differ from giving an existing psMetadata");

            // check for the extra metadata items
            psS32 intItem = psMetadataLookupS32(NULL,header, "MYINT");
            psF32 fltItem = psMetadataLookupF32(NULL,header, "MYFLT");
            psF64 dblItem = psMetadataLookupF64(NULL,header, "MYDBL");
            psMetadataItem* boolItem = psMetadataLookup(header, "MYBOOL");
            psString strItem = psMetadataLookupStr(NULL, header, "MYSTR");

            ok(intItem == hdunum, "Retrieved psS32 metadata item from file");

            ok(fabsf(fltItem - 1.0f/(float)(1+hdunum)) <= FLT_EPSILON,
               "Retrieved psF32 metadata item from file.  Got %f vs %f",
               fltItem,1.0f/(float)(1+hdunum));

            ok(abs(dblItem - 1.0/(double)(1+hdunum)) <= DBL_EPSILON,
               "Retrieved psF64 metadata item from file.  Got %g vs %g",
               dblItem, 1.0/(double)(1+hdunum));

            ok(boolItem != NULL && boolItem->type == PS_DATA_BOOL,
               "Retrieved psBool metadata item from file");

            ok(strItem != NULL && strncmp(strItem,extname,strlen(extname)) == 0,
               "Retrieved string metadata item from file.  Got '%s' vs '%s' (%d)",
               strItem,extname,strlen(extname));

            psFree(header);
            psFree(header2);
        }

        // Following should be an error (input psFits = NULL)
        // XXX: Verify error
        psMetadata* header = psFitsReadHeader(NULL,NULL);
        ok(header == NULL && psErrorGetStackSize() == 1,
           "psFitsReadHeader didn't error on a NULL psFits");

        psFree(fits);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // tst_psFitsReadHeaderSet()
    {
        psMemId id = psMemGetId();
        ok(makeMulti(), "Created 'multi' FITS file");

        psFits* fits = psFitsOpen(multiFilename,"r");
        ok(fits != NULL, "psFitsOpen returned non-NULL on existing file");

        int numHDUs = psFitsGetSize(fits);
        ok(numHDUs >= 8, "The 'multi' FITS file has multiple HDUs");

        // move to the middle
        psFitsMoveExtNum(fits,numHDUs/2, false);
        psMetadata* headerSet = psFitsReadHeaderSet(NULL,fits);
        ok(headerSet != NULL, "psFitsReadHeaderSet returned non-NULL");

        ok(psFitsGetExtNum(fits) == numHDUs/2, "psFitsReadHeaderSet did not change the CHU");

        char extname[80];
        for (int i = 0; i < numHDUs; i++)
        {
            if (i == 0) {
                snprintf(extname, 80, "PHU");
            } else {
                snprintf(extname, 80, "ext-%d", i);
            }

            psMetadata* header = psMetadataLookupPtr(NULL,headerSet, extname);

            ok(header != NULL, "psFitsReadHeader returned non-NULL for HDU#%d", i);

            psS32 intItem = psMetadataLookupS32(NULL, header, "MYINT");
            ok(intItem == i, "psFitsReadHeader for HDU#%d had a MYINT of %d, expected %d",
               intItem, i);
        }

        psMetadata* set3 = psFitsReadHeaderSet(NULL,fits);
        ok(set3 != NULL, "psFitsReadHeaderSet returned non-NULL");

        for (int i = 0; i < numHDUs; i++)
        {
            if (i == 0) {
                snprintf(extname, 80, "PHU");
            } else {
                snprintf(extname, 80, "ext-%d", i);
            }

            psMetadata* header = psMetadataLookupPtr(NULL, set3, extname);
            ok(header != NULL, "psFitsReadHeader returned non-NULL for HDU#%d", i);

            psS32 intItem = psMetadataLookupS32(NULL, header, "MYINT");
            ok(intItem == i, "psFitsReadHeader for HDU#%d had a MYINT of %d, expected %d",
               intItem, i);
        }

        set3 = psFitsReadHeaderSet(set3,NULL);
        ok(set3 == NULL, "psFitsReadHeaderSet returned NULL given a NULL psFits");


        psFree(headerSet);
        psFitsClose(fits);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // tst_psFitsReadTable()
    if (1) {
        psMemId id = psMemGetId();
        ok(makeTable(), "Created 'table' FITS file");
        psFits* fits = psFitsOpen(tableFilename, "r");
        ok(fits != NULL, "psFitsOpen returned non-NULL on existing file");

        psFitsMoveExtNum(fits,1,false);
        psArray* table = psFitsReadTable(fits);
        ok(table != NULL, "psFitsReadTable returned non-NULL");
        ok(table->n == tableNumRows, "Expected %d rows, read %d",
           tableNumRows, table->n);

        for (int row = 0; row < table->n; row++) {
            psMetadata* rowData = table->data[row];

            psS32 intItem = psMetadataLookupS32(NULL, rowData, "MYINT");
            psF32 fltItem = psMetadataLookupF32(NULL, rowData, "MYFLT");
            psF64 dblItem = psMetadataLookupF64(NULL, rowData, "MYDBL");
            psBool boolItem = psMetadataLookupBool(NULL, rowData, "MYBOOL");
            psString strItem = psMetadataLookupStr(NULL, rowData, "MYSTR");
            psVector* vecItem = psMetadataLookupPtr(NULL, rowData, "MYVEC");

            ok(intItem == row,
               "Failed to retrieve psS32 metadata item from file (row=%d).  Got %d vs %d",
               row, intItem, row);

            ok(fabsf(fltItem - 1.0f/(float)(1+row)) <= FLT_EPSILON,
               "Retrieved psF32 metadata item from file (row=%d).  Got %f vs %f",
               row, fltItem,1.0f/(float)(1+row));

            ok(abs(dblItem - 1.0/(double)(1+row)) <= DBL_EPSILON,
               "Retrieved psF64 metadata item from file (row=%d).  Got %g vs %g",
               row, dblItem, 1.0/(double)(1+row));

            ok(!(boolItem != ((row&0x01) == 0)),
               "Retrieved psBool metadata item from file (row=%d). Got %d vs %d",
               row, boolItem, ((row&0x01) == 0));

            char strValue[16];
            snprintf(strValue,16,"row=%d",row+1);
            ok(strncmp(strItem,strValue,strlen(strValue)) == 0,
               "Retrieved psString metadata item from file (row=%d). Got '%s' vs '%s'",
               row, strItem, strValue);

            ok(vecItem != NULL, "Retrieved psVector metadata item from file (row=%d)", row);

            ok(vecItem->type.type == PS_DATA_S32 &&
               vecItem->data.S32[0] == row &&
               vecItem->data.S32[3] == row+30,
               "Retrieved psVector (row=%d) had expected values [%d,%d,%d,%d] vs [%d,%d,%d,%d]",
               row,vecItem->data.S32[0], vecItem->data.S32[1],
               vecItem->data.S32[2], vecItem->data.S32[3],
               row,row+10,row+20,row+30);
        }

        psFree(table);
        psFree(fits);

        psArray* nullTest = psFitsReadTable(NULL);
        ok(nullTest == NULL && psErrorGetStackSize() == 1,
           "psFitsReadTable returned NULL when given NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // tst_psFitsReadTableColumnNum()
    if (1) {
        psMemId id = psMemGetId();
        ok(makeTable(), "Created 'table' FITS file");
        psFits* fits = psFitsOpen(tableFilename,"r");
        ok(fits != NULL, "psFitsOpen returned non-NULL on existing file");

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
            ok(colData != NULL, "psFitsReadTableColumnNum returned non-NULL for col=%d");

            if (colData->type.type != type[col] &&
                    colData->type.type != altType[col]) {
                char* typeRead;
                char* typeExpected;
                PS_TYPE_NAME(typeRead, colData->type.type);
                PS_TYPE_NAME(typeExpected, type[col]);

                ok(false, "psFitsReadTableColumnNum returned different type, %s vs %s, for col=%d",
                   typeRead, typeExpected, col);
            } else {
                ok(true, "psFitsReadTableColumnNum returned same type");
            }
            ok(colData->n == tableNumRows,
               "psFitsReadTableColumnNum returned same number of rows, %d vs %d, for col=%d",
               colData->n, tableNumRows, col);
            for (int row = 0; row < tableNumRows; row++) {
                ok(abs(p_psVectorGetElementF64(colData,row) - expectedValues[col][row]) <= FLT_EPSILON,
                   "psFitsReadTableColumnNum returned values (%g vs %g) for col=%d",
                   p_psVectorGetElementF64(colData,row), expectedValues[col][row], col);
            }
            psFree(colData);
        }

        // Following should be an error
        // XXX: Verify error
        psErrorClear();
        psVector* data = psFitsReadTableColumnNum(NULL,colname[0]);
        psErr* err = psErrorLast();
        ok(data == NULL, "psFitsReadTableColumnNum did return NULL with NULL psFits");
        ok(err->code == PS_ERR_BAD_PARAMETER_NULL,
           "psFitsReadTableColumnNum did return error with NULL psFits");
        psFree(err);

        // Following should be an error
        // XXX: Verify error
        psErrorClear();
        data = psFitsReadTableColumnNum(fits,"BOGUS");
        err = psErrorLast();
        ok(data == NULL, "psFitsReadTableColumnNum did return NULL with bogus column name");
        ok(err->code == PS_ERR_IO, "psFitsReadTableColumnNum did return error with bogus column name");
        psFree(err);
        psFree(fits);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // tst_psFitsReadTableColumn()
    if (1) {
        psMemId id = psMemGetId();
        ok(makeTable(), "Created 'table' FITS file");
        psFits* fits = psFitsOpen(tableFilename,"r");
        ok(fits != NULL, "psFitsOpen returned non-NULL on existing file");

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

        bool errorFlag = false;
        for (int col = 0; col < 4; col++) {
            colData = psFitsReadTableColumn(fits,colname[col]);
            if (colData == NULL) {
                diag("psFitsReadTableColumn returned NULL for col=%d", col);
                errorFlag = true;
            }
            if (colData->n != tableNumRows) {
                diag("psFitsReadTableColumn returned different number of rows, %d vs %d, for col=%d",
                     colData->n, tableNumRows, col);
                errorFlag = true;
            }
            if (col < 3) {
                for (int row = 0; row < tableNumRows; row++) {
                    if (abs(atof((char*)colData->data[row]) - expectedValues[col][row]) > 0.0001) {
                        diag("psFitsReadTableColumn returned unexpected values (%g vs %g) for col=%d",
                             atof((char*)colData->data[row]), expectedValues[col][row], col);
                        errorFlag = true;
                    }
                }
            } else if (col == 3) {
                for (int row = 0; row < tableNumRows; row++) {
                    if (strncmp(colData->data[row],expectedBoolValues[row],
                                strlen(expectedBoolValues[row])) != 0) {
                        diag("psFitsReadTableColumn returned unexpected values ('%s' vs '%s') for col=%d",
                             (char*)colData->data[row], expectedBoolValues[row], col);
                        errorFlag = true;
                    }
                }
            } else if (col == 4) {
                for (int row = 0; row < tableNumRows; row++) {
                    if (strncmp(colData->data[row],expectedStrValues[row],
                                strlen(expectedStrValues[row])) != 0) {
                        diag("psFitsReadTableColumn returned unexpected values ('%s' vs '%s') for col=%d",
                             (char*)colData->data[row], expectedStrValues[row], col);
                        errorFlag = true;
                    }
                }
            }
            psFree(colData);
        }
        ok(!errorFlag, "psFitsReadTableColumn() produced the correct data");

        // Following should be an error
        // XXX: Verify error
        psErrorClear();
        psArray* data = psFitsReadTableColumn(NULL,"MYINT");
        psErr* err = psErrorLast();
        ok(data == NULL, "psFitsReadTableColumn did return NULL with NULL psFits");
        ok(err->code == PS_ERR_BAD_PARAMETER_NULL,
           "psFitsReadTableColumn did error with NULL psFits");
        psFree(err);

        // Following should be an error
        // XXX: Verify error
        psErrorClear();
        data = psFitsReadTableColumn(fits,"BOGUS");
        err = psErrorLast();
        ok(data == NULL, "psFitsReadTableColumn did return NULL with col=\"BOGUS\"");
        ok(err->code == PS_ERR_IO, "psFitsReadTableColumn did error with col=\"BOGUS\"");
        psFree(err);

        psFree(fits);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // tst_psFitsUpdateTable()
    if (1) {
        psMemId id = psMemGetId();
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
        ok(makeTable(), "Created 'table' FITS file");
        psFits* fits = psFitsOpen(tableFilename,"rw");
        ok(fits != NULL, "psFitsOpen returned NULL on existing file");
        psFitsMoveExtNum(fits,1,false);
        // change the data in the file, going past by one (implicit new row of data)
        bool errorFlag = false;
        for (int row = 0; row < tableNumRows+1; row++) {
            psMetadata* md = psMetadataAlloc();
            psMetadataAddF32(md,PS_LIST_TAIL,"MYFLT", 0,"",(float)row/-10.0);
            psMetadataAddF64(md,PS_LIST_TAIL,"MYDBL", 0,"",(double)row/-100.0);
            psMetadataAddS32(md,PS_LIST_TAIL,"MYINT", 0,"",-row);
            psMetadataAddBool(md,PS_LIST_TAIL,"MYBOOL", 0,"",((row & 1) == 1));
            psMetadataAddStr(md,PS_LIST_TAIL,"MYSTR", 0,"",strValue[row]);

            if (!psFitsUpdateTable(fits,md,row)) {
                diag("psFitsUpdateTable returned false, but expected true for row=%d", row);
                errorFlag = true;
            }
            psFree(md);
        }
        ok(!errorFlag, "psFitsUpdateTable() produced the correct data");

        for (int row = 0; row < tableNumRows+1; row++) {
            psMetadata* md = psFitsReadTableRow(fits, row);
            if (abs(psMetadataLookupF32(NULL,md,"MYFLT") - (float)row/-10.0) > FLT_EPSILON) {
                diag("psFitsUpdateTable did not change the float value for row=%d", row);
                errorFlag = true;
            }
            if (abs(psMetadataLookupF64(NULL,md,"MYDBL") - (float)row/-100.0) > FLT_EPSILON) {
                diag("psFitsUpdateTable did not change the double value for row=%d", row);
                errorFlag = true;
            }
            if (psMetadataLookupS32(NULL,md,"MYINT") != -row) {
                diag("psFitsUpdateTable did not change the integer value for row=%d", row);
                errorFlag = true;
            }
            if (psMetadataLookupBool(NULL,md,"MYBOOL") != ((row &1) == 1)) {
                diag("psFitsUpdateTable did not change the boolean value for row=%d", row);
                errorFlag = true;
            }
            psString mystr = psMetadataLookupStr(NULL,md,"MYSTR");
            if (strncmp(mystr,strValue[row], strlen(strValue[row])) != 0) {
                diag("psFitsUpdateTable did not change the string value for row=%d", row);
                errorFlag = true;
            }
            psFree(md);
        }
        ok(!errorFlag, "psFitsUpdateTable() produced the correct data");

        psMetadata* md = psMetadataAlloc();
        psMetadataAddF32(md,PS_LIST_TAIL,"BOGUS", 0,"",-1.0f);
        // Following should be a warning
        // XXX: Verify warning
        psErrorClear();
        ok(psFitsUpdateTable(fits,md,0), "psFitsUpdateTable did return false with bogus column data");
        psFree(md);

        md = psMetadataAlloc();
        psMetadataAddF32(md,PS_LIST_TAIL,"MYFLT", 0,"",-1.0f);
        psMetadataAddF64(md,PS_LIST_TAIL,"MYDBL", 0,"",-2.0);
        psMetadataAddS32(md,PS_LIST_TAIL,"MYINT", 0,"",-3);

        // Following should be an error
        // XXX: Verify error
        psErrorClear();
        ok(psFitsUpdateTable(NULL,md,0), "psFitsUpdateTable did return false with NULL psFits");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_NULL, "psFitsUpdateTable did error with NULL psFits");
        psFree(err);

        // Following should be an error
        // XXX: Verify error
        psErrorClear();
        ok(psFitsUpdateTable(fits,NULL,0), "psFitsUpdateTable did return false with NULL psMetadata");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_NULL, "psFitsUpdateTable did error with NULL psMetadata");
        psFree(err);

        // Following should be an error
        // XXX: Verify error
        psErrorClear();
        ok(psFitsUpdateTable(fits,md,-1), "psFitsUpdateTable did return false with row=-1");
        err = psErrorLast();
        ok(err->code == PS_ERR_IO, "psFitsUpdateTable did error with row=-1");

        psFree(err);
        psFree(md);
        psFitsClose(fits);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testImageRead()
    {
        psS32 N = 256;
        psS32 M = 128;

        // This function shall open the specified FITS file, read the specified data
        // and place the data into a psImage structure. This function shall generate
        // an error message and return NULL if any of the input parameters are out of
        // range, image file doesn't exist or image is zero or one dimensional.
        // Verify the returned psImage structure contains expected values, if the input
        // parameter filename specifies an available FITS file with known 2dimensional
        // data, input parameters col, row, ncol, nrow, z specify data range with the
        // FITS file. Cases should include 1x1, Nx1, 1xN, NxN and MxN sub images and
        // total FITS file image. (done in macro)
        // Verify the returned psImage structure is equal to the input parameter
        // 'output', if specified. (done in macro)

        #define testReadTypeSize(m, n, readM0, readN0, readM, readN, TYP, filename) \
        { \
            psMemId id = psMemGetId(); \
            psImage* img = NULL; \
            psImage* img2 = NULL; \
            psImage* img3 = NULL; \
            psImage* img4 = NULL; \
            \
            GENIMAGE(img,m,n,TYP,row+2*col); \
            img2 = psImageCopy(img2,img,PS_TYPE_##TYP); \
            GENIMAGE(img3,m,n,TYP,row+2*col); \
            psImageClip(img3,32.0,32.0,120.0,120.0); \
            img4 = psImageCopy(img4,img3,PS_TYPE_##TYP); \
            psFits* fits = psFitsOpen(filename, "w"); \
            ok(psFitsWriteImage(fits, NULL, img, 2, NULL), "Wrote test image %s",filename); \
            ok(psFitsUpdateImage(fits,img3, 0,0, 1), "Wrote test image %s",filename); \
            ok(psFitsWriteImage(fits,NULL, img3, 2, NULL), "Wrote test image %s",filename); \
            ok(psFitsUpdateImage(fits,img, 0, 0, 1), "Wrote test image %s",filename); \
            psFree(img); \
            psFree(fits); \
            img = NULL; \
            psFree(img3); \
            img3 = NULL; \
            fits = psFitsOpen(filename,"r"); \
            psRegion reg = {readM0, readM, readN0, readN}; \
            img = psFitsReadImage(fits, reg, 0); \
            img3 = psFitsReadImage(fits, reg, 1); \
            ok(img3 != NULL, "Read test image %s",filename); \
            bool errorFlag = false; \
            for (psU32 row = readN0; row < readN; row++) { \
                ps##TYP* imgRow = img->data.TYP[row-readN0]; \
                ps##TYP* img2Row = img2->data.TYP[row]; \
                ps##TYP* img3Row = img3->data.TYP[row-readN0]; \
                ps##TYP* img4Row = img4->data.TYP[row]; \
                for (psU32 col = readM0; col < readM; col++) { \
                    if (fabsf(imgRow[col-readM0]-img2Row[col]) > FLT_EPSILON) { \
                        diag("Image changed in I/O operation at %d,%d,0 (%.2f vs %.2f) for %s", \
                             col,row,(psF32)imgRow[col-readM0],(psF32)img2Row[col],filename); \
                        errorFlag = true; \
                    } \
                    if (fabsf(img3Row[col-readM0]-img4Row[col]) > FLT_EPSILON) { \
                        diag("Image changed in I/O operation at %d,%d,1 (%.2f vs %.2f) for %s", \
                             col,row,(psF32)img3Row[col-readM0],(psF32)img4Row[col],filename); \
                        errorFlag = true; \
                    } \
                } \
            } \
            ok(!errorFlag, "psFitsReadImage() produced the correct data"); \
            psFree(img); \
            img = NULL; \
            psFree(img3); \
            img3 = NULL; \
            psFitsMoveExtNum(fits,1, false); \
            img3 = psFitsReadImage(fits, reg, 0); \
            img = psFitsReadImage(fits, reg, 1); \
            ok(img != NULL, "Read test image %s",filename); \
            errorFlag = false; \
            for (psU32 row = readN0; row < readN; row++) { \
                ps##TYP* imgRow = img->data.TYP[row-readN0]; \
                ps##TYP* img2Row = img2->data.TYP[row]; \
                ps##TYP* img3Row = img3->data.TYP[row-readN0]; \
                ps##TYP* img4Row = img4->data.TYP[row]; \
                for (psU32 col = readM0; col < readM; col++) { \
                    if (fabsf(imgRow[col-readM0]-img2Row[col]) > FLT_EPSILON) { \
                        diag("Image changed in I/O operation at %d,%d,0 (%.2f vs %.2f) for %s", \
                             col,row,(psF32)imgRow[col-readM0],(psF32)img2Row[col],filename); \
                        errorFlag = true; \
                    } \
                    if (fabsf(img3Row[col-readM0]-img4Row[col]) > FLT_EPSILON) { \
                        diag("Image changed in I/O operation at %d,%d,1 (%.2f vs %.2f) for %s", \
                             col,row,(psF32)img3Row[col-readM0],(psF32)img4Row[col],filename); \
                        errorFlag = true; \
                    } \
                } \
            } \
            ok(!errorFlag, "psFitsReadImage() produced the correct data"); \
            psFree(img); \
            psFree(img2); \
            psFree(img3); \
            psFree(img4); \
            psFree(fits); \
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks"); \
        }

        #define testReadType(TYP,filename) \
        testReadTypeSize(1,1,0,0,0,0,TYP,"tmpImages/1x1_" filename); \
        testReadTypeSize(M,1,M/4,0,M*3/4,0,TYP,"tmpImages/Mx1_" filename); \
        testReadTypeSize(1,N,0,N/4,0,N*3/4,TYP,"tmpImages/1xN_" filename); \
        testReadTypeSize(M,N,M/4,N/4,M*3/4,N*3/4,TYP,"tmpImages/MxN_" filename);

        system("mkdir tmpImages");

        testReadType(U8,"U8.fits");
        testReadType(S8,"S8.fits");   // Not a requirement
        testReadType(S16,"S16.fits");
        testReadType(U16,"U16.fits"); // Not a requirement
        testReadType(S32,"S32.fits");
        testReadType(U32,"U32.fits"); // Not a requirement
        testReadType(F32,"F32.fits");
        testReadType(F64,"F64.fits");

        // Attempt to read from NULL fits object
        // Following should generate error message for NULL psFits
        // XXX: Verify error
        // XXXX: This is seg-faulting
        if (0)
        {
            psMemId id = psMemGetId();
            psRegion region = {
                                  0,0,0,0
                              };
            ok(psFitsReadImage(NULL,region,0) == NULL, "Did return NULL for NULL psFits");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
            \
        }
    }


    // testImageWrite()
    {
        psMemId id = psMemGetId();
        psImage* img = NULL;
        psImage* img2 = NULL;
        psS32 m = 64;
        psS32 n = 96;

        // This function shall write the specified section within a psImage structure
        // to a FITS file. If the specifiedfile exists, then data should overwrite the
        // section to write. If the specified file doesn't exist, it shall be created.
        // If an extenstion is specified, then a basic primary header data unit shall
        //  be created.
        //
        // Verify a FITS file named filename is generated and contains expected
        // values, if the input parameter input contains known data values, input
        // parameters col, row, ncol, nrow specify a valid data region within psImage
        // structure.
        //
        // Verify a FITS file named filename is generated and contains a primary
        // header data unit with extension with expected values, if the input
        // parameter input contains known data values, input parameters col, row,
        // ncol, nrow specify a valid data region within psImage structure and
        // extname and/or extnum specify an extenstion to write.
        //
        // N.B. : these are done in testImageRead tests, see above.
        //
        // Verify a FITS file named filename is overwritten and contains
        // expected values, if the input parameter input contains known data values,
        // input parameters col, row, ncol, nrow specify a valid data region within
        // psImage structure.

        GENIMAGE(img,m,n,F32,0);
        GENIMAGE(img2,m,n,F32,row+2*col);
        system("mkdir tmpImages");
        psFits* fits = psFitsOpen("tmpImages/writeTest.fits","w");

        ok(psFitsWriteImage(fits, NULL, img,1, NULL), "Wrote writeTest.fits");
        ok(psFitsUpdateImage(fits, img2, 0, 0, 0), "Updated writeTest.fits");
        psFree(img);
        psFree(img2);

        // Did it really overwrite the pixel values?  Let's read it in and see.
        psFitsClose(fits);

        psRegion region = {0,0,0,0};
        fits = psFitsOpen("tmpImages/writeTest.fits","r");
        img = NULL;
        img = psFitsReadImage(fits, region, 0);
        ok(img != NULL, "Read in writeTest.fits");
        bool errorFlag = false;
        for (psU32 row=0;row<n;row++)
        {
            psF32* imgRow = img->data.F32[row];
            for (psU32 col=0;col<m;col++) {
                if (fabsf(imgRow[col] - (row+2*col)) > FLT_EPSILON) {
                    diag("The image values were not overwritten at %d,%d (%.2f vs %.2f)",
                         col,row,imgRow[col],(row+2*col));
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "psFitsReadImage() produced the correct data");

        psFree(img);

        // Verify false is returned and program execution is not stopped, if the input image
        // is null.
        // Following should generate an error message because input image is null
        // XXX: Verify error
        ok(!psFitsWriteImage(fits,NULL,NULL, 1, NULL),
           "psImageWriteSection did return false when input image is NULL");
        psFree(fits);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // tst_psFitsWriteHeader()
    {
        psMemId id = psMemGetId();
        ok(makeMulti(), "Created 'multi' FITS file");

        psMetadata* header   = psMetadataAlloc();
        psFits*     fitsFile = psFitsOpen(multiFilename,"a+");

        // Test psFitsReadWrite generates files from psFitsWriteImage which calls psFitsWriteHeader
        // so these additional tests check for error conditions
        // Attempt call function with NULL metadata
        ok(!psFitsWriteHeader(fitsFile, NULL), "Expected return of true for NULL metadata pointer");
        psFree(fitsFile);

        // Attempt to call function with NULL fits
        ok(!psFitsWriteHeader(NULL, header), "Expected return of true for NULL fits file pointer");
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
