/** @file tst_pmMaskBadPixels.c
 *
 *  @brief Contains the tests for pmMaskBadPixels:
 *
 *    Test A - Create mask based on maskVal argument
 *    Test B - Create mask based on saturation argument
 *    Test C - Create mask based on growVal and grow arguments
 *    Test D - Auto Create mask based on maskVal argument
 *    Test E - Attempt to use null mask
 *    Test F - Attempt tp use null input image
 *    Test G - Attempt to use input image bigger than mask
 *    Test H - Attempt to use input image mask bigger than mask
 *    Test I - Attempt to use offset greater than input image
 *    Test J - Attempt to use complex input image
 *    Test K - Attempt to use non-mask type mask image
 *
 *  @author Ross Harman, MHPCC
 *
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2005-11-15 20:09:03 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */


#include "psTest.h"
#include "pslib.h"
#include "pmMaskBadPixels.h"


#define PRINT_MATRIX(IMAGE,TYPE,STRING)                                                                      \
printf(STRING);                                                                                              \
printf("\n");                                                                                                \
for(int i=(IMAGE)->numRows-1; i>-1; i--) {                                                                     \
    for(int j=0; j<(IMAGE)->numCols; j++) {                                                                    \
        if(PS_IS_PSELEMTYPE_COMPLEX((IMAGE)->type.type)) {                                                     \
            printf("%f+%fi ", creal((IMAGE)->data.TYPE[i][j]), cimag((IMAGE)->data.TYPE[i][j]));                 \
        } else if(PS_IS_PSELEMTYPE_INT((IMAGE)->type.type)) {                                                  \
            printf("%d", (int)(IMAGE)->data.TYPE[i][j]);                                                       \
        } else {                                                                                             \
            printf("%f", (double)(IMAGE)->data.TYPE[i][j]);                                                    \
        }                                                                                                    \
    }                                                                                                        \
    printf("\n");                                                                                            \
}                                                                                                            \
printf("\n");


#define CREATE_AND_SET_IMAGE(NAME,TYPE,VALUE,NROWS,NCOLS)                                                    \
(NAME) = (psImage*)psImageAlloc(NCOLS,NROWS,PS_TYPE_##TYPE);                                             \
for(int i=0; i<(NAME)->numRows; i++) {                                                                         \
    for(int j=0; j<(NAME)->numCols; j++) {                                                                     \
        (NAME)->data.TYPE[i][j] = VALUE;                                                                       \
    }                                                                                                        \
}

static int testMaskBadPixels1(void);
static int testMaskBadPixels2(void);
static int testMaskBadPixels3(void);
static int testMaskBadPixels4(void);
static int testMaskBadPixels5(void);
static int testMaskBadPixels6(void);
static int testMaskBadPixels7(void);
static int testMaskBadPixels8(void);
static int testMaskBadPixels9(void);
static int testMaskBadPixels10(void);
static int testMaskBadPixels11(void);


testDescription tests[] = {
                              {testMaskBadPixels1, 885, "pmMaskBadPixels - Create mask based on maskVal argument", 0, false},
                              {testMaskBadPixels2, 885, "pmMaskBadPixels - Create mask based on saturation argument", 0, false},
                              {testMaskBadPixels3, 885, "pmMaskBadPixels - Create mask based on growVal and grow arguments", 0, false},
                              {testMaskBadPixels4, 885, "pmMaskBadPixels - Auto create mask based on maskVal argument", 0, false},
                              {testMaskBadPixels5, 885, "pmMaskBadPixels - Attempt to use null mask", 0, false},
                              {testMaskBadPixels6, 885, "pmMaskBadPixels - Attempt tp use null input image", 0, false},
                              {testMaskBadPixels7, 885, "pmMaskBadPixels - Attempt to use input image bigger than mask", 0, false},
                              {testMaskBadPixels8, 885, "pmMaskBadPixels - Attempt to use input image mask bigger than mask", 0, false},
                              {testMaskBadPixels9, 885, "pmMaskBadPixels - Attempt to use offset greater than input image", 0, false},
                              {testMaskBadPixels10, 885, "pmMaskBadPixels - Attempt to use complex input image", 0, false},
                              {testMaskBadPixels11, 885, "pmMaskBadPixels - Attempt to use non-mask type mask image", 0, false        },
                              {NULL}
                          };

/*
    #define PS_TYPE_MASK PS_TYPE_U8
    #define PS_TYPE_MASK_DATA U8
    #define PS_TYPE_MASK_NAME "psU8"
*/

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    return !runTestSuite(stderr, "Test Point Driver", tests, argc, argv);
}


