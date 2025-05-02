/**
 *  C Implementation: tap_psMetadata_polynomials
 *
 * Description:  Tests for psPolynomial(2,3,4)D(to/from)MD
 *
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

int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(88);
    // psPolynomial(1D, 2D, 3D, & 4D)(to/from)Metadata tests


    // psPolynomial1DtoMetadata & psPolynomial1DfromMetadata functions
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        psPolynomial1D *p1d = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1);
        p1d->coeff[0] = 1.1;
        p1d->coeff[1] = 2.2;
        p1d->coeffErr[0] = 0.1;
        p1d->coeffErr[1] = 0.2;

        //psPolynomial1DtoMetadata
        //Return a valid metadata containing a polynomial-metadata structure
        {
            note ("example of 1DtoMD using the names not the sequence");

            ok( psPolynomial1DtoMetadata(md, p1d, "polyMD"),
                "psPolynomial1DtoMetadata:      return true for valid inputs.");
            psMetadata *polyMDtemp = psMetadataLookupMetadata(NULL, md, "polyMD");
            skip_start( polyMDtemp == NULL , 1,
                        "Skipping 1 tests because psPolynomial1DtoMetadata has errors");

            bool status;
            int nX = psMetadataLookupS32(&status, polyMDtemp, "NORDER_X");
            ok(status,
               "psPolynomial1DtoMetadata:      found NORDER_X");
            ok(nX == 1,
               "psPolynomial1DtoMetadata:      NORDER_X is 1");

            double val;
            val = psMetadataLookupF64(&status, polyMDtemp, "VAL_X00");
            ok(status,
               "psPolynomial1DtoMetadata:      found VAL_X00");
            is_double(val, 1.1,
                      "psPolynomial1DtoMetadata:      VAL_X00 is %lf", val);

            val = psMetadataLookupF64(&status, polyMDtemp, "VAL_X01");
            ok(status,
               "psPolynomial1DtoMetadata:      found VAL_X01");
            is_double(val, 2.2,
                      "psPolynomial1DtoMetadata:      VAL_X01 is %lf", val);
            skip_end();
        }

        //Return false for no-name polynomial
        {
            ok( !psPolynomial1DtoMetadata(md, p1d, " "),
                "psPolynomial1DtoMetadata:      return false for no-name.");
        }
        //Return false for NULL-name polynomial
#if 0 // Caught by compiler
        {
            ok( !psPolynomial1DtoMetadata(md, p1d, NULL),
                "psPolynomial1DtoMetadata:      return false for NULL name input.");
        }
#endif
        //Return false for NULL metadata input
        {
            ok( !psPolynomial1DtoMetadata(NULL, p1d, "polyMD"),
                "psPolynomial1DtoMetadata:     return false for NULL metadata input.");
        }
        //Return false for NULL polynomial input
        {
            ok( !psPolynomial1DtoMetadata(md, NULL, "polyMD"),
                "psPolynomial1DtoMetadata:     return false for NULL polynomial input.");
        }
        //Return true for polynomial with 1 element, a constant
        {
            psPolynomial1D *constPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 0);
            constPoly->coeff[0] = 6.66;
            ok( psPolynomial1DtoMetadata(md, constPoly, "polyMD"),
                "psPolynomial1DtoMetadata:     return true for constant polynomial (1 element != 0).");
            psFree(constPoly);
        }
        //Return false for non-ordinary polynomial
        {
            psPolynomial1D *p1d1 = psPolynomial1DAlloc(PS_POLYNOMIAL_CHEB, 1);
            p1d1->coeff[0] = 1.1;
            ok( !psPolynomial1DtoMetadata(md, p1d1, "polyMD"),
                "psPolynomial1DtoMetadata:     return false for chebyshev polynomial");
            psFree(p1d1);
        }

        //psPolynomial1DfromMetadata Tests
        //Return NULL for NULL metadata input.
        {
            psPolynomial1D *emptyPoly1D = NULL;
            emptyPoly1D = psPolynomial1DfromMetadata(NULL);
            ok( emptyPoly1D == NULL,
                "psPolynomial1DfromMetadata:   return NULL for NULL metadata input.");
        }
        //Get a polynomial from Metadata.
        psPolynomial1D *outPoly = NULL;
        psMetadata *polyMDtemp = psMetadataLookupMetadata(NULL, md, "polyMD");
        outPoly = psPolynomial1DfromMetadata(polyMDtemp);
        {
            skip_start( outPoly == NULL, 1,
                        "Skipping 1 tests because psPolynomial1DfromMetadata has errors.");
            ok( outPoly->type == PS_POLYNOMIAL_ORD && outPoly->nX == 1 &&
                abs(outPoly->coeff[0] - 1.1) < DBL_EPSILON,
                "psPolynomial1DfromMetadata:   return correct polynomial from metadata");
            skip_end();
        }
        //Get a polynomial from Metadata with mask[0] == 1.
        psFree(outPoly);
        outPoly = NULL;
        psPolynomial1D *maskPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 0);
        maskPoly->coeff[0] = 6.66;
        maskPoly->coeffMask[0] = 1;
        psMetadata *newmd = psMetadataAlloc();
        psPolynomial1DtoMetadata(newmd, maskPoly, "polyMD");
        psMetadata *polyMask = psMetadataLookupMetadata(NULL, newmd, "polyMD");
        outPoly = psPolynomial1DfromMetadata(polyMask);
        {
            ok( outPoly->type == PS_POLYNOMIAL_ORD && outPoly->nX == 0 &&
                outPoly->coeffMask[0] == 1,
                "psPolynomial1DfromMetadata:   return correct polynomial (w/mask) from metadata");
        }
        psFree(maskPoly);
        psFree(newmd);
        //Return NULL for wrong NELEMENTS
        psMetadataItem *nElementItem = psMetadataGet(polyMDtemp, PS_LIST_TAIL);
        nElementItem->data.S32 -= 1;
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial1DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial1DfromMetadata:   "
                "return NULL for metadata-polynomial with wrong NELEMENTS");
        }
        //Return NULL for missing NELEMENTS
        psMetadataRemoveIndex(polyMDtemp, PS_LIST_TAIL);
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial1DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial1DfromMetadata:   "
                "return NULL for metadata-polynomial with no NELEMENTS");
        }

        //Return NULL for polynomial in metadata with no x-order
        psMetadataRemoveKey(polyMDtemp, "NORDER_X");
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial1DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial1DfromMetadata:   "
                "return NULL for metadata-polynomial with no x-order");
        }

        psFree(outPoly);
        psFree(p1d);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // psPolynomial2DtoMetadata & psPolynomial2DfromMetadata functions
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        psPolynomial2D *p2d = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 1, 1);
        p2d->coeff[0][0] = 1.1;
        p2d->coeff[0][1] = 2.2;
        p2d->coeff[1][0] = 3.3;
        p2d->coeff[1][1] = 4.4;
        p2d->coeffErr[0][0] = 0.1;
        p2d->coeffErr[0][1] = 0.2;
        p2d->coeffErr[1][0] = 0.3;
        p2d->coeffErr[1][1] = 0.4;

        //psPolynomial2DtoMetadata
        //Return a valid metadata containing a polynomial-metadata structure
        {
            ok( psPolynomial2DtoMetadata(md, p2d, "polyMD"),
                "psPolynomial2DtoMetadata:     return true for valid inputs.");

            // psMetadataConfigWrite (md, "test.md");

            //XXX        ok (0, "these tests are relying on the ORDER of the MD components not the NAMES");
            //XXX        ok (0, "these seem like irrelevant tests");
            psMetadata *polyMDtemp = psMetadataLookupMetadata(NULL, md, "polyMD");
            skip_start( polyMDtemp == NULL , 1,
                        "Skipping 1 tests because psPolynomial2DtoMetadata has errors");
            psMetadataItem *polyItem = psMetadataGet(polyMDtemp, 0);
            skip_start( polyItem == NULL, 2,
                        "Skipping 2 tests because psPolynomial2DtoMetadata has errors in order elements");
            ok( !strncmp(polyItem->name, "NORDER_X", 10) && polyItem->data.S32 == 1,
                "psPolynomial2DtoMetadata:     return correct number of x orders.");
            polyItem = psMetadataGet(polyMDtemp, 1);
            ok( !strncmp(polyItem->name, "NORDER_Y", 10) && polyItem->data.S32 == 1,
                "psPolynomial2DtoMetadata:     return correct number of y orders.");
            skip_end();
            polyItem = psMetadataGet(polyMDtemp, 2);
            skip_start( polyItem == NULL, 2,
                        "Skipping 2 tests because psPolynomial2DtoMetadata has errors in coeff elements");
            ok( !strncmp(polyItem->name, "VAL_X00_Y00", 14) &&
                abs(polyItem->data.F64-1.1) < DBL_EPSILON,
                "psPolynomial2DtoMetadata:     return correct first element.");
            polyItem = psMetadataGet(polyMDtemp, PS_LIST_TAIL-1);
            ok( !strncmp(polyItem->name, "ERR_X01_Y01", 14) &&
                abs(polyItem->data.F64-0.4) < DBL_EPSILON,
                "psPolynomial2DtoMetadata:     return correct last element.");
            skip_end();
            skip_end();
        }

        //psPolynomial2DtoMetadata
        //Return a valid metadata containing a polynomial-metadata structure
        {
            note ("example of 2DtoMD using the names not the sequence");

            ok( psPolynomial2DtoMetadata(md, p2d, "polyMD"),
                "psPolynomial2DtoMetadata:     return true for valid inputs.");

            // psMetadataConfigWrite (md, "test.md");

            // of the components.  these seem like irrelevant tests.
            psMetadata *polyMDtemp = psMetadataLookupMetadata(NULL, md, "polyMD");
            skip_start( polyMDtemp == NULL , 1,
                        "Skipping 1 tests because psPolynomial2DtoMetadata has errors");

            bool status;
            int nX = psMetadataLookupS32(&status, polyMDtemp, "NORDER_X");
            ok(status,
               "psPolynomial2DtoMetadata:     found NORDER_X");
            ok(nX == 1,
               "psPolynomial2DtoMetadata:     NORDER_X is 1");

            int nY = psMetadataLookupS32(&status, polyMDtemp, "NORDER_Y");
            ok(status,
               "psPolynomial2DtoMetadata:     found NORDER_Y");
            ok(nY == 1,
               "psPolynomial2DtoMetadata:     NORDER_Y is 1");

            double val;
            val = psMetadataLookupF64(&status, polyMDtemp, "VAL_X00_Y00");
            ok(status,
               "psPolynomial2DtoMetadata:     found VAL_X00_Y00");
            is_double(val, 1.1,
                      "psPolynomial2DtoMetadata:     VAL_X00_Y00 is %lf", val);

            val = psMetadataLookupF64(&status, polyMDtemp, "VAL_X01_Y01");
            ok(status,
               "psPolynomial2DtoMetadata:     found VAL_X01_Y01");
            is_double(val, 4.4,
                      "psPolynomial2DtoMetadata:     VAL_X01_Y01 is %lf", val);

            skip_end();
        }

        //Return false for no-name polynomial
        {
            ok( !psPolynomial2DtoMetadata(md, p2d, " "),
                "psPolynomial2DtoMetadata:     return false for no-name.");
        }
        //Return false for NULL-name polynomial
#if 0 // Caught by compiler
        {
            ok( !psPolynomial2DtoMetadata(md, p2d, NULL),
                "psPolynomial2DtoMetadata:     return false for NULL name input.");
        }
#endif
        //Return false for NULL metadata input
        {
            ok( !psPolynomial2DtoMetadata(NULL, p2d, "polyMD"),
                "psPolynomial2DtoMetadata:     return false for NULL metadata input.");
        }
        //Return false for NULL polynomial input
        {
            ok( !psPolynomial2DtoMetadata(md, NULL, "polyMD"),
                "psPolynomial2DtoMetadata:     return false for NULL polynomial input.");
        }
        //Return true for polynomial with 1 element, a constant
        {
            psPolynomial2D *constPoly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 0, 0);
            constPoly->coeff[0][0] = 6.66;
            ok( psPolynomial2DtoMetadata(md, constPoly, "polyMD"),
                "psPolynomial2DtoMetadata:     return true for constant polynomial (1 element != 0).");
            psFree(constPoly);
        }
        //Return false for non-ordinary polynomial
        {
            psPolynomial2D *p2d2 = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, 1, 1);
            p2d2->coeff[0][0] = 1.1;
            ok( !psPolynomial2DtoMetadata(md, p2d2, "polyMD"),
                "psPolynomial2DtoMetadata:     return false for chebyshev polynomial");
            psFree(p2d2);
        }

        //psPolynomial2DfromMetadata Tests
        //Return NULL for NULL metadata input.
        {
            psPolynomial2D *emptyPoly2D = NULL;
            emptyPoly2D = psPolynomial2DfromMetadata(NULL);
            ok( emptyPoly2D == NULL,
                "psPolynomial2DfromMetadata:   return NULL for NULL metadata input.");
        }
        //Get a polynomial from Metadata.
        psPolynomial2D *outPoly = NULL;
        psMetadata *polyMDtemp = psMetadataLookupMetadata(NULL, md, "polyMD");
        outPoly = psPolynomial2DfromMetadata(polyMDtemp);
        {
            skip_start( outPoly == NULL, 1,
                        "Skipping 1 tests because psPolynomial2DfromMetadata has errors.");
            ok( outPoly->type == PS_POLYNOMIAL_ORD && outPoly->nX == 1 &&
                outPoly->nY == 1 && abs(outPoly->coeff[0][0] - 1.1) < DBL_EPSILON,
                "psPolynomial2DfromMetadata:   return correct polynomial from metadata");
            skip_end();
        }
        //Get a polynomial from Metadata with mask[0][0] == 1.
        psFree(outPoly);
        outPoly = NULL;
        psPolynomial2D *maskPoly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 0, 0);
        maskPoly->coeff[0][0] = 6.66;
        maskPoly->coeffMask[0][0] = 1;
        psMetadata *newmd = psMetadataAlloc();
        psPolynomial2DtoMetadata(newmd, maskPoly, "polyMD");
        psMetadata *polyMask = psMetadataLookupMetadata(NULL, newmd, "polyMD");
        outPoly = psPolynomial2DfromMetadata(polyMask);
        {
            ok( outPoly->type == PS_POLYNOMIAL_ORD && outPoly->nX == 0 &&
                outPoly->nY == 0 && outPoly->coeffMask[0][0] == 1,
                "psPolynomial2DfromMetadata:   return correct polynomial (w/mask) from metadata");
        }
        psFree(maskPoly);
        psFree(newmd);
        //Return NULL for wrong NELEMENTS
        psMetadataItem *nElementItem = psMetadataGet(polyMDtemp, PS_LIST_TAIL);
        nElementItem->data.S32 -= 1;
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial2DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial2DfromMetadata:   "
                "return NULL for metadata-polynomial with wrong NELEMENTS");
        }
        //Return NULL for missing NELEMENTS
        psMetadataRemoveIndex(polyMDtemp, PS_LIST_TAIL);
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial2DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial2DfromMetadata:   "
                "return NULL for metadata-polynomial with no NELEMENTS");
        }

        //Return NULL for polynomial in metadata with no y-order
        psMetadataRemoveKey(polyMDtemp, "NORDER_Y");
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial2DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial2DfromMetadata:   "
                "return NULL for metadata-polynomial with no y-order");
        }
        //Return NULL for polynomial in metadata with no x-order
        psMetadataRemoveKey(polyMDtemp, "NORDER_X");
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial2DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial2DfromMetadata:   "
                "return NULL for metadata-polynomial with no x-order");
        }

        psFree(outPoly);
        psFree(p2d);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // psPolynomial3DtoMetadata & psPolynomial3DfromMetadata functions
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        psPolynomial3D *p3d = psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, 1, 1, 1);
        p3d->coeff[0][0][0] = 1.1;
        p3d->coeff[1][0][0] = 2.2;
        p3d->coeff[0][1][0] = 3.3;
        p3d->coeff[0][0][1] = 4.4;
        p3d->coeff[1][1][0] = 5.5;
        p3d->coeff[1][0][1] = 6.6;
        p3d->coeff[0][1][1] = 7.7;
        p3d->coeff[1][1][1] = 8.8;
        p3d->coeffErr[0][0][0] = 0.1;
        p3d->coeffErr[1][0][0] = 0.2;
        p3d->coeffErr[0][1][0] = 0.3;
        p3d->coeffErr[0][0][1] = 0.4;
        p3d->coeffErr[1][1][0] = 0.5;
        p3d->coeffErr[1][0][1] = 0.6;
        p3d->coeffErr[0][1][1] = 0.7;
        p3d->coeffErr[1][1][1] = 0.8;

        //psPolynomial3DtoMetadata
        //Return a valid metadata containing a polynomial-metadata structure
        {
            ok( psPolynomial3DtoMetadata(md, p3d, "polyMD"),
                "psPolynomial3DtoMetadata:     return true for valid inputs.");
            psMetadata *polyMDtemp = psMetadataLookupMetadata(NULL, md, "polyMD");
            skip_start( polyMDtemp == NULL , 1,
                        "Skipping 1 tests because psPolynomial3DtoMetadata has errors");
            psMetadataItem *polyItem = psMetadataGet(polyMDtemp, 0);
            skip_start( polyItem == NULL, 2,
                        "Skipping 3 tests because psPolynomial3DtoMetadata has errors in order elements");
            ok( !strncmp(polyItem->name, "NORDER_X", 10) && polyItem->data.S32 == 1,
                "psPolynomial3DtoMetadata:     return correct number of x orders.");
            polyItem = psMetadataGet(polyMDtemp, 1);
            ok( !strncmp(polyItem->name, "NORDER_Y", 10) && polyItem->data.S32 == 1,
                "psPolynomial3DtoMetadata:     return correct number of y orders.");
            polyItem = psMetadataGet(polyMDtemp, 2);
            ok( !strncmp(polyItem->name, "NORDER_Z", 10) && polyItem->data.S32 == 1,
                "psPolynomial3DtoMetadata:     return correct number of z orders.");
            skip_end();
            polyItem = psMetadataGet(polyMDtemp, 3);
            skip_start( polyItem == NULL, 2,
                        "Skipping 2 tests because psPolynomial3DtoMetadata has errors in coeff elements");
            ok( !strncmp(polyItem->name, "VAL_X00_Y00_Z00", 14) &&
                abs(polyItem->data.F64-1.1) < DBL_EPSILON,
                "psPolynomial3DtoMetadata:     return correct first element.");
            polyItem = psMetadataGet(polyMDtemp, PS_LIST_TAIL-1);
            ok( !strncmp(polyItem->name, "ERR_X01_Y01_Z01", 14) &&
                abs(polyItem->data.F64-0.8) < DBL_EPSILON,
                "psPolynomial3DtoMetadata:     return correct last element.");
            skip_end();
            skip_end();
        }
        //Return false for no-name polynomial
        {
            ok( !psPolynomial3DtoMetadata(md, p3d, " "),
                "psPolynomial3DtoMetadata:     return false for no-name.");
        }
        //Return false for NULL-name polynomial
#if 0 // Caught by compiler
        {
            ok( !psPolynomial3DtoMetadata(md, p3d, NULL),
                "psPolynomial3DtoMetadata:     return false for NULL name input.");
        }
#endif
        //Return false for NULL metadata input
        {
            ok( !psPolynomial3DtoMetadata(NULL, p3d, "polyMD"),
                "psPolynomial3DtoMetadata:     return false for NULL metadata input.");
        }
        //Return false for NULL polynomial input
        {
            ok( !psPolynomial3DtoMetadata(md, NULL, "polyMD"),
                "psPolynomial3DtoMetadata:     return false for NULL polynomial input.");
        }
        /*    //Return false for empty polynomial
            {
                psPolynomial3D *emptyPoly = psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, 0, 0, 0);
                ok( !psPolynomial3DtoMetadata(md, emptyPoly, "polyMD"),
                    "psPolynomial3DtoMetadata:     return false for empty polynomial input.");
                psFree(emptyPoly);
            }
        */
        //Return true for polynomial with 1 element, a constant
        {
            psPolynomial3D *constPoly = psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, 0, 0, 0);
            constPoly->coeff[0][0][0] = 6.66;
            ok( psPolynomial3DtoMetadata(md, constPoly, "polyMD"),
                "psPolynomial3DtoMetadata:     return true for constant polynomial (1 element != 0).");
            psFree(constPoly);
        }
        //Return false for non-ordinary polynomial
        {
            psPolynomial3D *p3d2 = psPolynomial3DAlloc(PS_POLYNOMIAL_CHEB, 1, 1, 1);
            p3d2->coeff[0][0][0] = 1.1;
            ok( !psPolynomial3DtoMetadata(md, p3d2, "polyMD"),
                "psPolynomial3DtoMetadata:     return false for chebyshev polynomial");
            psFree(p3d2);
        }

        //psPolynomial3DfromMetadata Tests
        //Return NULL for NULL metadata input.
        {
            psPolynomial3D *emptyPoly2D = NULL;
            emptyPoly2D = psPolynomial3DfromMetadata(NULL);
            ok( emptyPoly2D == NULL,
                "psPolynomial3DfromMetadata:   return NULL for NULL metadata input.");
        }
        //Get a polynomial from Metadata.
        psPolynomial3D *outPoly = NULL;
        psMetadata *polyMDtemp = psMetadataLookupMetadata(NULL, md, "polyMD");
        outPoly = psPolynomial3DfromMetadata(polyMDtemp);
        {
            skip_start( outPoly == NULL, 1,
                        "Skipping 1 tests because psPolynomial3DfromMetadata has errors.");
            ok( outPoly->type == PS_POLYNOMIAL_ORD && outPoly->nX == 1 &&
                outPoly->nY == 1 && outPoly->nZ == 1 &&
                abs(outPoly->coeff[0][0][0] - 1.1) < DBL_EPSILON,
                "psPolynomial3DfromMetadata:   return correct polynomial from metadata");
            skip_end();
        }

        //Get a polynomial from Metadata with mask[0][0][0] == 1.
        psFree(outPoly);
        outPoly = NULL;
        psPolynomial3D *maskPoly = psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, 0, 0, 0);
        maskPoly->coeff[0][0][0] = 6.66;
        maskPoly->coeffMask[0][0][0] = 1;
        psMetadata *newmd = psMetadataAlloc();
        psPolynomial3DtoMetadata(newmd, maskPoly, "polyMD");
        psMetadata *polyMask = psMetadataLookupMetadata(NULL, newmd, "polyMD");
        outPoly = psPolynomial3DfromMetadata(polyMask);
        {
            ok( outPoly->type == PS_POLYNOMIAL_ORD && outPoly->nX == 0 && outPoly->nY == 0
                && outPoly->nZ == 0 && outPoly->coeffMask[0][0][0] == 1,
                "psPolynomial3DfromMetadata:   return correct polynomial (w/mask) from metadata");
        }
        psFree(maskPoly);
        psFree(newmd);
        //Return NULL for wrong NELEMENTS
        psMetadataItem *nElementItem = psMetadataGet(polyMDtemp, PS_LIST_TAIL);
        nElementItem->data.S32 -= 1;
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial3DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial3DfromMetadata:   "
                "return NULL for metadata-polynomial with wrong NELEMENTS");
        }
        //Return NULL for missing NELEMENTS
        psMetadataRemoveIndex(polyMDtemp, PS_LIST_TAIL);
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial3DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial3DfromMetadata:   "
                "return NULL for metadata-polynomial with no NELEMENTS");
        }

        //Return NULL for polynomial in metadata with no z-order
        psMetadataRemoveKey(polyMDtemp, "NORDER_Z");
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial3DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial3DfromMetadata:   return NULL for metadata-polynomial with no z-order");
        }
        //Return NULL for polynomial in metadata with no y-order
        psMetadataRemoveKey(polyMDtemp, "NORDER_Y");
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial3DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial3DfromMetadata:   return NULL for metadata-polynomial with no y-order");
        }
        //Return NULL for polynomial in metadata with no x-order
        psMetadataRemoveKey(polyMDtemp, "NORDER_X");
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial3DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial3DfromMetadata:   return NULL for metadata-polynomial with no x-order");
        }

        psFree(outPoly);
        psFree(p3d);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // psPolynomial4DtoMetadata & psPolynomial4DfromMetadata functions
    {
        psMemId id = psMemGetId();
        psMetadata *md = psMetadataAlloc();
        psPolynomial4D *p4d = psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, 1, 1, 1, 1);
        p4d->coeff[0][0][0][0] = 1.1;
        p4d->coeff[0][0][0][1] = 2.2;
        p4d->coeff[0][0][1][0] = 3.3;
        p4d->coeff[0][1][0][0] = 4.4;
        p4d->coeff[1][0][0][0] = 5.5;

        p4d->coeff[0][0][1][1] = 6.6;
        p4d->coeff[0][1][0][1] = 7.7;
        p4d->coeff[1][0][0][1] = 8.8;
        p4d->coeff[0][1][1][0] = 9.9;
        p4d->coeff[1][0][1][0] = 10.10;
        p4d->coeff[1][1][0][0] = 11.11;

        p4d->coeff[0][1][1][1] = 12.12;
        p4d->coeff[1][0][1][1] = 13.13;
        p4d->coeff[1][1][0][1] = 14.14;
        p4d->coeff[1][1][1][0] = 15.15;

        p4d->coeff[1][1][1][1] = 16.16;

        p4d->coeffErr[0][0][0][0] = 0.1;
        p4d->coeffErr[0][0][0][1] = 0.2;
        p4d->coeffErr[0][0][1][0] = 0.3;
        p4d->coeffErr[0][1][0][0] = 0.4;
        p4d->coeffErr[1][0][0][0] = 0.5;

        p4d->coeffErr[0][0][1][1] = 0.6;
        p4d->coeffErr[0][1][0][1] = 0.7;
        p4d->coeffErr[1][0][0][1] = 0.8;
        p4d->coeffErr[0][1][1][0] = 0.9;
        p4d->coeffErr[1][0][1][0] = 0.10;
        p4d->coeffErr[1][1][0][0] = 0.11;

        p4d->coeffErr[0][1][1][1] = 0.12;
        p4d->coeffErr[1][0][1][1] = 0.13;
        p4d->coeffErr[1][1][0][1] = 0.14;
        p4d->coeffErr[1][1][1][0] = 0.15;

        p4d->coeffErr[1][1][1][1] = 0.16;

        //psPolynomial4DtoMetadata
        //Return a valid metadata containing a polynomial-metadata structure
        {
            ok( psPolynomial4DtoMetadata(md, p4d, "polyMD"),
                "psPolynomial4DtoMetadata:     return true for valid inputs.");
            psMetadata *polyMDtemp = psMetadataLookupMetadata(NULL, md, "polyMD");
            skip_start( polyMDtemp == NULL , 1,
                        "Skipping 1 tests because psPolynomial4DtoMetadata has errors");
            psMetadataItem *polyItem = psMetadataGet(polyMDtemp, 0);
            skip_start( polyItem == NULL, 2,
                        "Skipping 4 tests because psPolynomial4DtoMetadata has errors in order elements");
            ok( !strncmp(polyItem->name, "NORDER_X", 10) && polyItem->data.S32 == 1,
                "psPolynomial4DtoMetadata:     return correct number of x orders.");
            polyItem = psMetadataGet(polyMDtemp, 1);
            ok( !strncmp(polyItem->name, "NORDER_Y", 10) && polyItem->data.S32 == 1,
                "psPolynomial4DtoMetadata:     return correct number of y orders.");
            polyItem = psMetadataGet(polyMDtemp, 2);
            ok( !strncmp(polyItem->name, "NORDER_Z", 10) && polyItem->data.S32 == 1,
                "psPolynomial4DtoMetadata:     return correct number of z orders.");
            polyItem = psMetadataGet(polyMDtemp, 3);
            ok( !strncmp(polyItem->name, "NORDER_T", 10) && polyItem->data.S32 == 1,
                "psPolynomial4DtoMetadata:     return correct number of t orders.");
            skip_end();
            polyItem = psMetadataGet(polyMDtemp, 4);
            skip_start( polyItem == NULL, 2,
                        "Skipping 2 tests because psPolynomial4DtoMetadata has errors in coeff elements");
            ok( !strncmp(polyItem->name, "VAL_X00_Y00_Z00_T00", 14) &&
                abs(polyItem->data.F64-1.1) < DBL_EPSILON,
                "psPolynomial4DtoMetadata:     return correct first element.");
            polyItem = psMetadataGet(polyMDtemp, PS_LIST_TAIL-1);
            ok( !strncmp(polyItem->name, "ERR_X01_Y01_Z01_T01", 14) &&
                abs(polyItem->data.F64-0.16) < DBL_EPSILON,
                "psPolynomial4DtoMetadata:     return correct last element.");
            skip_end();
            skip_end();
        }
        //Return false for no-name polynomial
        {
            ok( !psPolynomial4DtoMetadata(md, p4d, " "),
                "psPolynomial4DtoMetadata:     return false for no-name.");
        }
        //Return false for NULL-name polynomial
