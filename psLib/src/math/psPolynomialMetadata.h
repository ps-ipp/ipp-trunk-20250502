/* @file  psPolyMetdata.h
 * @brief Standard Mathematical Functions.
 *
 * This file will hold the prototypes for procedures which allocate, free,
 * and evaluate various polynomials.  Those polynomial structures are also
 * defined here.
 *
 * @author GLG, MHPCC
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-08-09 01:40:07 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_POLYMETADATA_H
#define PS_POLYMETADATA_H

/// @addtogroup MathOps Mathematical Operations
/// @{

/** Allocates a new psPolynomial1D structure with information from a psMetadata.
 *
 *  Parses a psMetadata container with psPolynomial1D information.  The first two
 *  elements of the metadata folder specify the order of the x & y variables.  (ie,
 *  NORDER_X, NORDER_Y).  The following elements are the values of the coefficients
 *  and the coefficient errors.  (ie, VAL_X00_Y00, ERR_X00_Y00, etc.).  If the orders
 *  or any coefficients are missing or have incorrect syntax, NULL is returned.
 *
 *  @return psPolynomial1D*:        Newly allocated psPolynomial1D from metadata.
 */
psPolynomial1D *psPolynomial1DfromMetadata(
    const psMetadata *folder                 ///< folder containing the polynomial info.
);

/** Stores the information from a psPolynomial1D structure in a psMetadata container.
 *
 *  Creates a psMetadata folder with psPolynomial1D information.  The first two
 *  elements of the metadata folder specify the order of the x & y variables.  (ie,
 *  NORDER_X, NORDER_Y).  The following elements are the values of the coefficients
 *  and the coefficient errors.  (ie, VAL_X00_Y00, ERR_X00_Y00, etc.).  The input
 *  polynomial must be of ordinary type and have a valid name format.  False is also
 *  returned if any inputs are NULL.  *If a particular mask element is non-zero, that
 *  polynomial coefficient (and error) are skipped.
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool psPolynomial1DtoMetadata(
    psMetadata *md,                    ///< Metadata container for polynomial storage.
    const psPolynomial1D *poly,        ///< Polynomial information to be stored.
    const char *format,                ///< Name of polynomial folder.
    ...                                ///< Arguments for name formatting.
) PS_ATTR_FORMAT(printf,3,4);

/** Allocates a new psPolynomial2D structure with information from a psMetadata.
 *
 *  Parses a psMetadata container with psPolynomial2D information.  The first two
 *  elements of the metadata folder specify the order of the x & y variables.  (ie,
 *  NORDER_X, NORDER_Y).  The following elements are the values of the coefficients
 *  and the coefficient errors.  (ie, VAL_X00_Y00, ERR_X00_Y00, etc.).  If the orders
 *  or any coefficients are missing or have incorrect syntax, NULL is returned.
 *
 *  @return psPolynomial2D*:        Newly allocated psPolynomial2D from metadata.
 */
psPolynomial2D *psPolynomial2DfromMetadata(
    const psMetadata *folder                 ///< folder containing the polynomial info.
);

/** Stores the information from a psPolynomial2D structure in a psMetadata container.
 *
 *  Creates a psMetadata folder with psPolynomial2D information.  The first two
 *  elements of the metadata folder specify the order of the x & y variables.  (ie,
 *  NORDER_X, NORDER_Y).  The following elements are the values of the coefficients
 *  and the coefficient errors.  (ie, VAL_X00_Y00, ERR_X00_Y00, etc.).  The input
 *  polynomial must be of ordinary type and have a valid name format.  False is also
 *  returned if any inputs are NULL.  *If a particular mask element is non-zero, that
 *  polynomial coefficient (and error) are skipped.
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool psPolynomial2DtoMetadata(
    psMetadata *md,                    ///< Metadata container for polynomial storage.
    const psPolynomial2D *poly,              ///< Polynomial information to be stored.
    const char *format,                      ///< Name of polynomial folder.
    ...                                ///< Arguments for name formatting.
) PS_ATTR_FORMAT(printf,3,4);

/** Allocates a new psPolynomial3D structure with information from a psMetadata.
 *
 *  Parses a psMetadata container with psPolynomial3D information.  The first three
 *  elements of the metadata folder specify the order of the x, y, & z variables.  (ie,
 *  NORDER_X, NORDER_Y, NORDER_Z).  The following elements are the values of the
 *  coefficients and the coefficient errors.  (ie, VAL_X00_Y00_Z00, ERR_X00_Y00_Z00,
 *  etc.).  If the orders or any coefficients are missing or have incorrect syntax,
 *  NULL is returned.
 *
 *  @return psPolynomial3D*:        Newly allocated psPolynomial3D from metadata.
 */
