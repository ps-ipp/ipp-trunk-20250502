/* @file  psFitsTableNew.h
 * @brief Contains Fits I/O routines
 *
 * @author EAM, PAP, JH
 * @author Robert DeSonia, MHPCC
 *
 * @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-10-09 02:56:23 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_FITSTABLENEW_H
#define PS_FITSTABLENEW_H

typedef struct {
    psString    name;
    psDataType  type;
    psDataType  vectorType;             // type inside if Vector
    int         elementSize;            // 1 for primitives, strlen for strings
                                        // vector length for vectors
    // only types supported by fits tables will be implemented
    union {
        bool *BOOL;                     ///< boolean data
        psS8 *S8;                       ///< Signed 8-bit integer data.
        psS16 *S16;                     ///< Signed 16-bit integer data.
        psS32 *S32;                     ///< Signed 32-bit integer data.
        psS64 *S64;                     ///< Signed 64-bit integer data.
        psU8 *U8;                       ///< Unsigned 8-bit integer data.
        psU16 *U16;                     ///< Unsigned 16-bit integer data.
        psU32 *U32;                     ///< Unsigned 32-bit integer data.
        psU64 *U64;                     ///< Unsigned 64-bit integer data.
        psF32 *F32;                     ///< Single-precision float data.
        psF64 *F64;                     ///< Double-precision float data.
        psString *str;                  ///< string data
        psVector **vec;                 ///< vector
    } data;                             ///< Union for data types.
} psFitsTableColumn;

typedef struct {
    int numRows;
    int numCols;
    psMetadata  *index;
    psFitsTableColumn *columns;
} psFitsTable;

// return column number of column with name (-1 if not found)
int psFitsTableGetColumnNumber(psFitsTable *table, const char *name);

psFitsTable *psFitsReadTableNew(const psFits *fits);
bool psFitsWriteTableNew(psFits *fits, const psMetadata *header, psFitsTable *table, const char *extname);

// remove rows from table whose entry in the supplied array are true
bool psFitsTableCensor(psFitsTable *table, bool *rowMask);

// Get value for given row and column name
psBool psFitsTableGetBool(bool *status, psFitsTable *table, int row, const char* name);
psS8  psFitsTableGetS8(bool *status, psFitsTable *table, int row, const char* name);
psU8  psFitsTableGetU8(bool *status, psFitsTable *table, int row, const char* name);
psS16 psFitsTableGetS16(bool *status, psFitsTable *table, int row, const char* name);
psU16 psFitsTableGetU16(bool *status, psFitsTable *table, int row, const char* name);
psS32 psFitsTableGetS32(bool *status, psFitsTable *table, int row, const char* name);
psU32 psFitsTableGetU32(bool *status, psFitsTable *table, int row, const char* name);
psS64 psFitsTableGetS64(bool *status, psFitsTable *table, int row, const char* name);
psU64 psFitsTableGetU64(bool *status, psFitsTable *table, int row, const char* name);
psF32 psFitsTableGetF32(bool *status, psFitsTable *table, int row, const char* name);
psF64 psFitsTableGetF64(bool *status, psFitsTable *table, int row, const char* name);

psFitsTable *psFitsTableCreate (psArray *tableColumns, int numRows);
bool psFitsTableColumnAdd (psArray *tableColumns, char *name, psDataType type);
psFitsTableColumn *psFitsTableColumnAlloc (char *name, psDataType type);

bool psFitsTableSetS8 (psFitsTable *table, int row, const char* name, psS8  value);
bool psFitsTableSetU8 (psFitsTable *table, int row, const char* name, psU8  value);
bool psFitsTableSetS16(psFitsTable *table, int row, const char* name, psS16 value);
bool psFitsTableSetU16(psFitsTable *table, int row, const char* name, psU16 value);
bool psFitsTableSetS32(psFitsTable *table, int row, const char* name, psS32 value);
bool psFitsTableSetU32(psFitsTable *table, int row, const char* name, psU32 value);
bool psFitsTableSetS64(psFitsTable *table, int row, const char* name, psS64 value);
bool psFitsTableSetU64(psFitsTable *table, int row, const char* name, psU64 value);
bool psFitsTableSetF32(psFitsTable *table, int row, const char* name, psF32 value);
bool psFitsTableSetF64(psFitsTable *table, int row, const char* name, psF64 value);

#endif