#if 0 // Caught by compiler
        {
            ok( !psPolynomial4DtoMetadata(md, p4d, NULL),
                "psPolynomial4DtoMetadata:     return false for NULL name input.");
        }
#endif
        //Return false for NULL metadata input
        {
            ok( !psPolynomial4DtoMetadata(NULL, p4d, "polyMD"),
                "psPolynomial4DtoMetadata:     return false for NULL metadata input.");
        }
        //Return false for NULL polynomial input
        {
            ok( !psPolynomial4DtoMetadata(md, NULL, "polyMD"),
                "psPolynomial4DtoMetadata:     return false for NULL polynomial input.");
        }
        /*    //Return false for empty polynomial
            {
                psPolynomial4D *emptyPoly = psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, 0, 0, 0, 0);
                ok( !psPolynomial4DtoMetadata(md, emptyPoly, "polyMD"),
                    "psPolynomial4DtoMetadata:     return false for empty polynomial input.");
                psFree(emptyPoly);
            }
        */
        //Return true for polynomial with 1 element, a constant
        {
            psPolynomial4D *constPoly = psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, 0, 0, 0, 0);
            constPoly->coeff[0][0][0][0] = 6.66;
            ok( psPolynomial4DtoMetadata(md, constPoly, "polyMD"),
                "psPolynomial4DtoMetadata:     return true for constant polynomial (1 element != 0).");
            psFree(constPoly);
        }
        //Return false for non-ordinary polynomial
        {
            psPolynomial4D *p4d2 = psPolynomial4DAlloc(PS_POLYNOMIAL_CHEB, 1, 1, 1, 1);
            p4d2->coeff[0][0][0][0] = 1.1;
            ok( !psPolynomial4DtoMetadata(md, p4d2, "polyMD"),
                "psPolynomial4DtoMetadata:     return false for chebyshev polynomial");
            psFree(p4d2);
        }

        //psPolynomial4DfromMetadata Tests
        //Return NULL for NULL metadata input.
        {
            psPolynomial4D *emptyPoly2D = NULL;
            emptyPoly2D = psPolynomial4DfromMetadata(NULL);
            ok( emptyPoly2D == NULL,
                "psPolynomial4DfromMetadata:   return NULL for NULL metadata input.");
        }
        //Get a polynomial from Metadata.
        psPolynomial4D *outPoly = NULL;
        psMetadata *polyMDtemp = psMetadataLookupMetadata(NULL, md, "polyMD");
        outPoly = psPolynomial4DfromMetadata(polyMDtemp);
        {
            skip_start( outPoly == NULL, 1,
                        "Skipping 1 tests because psPolynomial4DfromMetadata has errors.");
            ok( outPoly->type == PS_POLYNOMIAL_ORD && outPoly->nX == 1 &&
                outPoly->nY == 1 && outPoly->nZ == 1 && outPoly->nT == 1 &&
                abs(outPoly->coeff[0][0][0][0] - 1.1) < DBL_EPSILON,
                "psPolynomial4DfromMetadata:   return correct polynomial from metadata");
            skip_end();
        }
        //Get a polynomial from Metadata with mask[0][0][0][0] == 1.
        psFree(outPoly);
        outPoly = NULL;
        psPolynomial4D *maskPoly = psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, 0, 0, 0, 0);
        maskPoly->coeff[0][0][0][0] = 6.66;
        maskPoly->coeffMask[0][0][0][0] = 1;
        psMetadata *newmd = psMetadataAlloc();
        psPolynomial4DtoMetadata(newmd, maskPoly, "polyMD");
        psMetadata *polyMask = psMetadataLookupMetadata(NULL, newmd, "polyMD");
        outPoly = psPolynomial4DfromMetadata(polyMask);
        {
            ok( outPoly->type == PS_POLYNOMIAL_ORD && outPoly->nX == 0 && outPoly->nY == 0
                && outPoly->nZ == 0 && outPoly->nT == 0 && outPoly->coeffMask[0][0][0][0] == 1,
                "psPolynomial4DfromMetadata:   return correct polynomial (w/mask) from metadata");
        }
        psFree(maskPoly);
        psFree(newmd);
        //Return NULL for wrong NELEMENTS
        psMetadataItem *nElementItem = psMetadataGet(polyMDtemp, PS_LIST_TAIL);
        nElementItem->data.S32 -= 1;
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial4DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial4DfromMetadata:   "
                "return NULL for metadata-polynomial with wrong NELEMENTS");
        }
        //Return NULL for missing NELEMENTS
        psMetadataRemoveIndex(polyMDtemp, PS_LIST_TAIL);
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial4DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial4DfromMetadata:   "
                "return NULL for metadata-polynomial with no NELEMENTS");
        }

        //Return NULL for polynomial in metadata with no t-order
        psMetadataRemoveKey(polyMDtemp, "NORDER_T");
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial4DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial4DfromMetadata:   return NULL for metadata-polynomial with no t-order");
        }
        //Return NULL for polynomial in metadata with no z-order
        psMetadataRemoveKey(polyMDtemp, "NORDER_Z");
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial4DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial4DfromMetadata:   return NULL for metadata-polynomial with no z-order");
        }
        //Return NULL for polynomial in metadata with no y-order
        psMetadataRemoveKey(polyMDtemp, "NORDER_Y");
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial4DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial4DfromMetadata:   return NULL for metadata-polynomial with no y-order");
        }
        //Return NULL for polynomial in metadata with no x-order
        psMetadataRemoveKey(polyMDtemp, "NORDER_X");
        psFree(outPoly);
        outPoly = NULL;
        outPoly = psPolynomial4DfromMetadata(polyMDtemp);
        {
            ok( outPoly == NULL,
                "psPolynomial4DfromMetadata:   return NULL for metadata-polynomial with no x-order");
        }

        psFree(outPoly);
        psFree(p4d);
        psFree(md);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}




