/* @file  psFitsTable.h
 * @brief Contains Fits I/O routines
 *
 * @author EAM, PAP, JH
 * @author Robert DeSonia, MHPCC
 *
 * @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-10-09 02:56:23 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_FITSTABLE_H
#define PS_FITSTABLE_H

/// @addtogroup FileIO Input/Output
/// @{

#include "psFits.h"

#include "psType.h"
#include "psArray.h"
#include "psVector.h"
#include "psMetadata.h"
#include "psImage.h"

/// Return the number of rows in the current table
///
/// The current HDU type must be either PS_FITS_TYPE_BINARY_TABLE or PS_FITS_TYPE_ASCII_TABLE.
long psFitsTableSize(const psFits *fits ///< FITS file
                     );

/** Reads a table row.  The current HDU type must be either
 *  PS_FITS_TYPE_BINARY_TABLE or PS_FITS_TYPE_ASCII_TABLE.
 *
 *  @return psMetadata*    The table row's data.  The keys are the column names.
 */
psMetadata* psFitsReadTableRow(
    const psFits* fits,                ///< the psFits object
    int row                            ///< row number to read
);

/** Reads a table column.  The current HDU type must be either
 *  PS_FITS_TYPE_BINARY_TABLE or PS_FITS_TYPE_ASCII_TABLE.
 *
 *  @return psArray*    Array of data items for the specified column or NULL
 *                      if an error occurred.
 */
psArray* psFitsReadTableColumn(
    const psFits* fits,                ///< the psFits object
    const char* colname                ///< the column name
);

/** Reads a table column of numbers.  The current HDU type must be either
 *  PS_FITS_TYPE_BINARY_TABLE or PS_FITS_TYPE_ASCII_TABLE.
 *
 *  @return psVector*    Vector of data for the specified column or NULL
 *                       if an error occurred.
 */
psVector* psFitsReadTableColumnNum(
    const psFits* fits,                ///< the psFits object
    const char* colname                ///< the column name
);

/** Read all table columns.
 *
 * String columns are read into arrays, number columns are read into vectors.
 */
psMetadata *psFitsReadTableAllColumns(const psFits *fits // FITS file pointer
                                      );

/** Write all table columns.
 *
 * Uses the same format as for psFitsReadTableAllColumns.
 */
bool psFitsWriteTableAllColumns(
                                psFits *fits, // FITS file pointer
                                psMetadata *header, // Header to write, or NULL
                                const psMetadata *table, // Table to write
                                const char *extname      // Extension name, or NULL
                                );

/** Reads a whole FITS table.  The current HDU type must be either
 *  PS_FITS_TYPE_BINARY_TABLE or PS_FITS_TYPE_ASCII_TABLE.
 *
 *  @return psArray*     Array of psMetadata items, which contains the output
 *                       data items of each row.
 *
 *  @see psFitsReadTableRow
 */
psArray* psFitsReadTable(
    const psFits* fits                  ///< the psFits object
);

/** Writes a whole FITS table. A new HDU of the type BINTABLE is appended
 *  to the file.
 *
 *  @return bool        TRUE if the write was successful, otherwise FALSE
 *
 *  @see psFitsReadTable
 *  @see psFitsInsertTable
 */
bool psFitsWriteTable(
    psFits* fits,                      ///< the psFits object
    const psMetadata* header,          ///< header items for the new HDU.  Can be NULL.
    const psArray* table, ///< Array of psMetadata items, which contains the output data items of each row.
    const char *extname                 ///< Extension name
);

/// Write an empty table
bool psFitsWriteTableEmpty(
    psFits *fits,                       ///< FITS file pointer
    const psMetadata *header,           ///< Header to write
    const psMetadata *columns,          ///< Column definitions; no data used except name,type
    const char *extname                 ///< Extension name for table
    );

/** Inserts a whole FITS table. A new HDU of the type BINTABLE is inserted either
 *  before or after, depending on the AFTER parameter, the current HDU.
 *
 *  @return bool        TRUE if the insert/write was successful, otherwise FALSE
 *
 *  @see psFitsWriteTable
 */
bool psFitsInsertTable(
    psFits* fits,                  ///< the psFits object
    const psMetadata* header,      ///< header items for the new HDU.  Can be NULL.
    const psArray* table, ///< Array of psMetadata items, which contains the output data items of each row.
    const char *extname,                ///< Extension name
    bool after    ///< TRUE if insert is done after CHDU, otherwise table is inserted before CHDU
);

/// Insert an empty table
bool psFitsInsertTableEmpty(
    psFits *fits,              ///< FITS file pointer
    const psMetadata *header,  ///< Header to write
    const psMetadata *columns, ///< Column definitions; no data used except name,type
    const char *extname,       ///< Extension name for table
    bool after                 ///< Insert after current HDU?
    );


/** Updates a FITS table.  The current HDU type must be either
 *  PS_FITS_TYPE_BINARY_TABLE or PS_FITS_TYPE_ASCII_TABLE.
 *
 *  @return bool        TRUE if the write was successful, otherwise FALSE
 *
 *  @see psFitsWriteTable
 */
bool psFitsUpdateTable(
    psFits* fits,                      ///< the psFits object
    const psMetadata* data,
    ///< Array of psMetadata items, which contains the output data items of each row.
    int row                            ///< the row number to update.
);

/// @}
#endif // #ifndef PS_FITS_H
