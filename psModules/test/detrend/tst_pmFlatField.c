/** @file tst_pmFlatField.c
 *
 *  @brief Contains the tests for pmFlatField.c:
 *
 *    Test A - Divide input image by flat image
 *    Test B - Mask flat image data
 *    Test C - Mask flat image data starting with non-null mask
 *    Test E - Attempt to use null input image
 *    Test F - Attempt tp use null flat image
 *    Test G - Attempt to use input image bigger than flat image
 *    Test H - Attempt to use input image mask bigger than flat image
 *    Test I - Attempt to use offset greater than input image
 *    Test J - Attempt to use complex input image
 *    Test K - Attempt to use complex flat image
 *    Test L - Attempt to use non-equal input and flat image types
 *    Test M - Attempt to use non-mask type mask image
 *
 * XXX: Added a mask argument to pmFlatField().  Must add tests.  For now, all
 * masks are NULL.
 *
 *  @author Ross Harman, MHPCC
 *
 *  XXX: I added the CELL.TRIMSEC region code but there are not tests for it.
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2005-11-15 20:09:03 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */


#include "psTest.h"
#include "pslib.h"
#include "pmFlatField.h"


#define PRINT_MATRIX(IMAGE,TYPE,STRING)                                                                      \
printf(STRING);                                                                                              \
printf("\n");                                                                                                \
for(int i=IMAGE->numRows-1; i>-1; i--) {                                                                     \
    for(int j=0; j<IMAGE->numCols; j++) {                                                                    \
        if(PS_IS_PSELEMTYPE_COMPLEX(IMAGE->type.type)) {                                                     \
            printf("%f+%fi ", creal(IMAGE->data.TYPE[i][j]), cimag(IMAGE->data.TYPE[i][j]));                 \
        } else if(PS_IS_PSELEMTYPE_INT(IMAGE->type.type)) {                                                  \
            printf("%d ", (int)IMAGE->data.TYPE[i][j]);                                                      \
        } else {                                                                                             \
            printf("%f ", (double)IMAGE->data.TYPE[i][j]);                                                   \
        }                                                                                                    \
    }                                                                                                        \
    printf("\n");                                                                                            \
}                                                                                                            \
printf("\n");


#define CREATE_AND_SET_IMAGE(NAME,TYPE,VALUE,NROWS,NCOLS)                                                    \
psImage *NAME = (psImage*)psImageAlloc(NCOLS,NROWS,PS_TYPE_##TYPE);                                          \
for(int i=0; i<NAME->numRows; i++) {                                                                         \
    for(int j=0; j<NAME->numCols; j++) {                                                                     \
        NAME->data.TYPE[i][j] = VALUE;                                                                       \
    }                                                                                                        \
}


static int testFlatField(void);


testDescription tests[] = {
                              {testFlatField, 753, "pmFlatField", 0, false},
                              {NULL}
                          };


int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    return !runTestSuite(stderr, "Test Point Driver", tests, argc, argv);
}


