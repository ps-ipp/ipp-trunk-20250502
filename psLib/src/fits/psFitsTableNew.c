/** @file  psFitsTableNew.c
 *
 *  @brief Contains Memory efficient functions for accessing  Fits tables
 *
 *  @ingroup FileIO
 *
 *  @author Bill Sweeney IfA
 *
 *
 *  Copyright 2011 Institute for Astronomy, University of Hawaii
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
#include "psFitsTableNew.h"
#include "psFitsHeader.h"
#include "psAbort.h"
#include "psAssert.h"

// #define MAX_STRING_LENGTH 256  // maximum length string for FITS routines


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

void
freeTable(psFitsTable *table)
{
    if (!table) return;
    for (int col = 0; col < table->numCols; col++) {
        psFitsTableColumn *column = &table->columns[col];
        psFree(column->name);
        if (column->type == PS_DATA_STRING) {
            for (int row = 0; row < table->numRows; row++) {
                psFree(column->data.str);
            }
        } else if (column->type == PS_DATA_VECTOR) {
            for (int row = 0; row < table->numRows; row++) {
                psFree(column->data.vec);
            }
        }
        // all of the members in the data union are pointers so just pick S32
        psFree(column->data.S32);
    }
    psFree(table->columns);
    psFree(table->index);
}

psFitsTable* psFitsTableAlloc(int numCols, int numRows) {
    if (numCols < 0) {
        // empty table
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Can't allocate a psFitsTable with %d columns.", numCols);
        return NULL;
    }

    psFitsTable *table = psAlloc(sizeof(psFitsTable));
    table->index = psMetadataAlloc();
    table->numRows = numRows;
    table->numCols = numCols;
    if (numCols > 0) {
        table->columns = psAlloc(sizeof(psFitsTableColumn) * numCols);
        memset(table->columns, 0, sizeof(psFitsTableColumn) * numCols);
    } else {
        table->columns = NULL;
    }
    psMemSetDeallocator(table, (psFreeFunc) freeTable);

    return table;
}

psFitsTable *psFitsTableCreate (psArray *tableColumns, int numRows) {

    psFitsTable *table = psFitsTableAlloc (tableColumns->n, numRows);

    // now define the table colums
    for (int i = 0; i < tableColumns->n; i++) {
	psFitsTableColumn *column = &table->columns[i];

	psFitsTableColumn *myColumn = tableColumns->data[i];

	column->name = psStringCopy (myColumn->name);
	column->type = myColumn->type;
	column->vectorType = myColumn->vectorType;
	column->elementSize = myColumn->elementSize;
	
        psMetadataAddS32(table->index, PS_LIST_TAIL, column->name, 0, NULL, i);

	switch (column->type) {
	  case PS_DATA_S8:  column->data.S8  = psAlloc(sizeof(psS8) *table->numRows); break;
	  case PS_DATA_S16: column->data.S16 = psAlloc(sizeof(psS16)*table->numRows); break;
	  case PS_DATA_S32: column->data.S32 = psAlloc(sizeof(psS32)*table->numRows); break;
	  case PS_DATA_S64: column->data.S64 = psAlloc(sizeof(psS64)*table->numRows); break;
	  case PS_DATA_U8:  column->data.U8  = psAlloc(sizeof(psU8) *table->numRows); break;
	  case PS_DATA_U16: column->data.U16 = psAlloc(sizeof(psU16)*table->numRows); break;
	  case PS_DATA_U32: column->data.U32 = psAlloc(sizeof(psU32)*table->numRows); break;
	  case PS_DATA_U64: column->data.U64 = psAlloc(sizeof(psU64)*table->numRows); break;
	  case PS_DATA_F32: column->data.F32 = psAlloc(sizeof(psF32)*table->numRows); break;
	  case PS_DATA_F64: column->data.F64 = psAlloc(sizeof(psF64)*table->numRows); break;
	  default: break;
	}
    }
    return table;
}

static void freeColumn(psFitsTableColumn *column)
{
    // add any element frees here
    if (!column) return;

    psFree (column->name);
    return;
}

psFitsTableColumn *psFitsTableColumnAlloc (char *name, psDataType type) {

    psFitsTableColumn *column = psAlloc(sizeof(psFitsTableColumn));
    psMemSetDeallocator(column, (psFreeFunc) freeColumn);

    column->name = psStringCopy (name);
    column->type = type;
    column->vectorType  = 0;
    column->elementSize = 1;
    column->data.S8 = NULL; // no data yet allocated, place-holder only
    return column;
}

bool psFitsTableColumnAdd (psArray *tableColumns, char *name, psDataType type) {
    psFitsTableColumn *column = psFitsTableColumnAlloc (name, type);
    psArrayAdd (tableColumns, 10, column);
    psFree (column);
    return true;
}

psFitsTable* psFitsReadTableNew(const psFits* fits)
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);
    // PS_ASSERT_INT_NONNEGATIVE(row, NULL);

    if (!readTableCheck(fits)) {
        if (emptyTableCheck(fits)) {
            psFitsTable *table = psFitsTableAlloc(0, 0);
            return table;
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

    psFitsTable *table = psFitsTableAlloc(numCols, numRows);

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
        double tscal;
        double tzero;
        if (hdutype == BINARY_TBL) {
            fits_get_bcolparms(fits->fd, col, name,
                               NULL, NULL, NULL, &tscal, &tzero, NULL, NULL, &status);
        } else {
            fits_get_acolparms(fits->fd, col, name,
                               NULL, NULL, NULL, NULL, NULL, NULL, NULL, &status);
        }
        // get the column type
        fits_get_coltype(fits->fd, col, &typecode, &repeat, &width, &status);
        if (psFitsError(status, true, "Unable to get column type for column %d", col)) {
            psFree(table);
            return NULL;
        }

#define READ_TABLE_ROW_CASE(FITSTYPE, NATIVETYPE, TYPE, VECTYPE) \
        case FITSTYPE: { \
                column->type = PS_DATA_##TYPE; \
                column->data.TYPE = psAlloc(sizeof(ps##TYPE)*table->numRows); \
                if (repeat == 1) { \
                    column->elementSize = 1; \
                    NATIVETYPE *values = (NATIVETYPE *) psAlloc(sizeof(NATIVETYPE) * table->numRows); \
                    int anynul = 0; \
                    fits_read_col(fits->fd, FITSTYPE, col, 1, \
                                  1, numRows, NULL, values, &anynul, &status); \
                    for (int lcv = 0; lcv < table->numRows; lcv++) { \
                        column->data.TYPE[lcv] = values[lcv]; \
                    } \
                    psTrace("psLib.fits",5,"Column #%i, '%s', is type %i, repeat %li\n", \
                            col, name, typecode, repeat); \
                    psFree(values); \
                } else { \
                    column->elementSize = repeat; \
                    column->type = PS_DATA_VECTOR; \
                    column->vectorType = PS_DATA_##TYPE; \
                    column->data.vec = psAlloc(sizeof(psVector*) * table->numRows); \
                    for (int row = 0; row < table->numRows; row++) { \
                        NATIVETYPE* values = psAlloc(sizeof(NATIVETYPE)*repeat); \
                        psVector* vec = psVectorAlloc(repeat, PS_TYPE_##VECTYPE); \
                        int anynul = 0; \
                        fits_read_col(fits->fd, FITSTYPE, col,row+1, \
                                  1, repeat, NULL, values, &anynul, &status); \
                        for (int lcv = 0; lcv < repeat; lcv++) { \
                            vec->data.VECTYPE[lcv] = values[lcv]; \
                        } \
                        column->data.vec[row] = vec; \
                        psFree(values); \
                    } \
                    \
                } \
                break; \
            }

        // Handle cases where the data is "really" unsigned
        if (typecode == TLONG && tzero == 2147483648) {
            typecode = TULONG;
        } else if (typecode == TSHORT && tzero == 32768) {
            typecode = TUSHORT;
        }

        // Save the column number for this column in the index metadata
        psMetadataAddS32(table->index, PS_LIST_TAIL, name, 0, NULL, col-1);

        psFitsTableColumn *column = &table->columns[col-1];
        // This second copy of the name makes it easily available when writing the table
        column->name = psStringCopy(name);
        switch (typecode) {
            READ_TABLE_ROW_CASE(TBYTE, long, S32, S32);
            READ_TABLE_ROW_CASE(TSHORT, short, S16, S16);
            READ_TABLE_ROW_CASE(TUSHORT, unsigned short, U16, U16);
            READ_TABLE_ROW_CASE(TULONG, unsigned long, U32, U32);
            READ_TABLE_ROW_CASE(TLONG, long, S32, S32);
            READ_TABLE_ROW_CASE(TLONGLONG, psS64, S64, S64);
            READ_TABLE_ROW_CASE(TFLOAT, float, F32, F32);
            READ_TABLE_ROW_CASE(TDOUBLE, double, F64, F64);
            READ_TABLE_ROW_CASE(TLOGICAL, bool, BOOL, S8);
          case TSTRING: {
              column->data.str = psAlloc(sizeof(psString) * table->numRows);
              column->type = PS_DATA_STRING;
              column->elementSize = 0;
              for (int row = 0; row < table->numRows; row++) {
                  psString value = psStringAlloc(repeat);
                  int anynul = 0;
                  fits_read_col(fits->fd, TSTRING, col,row+1, 1, 1, NULL, &value, &anynul, &status);
                  psTrace("psLib.fits", 5, "Column #%i, '%s', is type %i, repeat %li, value = %s\n",
                          col, name, typecode, repeat, value);
                  if (anynul) {
                      psFree(value);
                      value = NULL;
                  }
                  column->data.str[row] = value;
                  int len = strlen(value);
                  if (len > column->elementSize) {
                    column->elementSize = len;
                  }
              }
              break;
          }
          default:
            psWarning("Data type (%d) not supportted for column %d", typecode, col);
            psTrace("psLib.fits", 2, "Column %d was of a non primitive type, %d",
                    col, typecode);
        }

        if (psFitsError(status, true, "Failed to retrieve table column %d", col)) {
            psFree(table);
            return NULL;
        }
    }

    return table;
}

// return the column number with a given name
int psFitsTableGetColumnNumber(psFitsTable *table, const char *name)
{
    bool mdok;
    int col = psMetadataLookupS32(&mdok, table->index, name);

    if (!mdok) {
        psFitsError(mdok, true, "Failed to lookup column index for %s", name);
        return -1;
    }

    return col;
}

#define psFitsTableGetNumTYPE(TYPE, NAME) \
ps##TYPE psFitsTableGet##NAME(bool *status, psFitsTable *table, int row, const char *name) \
{ \
    ps##TYPE value = 0; \
    if (status) { \
        *status = true; \
    } \
    \
    if (row >= table->numRows || row < 0) { \
        if (status) { \
            *status = false; \
        } \
        return 0; \
    } \
    bool mdok; \
    int col = psMetadataLookupS32(&mdok, table->index, name); \
    if (!mdok) { \
        if (status) { \
            *status = false; \
        } \
        return 0; \
    } \
    if (col < 0) { \
        if (status) { \
            *status = false; \
        } \
        return 0; \
    } \
    psFitsTableColumn *column = &table->columns[col]; \
    switch (column->type) { \
        case PS_DATA_S8: \
        value = (ps##TYPE)column->data.S8[row]; \
        break; \
    case PS_DATA_S16: \
        value = (ps##TYPE)column->data.S16[row]; \
        break; \
    case PS_DATA_S32: \
        value = (ps##TYPE)column->data.S32[row]; \
        break; \
    case PS_DATA_S64: \
        value = (ps##TYPE)column->data.S64[row]; \
        break; \
    case PS_DATA_U8: \
        value = (ps##TYPE)column->data.U8[row]; \
        break; \
    case PS_DATA_U16: \
        value = (ps##TYPE)column->data.U16[row]; \
        break; \
    case PS_DATA_U32: \
        value = (ps##TYPE)column->data.U32[row]; \
        break; \
    case PS_DATA_U64: \
        value = (ps##TYPE)column->data.U64[row]; \
        break; \
    case PS_DATA_F32: \
        value = (ps##TYPE)column->data.F32[row]; \
        break; \
    case PS_DATA_F64: \
        value = (ps##TYPE)column->data.F64[row]; \
        break; \
    case PS_DATA_BOOL: \
        if (column->data.BOOL[row]) { \
            value = 1; \
        } \
        break; \
    default: \
        /* if you get to this point, the value is not a number. */ \
        if (status) {  \
            *status = false; \
        }  \
        break; \
    } \
    return value; \
}