psPolynomial3D *psPolynomial3DfromMetadata(
    const psMetadata *folder                 ///< folder containing the polynomial info.
);

/** Stores the information from a psPolynomial3D structure in a psMetadata container.
 *
 *  Creates a psMetadata folder with psPolynomial3D information.  The first three
 *  elements of the metadata folder specify the order of the x, y, & z variables.  (ie,
 *  NORDER_X, NORDER_Y, NORDER_Z).  The following elements are the values of the
 *  coefficients and the coefficient errors.  (ie, VAL_X00_Y00_Z00, ERR_X00_Y00_Z00,
 *  etc.).  The input polynomial must be of ordinary type and have a valid name format.
 *  False is also returned if any inputs are NULL.  *If a particular mask element is
 *  non-zero, that polynomial coefficient (and error) are skipped.
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool psPolynomial3DtoMetadata(
    psMetadata *md,                    ///< Metadata container for polynomial storage.
    const psPolynomial3D *poly,              ///< Polynomial information to be stored.
    const char *format,                      ///< Name of polynomial folder.
    ...                                ///< Arguments for name formatting.
) PS_ATTR_FORMAT(printf, 3, 4);

/** Allocates a new psPolynomial4D structure with information from a psMetadata.
 *
 *  Parses a psMetadata container with psPolynomial4D information.  The first four
 *  elements of the metadata folder specify the order of the x, y, z, & t variables.
 *  (ie, NORDER_X, NORDER_Y, NORDER_Z, NORDER_T).  The following elements are the
 *  values of the coefficients and the coefficient errors.  (ie, VAL_X00_Y00_Z00_T00,
 *  ERR_X00_Y00_Z00_T00, etc.).  If the orders or any coefficients are missing or
 *  have incorrect syntax, NULL is returned.
 *
 *  @return psPolynomial4D*:        Newly allocated psPolynomial4D from metadata.
 */
psPolynomial4D *psPolynomial4DfromMetadata(
    const psMetadata *folder                 ///< folder containing the polynomial info.
);

/** Stores the information from a psPolynomial4D structure in a psMetadata container.
 *
 *  Creates a psMetadata folder with psPolynomial4D information.  The first four
 *  elements of the metadata folder specify the order of the x, y, z, & t variables.
 *  (ie, NORDER_X, NORDER_Y, NORDER_Z, NORDER_T).  The following elements are the values
 *  of the coefficients and the coefficient errors.  (ie, VAL_X00_Y00_Z00_T00,
 *  ERR_X00_Y00_Z00_T00, etc.).  The input polynomial must be of ordinary type and have
 *  a valid name format.  False is also returned if any inputs are NULL.  *If a particular
 *  mask element is non-zero, that polynomial coefficient (and error) are skipped.
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool psPolynomial4DtoMetadata(
    psMetadata *md,                    ///< Metadata container for polynomial storage.
    const psPolynomial4D *poly,              ///< Polynomial information to be stored.
    const char *format,                      ///< Name of polynomial folder.
    ...                                ///< Arguments for name formatting.
) PS_ATTR_FORMAT(printf, 3, 4);

/// @}
#endif // #ifndef PS_POLYMETADATA_H
