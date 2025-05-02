#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
//#include <unistd.h>

#define GENIMAGE(img,c,r,TYP, valueFcn) \
img = psImageAlloc(c,r,PS_TYPE_##TYP); \
for (psU32 row=0;row<r;row++) { \
    ps##TYP* imgRow = img->data.TYP[row]; \
    for (psU32 col=0;col<c;col++) { \
        imgRow[col] = (ps##TYP)(valueFcn); \
    } \
}
const char* fitsFilename = "tmp.fits";
const char* fitsFilename2 = "tmp2.fits";

bool createFitsFile(void)
{
    psFits* fitsFile = psFitsOpen(fitsFilename, "w");

    if (fitsFile == NULL) {
        diag("Could not create 'multi' FITS file");
        return false;
    }

    psImage* image = psImageAlloc(16, 16, PS_TYPE_F32);

    char extname[80];
    for (int lcv = 0; lcv < 8; lcv++) {
        snprintf(extname, 80, "ext-%d", lcv);

        psMetadata* header = psMetadataAlloc();

        psMetadataAdd(header, PS_LIST_TAIL, "MYINT",
                      PS_DATA_S32,
                      "psS32 Item", (psS32)lcv);

        psMetadataAdd(header, PS_LIST_TAIL, "MYFLT",
                      PS_DATA_F32,
                      "psF32 Item", (float)(1.0f/(float)(1+lcv)));

        psMetadataAdd(header, PS_LIST_TAIL, "MYDBL",
                      PS_DATA_F64,
                      "psF64 Item", (double)(1.0/(double)(1+lcv)));

        psMetadataAdd(header, PS_LIST_TAIL, "MYBOOL",
                      PS_DATA_BOOL,
                      "psBool Item",
                      (lcv%2 == 0));

        psMetadataAdd(header, PS_LIST_TAIL, "MYSTR",
                      PS_DATA_STRING,
                      "String Item",
                      extname);

        // set the pixels in the image
        psImageInit(image, (float)lcv);
        if (!psFitsWriteImage(fitsFile, header, image, 0, extname)) {
            diag("Could not write image");
        }
        psFree(header);
    }
    psFree(image);
    psFree(fitsFile);

    return true;
}


bool createFitsFile2(void)
{
    psFits* fitsFile = psFitsOpen(fitsFilename2, "w");

    if (fitsFile == NULL) {
        diag("Could not create 'multi' FITS file");
        return false;
    }

    psImage* image = psImageAlloc(16, 16, PS_TYPE_F32);

    char extname[80];
    for (int lcv = 0; lcv < 8; lcv++) {
        snprintf(extname, 80, "ext-%d", lcv);
        pmHDU *hdu = pmHDUAlloc(extname);
        hdu->header = psMetadataAlloc();

        psMetadataAdd(hdu->header, PS_LIST_TAIL, "MYINT",
                      PS_DATA_S32,
                      "psS32 Item", (psS32)lcv);

        psMetadataAdd(hdu->header, PS_LIST_TAIL, "MYFLT",
                      PS_DATA_F32,
                      "psF32 Item", (float)(1.0f/(float)(1+lcv)));

        psMetadataAdd(hdu->header, PS_LIST_TAIL, "MYDBL",
                      PS_DATA_F64,
                      "psF64 Item", (double)(1.0/(double)(1+lcv)));

        psMetadataAdd(hdu->header, PS_LIST_TAIL, "MYBOOL",
                      PS_DATA_BOOL,
                      "psBool Item",
                      (lcv%2 == 0));

        psMetadataAdd(hdu->header, PS_LIST_TAIL, "MYSTR",
                      PS_DATA_STRING,
                      "String Item",
                      extname);

        // set the pixels in the image
        psImageInit(image, (float)lcv);
        if(!pmHDUWrite(hdu, fitsFile)) {
            diag("Could not write header");
            psErrorStackPrint(stdout, "THIS");

exit(0);
        }
        if (!psFitsWriteImage(fitsFile, NULL, image, 0, extname)) {
            diag("Could not write image");
        }

        psFree(hdu);
    }
    psFree(image);
    psFree(fitsFile);

    return true;
}




psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(18);
      psError(PS_ERR_IO, true, "HEY: ERROR");

    psHistogram *myHist = psHistogramAlloc(10, 5, 1);
    psFree(myHist);

    // Test pmHDUAlloc()
    // Use a NULL extname
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUAlloc(NULL);
        ok(hdu, "pmHDUAlloc(NULL) returned non-NULL");
        skip_start(!hdu, 7, "Skipping tests because pmHDUAlloc(NULL) returned NULL");
        ok(hdu->blankPHU == true, "pmHDUAlloc(NULL) set hdu->blankPHU correctly");
        ok(hdu->extname == NULL, "pmHDUAlloc(NULL) set hdu->extname correctly");
        ok(hdu->format == NULL, "pmHDUAlloc(NULL) set hdu->format correctly");
        ok(hdu->header == NULL, "pmHDUAlloc(NULL) set hdu->header correctly");
        ok(hdu->images == NULL, "pmHDUAlloc(NULL) set hdu->images correctly");
        ok(hdu->weights == NULL, "pmHDUAlloc(NULL) set hdu->weights correctly");
        ok(hdu->masks == NULL, "pmHDUAlloc(NULL) set hdu->masks correctly");
        psFree(hdu);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Use a non-NULL extname
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUAlloc("ext-0");
        ok(hdu, "pmHDUAlloc(extname) returned non-NULL");
        skip_start(!hdu, 7, "Skipping tests because pmHDUAlloc(extname) returned NULL");
        ok(hdu->blankPHU == false, "pmHDUAlloc(extname) set hdu->blankPHU correctly");
        ok(0 == strcmp(hdu->extname, "ext-0"), "pmHDUAlloc(extname) set hdu->extname correctly");
        ok(hdu->format == NULL, "pmHDUAlloc(extname) set hdu->format correctly");
        ok(hdu->header == NULL, "pmHDUAlloc(extname) set hdu->header correctly");
        ok(hdu->images == NULL, "pmHDUAlloc(extname) set hdu->images correctly");
        ok(hdu->weights == NULL, "pmHDUAlloc(extname) set hdu->weights correctly");
        ok(hdu->masks == NULL, "pmHDUAlloc(extname) set hdu->masks correctly");
        psFree(hdu);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // tst_psFitsReadHeader()
    {
        ok(createFitsFile2(), "Created test FITS file");
        psFits* fits = psFitsOpen(fitsFilename2,"r");
        ok(fits != NULL, "psFitsOpen returned non-NULL on existing file");
        int numHDUs = psFitsGetSize(fits);
        ok(numHDUs == 8, "The test FITS file has %d HDUs", numHDUs);

        for (int hdunum = 0; hdunum < numHDUs; hdunum++)
        {
            psMemId id = psMemGetId();
            char extname[80];
            snprintf(extname,80,"ext-%d",hdunum);
            psFitsMoveExtNum(fits, hdunum, false);
            pmHDU *hdu = pmHDUAlloc(extname);
            bool rc = pmHDUReadHeader(hdu, fits);
            ok(rc == true, "pmHDUReadHeader() returned TRUE");
            ok(hdu->header != NULL &&
               psMemCheckMetadata(hdu->header), "pmHDUReadHeader() correctly returned the hdu->header member");

            // check for the extra metadata items
            psS32 intItem = psMetadataLookupS32(NULL, hdu->header, "MYINT");
            psF32 fltItem = psMetadataLookupF32(NULL, hdu->header, "MYFLT");
            psF64 dblItem = psMetadataLookupF64(NULL, hdu->header, "MYDBL");
            psMetadataItem* boolItem = psMetadataLookup(hdu->header, "MYBOOL");
            psString strItem = psMetadataLookupStr(NULL, hdu->header, "MYSTR");

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
            psFree(hdu);
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks (header %d)", hdunum);
        }


        // Call pmHDUReadHeader() with NULL pmHDU input.  Should returne false.
        {
            psMemId id = psMemGetId();
//            pmHDU *hdu = pmHDUAlloc(NULL);
            bool rc = pmHDUReadHeader(NULL, fits);
            ok(rc == false, "pmHDUReadHeader() returned FALSE with NULL hdu input");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

        // Call pmHDUReadHeader() with NULL pmFits input.  Should returne false.
        {
            psMemId id = psMemGetId();
            pmHDU *hdu = pmHDUAlloc(NULL);
            bool rc = pmHDUReadHeader(hdu, NULL);
            ok(rc == false, "pmHDUReadHeader() returned FALSE with NULL pmFits input");
            psFree(hdu);
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

        psFree(fits);
    }


/* HERE
    // tst_psFitsWriteHeader()
    {
        psMemId id = psMemGetId();
        ok(createFitsFile2(), "Created 'multi' FITS file");

        psMetadata* header   = psMetadataAlloc();
        psFits*     fitsFile = psFitsOpen(fitsFilename,"a+");

        // Test psFitsReadWrite generates files from psFitsWriteImage which 
        // calls psFitsWriteHeader so these additional tests check for error conditions
        // Attempt call function with NULL metadata
        ok(!psFitsWriteHeader(fitsFile, NULL), "Expected return of true for NULL metadata pointer");
        psFree(fitsFile);

        // Attempt to call function with NULL fits
        ok(!psFitsWriteHeader(NULL, header), "Expected return of true for NULL fits file pointer");
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
HERE*/
}