// #define psFitsTableGetNumTYPE(TYPE, NAME)

psFitsTableGetNumTYPE(S8, S8);
psFitsTableGetNumTYPE(U8, U8);
psFitsTableGetNumTYPE(S16, S16);
psFitsTableGetNumTYPE(U16, U16);
psFitsTableGetNumTYPE(S32, S32);
psFitsTableGetNumTYPE(U32, U32);
psFitsTableGetNumTYPE(S64, S64);
psFitsTableGetNumTYPE(U64, U64);
psFitsTableGetNumTYPE(F32, F32);
psFitsTableGetNumTYPE(F64, F64);
psFitsTableGetNumTYPE(Bool, BOOL);

#define psFitsTableSetNumTYPE(TYPE, NAME) \
    bool psFitsTableSet##NAME(psFitsTable *table, int row, const char *name, ps##TYPE value) { \
									\
    if (row >= table->numRows || row < 0) { return false; }		\
									\
    bool mdok;								\
    int col = psMetadataLookupS32(&mdok, table->index, name);		\
    if (!mdok) { return false; }					\
    if (col < 0) { return false; }					\
									\
    psFitsTableColumn *column = &table->columns[col];			\
    switch (column->type) {						\
      case PS_DATA_S8:							\
	column->data.S8[row] = (psS8) value;				\
        break;								\
      case PS_DATA_S16:							\
	column->data.S16[row] = (psS16) value;				\
        break;								\
      case PS_DATA_S32:							\
	column->data.S32[row] = (psS32) value;				\
        break;								\
      case PS_DATA_S64:							\
	column->data.S64[row] = (psS64) value;				\
        break;								\
      case PS_DATA_U8:							\
	column->data.U8[row] = (psU8) value;				\
        break;								\
      case PS_DATA_U16:							\
	column->data.U16[row] = (psU16) value;				\
        break;								\
      case PS_DATA_U32:							\
	column->data.U32[row] = (psU32) value;				\
        break;								\
      case PS_DATA_U64:							\
	column->data.U64[row] = (psU64) value;				\
        break;								\
      case PS_DATA_F32:							\
	column->data.F32[row] = (psF32) value;				\
        break;								\
      case PS_DATA_F64:							\
	column->data.F64[row] = (psF64) value;				\
        break;								\
      case PS_DATA_BOOL:						\
	if (value) { column->data.F64[row] = 1; }			\
	else { column->data.F64[row] = 0; }				\
        break;								\
      default:								\
        /* if you get to this point, the value is not a number. */	\
	return false;							\
        break;								\
    }									\
    return true;							\
}

