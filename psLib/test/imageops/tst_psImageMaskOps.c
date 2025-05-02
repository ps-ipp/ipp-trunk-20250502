/** @file  tst_psImageMaskOps.c
 *
 *  @brief Contains the tests for psMaskOps.[ch]
 *
 *
 *  @author David Robbins, MHPCC
 *
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2005-10-08 03:51:20 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#include <complex.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>                    // for memset
#include <sys/stat.h>
#include <sys/types.h>

#include "psTest.h"
#include "pslib_strict.h"
#include "psType.h"

static psS32 testImageKeepMask(void);
static psS32 testImageGrowMask(void);

testDescription tests[] = {
                              {testImageKeepMask,0,"psImageKeep and Mask",0,false},
                              {testImageGrowMask,0,"psImageGrowMask",0,false},
                              {NULL}
                          };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetLevel(PS_LOG_INFO);

    return ! runTestSuite(stderr,"psImage",tests,argc,argv);
}

psS32 testImageKeepMask(void)
{
    //psImageMaskRegion
    //psImageKeepRegion
    //psImageMaskCircle
    //psImageKeepCircle
    psImage *in = psImageAlloc(3,3,PS_TYPE_MASK);
    psRegion reg;
    reg.x0 = 0;
    reg.x1 = 1;
    reg.y0 = 0;
    reg.y1 = 1;
    psMaskType mask = 2;
    in->data.PS_TYPE_MASK_DATA[0][0] = 2;
    in->data.PS_TYPE_MASK_DATA[0][1] = 0;
    in->data.PS_TYPE_MASK_DATA[0][2] = 1;
    in->data.PS_TYPE_MASK_DATA[1][0] = 4;
    in->data.PS_TYPE_MASK_DATA[1][1] = 0;
    in->data.PS_TYPE_MASK_DATA[1][2] = 3;
    in->data.PS_TYPE_MASK_DATA[2][0] = 2;
    in->data.PS_TYPE_MASK_DATA[2][1] = 1;
    in->data.PS_TYPE_MASK_DATA[2][2] = 2;

    printf("\n Mask Region------");
    psImageMaskRegion(in, reg, "|", mask);
    for(int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            printf("\nin->data.u8 [i][j] i=%d, j=%d = %u", i, j, in->data.PS_TYPE_MASK_DATA[i][j]);
        }
    }

    in->data.PS_TYPE_MASK_DATA[0][0] = 2;
    in->data.PS_TYPE_MASK_DATA[0][1] = 0;
    in->data.PS_TYPE_MASK_DATA[0][2] = 1;
    in->data.PS_TYPE_MASK_DATA[1][0] = 4;
    in->data.PS_TYPE_MASK_DATA[1][1] = 0;
    in->data.PS_TYPE_MASK_DATA[1][2] = 3;
    in->data.PS_TYPE_MASK_DATA[2][0] = 2;
    in->data.PS_TYPE_MASK_DATA[2][1] = 1;
    in->data.PS_TYPE_MASK_DATA[2][2] = 2;
    psImageKeepRegion(in, reg, "AND", mask);
    printf("\n Keep Region------");
    for(int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            printf("\nin->data.u8 [i][j] i=%d, j=%d = %u", i, j, in->data.PS_TYPE_MASK_DATA[i][j]);
        }
    }

    //Mask Circle and Keep Circle Functions
    in->data.PS_TYPE_MASK_DATA[0][0] = 2;
    in->data.PS_TYPE_MASK_DATA[0][1] = 0;
    in->data.PS_TYPE_MASK_DATA[0][2] = 1;
    in->data.PS_TYPE_MASK_DATA[1][0] = 4;
    in->data.PS_TYPE_MASK_DATA[1][1] = 0;
    in->data.PS_TYPE_MASK_DATA[1][2] = 3;
    in->data.PS_TYPE_MASK_DATA[2][0] = 2;
    in->data.PS_TYPE_MASK_DATA[2][1] = 1;
    in->data.PS_TYPE_MASK_DATA[2][2] = 2;
    psImageMaskCircle(in, 1, 1, 1, "XOR", mask);
    printf("\n Mask Circle------");
    for(int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            printf("\nin->data.u8 [i][j] i=%d, j=%d = %u", i, j, in->data.PS_TYPE_MASK_DATA[i][j]);
        }
    }

    in->data.PS_TYPE_MASK_DATA[0][0] = 2;
    in->data.PS_TYPE_MASK_DATA[0][1] = 0;
    in->data.PS_TYPE_MASK_DATA[0][2] = 1;
    in->data.PS_TYPE_MASK_DATA[1][0] = 4;
    in->data.PS_TYPE_MASK_DATA[1][1] = 0;
    in->data.PS_TYPE_MASK_DATA[1][2] = 3;
    in->data.PS_TYPE_MASK_DATA[2][0] = 2;
    in->data.PS_TYPE_MASK_DATA[2][1] = 1;
    in->data.PS_TYPE_MASK_DATA[2][2] = 2;
    psImageKeepCircle(in, 1, 1, 1, "=", mask);
    printf("\n Keep Circle------");
    for(int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            printf("\nin->data.u8 [i][j] i=%d, j=%d = %u", i, j, in->data.PS_TYPE_MASK_DATA[i][j]);
        }
    }
    fflush(stdout);

    //Error Checks
    //incorrect logical operation
    printf("\n");
    psImageKeepRegion(in, reg, "+", mask);
    //null image
    psImage *none = NULL;
    psImageMaskCircle(none, 1, 1, 1, "&", mask);

    psFree(in);
    return 0;
}

psS32 testImageGrowMask(void)
{
    psImage *in = NULL;
    psImage *out = NULL;
    psImage *test = NULL;
    psMaskType maskVal = 0;
    psMaskType growVal = 0;
    unsigned int growSize = 0;
    //return null for null input image
    out = psImageGrowMask(out, in, maskVal, growSize, growVal);
    if (out != NULL) {
        fprintf(stderr,
                "psImageGrowMask failed to return NULL for NULL image input.\n");
        return 1;
    }
    in = psImageAlloc(5, 5, PS_TYPE_MASK);
    //return null for incompatible image size
    test = psImageAlloc(2, 2, PS_TYPE_MASK);
    out = psImageGrowMask(test, in, maskVal, growSize, growVal);
    if (out != NULL) {
        fprintf(stderr,
                "psImageGrowMask failed to return NULL for incompatible image size.\n");
        return 2;
    }
    //return null for incompatible out image type
    test = psImageRecycle(test, 5, 5, PS_TYPE_F32);
    out = psImageGrowMask(test, in, maskVal, growSize, growVal);
    if (out != NULL) {
        fprintf(stderr,
                "psImageGrowMask failed to return NULL for incompatible output image type.\n");
        return 3;
    }
    //return NULL for input image that doesn't match PS_TYPE_MASK
    in = psImageRecycle(in, 5, 5, PS_TYPE_F32);
    out = psImageGrowMask(test, in, maskVal, growSize, growVal);
    if (out != NULL) {
        fprintf(stderr,
                "psImageGrowMask failed to return NULL for incompatible input image type.\n");
        return 4;
    }
    //Test for valid function (image growth)
    in = psImageRecycle(in, 5, 5, PS_TYPE_MASK);
    in->data.PS_TYPE_MASK_DATA[0][0] = 1;
    in->data.PS_TYPE_MASK_DATA[0][1] = 1;
    in->data.PS_TYPE_MASK_DATA[0][2] = 1;
    in->data.PS_TYPE_MASK_DATA[0][3] = 1;
    in->data.PS_TYPE_MASK_DATA[0][4] = 1;
    in->data.PS_TYPE_MASK_DATA[1][0] = 1;
    in->data.PS_TYPE_MASK_DATA[1][1] = 1;
    in->data.PS_TYPE_MASK_DATA[1][2] = 2;
    in->data.PS_TYPE_MASK_DATA[1][3] = 1;
    in->data.PS_TYPE_MASK_DATA[1][4] = 1;
    in->data.PS_TYPE_MASK_DATA[2][0] = 1;
    in->data.PS_TYPE_MASK_DATA[2][1] = 1;
    in->data.PS_TYPE_MASK_DATA[2][2] = 2;
    in->data.PS_TYPE_MASK_DATA[2][3] = 1;
    in->data.PS_TYPE_MASK_DATA[2][4] = 1;
    in->data.PS_TYPE_MASK_DATA[3][0] = 1;
    in->data.PS_TYPE_MASK_DATA[3][1] = 1;
    in->data.PS_TYPE_MASK_DATA[3][2] = 1;
    in->data.PS_TYPE_MASK_DATA[3][3] = 1;
    in->data.PS_TYPE_MASK_DATA[3][4] = 2;
    in->data.PS_TYPE_MASK_DATA[4][0] = 1;
    in->data.PS_TYPE_MASK_DATA[4][1] = 1;
    in->data.PS_TYPE_MASK_DATA[4][2] = 1;
    in->data.PS_TYPE_MASK_DATA[4][3] = 1;
    in->data.PS_TYPE_MASK_DATA[4][4] = 1;
    maskVal = 2;
    growSize = 1;
    growVal = 2;

    out = psImageGrowMask(out, in, maskVal, growSize, growVal);
    //0,2 1,1 1,3 2,1 2,3 3,2 should all be 3.  All other should be unchanged.
    for (int i = 0; i < 5; i++) {
        printf("\n ");
        for (int j = 0; j < 5; j++) {
            printf("  %d,%d= %d  ", i, j, out->data.PS_TYPE_MASK_DATA[i][j]);
        }
    }
    psFree(out);
    psFree(in);
    psFree(test);
    return 0;
}