#define NUM_ROWS 50
#define NUM_COLS 50
#define DEFAULT_IMAGE_VAL 0.0
#define DEFAULT_MASK_VAL 0
#define MASK_VAL 1
#define SAT_VAL  100.0
#define GROW_VAL 1
#define GROW_RAD 10
int testMaskBadPixels1( void )
{
    //
    // Test A - Create mask based on maskVal argument
    //

    pmReadout *inReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(inReadout->image, F32, DEFAULT_IMAGE_VAL, NUM_ROWS, NUM_COLS);
    inReadout->image->row0 = 0;
    inReadout->image->col0 = 0;
    inReadout->row0 = 0;
    inReadout->col0 = 0;

    pmReadout *maskReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(maskReadout->image, U8, DEFAULT_MASK_VAL, NUM_ROWS, NUM_COLS)
    maskReadout->image->data.PS_TYPE_MASK_DATA[NUM_ROWS/2][NUM_COLS/2]=1;
    maskReadout->image->row0 = 0;
    maskReadout->image->col0 = 0;

    PRINT_MATRIX(maskReadout->image, U8, "Data mask:");

    pmMaskBadPixels(inReadout, maskReadout, MASK_VAL, SAT_VAL, GROW_VAL, GROW_RAD);
    PRINT_MATRIX(inReadout->mask, PS_TYPE_MASK_DATA, "Resulting mask:");

    psFree(inReadout);
    psFree(maskReadout);

    return 0;
}

int testMaskBadPixels2( void )
{
    //
    // Test B - Create mask based on saturation argument
    //

    pmReadout *inReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(inReadout->image, F32, DEFAULT_IMAGE_VAL, NUM_ROWS, NUM_COLS);
    inReadout->image->row0 = 0;
    inReadout->image->col0 = 0;
    inReadout->row0 = 0;
    inReadout->col0 = 0;
    inReadout->image->data.F32[NUM_ROWS/2][NUM_COLS/2] = 150.0;

    pmReadout *maskReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(maskReadout->image, U8, DEFAULT_MASK_VAL, NUM_ROWS, NUM_COLS)

    //PS_IMAGE_PRINT_F32(inReadout->image);
    PRINT_MATRIX(maskReadout->image, U8, "Data mask:");
    pmMaskBadPixels(inReadout, maskReadout, MASK_VAL, SAT_VAL, GROW_VAL, GROW_RAD);
    PRINT_MATRIX(inReadout->mask, U8, "Resulting mask:");

    psFree(inReadout);
    psFree(maskReadout);

    return 0;
}

int testMaskBadPixels3( void )
{
    //
    // Test C - Create mask based on growVal and grow arguments
    //

    pmReadout *inReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(inReadout->image, F32, DEFAULT_IMAGE_VAL, NUM_ROWS, NUM_COLS);
    inReadout->image->row0 = 0;
    inReadout->image->col0 = 0;
    inReadout->row0 = 0;
    inReadout->col0 = 0;
    inReadout->image->data.F32[NUM_ROWS/2][NUM_COLS/2]=GROW_VAL;
    inReadout->image->data.F32[NUM_ROWS/4][NUM_COLS/4]=GROW_VAL;
    inReadout->image->data.F32[NUM_ROWS/4][NUM_COLS-(NUM_COLS/4)]=GROW_VAL;

    pmReadout *maskReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(maskReadout->image, U8, DEFAULT_MASK_VAL, NUM_ROWS, NUM_COLS);

    PRINT_MATRIX(maskReadout->image, U8, "Data mask:");
    //PS_IMAGE_PRINT_F32(inReadout->image);
    pmMaskBadPixels(inReadout, maskReadout, MASK_VAL, SAT_VAL, GROW_VAL, GROW_RAD);
    PRINT_MATRIX(inReadout->mask, U8, "Resulting mask:");

    psFree(inReadout);
    psFree(maskReadout);

    return 0;
}

int testMaskBadPixels4( void )
{
    //
    // Test D - Auto Create mask based on maskVal argument
    //

    pmReadout *inReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(inReadout->image, F32, DEFAULT_IMAGE_VAL, NUM_ROWS, NUM_COLS);
    inReadout->image->row0 = 0;
    inReadout->image->col0 = 0;
    inReadout->row0 = 0;
    inReadout->col0 = 0;

    pmReadout *maskReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(maskReadout->image, U8, DEFAULT_MASK_VAL, NUM_ROWS, NUM_COLS)
    maskReadout->image->data.PS_TYPE_MASK_DATA[NUM_ROWS/2][NUM_COLS/2]=1;

    PRINT_MATRIX(maskReadout->image, U8, "Data mask:");
    pmMaskBadPixels(inReadout, maskReadout, MASK_VAL, SAT_VAL, GROW_VAL, GROW_RAD);
    PRINT_MATRIX(inReadout->mask, U8, "Resulting mask:");

    psFree(inReadout);
    psFree(maskReadout);

    return 0;
}

int testMaskBadPixels5( void )
{
    //
    // Test E - Attempt to use null mask
    //

    pmReadout *inReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(inReadout->image, F32, DEFAULT_IMAGE_VAL, NUM_ROWS, NUM_COLS);
    inReadout->image->row0 = 0;
    inReadout->image->col0 = 0;
    inReadout->row0 = 0;
    inReadout->col0 = 0;

    pmMaskBadPixels(inReadout, NULL, MASK_VAL, SAT_VAL, GROW_VAL, GROW_RAD);
    psFree(inReadout);

    return 0;
}