psFitsTableSetNumTYPE(S8, S8);
psFitsTableSetNumTYPE(U8, U8);
psFitsTableSetNumTYPE(S16, S16);
psFitsTableSetNumTYPE(U16, U16);
psFitsTableSetNumTYPE(S32, S32);
psFitsTableSetNumTYPE(U32, U32);
psFitsTableSetNumTYPE(S64, S64);
psFitsTableSetNumTYPE(U64, U64);
psFitsTableSetNumTYPE(F32, F32);
psFitsTableSetNumTYPE(F64, F64);
psFitsTableSetNumTYPE(Bool, BOOL);

// remove rows from table that are marked in the mask array as censored.
bool psFitsTableCensor(psFitsTable *table, bool *censorMask)
{
    int newRow = 0;
    for (int row = 0; row < table->numRows; row++) {
        if (censorMask[row]) {
            for (int col = 0; col < table->numCols; col++) {
                psFitsTableColumn *column = &table->columns[col];
                if (column->type == PS_DATA_VECTOR) {
                    psFree(column->data.vec[row]);
                    column->data.vec[row] = NULL;
                } else if (column->type == PS_DATA_STRING) {
                    psFree(column->data.str[row]);
                    column->data.str[row] = NULL;
                }
            }
        } else {
            if (newRow < row) {
                // slide data for this row into new destination
                for (int col = 0; col < table->numCols; col++) {
                    psFitsTableColumn *column = &table->columns[col];
                    switch (column->type) {
                    case PS_DATA_S16:    
                        column->data.S16[newRow] = column->data.S16[row];
                        break;
                    case PS_DATA_U16:    
                        column->data.U16[newRow] = column->data.U16[row];
                        break;
                    case PS_DATA_S32:    
                        column->data.S32[newRow] = column->data.S32[row];
                        break;
                    case PS_DATA_U32:    
                        column->data.U32[newRow] = column->data.U32[row];
                        break;
                    case PS_DATA_S64:    
                        column->data.S64[newRow] = column->data.S64[row];
                        break;
                    case PS_DATA_F32:    
                        column->data.F32[newRow] = column->data.F32[row];
                        break;
                    case PS_DATA_F64:    
                        column->data.F64[newRow] = column->data.F64[row];
                        break;
                    case PS_DATA_BOOL:
                        column->data.S8[newRow] = column->data.S8[row];
                        break;
                    case    PS_DATA_STRING:
                        // dest pointer has been freed above or moved
                        column->data.str[newRow] = column->data.str[row];
                        // zero source to avoid double free
                        column->data.str[row] = NULL;
                        break;
                    case    PS_DATA_VECTOR:
                        // dest pointer was freed above or zeroed.
                        column->data.vec[newRow] = column->data.vec[row];
                        // zero source to avoid double free
                        column->data.vec[row] = NULL;
                        break;
                    default:
                        psError(PS_ERR_PROGRAMMING, true, "unexpected column type: %d found", column->type);
                        return false;
                    }
                }
            }
            newRow++;
        }
    }

    // set the new number of rows
    table->numRows = newRow;

    return true;
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

static bool fitsInsertTableNew(psFits* fits,          // FITS file
                            const psMetadata* header, // FITS header to write
                            psFitsTable *table,       // internal representation of the table
                            const char *extname,      // Extension name to give table
                            bool after,               // Write table after current extension?
                            bool writeData            // Write data?
                            )
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PS_ASSERT_PTR_NON_NULL(table, false);

    int status = 0;

    long numRows = table->numRows;
    if (writeData && numRows < 1) {
        // no table data, what can I do?
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                _("Can't create a table without any rows."));
        return false;
    }

    // Create array of column names and types.
    psArray *columnNames = psArrayAlloc(table->numCols); // Array of column names, for cfitsio
    psArray *columnTypes = psArrayAlloc(table->numCols); // Array of column types, for cfitsio
    for (long i = 0; i < table->numCols; i++) {
        psFitsTableColumn *column = &table->columns[i];
        columnNames->data[i] = psMemIncrRefCounter(column->name);
        psString colType = NULL;        // The FITS column type
        size_t size = column->elementSize;
        if (column->type == PS_DATA_VECTOR) {
            psStringAppend(&colType, "%zd%c", size, getTForm(column->vectorType));
        } else {
            psStringAppend(&colType, "%zd%c", size, getTForm(column->type));
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
                        table->numCols, // number of columns in table
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
                         table->numCols, // number of columns in table
                         (char**)columnNames->data, // names of the columns
                         (char**)columnTypes->data, // format of the columns
                         NULL, // physical unit of columns
                         NULL, // skip extension name: we set this by hand below
                         0, &status);
    }
    psFree(columnNames);
    psFree(columnTypes);

    if (status != 0) {
        psFitsError(status, true, "Unable to create FITS table with %d columns and %d rows",
                    table->numCols, table->numRows);
        return false;
    }

    // Write header
    if (header && !psFitsWriteHeader(fits, header)) {
        psError(PS_ERR_IO, false, "Unable to write FITS header.\n");
        return false;
    }

    // write the header, if any.
    if (extname && strlen(extname) > 0) {
        if (!psFitsSetExtName(fits, extname)) {
            psError(PS_ERR_IO, false, "Unable to write FITS header extension name.\n");
            return false;
        }
    }

    if (writeData) {
        // psMetadataIteratorSet(colSpecsIter, PS_LIST_HEAD);
        for (long colNum = 1; colNum <= table->numCols; colNum++) {
            // Note: colNum is unit-indexed, because it's for cfitsio
            psFitsTableColumn *column = &table->columns[colNum - 1];
            // colSpec *spec = colSpecItem->data.V; // The specification
            if (PS_DATA_IS_PRIMITIVE(column->type)) {
                int fitsDataType;           // Data type for cfitsio
                p_psFitsTypeToCfitsio(column->type, NULL, NULL, &fitsDataType);
                fits_write_col(fits->fd,
                               fitsDataType,
                               colNum, // column number
                               1, // first row
                               1, // first element
                               table->numRows, // number of rows
                               column->data.U8, // the data
                               &status);
                // psFree(columnData);
            } else {
                switch (column->type) {
                  case PS_DATA_STRING: {
                      fits_write_col_str(fits->fd, colNum, 1, 1, table->numRows, (char**)column->data.str, &status);
                      // psFree(strings);
                      break;
                  }
                  case PS_DATA_VECTOR: {
                      size_t dataSize = PSELEMTYPE_SIZEOF(column->vectorType); // Size of data, in bytes
                      psVector *columnData = psVectorAlloc(column->elementSize * table->numRows * dataSize, PS_TYPE_U8);
                      psVectorInit(columnData, 0);
                      for (long i = 0; i < table->numRows; i++) {
                          psVector *vector = column->data.vec[i];
                          memcpy(&columnData->data.U8[i * dataSize * column->elementSize], vector->data.U8,
                                 vector->n * dataSize);
                      }

                      int fitsDataType;           // Data type for cfitsio
                      p_psFitsTypeToCfitsio(column->vectorType, NULL, NULL, &fitsDataType);
                      fits_write_col(fits->fd, fitsDataType, colNum, 1, 1, table->numRows * column->elementSize,
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
                return false;
            }
        }
    }

    // This forces a re-scan of the header to ensure everything's kosher.  We found this occassionally
    // necessary for compressed images, which are tables, so perhaps it helps here too.  I guess it can't
    // hurt.
    ffrdef(fits->fd, &status);
    if (psFitsError(status, true, "Could not re-scan HDU.")) {
        return false;
    }

    return true;
}

bool psFitsWriteTableNew(psFits* fits,
                      const psMetadata* header,
                      psFitsTable* table,
                      const char *extname)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    if (!psFitsMoveLast(fits)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to move to last extension to write table");
        return false;
    }
    bool writeData = table->numRows > 0;
    return fitsInsertTableNew(fits, header, table, extname, true, writeData);
}
