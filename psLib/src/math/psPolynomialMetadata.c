/** @file  psPolyMetadata.c
 *
 *
 *  @brief Contains metadata structures, enumerations and functions prototypes.
 *
 *  This file defines metadata item, metadata type, metadata flags, metadata containers, and function
 *  prototypes necessary creating psLib metadata APIs
 *
 *  @ingroup Metadata
 *
 *  @author Robert DeSonia, MHPCC
 *  @author Ross Harman, MHPCC
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-05-05 00:09:04 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "psType.h"
#include "psMemory.h"
#include "psError.h"
#include "psMetadata.h"
#include "psString.h"

#include "psAssert.h"

psPolynomial1D *psPolynomial1DfromMetadata(const psMetadata *folder)
{
    PS_ASSERT_PTR_NON_NULL(folder, NULL);
    bool status;
    char keyword[80];

    // get polynomial orders
    int nXorder = psMetadataLookupS32 (&status, folder, "NORDER_X");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial1D in metadata is missing NORDER_X");
        return NULL;
    }
    // how many polynomial coeffs are expected?
    int nElementsExpected = psMetadataLookupS32 (&status, folder, "NELEMENTS");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial1D in metadata is missing NELEMENTS");
        return NULL;
    }

    psPolynomial1D *poly = psPolynomial1DAlloc (PS_POLYNOMIAL_ORD, nXorder);

    int nElements = 0;
    for (int nx = 0; nx < poly->nX + 1; nx++) {
        sprintf (keyword, "VAL_X%02d", nx);
        poly->coeff[nx] = psMetadataLookupF64 (&status, folder, keyword);
        if (!status) {
            // an undefined component implies the component was masked
            // this is symmetrical with the 1DtoMD function
            poly->coeff[nx] = 0;
            poly->coeffErr[nx] = 0;
            poly->coeffMask[nx] = PS_POLY_MASK_SET;
        } else {
            poly->coeffMask[nx] = PS_POLY_MASK_NONE;
            nElements ++;
        }
        sprintf (keyword, "ERR_X%02d", nx);
        poly->coeffErr[nx] = psMetadataLookupF64 (&status, folder, keyword);
    }
    if (nElements != nElementsExpected) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psPolynomial1D in metadata does not have the correct number of coefficients: "
                "%d found vs %d expected", nElements, nElementsExpected);
        psFree(poly);
        return NULL;
    }
    return (poly);
}

// XXX : these may need F64, or %g format for output
bool psPolynomial1DtoMetadata(psMetadata *md,
                              const psPolynomial1D *poly,
                              const char *format,
                              ...)
{
    PS_ASSERT_PTR_NON_NULL(md, false);
    PS_ASSERT_PTR_NON_NULL(poly, false);
    PS_ASSERT_PTR_NON_NULL(format, false);

    //XXX:  Current implementation only supports ordinary polynomials.
    if (poly->type != PS_POLYNOMIAL_ORD)
        return false;

    int Nbyte;
    char tmp;
    char *root;
    va_list argp;

    // skip past whitespace (output name should not include whitespace)
    char *fmt = (char *) format;
    while (isspace(*fmt)) fmt++;

    va_start (argp, format);
    Nbyte = vsnprintf (&tmp, 0, fmt, argp);
    va_end (argp);

    if (Nbyte <= 0) return false;

    va_start (argp, format);
    root = (char *) psAlloc (Nbyte + 1);
    memset (root, 0, Nbyte + 1);
    vsnprintf (root, Nbyte + 1, fmt, argp);
    va_end (argp);

    psMetadata *folder = psMetadataAlloc ();

    // specify the polynomial orders
    psMetadataAdd (folder, PS_LIST_TAIL, "NORDER_X", PS_DATA_S32, "number of x orders", poly->nX);

    char namespace[80];
    char namespace_err[80];
    int nElements = 0;   // count the number of unmasked elements

    // place polynomial entries on folder
    for (int nx = 0; nx < poly->nX + 1; nx++) {
        if (!(poly->coeffMask[nx] & PS_POLY_MASK_SET)) {
            sprintf(namespace, "VAL_X%02d", nx);
            sprintf(namespace_err, "ERR_X%02d", nx);
            psMetadataAdd (folder, PS_LIST_TAIL, namespace, PS_DATA_F64,
                           "polynomial coefficient", poly->coeff[nx]);
            psMetadataAdd (folder, PS_LIST_TAIL, namespace_err, PS_DATA_F64,
                           "polynomial coefficient error", poly->coeffErr[nx]);
            nElements ++;
        }
    }
    psMetadataAdd (folder, PS_LIST_TAIL, "NELEMENTS", PS_DATA_S32, "number of unmasked coeffs", nElements);
    psMetadataAdd (md, PS_LIST_TAIL, root, PS_DATA_METADATA, "folder for 1D polynomial", folder);
    psFree (root);
    psFree(folder);
    return true;
}

psPolynomial2D *psPolynomial2DfromMetadata(const psMetadata *folder)
{
    PS_ASSERT_PTR_NON_NULL(folder, NULL);
    bool status;
    char keyword[80];

    // get polynomial orders
    int nXorder = psMetadataLookupS32 (&status, folder, "NORDER_X");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial2D in metadata is missing NORDER_X");
        return NULL;
    }
    int nYorder = psMetadataLookupS32 (&status, folder, "NORDER_Y");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial2D in metadata is missing NORDER_Y");
        return NULL;
    }
    // how many polynomial coeffs are expected?
    int nElementsExpected = psMetadataLookupS32 (&status, folder, "NELEMENTS");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial2D in metadata is missing NELEMENTS");
        return NULL;
    }

    psPolynomial2D *poly = psPolynomial2DAlloc (PS_POLYNOMIAL_ORD, nXorder, nYorder);

    int nElements = 0;
    for (int nx = 0; nx < poly->nX + 1; nx++) {
        for (int ny = 0; ny < poly->nY + 1; ny++) {
            sprintf (keyword, "VAL_X%02d_Y%02d", nx, ny);
            poly->coeff[nx][ny] = psMetadataLookupF64 (&status, folder, keyword);
            if (!status) {
                // an undefined component implies the component was masked
                // this is symmetrical with the 2DtoMD function
                poly->coeff[nx][ny] = 0;
                poly->coeffErr[nx][ny] = 0;
                poly->coeffMask[nx][ny] = PS_POLY_MASK_SET;
            } else {
                poly->coeffMask[nx][ny] = PS_POLY_MASK_NONE;
                nElements ++;
            }
            sprintf (keyword, "ERR_X%02d_Y%02d", nx, ny);
            poly->coeffErr[nx][ny] = psMetadataLookupF64 (&status, folder, keyword);
        }
    }
    if (nElements != nElementsExpected) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psPolynomial2D in metadata does not have the correct number of coefficients: "
                "%d found vs %d expected", nElements, nElementsExpected);
        psFree(poly);
        return NULL;
    }
    return (poly);
}

// XXX : these may need F64, or %g format for output
bool psPolynomial2DtoMetadata (psMetadata *md,
                               const psPolynomial2D *poly,
                               const char *format,
                               ...)
{
    PS_ASSERT_PTR_NON_NULL(md, false);
    PS_ASSERT_PTR_NON_NULL(poly, false);
    PS_ASSERT_PTR_NON_NULL(format, false);

    // XXX Current implementation only supports ordinary polynomials.
    if (poly->type != PS_POLYNOMIAL_ORD)
        return false;

    int Nbyte;
    char tmp;
    char *root;
    va_list argp;

    // skip past whitespace (output name should not include whitespace)
    char *fmt = (char *) format;
    while (isspace(*fmt)) fmt++;

    va_start (argp, format);
    Nbyte = vsnprintf (&tmp, 0, fmt, argp);
    va_end (argp);

    if (Nbyte <= 0)
        return false;

    va_start (argp, format);
    root = (char *) psAlloc (Nbyte + 1);
    memset (root, 0, Nbyte + 1);
    vsnprintf (root, Nbyte + 1, fmt, argp);
    va_end (argp);

    psMetadata *folder = psMetadataAlloc ();

    // specify the polynomial orders
    psMetadataAdd (folder, PS_LIST_TAIL, "NORDER_X", PS_DATA_S32, "number of x orders", poly->nX);
    psMetadataAdd (folder, PS_LIST_TAIL, "NORDER_Y", PS_DATA_S32, "number of y orders", poly->nY);

    char namespace[80];
    char namespace_err[80];
    int nElements = 0;   // count the number of unmasked elements

    // place polynomial entries on folder
    for (int nx = 0; nx < poly->nX + 1; nx++) {
        for (int ny = 0; ny < poly->nY + 1; ny++) {
            if (!(poly->coeffMask[nx][ny] & PS_POLY_MASK_SET)) {
                sprintf(namespace, "VAL_X%02d_Y%02d", nx, ny);
                sprintf(namespace_err, "ERR_X%02d_Y%02d", nx, ny);
                psMetadataAdd (folder, PS_LIST_TAIL, namespace, PS_DATA_F64,
                               "polynomial coefficient", poly->coeff[nx][ny]);
                psMetadataAdd (folder, PS_LIST_TAIL, namespace_err, PS_DATA_F64,
                               "polynomial coefficient error", poly->coeffErr[nx][ny]);
                nElements ++;
            }
        }
    }
    psMetadataAdd (folder, PS_LIST_TAIL, "NELEMENTS", PS_DATA_S32, "number of unmasked coeffs", nElements);
    psMetadataAdd (md, PS_LIST_TAIL, root, PS_DATA_METADATA, "folder for 2D polynomial", folder);
    psFree (root);
    psFree(folder);
    return true;
}

psPolynomial3D *psPolynomial3DfromMetadata (const psMetadata *folder)
{
    PS_ASSERT_PTR_NON_NULL(folder, NULL);

    bool status;
    char keyword[80];

    // get polynomial orders
    int nXorder = psMetadataLookupS32 (&status, folder, "NORDER_X");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial3D in metadata is missing NORDER_X");
        return NULL;
    }
    int nYorder = psMetadataLookupS32 (&status, folder, "NORDER_Y");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial3D in metadata is missing NORDER_Y");
        return NULL;
    }
    int nZorder = psMetadataLookupS32 (&status, folder, "NORDER_Z");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial3D in metadata is missing NORDER_Z");
        return NULL;
    }
    // how many polynomial coeffs are expected?
    int nElementsExpected = psMetadataLookupS32 (&status, folder, "NELEMENTS");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial3D in metadata is missing NELEMENTS");
        return NULL;
    }

    psPolynomial3D *poly = psPolynomial3DAlloc (PS_POLYNOMIAL_ORD, nXorder, nYorder, nZorder);

    int nElements = 0;
    for (int nx = 0; nx < poly->nX + 1; nx++) {
        for (int ny = 0; ny < poly->nY + 1; ny++) {
            for (int nz = 0; nz < poly->nZ + 1; nz++) {
                sprintf (keyword, "VAL_X%02d_Y%02d_Z%02d", nx, ny, nz);
                poly->coeff[nx][ny][nz] = psMetadataLookupF64 (&status, folder, keyword);
                if (!status) {
                    // an undefined component implies the component was masked
                    // this is symmetrical with the 3DtoMD function
                    poly->coeff[nx][ny][nz] = 0;
                    poly->coeffErr[nx][ny][nz] = 0;
                    poly->coeffMask[nx][ny][nz] = PS_POLY_MASK_SET;
                } else {
                    poly->coeffMask[nx][ny][nz] = PS_POLY_MASK_NONE;
                    nElements ++;
                }
                sprintf (keyword, "ERR_X%02d_Y%02d_Z%02d", nx, ny, nz);
                poly->coeffErr[nx][ny][nz] = psMetadataLookupF64 (&status, folder, keyword);
            }
        }
    }
    if (nElements != nElementsExpected) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psPolynomial3D in metadata does not have the correct number of coefficients: "
                "%d found vs %d expected", nElements, nElementsExpected);
        psFree(poly);
        return NULL;
    }
    return (poly);
}

bool psPolynomial3DtoMetadata (psMetadata *md,
                               const psPolynomial3D *poly,
                               const char *format,
                               ...)
{
    PS_ASSERT_PTR_NON_NULL(md, false);
    PS_ASSERT_PTR_NON_NULL(poly, false);
    PS_ASSERT_PTR_NON_NULL(format, false);

    //XXX:  Current implementation only supports ordinary polynomials.
    if (poly->type != PS_POLYNOMIAL_ORD)
        return false;

    int Nbyte;
    char tmp;
    char *root;
    va_list argp;

    // skip past whitespace (output name should not include whitespace)
    char *fmt = (char *) format;
    while (isspace(*fmt)) fmt++;

    va_start (argp, format);
    Nbyte = vsnprintf (&tmp, 0, fmt, argp);
    va_end (argp);

    if (Nbyte <= 0)
        return false;

    va_start (argp, format);
    root = (char *) psAlloc (Nbyte + 1);
    memset (root, 0, Nbyte + 1);
    vsnprintf (root, Nbyte + 1, fmt, argp);
    va_end (argp);

    psMetadata *folder = psMetadataAlloc ();

    // specify the polynomial orders
    psMetadataAdd (folder, PS_LIST_TAIL, "NORDER_X", PS_DATA_S32, "number of x orders", poly->nX);
    psMetadataAdd (folder, PS_LIST_TAIL, "NORDER_Y", PS_DATA_S32, "number of y orders", poly->nY);
    psMetadataAdd (folder, PS_LIST_TAIL, "NORDER_Z", PS_DATA_S32, "number of z orders", poly->nZ);

    char namespace[80];
    char namespace_err[80];
    int nElements = 0;   // count the number of unmasked elements

    // place polynomial entries on folder
    for (int nx = 0; nx < poly->nX + 1; nx++) {
        for (int ny = 0; ny < poly->nY + 1; ny++) {
            for (int nz = 0; nz < poly->nZ + 1; nz++) {
                if (!(poly->coeffMask[nx][ny][nz] & PS_POLY_MASK_SET)) {
                    sprintf(namespace, "VAL_X%02d_Y%02d_Z%02d", nx, ny, nz);
                    sprintf(namespace_err, "ERR_X%02d_Y%02d_Z%02d", nx, ny, nz);
                    psMetadataAdd (folder, PS_LIST_TAIL, namespace,
                                   PS_DATA_F64, "polynomial coefficient",
                                   poly->coeff[nx][ny][nz], nx, ny, nz);
                    psMetadataAdd (folder, PS_LIST_TAIL, namespace_err,
                                   PS_DATA_F64, "polynomial coeffficient error",
                                   poly->coeffErr[nx][ny][nz], nx, ny, nz);
                    nElements ++;
                }
            }
        }
    }
    psMetadataAdd (folder, PS_LIST_TAIL, "NELEMENTS", PS_DATA_S32, "number of unmasked coeffs", nElements);
    psMetadataAdd (md, PS_LIST_TAIL, root, PS_DATA_METADATA, "folder for 3D polynomial", folder);
    psFree(root);
    psFree(folder);
    return true;
}

psPolynomial4D *psPolynomial4DfromMetadata(const psMetadata *folder)
{
    PS_ASSERT_PTR_NON_NULL(folder, NULL);

    bool status;
    char keyword[80];

    int nXorder = psMetadataLookupS32 (&status, folder, "NORDER_X");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial4D in metadata is missing NORDER_X");
        return NULL;
    }
    int nYorder = psMetadataLookupS32 (&status, folder, "NORDER_Y");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial4D in metadata is missing NORDER_Y");
        return NULL;
    }
    int nZorder = psMetadataLookupS32 (&status, folder, "NORDER_Z");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial4D in metadata is missing NORDER_Z");
        return NULL;
    }
    int nTorder = psMetadataLookupS32 (&status, folder, "NORDER_T");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial4D in metadata is missing NORDER_T");
        return NULL;
    }
    // how many polynomial coeffs are expected?
    int nElementsExpected = psMetadataLookupS32 (&status, folder, "NELEMENTS");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "psPolynomial4D in metadata is missing NELEMENTS");
        return NULL;
    }

    psPolynomial4D *poly = psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, nXorder, nYorder, nZorder, nTorder);

    int nElements = 0;
    for (int nx = 0; nx < poly->nX + 1; nx++) {
        for (int ny = 0; ny < poly->nY + 1; ny++) {
            for (int nz = 0; nz < poly->nZ + 1; nz++) {
                for (int nt = 0; nt < poly->nT + 1; nt++) {
                    sprintf (keyword, "VAL_X%02d_Y%02d_Z%02d_T%02d", nx, ny, nz, nt);
                    poly->coeff[nx][ny][nz][nt] = psMetadataLookupF64 (&status, folder, keyword);
                    if (!status) {
                        // an undefined component implies the component was masked
                        // this is symmetrical with the 4DtoMD function
                        poly->coeff[nx][ny][nz][nt] = 0;
                        poly->coeffErr[nx][ny][nz][nt] = 0;
                        poly->coeffMask[nx][ny][nz][nt] = PS_POLY_MASK_SET;
                    } else {
                        poly->coeffMask[nx][ny][nz][nt] = PS_POLY_MASK_NONE;
                        nElements ++;
                    }
                    sprintf (keyword, "ERR_X%02d_Y%02d_Z%02d_T%02d", nx, ny, nz, nt);
                    poly->coeffErr[nx][ny][nz][nt] = psMetadataLookupF64 (&status, folder, keyword);
                }
            }
        }
    }
    if (nElements != nElementsExpected) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psPolynomial4D in metadata does not have the correct number of coefficients: "
                "%d found vs %d expected", nElements, nElementsExpected);
        psFree(poly);
        return NULL;
    }
    return (poly);
}

bool psPolynomial4DtoMetadata (psMetadata *md,
                               const psPolynomial4D *poly,
                               const char *format,
                               ...)
{
    PS_ASSERT_PTR_NON_NULL(md, false);
    PS_ASSERT_PTR_NON_NULL(poly, false);
    PS_ASSERT_PTR_NON_NULL(format, false);

    //XXX:  Current implementation only supports ordinary polynomials.
    if (poly->type != PS_POLYNOMIAL_ORD)
        return false;

    int Nbyte;
    char tmp;
    char *root;
    va_list argp;

    // skip past whitespace (output name should not include whitespace)
    char *fmt = (char *) format;
    while (isspace(*fmt)) fmt++;

    va_start (argp, format);
    Nbyte = vsnprintf (&tmp, 0, fmt, argp);
    va_end (argp);

    if (Nbyte <= 0)
        return false;

    va_start (argp, format);
    root = (char *) psAlloc (Nbyte + 1);
    memset (root, 0, Nbyte + 1);
    vsnprintf (root, Nbyte + 1, fmt, argp);
    va_end (argp);

    psMetadata *folder = psMetadataAlloc ();

    // specify the polynomial orders
    psMetadataAdd (folder, PS_LIST_TAIL, "NORDER_X", PS_DATA_S32, "number of x orders", poly->nX);
    psMetadataAdd (folder, PS_LIST_TAIL, "NORDER_Y", PS_DATA_S32, "number of y orders", poly->nY);
    psMetadataAdd (folder, PS_LIST_TAIL, "NORDER_Z", PS_DATA_S32, "number of z orders", poly->nZ);
    psMetadataAdd (folder, PS_LIST_TAIL, "NORDER_T", PS_DATA_S32, "number of t orders", poly->nT);

    char namespace[80];
    char namespace_err[80];
    int nElements = 0;   // count the number of unmasked elements

    // place polynomial entries on folder
    for (int nx = 0; nx < poly->nX + 1; nx++) {
        for (int ny = 0; ny < poly->nY + 1; ny++) {
            for (int nz = 0; nz < poly->nZ + 1; nz++) {
                for (int nt = 0; nt < poly->nT + 1; nt++) {
                    if (!(poly->coeffMask[nx][ny][nz][nt] & PS_POLY_MASK_SET)) {
                        sprintf(namespace, "VAL_X%02d_Y%02d_Z%02d_T%02d", nx, ny, nz, nt);
                        sprintf(namespace_err, "ERR_X%02d_Y%02d_Z%02d_T%02d", nx, ny, nz, nt);
                        psMetadataAdd (folder, PS_LIST_TAIL, namespace,
                                       PS_DATA_F64, "polynomial coefficient",
                                       poly->coeff[nx][ny][nz][nt], nx, ny, nz, nt);
                        psMetadataAdd (folder, PS_LIST_TAIL, namespace_err,
                                       PS_DATA_F64, "polynomial coeffficient error",
                                       poly->coeffErr[nx][ny][nz][nt], nx, ny, nz, nt);
                        nElements ++;
                    }
                }
            }
        }
    }
    psMetadataAdd (folder, PS_LIST_TAIL, "NELEMENTS", PS_DATA_S32, "number of unmasked coeffs", nElements);
    psMetadataAdd (md, PS_LIST_TAIL, root, PS_DATA_METADATA, "folder for 4D polynomial", folder);
    psFree(root);
    psFree(folder);
    return true;
}