int testMaskBadPixels6( void )
{
    //
    // Test F - Attempt tp use null input image
    //

    pmReadout *inReadout = pmReadoutAlloc(NULL);
    inReadout->row0 = 0;
    inReadout->col0 = 0;

    pmReadout *maskReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(maskReadout->image, U8, DEFAULT_MASK_VAL, NUM_ROWS, NUM_COLS)

    pmMaskBadPixels(inReadout, maskReadout, MASK_VAL, SAT_VAL, GROW_VAL, GROW_RAD);

    psFree(inReadout);
    psFree(maskReadout);

    return 0;
}

int testMaskBadPixels7( void )
{
    //
    // Test G - Attempt to use input image bigger than mask
    //

    pmReadout *inReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(inReadout->image, F32, DEFAULT_IMAGE_VAL, NUM_ROWS+10, NUM_COLS+10);
    inReadout->image->row0 = 0;
    inReadout->image->col0 = 0;
    inReadout->row0 = 0;
    inReadout->col0 = 0;

    pmReadout *maskReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(maskReadout->image, U8, DEFAULT_MASK_VAL, NUM_ROWS, NUM_COLS)

    pmMaskBadPixels(inReadout, maskReadout, MASK_VAL, SAT_VAL, GROW_VAL, GROW_RAD);

    psFree(inReadout);
    psFree(maskReadout);

    return 0;
}

int testMaskBadPixels8( void )
{
    //
    // Test H - Attempt to use mask bigger than image
    //

    pmReadout *inReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(inReadout->image, F32, DEFAULT_IMAGE_VAL, NUM_ROWS, NUM_COLS);
    inReadout->image->row0 = 0;
    inReadout->image->col0 = 0;
    inReadout->row0 = 0;
    inReadout->col0 = 0;

    pmReadout *maskReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(maskReadout->image, U8, DEFAULT_MASK_VAL, NUM_ROWS+10, NUM_COLS+10)

    pmMaskBadPixels(inReadout, maskReadout, MASK_VAL, SAT_VAL, GROW_VAL, GROW_RAD);

    psFree(inReadout);
    psFree(maskReadout);

    return 0;
}

int testMaskBadPixels9( void )
{
    //
    // Test I - Attempt to use offset greater than input image
    //

    pmReadout *inReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(inReadout->image, F32, DEFAULT_IMAGE_VAL, NUM_ROWS, NUM_COLS);
    inReadout->image->row0 = 0;
    inReadout->image->col0 = 0;
    inReadout->row0 = 0;
    inReadout->col0 = 0;
    *(int*)&inReadout->image->col0 = 150;
    *(int*)&inReadout->image->row0 = 150;

    pmReadout *maskReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(maskReadout->image, U8, DEFAULT_MASK_VAL, NUM_ROWS, NUM_COLS)
    pmMaskBadPixels(inReadout, maskReadout, MASK_VAL, SAT_VAL, GROW_VAL, GROW_RAD);

    psFree(inReadout);
    psFree(maskReadout);

    return 0;
}

int testMaskBadPixels10( void )
{
    //
    // Test J - Attempt to use complex input image
    //

    pmReadout *inReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(inReadout->image, C64, DEFAULT_IMAGE_VAL, NUM_ROWS, NUM_COLS);
    inReadout->image->row0 = 0;
    inReadout->image->col0 = 0;
    inReadout->row0 = 0;
    inReadout->col0 = 0;

    pmReadout *maskReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(maskReadout->image, U8, DEFAULT_MASK_VAL, NUM_ROWS, NUM_COLS)

    pmMaskBadPixels(inReadout, maskReadout, MASK_VAL, SAT_VAL, GROW_VAL, GROW_RAD);

    psFree(inReadout);
    psFree(maskReadout);

    return 0;
}

int testMaskBadPixels11( void )
{
    //
    // Test K - Attempt to use mask image with wrong data type.
    //

    pmReadout *inReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(inReadout->image, F32, DEFAULT_IMAGE_VAL, NUM_ROWS, NUM_COLS);
    inReadout->image->row0 = 0;
    inReadout->image->col0 = 0;
    inReadout->row0 = 0;
    inReadout->col0 = 0;

    pmReadout *maskReadout = pmReadoutAlloc(NULL);
    CREATE_AND_SET_IMAGE(maskReadout->image, F64, DEFAULT_MASK_VAL, NUM_ROWS, NUM_COLS)

    pmMaskBadPixels(inReadout, maskReadout, MASK_VAL, SAT_VAL, GROW_VAL, GROW_RAD);

    psFree(inReadout);
    psFree(maskReadout);

    return 0;
}

