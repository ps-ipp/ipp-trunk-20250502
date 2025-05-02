/** @file  psFitsTable.c
 *
 *  @brief Contains Fits I/O routines
 *
 *  @ingroup FileIO
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.33 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-07-04 03:18:06 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "psFits.h"
#include "string.h"
#include "psError.h"

#include "psImageStructManip.h"
#include "psMemory.h"
#include "psString.h"
#include "psLogMsg.h"
#include "psTrace.h"
#include "psVector.h"
#include "psFitsTable.h"
#include "psFitsHeader.h"
#include "psAbort.h"
#include "psAssert.h"

#define MAX_STRING_LENGTH 256  // maximum length string for FITS routines

long psFitsTableSize(const psFits *fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, 0);

    int status = 0;                     // CFITSIO status

    // Check for empty table, which looks like an image
    int hdutype;                        // Type of HDU
    fits_get_hdu_type(fits->fd, &hdutype, &status);
    if (psFitsError(status, true, "Could not determine the HDU type.")) {
        return -1;
    }
    if (hdutype == IMAGE_HDU) {
        // It could be an empty table
        int naxis = 0;                  // Dimensions of image
        if (fits_get_img_dim(fits->fd, &naxis, &status) != 0) {
            psFitsError(status, true, "Unable to determine dimension for table.");
            return -1;
        }
        if (naxis != 0) {
            psFitsError(PS_ERR_BAD_FITS, true, "Current FITS HDU is not a table.");
            return -1;
        }
        return 0;
    }

    long numRows;                       // Number of rows
    if (fits_get_num_rows(fits->fd, &numRows, &status)) {
        psFitsError(status, true, "Unable to determine number of rows in table.");
        return -1;
    }

    return numRows;
}

// Check if the FITS file is an empty table
static bool emptyTableCheck(const psFits *fits // FITS file
                            )
{
    PS_ASSERT_FITS_NON_NULL(fits, false);

    if (psFitsGetExtNum(fits) == 0 && !psFitsMoveExtNum(fits, 1, false)) {
        psError(PS_ERR_IO, false, "Unable to move to first extension to read table.");
        return false;
    }

    int status = 0;                     // CFITSIO status
    int hdutype;                        // Type of HDU
    fits_get_hdu_type(fits->fd, &hdutype, &status);
    if (status) {
        psFitsError(status, true, "Could not determine the HDU type.");
        return false;
    }
    if (hdutype == IMAGE_HDU) {
        // It could be an empty table
        int naxis = 0;                  // Dimensions of image
        if (fits_get_img_dim(fits->fd, &naxis, &status) != 0) {
            psFitsError(status, true, "Unable to determine dimension for table.");
            return false;
        }
        if (naxis != 0) {
            psFitsError(PS_ERR_BAD_FITS, true, "Current FITS HDU is not a table.");
            return false;
        }
        return true;
    }

    return false;
}

// Check the FITS file in preparation for reading a table
static bool readTableCheck(const psFits *fits // FITS file
                           )
{
    PS_ASSERT_FITS_NON_NULL(fits, false);

    if (psFitsGetExtNum(fits) == 0 && !psFitsMoveExtNum(fits, 1, false)) {
        psError(PS_ERR_IO, false, "Unable to move to first extension to read table.");
        return false;
    }

    // check that we are positioned on a table HDU
    int status = 0;                     // CFITSIO status
    int hdutype;                        // Type of HDU
    fits_get_hdu_type(fits->fd, &hdutype, &status);
    if (status) {
        psFitsError(status, true, "Could not determine the HDU type.");
        return false;
    }
    if (hdutype != ASCII_TBL && hdutype != BINARY_TBL) {
        return false;
    }
    return true;
}


psMetadata* psFitsReadTableRow(const psFits* fits,
                               int row)
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);
    PS_ASSERT_INT_NONNEGATIVE(row, NULL);

    if (!readTableCheck(fits)) {
        if (emptyTableCheck(fits)) {
            return psMetadataAlloc();
        }
        psError(PS_ERR_BAD_FITS, true, "Extension is not a table");
        return NULL;
    }

    // get the size of the FITS table
    long numRows;
    int numCols;
    int status = 0;
    fits_get_num_rows(fits->fd, &numRows, &status);
    fits_get_num_cols(fits->fd, &numCols, &status);
    if (status) {
        psFitsError(status, true, "Failed to determine the size of the current HDU table.");
        return NULL;
    }

    psTrace("psLib.fits",5,"Table size is %ix%li\n",numCols, numRows);
    // the row parameter in the proper range?
    if (row < 0 || row >= numRows) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified row, %d, is not valid for current table of %ld rows."),
                row, numRows);
        return NULL;
    }

    psMetadata* data = psMetadataAlloc();

    int hdutype;                        // Type of HDU: need to distinguish ASCII and binary tables
    fits_get_hdu_type(fits->fd, &hdutype, &status);
    if (psFitsError(status, true, "Could not determine the HDU type.")) {
        return false;
    }

    int typecode;
    long repeat;
    long width;
    char name[60];
    for (int col = 1; col <= numCols; col++) {
        // get the column name
        if (hdutype == BINARY_TBL) {
            fits_get_bcolparms(fits->fd, col, name,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, &status);
        } else {
            fits_get_acolparms(fits->fd, col, name,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, &status);
        }
        // get the column type
        fits_get_coltype(fits->fd, col, &typecode, &repeat, &width, &status);
        if (psFitsError(status, true, "Unable to get column type for column %d", col)) {
            psFree(data);
            return NULL;
        }

#define READ_TABLE_ROW_CASE(FITSTYPE, NATIVETYPE, TYPE, VECTYPE) \
        case FITSTYPE: { \
                if (repeat == 1) { \
                    NATIVETYPE value; \
                    int anynul = 0; \
                    fits_read_col(fits->fd, FITSTYPE, col,row+1, \
                                  1, 1, NULL, &value, &anynul, &status); \
                    psTrace("psLib.fits",5,"Column #%i, '%s', is type %i, repeat %li, Value = %g\n", \
                            col, name, typecode, repeat, (double)value); \
                    psMetadataAdd(data,PS_LIST_TAIL, name, \
                                  PS_DATA_##TYPE, \
                                  "", (ps##TYPE)value); \
                } else { \
                    NATIVETYPE* value = psAlloc(sizeof(NATIVETYPE)*repeat); \
                    psVector* vec = psVectorAlloc(repeat,PS_TYPE_##VECTYPE); \
                    int anynul = 0; \
                    fits_read_col(fits->fd, FITSTYPE, col,row+1, \
                                  1, repeat, NULL, value, &anynul, &status); \
                    for (int lcv = 0; lcv < repeat; lcv++) { \
                        vec->data.VECTYPE[lcv] = value[lcv]; \
                    } \
                    psMetadataAdd(data,PS_LIST_TAIL, name, PS_DATA_VECTOR, "", vec); \
                    psFree(value); \
                    psFree(vec); \
                } \
                break; \
            }

        switch (typecode) {
           // TBYTE and TSHORT fall though to read into psS32
          case TBYTE:
          case TSHORT:
            READ_TABLE_ROW_CASE(TLONG, long, S32, S32);
            READ_TABLE_ROW_CASE(TLONGLONG, long, S64, S64);
            READ_TABLE_ROW_CASE(TFLOAT, float, F32, F32);
            READ_TABLE_ROW_CASE(TDOUBLE, double, F64, F64);
            READ_TABLE_ROW_CASE(TLOGICAL, bool, BOOL, S8);
          case TSTRING: {
              psString value = psStringAlloc(repeat);
              int anynul = 0;
              fits_read_col(fits->fd, TSTRING, col,row+1, 1, 1, NULL, &value, &anynul, &status);
              psTrace("psLib.fits", 5, "Column #%i, '%s', is type %i, repeat %li, value = %s\n",
                      col, name, typecode, repeat, value);
              if (anynul == 0) {
                  psMetadataAdd(data,PS_LIST_TAIL, name, PS_DATA_STRING, NULL, value);
              }
              psFree(value);
              break;
          }
          default:
            psWarning("Data type (%d) not supportted for column %d", typecode, col);
            psTrace("psLib.fits", 2, "Column %d or row %d was of a non primitive type, %d",
                    col, row, typecode);
        }

        if (psFitsError(status, true, "Failed to retrieve table element (%d,%d)", col, row)) {
            psFree(data);
            return NULL;
        }
    }

    return data;
}

psArray* psFitsReadTableColumn(const psFits* fits,
                               const char* colname)
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);
    PS_ASSERT_STRING_NON_EMPTY(colname, NULL);

    if (!readTableCheck(fits)) {
        if (emptyTableCheck(fits)) {
            return psArrayAlloc(0);
        }
        psError(PS_ERR_BAD_FITS, true, "Extension is not a table");
        return NULL;
    }

    int colnum = 0;
    int status = 0;

    // find the column by name
    if (fits_get_colnum(fits->fd, CASESEN, (char*)colname, &colnum, &status) != 0) {
        psFitsError(status, true, "Specified column, %s, was not found.", colname);
        return NULL;
    }

    // get the number of rows
    long numRows = psFitsTableSize(fits);
    if (numRows == -1) {
        return NULL;
    }

    // get the column length.
    int width;
    if (fits_get_col_display_width(fits->fd, colnum, &width, &status) != 0) {
        psFitsError(status, true, "Could not determine the datatype of the table column.");
        return NULL;
    }

    // allocate the buffers
    psArray* result = psArrayAlloc(numRows);
    for (int row = 0; row < numRows; row++) {
        result->data[row] = psStringAlloc(width);
    }

    fits_read_col_str(fits->fd, colnum, 1, 1, numRows, "", (char**)result->data, NULL, &status);
    if (psFitsError(status, true, "Failed to read table column.")) {
        psFree(result);
        return NULL;
    }

    return result;
}

psVector* psFitsReadTableColumnNum(const psFits* fits,
                                   const char* colname)
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);
    PS_ASSERT_STRING_NON_EMPTY(colname, NULL);

    if (!readTableCheck(fits)) {
        psError(PS_ERR_BAD_FITS, true, "Extension is not a table");
        return NULL;
    }

    int status = 0;
    int colnum = 0;

    // find the column by name
    if (fits_get_colnum(fits->fd, CASESEN, (char*)colname, &colnum, &status) != 0) {
        psFitsError(status, true, "Specified column, %s, was not found.", colname);
        return NULL;
    }

    // get the number of rows
    long numRows = psFitsTableSize(fits);
    if (numRows == -1) {
        return NULL;
    }

    // get the column datatype.
    int typecode;
    long repeat;
    long width;
    if (fits_get_eqcoltype(fits->fd, colnum, &typecode, &repeat, &width, &status) != 0) {
        psFitsError(status, true, "Could not determine the datatype of the table column.");
        return NULL;
    }

    psVector* result = psVectorAlloc(numRows, p_psFitsTypeFromCfitsio(typecode));

    fits_read_col(fits->fd, typecode, colnum, 1, 1, numRows, NULL, (psPtr)(result->data.U8), NULL, &status);
    if (psFitsError(status, true, "Failed to read table column.")) {
        psFree(result);
        return NULL;
    }

    return result;
}


psArray* psFitsReadTable(const psFits* fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);

    if (!readTableCheck(fits)) {
        if (emptyTableCheck(fits)) {
            return psArrayAlloc(0);
        }
        psError(PS_ERR_BAD_FITS, true, "Extension is not a table");
        return NULL;
    }

    // get the number of rows
    long numRows = psFitsTableSize(fits);
    if (numRows == -1) {
        return NULL;
    }

    psArray* table = psArrayAlloc(numRows);

    for (int row = 0; row < numRows; row++) {
        psTrace("psLib.fits",5,"Reading row %i of %li\n", row, numRows);
        table->data[row] = psFitsReadTableRow(fits,row);
    }

    return table;
}

bool psFitsWriteTable(psFits* fits,
                      const psMetadata* header,
                      const psArray* table,
                      const char *extname)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    if (!psFitsMoveLast(fits)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to move to last extension to write table");
        return false;
    }
    return psFitsInsertTable(fits, header, table, extname, true);
}


// Return the size of the column
static inline size_t columnSize(const psMetadataItem *item // Item for which to get the size
                               )
{
    if (PS_DATA_IS_PRIMITIVE(item->type)) {
        return 1;
    }
    switch (item->type) {
    case PS_DATA_STRING:
        return strlen(item->data.V);
    case PS_DATA_VECTOR: {
            psVector *vector = item->data.V;
            return vector ? vector->n : 0;
        }
    default:
        psAbort("Shouldn't ever get here.");
    }
    return 0;
}

// Get the TFORM character, given a PS type
static inline char getTForm(psDataType type)
{
    switch (type) {
    case PS_TYPE_U8:
    case PS_TYPE_S8:
        return 'B';
    case PS_TYPE_S16:
        return 'I';
    case PS_TYPE_S32:
        return 'J';
    case PS_TYPE_U64:
    case PS_TYPE_S64:
        return 'K';
    case PS_TYPE_U16:
        return 'U';
    case PS_TYPE_U32:
        return 'V';
    case PS_TYPE_F32:
        return 'E';
    case PS_TYPE_F64:
        return 'D';
    case PS_TYPE_BOOL:
        return 'L';
    case PS_DATA_STRING:
        return 'A';
    default:
        psError(PS_ERR_UNKNOWN, true, "Unknown type: %x\n", type);
        return '?';
    }
}


// Column specification
// Included here, because there's no need for the user to have access to it
typedef struct {
    psDataType type;                    // psLib type (e.g., PS_DATA_STRING or PS_TYPE_F32)
    size_t size;                        // Size (number of repeats)
    psElemType vectorType;              // psLib type of vectors
} colSpec;


static bool fitsInsertTable(psFits* fits,             // FITS file
                            const psMetadata* header, // FITS header to write
                            const psArray* table,     // Table to write
                            const char *extname,      // Extension name to give table
                            bool after,               // Write table after current extension?
                            bool writeData            // Write data?
                            )
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PS_ASSERT_ARRAY_NON_NULL(table, false);

    int status = 0;

    long numRows = table->n;
    if (writeData && numRows < 1) {
        // no table data, what can I do?
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                _("Can't create a table without any rows."));
        return false;
    }

    // Find the unique items in the array of metadata 'rows', and their sizes
    psMetadata *colSpecs = psMetadataAlloc(); // Column specifications
    size_t rowSize = 0;                 // Size (in bytes) of each row
    for (long i = 0; i < numRows; i++) {
        psMetadata* row = table->data[i];
        if (!row) {
            continue;
        }
        psMetadataIterator *rowIter = psMetadataIteratorAlloc(row, PS_LIST_HEAD, NULL); // Iterator
        psMetadataItem *colItem;        // Column item, from iteration
        while ((colItem = psMetadataGetAndIncrement(rowIter))) {
            if (!(PS_DATA_IS_PRIMITIVE(colItem->type) || colItem->type == PS_DATA_STRING ||
                    colItem->type == PS_DATA_VECTOR)) {
                // unsupported type -- treating as an error
                psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                        "Unsupported data type (%d) for Metadata Item '%s' in row %ld.",
                        colItem->type, colItem->name, i);
                psFree(rowIter);
                psFree(colSpecs);
                return false;
            }

            size_t size = columnSize(colItem); // Size for this column

            // Check to see if we know about this one; or update the size if required
            psMetadataItem *colSpecItem = psMetadataLookup(colSpecs, colItem->name);
            if (!colSpecItem) {
                // A new one!
                colSpec *spec = psAlloc(sizeof(colSpec)); // Specification for this column
                // BOOL type is not a valid vector type, so we translate it to U8
                spec->type = colItem->type == PS_DATA_BOOL ? PS_DATA_U8 : colItem->type;
                spec->size = size;
                if (colItem->type == PS_DATA_VECTOR) {
                    psVector *vector = colItem->data.V; // The vector
                    spec->vectorType = vector->type.type;
                }
                psMetadataAddPtr(colSpecs, PS_LIST_TAIL, colItem->name, PS_DATA_UNKNOWN, "", spec);
                psFree(spec);           // Drop reference
                rowSize += PSELEMTYPE_SIZEOF(spec->type);
            } else {
                colSpec *spec = colSpecItem->data.V; // The specification
                if (size > spec->size) {
                    spec->size = size;
                }
                if (colItem->type != spec->type &&
                    colItem->type != PS_DATA_BOOL && spec->type != PS_DATA_U8) {
                    psWarning("Differing type found for column %s: %x vs %x --- using the first found.\n",
                              colSpecItem->name, colItem->type, spec->type);
                }
                if (colItem->type == PS_DATA_VECTOR) {
                    psVector *vector = colItem->data.V; // The vector
                    if (vector->type.type != spec->vectorType) {
                        psWarning("Differing vector type found for column %s: %x vs %x "
                                 "--- using the first found.\n", colSpecItem->name, vector->type.type,
                                 spec->vectorType);
                    }
                }
            }
        }
        psFree(rowIter);
    }

    long numColumns = colSpecs->list->n;// Number of columns
    if (numColumns == 0) {
        // No table columns found
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Did not find any column data to write to a table.");
        psFree(colSpecs);
        return false;
    }

    // Create array of column names and types.
    psArray *columnNames = psArrayAlloc(numColumns); // Array of column names, for cfitsio
    psArray *columnTypes = psArrayAlloc(numColumns); // Array of column types, for cfitsio
    psMetadataIterator *colSpecsIter = psMetadataIteratorAlloc(colSpecs, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *colSpecItem;        // Column specification item, from iteration
    for (long i = 0; (colSpecItem = psMetadataGetAndIncrement(colSpecsIter)); i++) {
        colSpec *spec = colSpecItem->data.V; // The specification
        columnNames->data[i] = psMemIncrRefCounter(colSpecItem->name);
        psString colType = NULL;        // The column type
        if (spec->type == PS_DATA_VECTOR) {
            psStringAppend(&colType, "%zd%c", spec->size, getTForm(spec->vectorType));
        } else {
            psStringAppend(&colType, "%zd%c", spec->size, getTForm(spec->type));
        }
        columnTypes->data[i] = colType;
    }

    // Create the table HDU
    int numHDUs = psFitsGetSize(fits);  // Number of HDUs in file
    if (numHDUs == 0) {
        // We're creating the first extension
        fits_create_tbl(fits->fd,
                        BINARY_TBL,
                        writeData ? numRows : 0, // number of rows in table
                        numColumns, // number of columns in table
                        (char**)columnNames->data, // names of the columns
                        (char**)columnTypes->data, // format of the columns
                        NULL, // physical unit of columns
                        NULL, // skip extension name: we set the by hand below
                        &status);
    } else {
        if (!after) {
            if (psFitsGetExtNum(fits) == 0) {
                // We're creating a replacement primary HDU.
                // Set status to signal fits_insert_img to insert a new primary HDU
                status = PREPEND_PRIMARY;
            } else {
                // Move back one to perform an insert after the previous HDU
                psFitsMoveExtNum(fits, -1, true);
            }
        }
        // Insert the table
        fits_insert_btbl(fits->fd,
                         writeData ? numRows : 0, // number of rows in table
                         numColumns, // number of columns in table
                         (char**)columnNames->data, // names of the columns
                         (char**)columnTypes->data, // format of the columns
                         NULL, // physical unit of columns
                         NULL, // skip extension name: we set this by hand below
                         0, &status);
    }
    psFree(columnNames);
    psFree(columnTypes);

    if (status != 0) {
        psFitsError(status, true, "Unable to create FITS table with %ld columns and %ld rows",
                    numColumns, table->n);
        psFree(colSpecsIter);
        psFree(colSpecs);
        return false;
    }

    // Write header
    if (header && !psFitsWriteHeader(fits, header)) {
        psError(PS_ERR_IO, false, "Unable to write FITS header.\n");
        psFree(colSpecsIter);
        psFree(colSpecs);
        return false;
    }

    // write the header, if any.
    if (extname && strlen(extname) > 0) {
        if (!psFitsSetExtName(fits, extname)) {
            psError(PS_ERR_IO, false, "Unable to write FITS header extension name.\n");
            psFree(colSpecsIter);
            psFree(colSpecs);
            return false;
        }
    }

    // cfitsio requires that we write the data by columns --- urgh!
    if (writeData) {
        psMetadataIteratorSet(colSpecsIter, PS_LIST_HEAD);
        for (long colNum = 1; (colSpecItem = psMetadataGetAndIncrement(colSpecsIter)); colNum++) {
            // Note: colNum is unit-indexed, because it's for cfitsio
            colSpec *spec = colSpecItem->data.V; // The specification
            if (PS_DATA_IS_PRIMITIVE(spec->type)) {
                size_t dataSize = PSELEMTYPE_SIZEOF(spec->type); // Size (in bytes) of this type
                psVector *columnData = psVectorAlloc(table->n, spec->type); // The raw row data, to be written
                psVectorInit(columnData, 0);
                for (long i = 0; i < table->n; i++) {
                    psMetadata *row = table->data[i]; // The row of interest
                    psMetadataItem *dataItem = psMetadataLookup(row, colSpecItem->name); // Value of interest
		    if (dataItem) {
			memcpy(&columnData->data.U8[i * dataSize], &dataItem->data, dataSize);
		    } else {
			// this element is missing from this row; insert an appropriate-sized place holder
			// XXX this should insert a NAN for float / double and an appropriate blank for int types
			// XXX for the moment I am putting in 0.0
			memset(&columnData->data.U8[i * dataSize], 0, dataSize);
		    }
                }

                int fitsDataType;           // Data type for cfitsio
                p_psFitsTypeToCfitsio(spec->type, NULL, NULL, &fitsDataType);
                fits_write_col(fits->fd,
                               fitsDataType,
                               colNum, // column number
                               1, // first row
                               1, // first element
                               table->n, // number of rows
                               columnData->data.U8, // the data
                               &status);
                psFree(columnData);
            } else {
                switch (spec->type) {
                  case PS_DATA_STRING: {
                      psArray *strings = psArrayAlloc(table->n); // Array of strings
                      for (long i = 0; i < table->n; i++) {
                          psMetadata *row = table->data[i]; // The row of interest
                          strings->data[i] = psMemIncrRefCounter(psMetadataLookupStr(NULL, row,
                                                                                     colSpecItem->name));
                      }
                      fits_write_col_str(fits->fd, colNum, 1, 1, table->n, (char**)strings->data, &status);
                      psFree(strings);
                      break;
                  }
                  case PS_DATA_VECTOR: {
                      size_t dataSize = PSELEMTYPE_SIZEOF(spec->vectorType); // Size of data, in bytes
                      psVector *columnData = psVectorAlloc(spec->size * table->n * dataSize, PS_TYPE_U8);
                      psVectorInit(columnData, 0);
                      for (long i = 0; i < table->n; i++) {
                          psMetadata *row = table->data[i]; // The row of interest
                          psMetadataItem* dataItem = psMetadataLookup(row, colSpecItem->name);
                          if (dataItem->type != PS_DATA_VECTOR) {
                              // Just in case --- get a zero instead of some weird result
                              continue;
                          }
                          psVector *vector = dataItem->data.V;
                          memcpy(&columnData->data.U8[i * dataSize * spec->size], vector->data.U8,
                                 vector->n * dataSize);
                      }

                      int fitsDataType;           // Data type for cfitsio
                      p_psFitsTypeToCfitsio(spec->vectorType, NULL, NULL, &fitsDataType);
                      fits_write_col(fits->fd, fitsDataType, colNum, 1, 1, table->n * spec->size,
                                     columnData->data.U8, &status);
                      psFree(columnData);
                      break;
                  }
                  default:
                    psAbort("Should never get here.\n");
                }
            }

            // Check error status from writing column
            if (status != 0) {
                psFitsError(status, true, "Unable to write column %ld of FITS table", colNum);
                psFree(colSpecsIter);
                psFree(colSpecs);
                return false;
            }
        }
    }

    psFree(colSpecsIter);
    psFree(colSpecs);

    // This forces a re-scan of the header to ensure everything's kosher.  We found this occassionally
    // necessary for compressed images, which are tables, so perhaps it helps here too.  I guess it can't
    // hurt.
    ffrdef(fits->fd, &status);
    if (psFitsError(status, true, "Could not re-scan HDU.")) {
        return false;
    }

    return true;
}


bool psFitsInsertTable(psFits* fits, const psMetadata* header, const psArray* table, const char *extname,
                       bool after)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    return fitsInsertTable(fits, header, table, extname, after, true);
}

bool psFitsWriteTableEmpty(psFits *fits, const psMetadata *header, const psMetadata *columns,
                           const char *extname)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    if (!psFitsMoveLast(fits)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to move to last extension to write table");
        return false;
    }
    psArray *table = psArrayAlloc(1);   // Dummy table carrying column definitions
    table->data[0] = psMemIncrRefCounter((psPtr)columns); // Casting away const
    bool status = fitsInsertTable(fits, header, table, extname, true, false); // Status of insertion
    psFree(table);
    return status;
}

bool psFitsInsertTableEmpty(psFits *fits, const psMetadata *header, const psMetadata *columns,
                            const char *extname, bool after)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    psArray *table = psArrayAlloc(1);   // Dummy table carrying column definitions
    table->data[0] = psMemIncrRefCounter((psPtr)columns); // Casting away const
    bool status = fitsInsertTable(fits, header, table, extname, after, false); // Status of insertion
    psFree(table);
    return status;
}


bool psFitsUpdateTable(psFits* fits,
                       const psMetadata* data,
                       int row)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PS_ASSERT_METADATA_NON_NULL(data, false);
    PS_ASSERT_INT_NONNEGATIVE(row, false);

    if (!readTableCheck(fits)) {
        psError(PS_ERR_BAD_FITS, true, "Extension is not a table");
        return NULL;
    }

    int status = 0;
    psMetadataIterator* iter = psMetadataIteratorAlloc((psPtr)data, PS_LIST_HEAD, NULL);
    psMetadataItem* item;
    while ( (item=psMetadataGetAndIncrement(iter)) != NULL) {
        if (PS_DATA_IS_PRIMITIVE(item->type) ||
                item->type == PS_DATA_BOOL ||
                item->type == PS_DATA_STRING) {
            // operating on primitive data type or string, i.e., not a complex object
            int colnum = 0;

            if (fits_get_colnum(fits->fd, CASESEN, item->name, &colnum, &status) == 0) {
                // cooresponding column found in table
                int dataType;
                p_psFitsTypeToCfitsio(item->type, NULL, NULL, &dataType);

                if (fits_write_col(fits->fd, dataType, colnum, row+1, 1, 1, &item->data, &status) != 0) {
                    psFitsError(status, true, "Could not write data to file.");
                    psFree(iter);
                    return false;
                }
            } else {
                // the column was not found.
                psWarning("No column with the name '%s' exists in the table.", item->name);
            }
        }
    }

    psFree(iter);

    // This forces a re-scan of the header to ensure everything's kosher.  We found this occassionally
    // necessary for compressed images, which are tables, so perhaps it helps here too.  I guess it can't
    // hurt.
    ffrdef(fits->fd, &status);
    if (psFitsError(status, true, "Could not re-scan HDU.")) {
        return false;
    }

    return true;
}


psMetadata *psFitsReadTableAllColumns(const psFits *fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);

    if (!readTableCheck(fits)) {
        if (emptyTableCheck(fits)) {
            return psMetadataAlloc();
        }
        psError(PS_ERR_BAD_FITS, true, "Extension is not a table");
        return NULL;
    }

    int status = 0;                     // CFITSIO return status

    long numRows = 0;                   // Number of rows in table
    int numCols = 0;                    // Number of columns in table
    fits_get_num_rows(fits->fd, &numRows, &status);
    fits_get_num_cols(fits->fd, &numCols, &status);
    if (status) {
        psFitsError(status, true, "Failed to determine the size of the current HDU table.");
        return NULL;
    }

    int hdutype;                        // Type of HDU: need to distinguish ASCII and binary tables
    fits_get_hdu_type(fits->fd, &hdutype, &status);
    if (status) {
        psFitsError(status, true, "Could not determine the HDU type.");
        return false;
    }

    psMetadata *table = psMetadataAlloc();     // Table to return
    for (int col = 1; col <= numCols; col++) { // Fortran indexing
        char name[FLEN_VALUE];           // Column name
        if (hdutype == BINARY_TBL) {
            fits_get_bcolparms(fits->fd, col, name,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, &status);
        } else {
            fits_get_acolparms(fits->fd, col, name,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, &status);
        }

        int cfitsioType = 0;             // Column type from CFITSIO
        long repeat;                     // Number of repeats
        fits_get_eqcoltype(fits->fd, col, &cfitsioType, &repeat, NULL, &status);
        if (status) {
            psFitsError(status, true, "Could not determine the column data for %s", name);
            psFree(table);
            return false;
        }

        psDataType pslibType = p_psFitsTypeFromCfitsio(cfitsioType); // Column type in psLib
        if (pslibType == PS_DATA_STRING) {
            // Strings
            int width;                  // Width of strings
            if (fits_get_col_display_width(fits->fd, col, &width, &status) != 0) {
                psFitsError(status, true, "Could not determine the width of column %s", name);
                psFree(table);
                return NULL;
            }
            psArray *array = psArrayAlloc(numRows); // Array of strings from table
            for (int i = 0; i < numRows; i++) {
                array->data[i] = psStringAlloc(width);
            }
            fits_read_col_str(fits->fd, col, 1, 1, numRows, "", (char**)array->data, NULL, &status);
            if (status) {
                psFitsError(status, true, "Failed to read column %s", name);
                psFree(array);
                psFree(table);
                return NULL;
            }
            if (!psMetadataAddArray(table, PS_LIST_TAIL, name, 0, NULL, array)) {
                psError(PS_ERR_BAD_FITS, false, "Unable to add column %s to table", name);
                psFree(array);
                psFree(table);
                return NULL;
            }
            psFree(array);
        } else if (repeat == 1) {
            // Single numbers
            psVector* vector = psVectorAlloc(numRows, pslibType); // Vector from table
            fits_read_col(fits->fd, cfitsioType, col, 1, 1, numRows, NULL,
                          vector->data.U8, NULL, &status);
            if (status) {
                psFitsError(status, true, "Failed to read column %s", name);
                psFree(vector);
                psFree(table);
                return NULL;
            }
            if (!psMetadataAddVector(table, PS_LIST_TAIL, name, 0, NULL, vector)) {
                psError(PS_ERR_BAD_FITS, false, "Unable to add column %s to table", name);
                psFree(vector);
                psFree(table);
                return NULL;
            }
            psFree(vector);
        } else  {
            // Vectors of numbers
            psAssert(pslibType != PS_DATA_STRING, "Vectors of strings not handled");
            psImage *image = psImageAlloc(repeat, numRows, pslibType); // Image to store vector of vectors
            for (int row = 1, i = 0; row <= numRows; row++, i++) { // Fortran indexing for row
                int anynul = 0;         // Any nulls in what was read?
                fits_read_col(fits->fd, cfitsioType, col, row, 1, repeat, NULL,
                              image->data.U8[i], &anynul, &status);
                if (psFitsError(status, true, "Failed to read column %s row %d", name, row)) {
                    psFree(image);
                    psFree(table);
                    return NULL;
                }
            }
            if (!psMetadataAddImage(table, PS_LIST_TAIL, name, 0, NULL, image)) {
                psError(PS_ERR_BAD_FITS, false, "Unable to add column %s to table", name);
                psFree(image);
                psFree(table);
                return NULL;
            }
            psFree(image);
        }
    }

    return table;
}

bool psFitsWriteTableAllColumns(psFits *fits, psMetadata *header, const psMetadata *table, const char *extname)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PS_ASSERT_METADATA_NON_NULL(table, false);

    psArray *columns = psListToArray(table->list); // Columns in psMetadataItems
    int numCols = columns->n;                      // Number of columns
    long numRows = 0;                              // Number of rows
    psArray *names = psArrayAlloc(numCols);        // Column names
    psArray *types = psArrayAlloc(numCols);        // Column types

    for (int i = 0; i < numCols; i++) {
        psMetadataItem *colItem = columns->data[i]; // Column
        names->data[i] = psMemIncrRefCounter(colItem->name);

        size_t size = 0;                   // Size of column
        char tform = 0;                 // Type in character for FITS
        long rows = 0;                   // Number of rows
        switch (colItem->type) {
          case PS_DATA_VECTOR:
            size = 1;
            psVector *vector = colItem->data.V; // Vector of interest
            tform = getTForm(vector->type.type);
            rows = vector->n;
            break;
          case PS_DATA_ARRAY:
            tform = getTForm(PS_DATA_STRING);
            psArray *array = colItem->data.V; // Array of interest
            rows = array->n;
            for (int i = 0; i < rows; i++) {
                const char *string = array->data[i];
                size = PS_MAX(size, strlen(string));
            }
            break;
          case PS_DATA_IMAGE: ;
            psImage *image = colItem->data.V; // Image of interest
            tform = getTForm(image->type.type);
            size = image->numCols;
            rows = image->numRows;
            break;
          default:
            psAbort("Unrecognised column type: %x", colItem->type);
        }
        psString type = NULL;           // Column type
        psStringAppend(&type, "%zd%c", size, tform);
        types->data[i] = type;
        if (i == 0) {
            numRows = rows;
        } else if (numRows != rows) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Column %d has differing size: %ld vs %ld",
                    i, rows, numRows);
            psFree(names);
            psFree(types);
            psFree(columns);
            return false;
        }
    }

    // Create the table HDU
    int numHDUs = psFitsGetSize(fits);  // Number of HDUs in file
    int status = 0;                     // Status from cfitsio
    if (numHDUs == 0) {
        // We're creating the first extension
        fits_create_tbl(fits->fd, BINARY_TBL, numRows, numCols, (char**)names->data, (char**)types->data,
                        NULL, NULL, &status);
    } else {
        fits_insert_btbl(fits->fd, numRows, numCols, (char**)names->data, (char**)types->data,
                         NULL, NULL, 0, &status);
    }
    psFree(names);
    psFree(types);
    if (status) {
        psFitsError(status, true, "Unable to create FITS table with %d columns and %ld rows",
                    numCols, numRows);
        psFree(columns);
        return false;
    }

    // Write header
    if (header && !psFitsWriteHeader(fits, header)) {
        psError(psErrorCodeLast(), false, "Unable to write FITS header.\n");
        psFree(columns);
        return false;
    }
    if (extname && strlen(extname) > 0 && !psFitsSetExtName(fits, extname)) {
        psError(psErrorCodeLast(), false, "Unable to write FITS header extension name.\n");
        psFree(columns);
        return false;
    }

    // cfitsio requires that we write the data by columns --- urgh!
    for (long col = 1, i = 0; col <= numCols; col++, i++) {      // Fortran indexing
        psMetadataItem *colItem = columns->data[i]; // Column
        switch (colItem->type) {
          case PS_DATA_VECTOR: {
              psVector *vector = colItem->data.V; // Vector
              int cfitsioType;                    // Data type for cfitsio
              p_psFitsTypeToCfitsio(vector->type.type, NULL, NULL, &cfitsioType);
              fits_write_col(fits->fd, cfitsioType, col, 1, 1, numRows, vector->data.U8, &status);
              break;
          }
          case PS_DATA_ARRAY: {
              psArray *array = colItem->data.V; // Array of strings
              fits_write_col_str(fits->fd, col, 1, 1, numRows, (char**)array->data, &status);
              break;
          }
          case PS_DATA_IMAGE: {
              psImage *image = colItem->data.V;                                        // Image of interest
              psDataType type = image->type.type;                                      // Type of data
              psVector *vector = psVectorAlloc(image->numCols * image->numRows, type); // Vector from image
              if (!p_psImageCopyToRawBuffer(vector->data.U8, image, type)) {
                  psError(psErrorCodeLast(), false, "Unable to copy image to buffer");
                  psFree(columns);
                  return false;
              }
              int cfitsioType;            // Data type for cfitsio
              p_psFitsTypeToCfitsio(type, NULL, NULL, &cfitsioType);
              fits_write_col(fits->fd, cfitsioType, col, 1, 1, image->numRows * image->numCols,
                             vector->data.U8, &status);
              break;
          }
          default:
            psAbort("Unrecognised column type: %x", colItem->type);
        }
        // Check error status from writing column
        if (status) {
            psFitsError(status, true, "Unable to write column %ld of FITS table", col);
            psFree(columns);
            return false;
        }
    }
    psFree(columns);

    // This forces a re-scan of the header to ensure everything's kosher.  We found this occassionally
    // necessary for compressed images, which are tables, so perhaps it helps here too.  I guess it can't
    // hurt.
    ffrdef(fits->fd, &status);
    if (status) {
        psFitsError(status, true, "Could not re-scan HDU.");
        return false;
    }

    return true;
}