int testFlatField( void )
{
    // Test A - Divide input image by flat image
    printPositiveTestHeader(stdout, "pmFlatField", "Test A - Divide input image by flat image");
    CREATE_AND_SET_IMAGE(inImage,F64,6.0,3,3)
    pmReadout *inReadout = pmReadoutAlloc(NULL);
    inReadout->image = inImage;
    inReadout->row0 = 0;
    inReadout->col0 = 0;
    CREATE_AND_SET_IMAGE(inMask, U8, 0, 3,3);
    inReadout->mask = inMask;
    PRINT_MATRIX((inReadout->mask),U8,"Input mask:");
    PRINT_MATRIX(inImage,F64,"Input image:");

    CREATE_AND_SET_IMAGE(flatImage1,F64,2.0,3,3)
    pmReadout *flatReadout = pmReadoutAlloc(NULL);
    flatReadout->row0 = 0;
    flatReadout->col0 = 0;
    flatReadout->image = flatImage1;
    PRINT_MATRIX(flatImage1,F64,"Flat image:");

    if ( !pmFlatField(inReadout, flatReadout) ) {
        psError(PS_ERR_UNKNOWN,true,"Test A - Returned false should be true");
        return 1;
    }
    PRINT_MATRIX(inImage,F64, "Resulting image:");
    printFooter(stdout, "pmFlatField", "Test A - Divide input image by flat image", true);
    printf("\n\n\n");


    // Test B - Mask flat image data
    printPositiveTestHeader(stdout, "pmFlatField", "Test B - Mask flat image data");
    PRINT_MATRIX(inImage, F64, "Input image:");
    CREATE_AND_SET_IMAGE(flatImage2,F64,0.0,3,3)
    PRINT_MATRIX(flatImage2, F64, "Flat image:");
    flatReadout->image = flatImage2;
    if ( !pmFlatField(inReadout, flatReadout) ) {
        psError(PS_ERR_UNKNOWN,true,"Test B - Returned false should be true");
        return 2;
    }
    PRINT_MATRIX(inReadout->mask, PS_TYPE_MASK_DATA, "Resulting mask:");
    PRINT_MATRIX(inImage,F64,"Resulting image:");
    printFooter(stdout, "pmFlatField", "Test B - Mask flat image data", true);
    printf("\n\n\n");


    // Test C - Mask flat image data starting with non-null mask
    printPositiveTestHeader(stdout, "pmFlatField", "Test C - Mask flat image data starting with non-null mask");
    PRINT_MATRIX(inImage, F64, "Input image:");
    flatImage2->data.F64[0][0] = 3.0;
    flatImage2->data.F64[0][1] = -3.0;
    PRINT_MATRIX(flatImage2, F64, "Flat image:");
    CREATE_AND_SET_IMAGE(mask1,U8,0,3,3);
    psFree(inReadout->mask);
    inReadout->mask = mask1;
    if ( !pmFlatField(inReadout, flatReadout) ) {
        psError(PS_ERR_UNKNOWN,true,"Test C - Returned false should be true");
        return 3;
    }
    PRINT_MATRIX(flatImage2, F64, "Flat image out:");
    PRINT_MATRIX(inReadout->mask, PS_TYPE_MASK_DATA,"Resulting mask:");
    PRINT_MATRIX(inImage,F64,"Resulting image:");
    printFooter(stdout, "pmFlatField", "Test C - Mask flat image data starting with non-null mask", true);
    printf("\n\n\n");


    // Test D - Attempt to use null flat readout
    printNegativeTestHeader(stdout,"pmFlatField", "Test D - Attempt to use null flat readout",
                            "Null not allowed for flat readout", 0);
    if( pmFlatField(inReadout, NULL) ) {
        psError(PS_ERR_UNKNOWN,true,"Test D - Returned true should be false");
        return 4;
    }
    printFooter(stdout, "pmFlatField", "Test D - Attempt to use null flat readout", true);
    printf("\n\n\n");


    // Test E - Attempt to use null input image
    printNegativeTestHeader(stdout,"pmFlatField", "Test E - Attempt to use null input image",
                            "Null not allowed for input image", 0);
    psImage *temp = inReadout->image;
    inReadout->image = NULL;
    if ( pmFlatField(inReadout, flatReadout) ) {
        psError(PS_ERR_UNKNOWN,true,"Test E - Returned true should be false" );
        return 5;
    }
    inReadout->image = temp    ;
    printFooter(stdout, "pmFlatField", "Test E - Attempt to use null input image", true);
    printf("\n\n\n");


    // Test F - Attempt tp use null flat image
    printNegativeTestHeader(stdout,"pmFlatField", "Test F - Attempt tp use null flat image",
                            "Null not allowed for flat image", 0);
    temp = flatReadout->image;
    flatReadout->image = NULL;
    if ( pmFlatField(inReadout, flatReadout) ) {
        psError(PS_ERR_UNKNOWN,true,"Test F - Returned true should be false" );
        return 6;
    }
    flatReadout->image = temp;
    printFooter(stdout, "pmFlatField", "Test F - Attempt tp use null flat image", true);
    printf("\n\n\n");


    // Test G - Attempt to use input image bigger than flat image
    printNegativeTestHeader(stdout,"pmFlatField", "Test G - Attempt to use input image bigger than flat image",
                            "Input image size exceeds that of flat image", 0);
    CREATE_AND_SET_IMAGE(smallFlat,F64,0.0,2,2);
    temp = flatReadout->image;
    flatReadout->image = smallFlat;
    if ( pmFlatField(inReadout, flatReadout) ) {
        psError(PS_ERR_UNKNOWN,true,"Test G - Returned true should be false");
        return 7;
    }
    flatReadout->image = temp;
    printFooter(stdout, "pmFlatField", "Test G - Attempt to use input image bigger than flat image", true);
    printf("\n\n\n");

    // Test H - Attempt to use input image mask bigger than flat image
    printNegativeTestHeader(stdout,"pmFlatField", "Test H - Attempt to use input image mask bigger than flat image",
                            "Input image mask size exceeds that of flat image", 0);
    CREATE_AND_SET_IMAGE(largeMask,F64,0.0,5,5);
    inReadout->mask = largeMask;
    if ( pmFlatField(inReadout, flatReadout) ) {
        psError(PS_ERR_UNKNOWN,true,"Test H - Returned true should be false");
        return 8;
    }
    printFooter(stdout, "pmFlatField", "Test H - Attempt to use input image mask bigger than flat image", true);
    printf("\n\n\n");
    inReadout->mask = mask1;

    // Test I - Attempt to use offset greater than input image
    printNegativeTestHeader(stdout,"pmFlatField", "Test I - Attempt to use offset greater than input image",
                            "Total offset >= input image size", 0);
    *(int*)&inReadout->col0 = 50;
    *(int*)&inReadout->row0 = 50;
    if ( pmFlatField(inReadout, flatReadout) ) {
        psError(PS_ERR_UNKNOWN,true,"Test I - Returned true should be false");
        return 9;
    }
    *(int*)&inReadout->col0 = 0;
    *(int*)&inReadout->row0 = 0;
    printFooter(stdout, "pmFlatField", "Test I - Attempt to use offset greater than input image", true);
    printf("\n\n\n");


    // Test J - Attempt to use complex input image
    printNegativeTestHeader(stdout,"pmFlatField", "Test J - Attempt to use complex input image",
                            "Complex types not allowed for input image", 0);
    *(psElemType* ) & inReadout->image->type.type = PS_TYPE_C64;
    if ( pmFlatField(inReadout, flatReadout) ) {
        psError(PS_ERR_UNKNOWN,true,"Test J - Returned true should be false");
        return 10;
    }
    *(psElemType* ) & inReadout->image->type.type = PS_TYPE_F64;
    printFooter(stdout, "pmFlatField", "Test J - Attempt to use complex input image", true);
    printf("\n\n\n");


    // Test K - Attempt to use complex flat image
    printNegativeTestHeader(stdout,"pmFlatField", "Test K - Attempt to use complex flat image",
                            "Complex types not allowed for flat image", 0);
    *(psElemType* ) & flatReadout->image->type.type = PS_TYPE_C64;
    if ( pmFlatField(inReadout, flatReadout) ) {
        psError(PS_ERR_UNKNOWN,true,"Test K - Returned ture should be false");
        return 11;
    }
    *(psElemType* ) & flatReadout->image->type.type = PS_TYPE_F64;
    printFooter(stdout, "pmFlatField", "Test K - Attempt to use complex flat image", true);
    printf("\n\n\n");


    // Test L - Attempt to use non-equal input and flat image types
    printNegativeTestHeader(stdout,"pmFlatField", "Test L - Attempt to use non-equal input and flat image types",
                            "Input and flat image types differ", 0);
    *(psElemType* ) & flatReadout->image->type.type = PS_TYPE_F32;
    if ( pmFlatField(inReadout, flatReadout) ) {
        psError(PS_ERR_UNKNOWN,true,"Test L - Returned true should be false");
        return 12;
    }
    *(psElemType* ) & flatReadout->image->type.type = PS_TYPE_F64;
    printFooter(stdout, "pmFlatField", "Test L - Attempt to use non-equal input and flat image types", true);
    printf("\n\n\n");


    // Test M - Attempt to use non-mask type mask image
    printNegativeTestHeader(stdout,"pmFlatField", "Test M - Attempt to use non-mask type mask image",
                            "Mask must be PS_TYPE_MASK type", 0);
    *(psElemType* ) & inReadout->mask->type.type = PS_TYPE_F32;
    if ( pmFlatField(inReadout, flatReadout) ) {
        psError(PS_ERR_UNKNOWN,true,"Test M - Returned true should be false");
        return 13;
    }
    *(psElemType* ) & inReadout->mask->type.type = PS_TYPE_MASK;
    printFooter(stdout, "pmFlatField", "Test M - Attempt to use non-mask type mask image", true);
    printf("\n\n\n");


    // Free memory
    psFree(inReadout);
    psFree(flatReadout);
    //psFree(inImage);
    psFree(flatImage1);
    //psFree(flatImage1);
    psFree(smallFlat);
    psFree(largeMask);

    return 0;
}

