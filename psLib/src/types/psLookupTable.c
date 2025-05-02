/** @file  psLookupTable.c
*
*  @brief This file defines the structure and functions for table lookups.
*
*  @ingroup types
*
*  @author Ross Harman, MHPCC
*
*  @version $Revision: 1.51 $ $Name: not supported by cvs2svn $
*  @date $Date: 2008-04-17 23:43:03 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, Univ. of Hawaii
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#undef __STRICT_ANSI__
#include <stdlib.h>
#define __STRICT_ANSI__
#include <limits.h>
#include <math.h>
#include <inttypes.h>

#include "psAbort.h"
#include "psMemory.h"
#include "psString.h"
#include "psError.h"
#include "psString.h"
#include "psSlurp.h"
#include "psLookupTable.h"

#include "psAssert.h"

/******************************************************************************/
/*  DEFINE STATEMENTS                                                         */
/******************************************************************************/

/** Maximum size of a string */
#define MAX_STRING_LENGTH   256
#define ARRAY_STRIDE        16
#define INITIAL_NUM         10          // Initial number of elements
/******************************************************************************/
/*  TYPE DEFINITIONS                                                          */
/******************************************************************************/

// None

/*****************************************************************************/
/*  GLOBAL VARIABLES                                                         */
/*****************************************************************************/

// None

/*****************************************************************************/
/*  FILE STATIC VARIABLES                                                    */
/*****************************************************************************/

// None

/*****************************************************************************/
/*  FUNCTION IMPLEMENTATION - LOCAL                                          */
/*****************************************************************************/

static bool ignoreLine(char *inString);
static char *cleanString(char *inString, int sLen);
static char* getToken(char **inString, char *delimiter, psParseErrorType *status);
static void parseValue(psVector *vec, psU64 index, char* strValue, psParseErrorType *status);
static void lookupTableFree(psLookupTable* table);

/* Determines if a line is blank (whitespace only) or a commentline. It returns true if so.
   The input string must be null terminated. */
static bool ignoreLine(char *inString)
{
    while (*inString!='\0' && *inString!='#') {
        if (!isspace(*inString)) {
            return false;
        }
        inString++;
    }
    return true;
}


/* Removes leading and trailing whitespace and # characters from a string. The
   cleaned string is a new null terminated copy of the original input string. */
static char *cleanString(char *inString,
                         int sLen)
{
    char *ptrB = NULL;
    char *ptrE = NULL;
    char *cleaned = NULL;

    ptrB = inString;

    // Skip over leading # or whitespace
    //    while (isspace(*ptrB) || *ptrB=='#') {
    //        ptrB++;
    //    }

    // Skip over trailing whitespace, null terminators, and # characters
    ptrE = inString + sLen;
    while (isspace(*ptrE) || *ptrE=='\0' || *ptrE=='#') {
        ptrE--;
    }

    // Length, sLen, does not include '\0'
    sLen = ptrE - ptrB + 1;

    // Adds '\0' to end of string and +1 to sLen
    cleaned = psStringNCopy(ptrB, sLen);

    return cleaned;
}


/* Returns cleaned token based on delimiter, but not including delimiter. Also changes
   the pointer location the beginning of the string. Tokens are newly allocated null
    terminated strings. */
static char* getToken(char **inString,
                      char *delimiter,
                      psParseErrorType *status)
{
    char *cleanToken = NULL;
    int sLen = 0;

    // Skip over leading whitespace
    while (isspace(**inString)) {
        (*inString)++;
    }

    // Length of token, not including delimiter
    sLen = strcspn(*inString, delimiter);
    if (sLen) {

        // Create new, cleaned, and null terminated token
        cleanToken = cleanString(*inString, sLen);

        // Move to end of token
        (*inString) += sLen;
    }
    /*    else if (**inString!='\0' && sLen==0) {
            *status = PS_PARSE_ERROR_GENERAL;
        }
    */
    return cleanToken;
}

#define PARSE_VALUE_INT_CASE(TYPE, FUNC) \
    case PS_TYPE_##TYPE: { \
        char *end = NULL; \
        ps##TYPE value = FUNC(strValue, &end, 0); \
        if (*end != '\0' && *end != '\n' && !isspace(*end)) { \
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Characters left over after parsing %s: %s", \
                strValue, end); \
            *status = PS_PARSE_ERROR_VALUE; \
        } \
        vec->data.TYPE[index] = value; \
        return; \
    }

#define PARSE_VALUE_FLOAT_CASE(TYPE, FUNC) \
    case PS_TYPE_##TYPE: { \
        char *end = NULL; \
        ps##TYPE value = FUNC(strValue, &end); \
        if (*end != '\0' && *end != '\n' && !isspace(*end)) { \
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Characters left over after parsing %s: %s", \
                strValue, end); \
            *status = PS_PARSE_ERROR_VALUE; \
        } \
        vec->data.TYPE[index] = value; \
        return; \
    }


/* Returns single parsed value as a double precision number. The input string must be
   cleaned and null terminated. */
static void parseValue(psVector *vec,
                       psU64 index,
                       char* strValue,
                       psParseErrorType *status)
{
    switch (vec->type.type) {
        PARSE_VALUE_INT_CASE(S32, strtol);
        PARSE_VALUE_INT_CASE(S64, strtoll);
        PARSE_VALUE_FLOAT_CASE(F32, strtof);
        PARSE_VALUE_FLOAT_CASE(F64, strtod);
      default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Bad type for vector: %x.\n", vec->type.type);
        *status = PS_PARSE_ERROR_TYPE;
    }
    return;
}

static void lookupTableFree(psLookupTable* table)
{
    psAssert(table, "impossible");
    psFree(table->values);
    psFree(table->filename);
    psFree(table->format);
    psFree(table->index);
}

/*****************************************************************************/
/* FUNCTION IMPLEMENTATION - PUBLIC                                          */
/*****************************************************************************/

bool psMemCheckLookupTable(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)lookupTableFree );
}


psLookupTable* p_psLookupTableAlloc(const char *file,
                                    unsigned int lineno,
                                    const char *func,
                                    const char *filename,
                                    const char *format,
                                    long indexCol)
{
    PS_ASSERT_STRING_NON_EMPTY(filename,NULL);
    PS_ASSERT_STRING_NON_EMPTY(format,NULL);

    psLookupTable *outTable = p_psAlloc(file, lineno, func, sizeof(psLookupTable));

    // Set deallocator
    psMemSetDeallocator(outTable, (psFreeFunc)lookupTableFree);

    // Allocate and set file name and format strings
    outTable->filename = psStringCopy(filename);
    outTable->format = psStringCopy(format);

    // Valid ranges. Automatically set by table read if both zero.
    *(double *)&outTable->validFrom = 0;
    *(double *)&outTable->validTo = 0;
    outTable->indexCol = indexCol;

    // Vector of independent index values. Filled by table read.
    outTable->index = NULL;

    // Array of dependent table values corresponding to index values. Filled by table read.
    outTable->values = NULL;

    return outTable;
}

psArray *psVectorsReadFromFile(const char *filename, const char *format)
{
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);
    PS_ASSERT_STRING_NON_EMPTY(format, NULL);

    psArray *outputArray = psArrayAllocEmpty(INITIAL_NUM); // Array of vectors to return
    psParseErrorType parseStatus = PS_PARSE_SUCCESS; // Status of parsing

    // Parse the format string to determine how many vectors and whether the format string is valid
    const char *tempFormat = format;    // Pointer into format
    psString strValue;                  // Format of interest
    int numCols = 0;                    // Number of columns found in format
    while ((strValue = getToken((char**)&tempFormat, " \t", &parseStatus)) &&
           parseStatus == PS_PARSE_SUCCESS) {
        if (strstr(strValue,"\%*") != 0) {
            // Don't increase number of columns
            continue;
        }
        psElemType type;                // Type specified
        if (strcmp(strValue,"\%d") == 0 ) {
            type = PS_TYPE_S32;
        } else if (strcmp(strValue,"\%ld") == 0) {
            type = PS_TYPE_S64;
        } else if (strcmp(strValue,"\%f") == 0) {
            type = PS_TYPE_F32;
        } else if (strcmp(strValue,"\%lf") == 0) {
            type = PS_TYPE_F64;
        } else {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Invalid format specifier: %s", strValue);
            psFree(strValue);
            psFree(outputArray);
            return NULL;
        }
        psVector *colVector = psVectorAllocEmpty(1, type); // Vector for type
        outputArray = psArrayAdd(outputArray, ARRAY_STRIDE, colVector);
        psFree(colVector);
        numCols++;
        psFree(strValue);
    }
    if (parseStatus != PS_PARSE_SUCCESS) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Failed to parse format at column %d: %s",
                numCols, strValue);
        psFree(strValue);
        psFree(outputArray);
        return NULL;
    }

    if (numCols == 0) {
        // Format string parse error detected
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Format string was not parsed sucessfully");
        psFree(outputArray);
        return NULL;
    }

    // If the format string was parsed successfully and return numCols the
    // prepare to open file and read values

    psString file = psSlurpFilename(filename); // Contents of file
    if (!file) {
        psError(psErrorCodeLast(), false, "Unable to read file of vectors");
        psFree(outputArray);
        return NULL;
    }

    psArray *lines = psStringSplitArray(file, "\n", false); // Lines of file
    psFree(file);
    long numRows = 0;                                  // Number of rows
    for (long i = 0; i < lines->n; i++) {
        psString line = lines->data[i]; // Line of interest
        if (ignoreLine(line)) {
            continue;
        }
        numRows++;

        char *linePtr = line;           // Pointer into line

        // Copy format pointer for parsing
        const char *tempFormat = format; // Pointer into format
        long arrayIndex = 0;            // Index in array
        parseStatus = PS_PARSE_SUCCESS;

        // Loop through format and line strings to get values in text table file
        char *strNum;                   // Number within line
        while ((strValue=getToken((char**)&tempFormat," \t",&parseStatus)) &&
               (strNum=getToken((char**)&linePtr," \t",&parseStatus)) &&
               parseStatus == PS_PARSE_SUCCESS) {
            if (strstr(strValue,"\%*") != 0) {
                continue;
            }

            // Set column vector
            psVector *colVector = outputArray->data[arrayIndex]; // Column vector of interest

            outputArray->data[arrayIndex] = colVector = psVectorRecycle(colVector, numRows,
                                                                        colVector->type.type);
            parseValue(colVector, numRows - 1, strNum, &parseStatus);
            arrayIndex++;

            if (parseStatus != PS_PARSE_SUCCESS) {
                psError(PS_ERR_UNKNOWN, false, "Parsing text file failed: %s as %s", strNum, strValue);
                psFree(outputArray);
                psFree(lines);
                psFree(strNum);
                psFree(strValue);
                return NULL;
            }
            psFree(strValue);
            psFree(strNum);

        }
        if (strValue != NULL && strNum == NULL) {
            psError(PS_ERR_UNKNOWN, true,
                    "Parsing text file failed - missing table value(s).");
            psFree(outputArray);
            psFree(lines);
            psFree(strValue);
            return NULL;
        }
    }
    if (parseStatus != PS_PARSE_SUCCESS) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Failed to parse format at column %d: %s",
                numCols, strValue);
        psFree(strValue);
        psFree(outputArray);
        return NULL;
    }

    psFree(lines);

    // Return populated array
    return outputArray;
}

bool p_psLookupTableImport(const char *file,
                           unsigned int lineno,
                           const char *func,
                           psLookupTable *table,
                           psArray *vectors,
                           long indexCol)
{
    PS_ASSERT_LOOKUPTABLE_NON_NULL(table, false);
    PS_ASSERT_ARRAY_NON_NULL(vectors, false);
    PS_ASSERT_INT_NONNEGATIVE(indexCol, false);
    if (indexCol >= vectors->n) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Index column, %ld, is larger"
                " than number of columns, %ld.", indexCol, vectors->n);
        return false;
    }

    psVector *indexVector = vectors->data[indexCol]; // The vector that provides the index
    long numRows = indexVector->n;      // Number of rows in table
    long numCols = vectors->n;          // Number of columns in table

#define CHECK_SORT_CASE(TYPE) \
    case PS_TYPE_##TYPE: \
        for (long i = 1; i < numRows; i++) { \
             if (indexVector->data.TYPE[i] < indexVector->data.TYPE[i - 1]) { \
                 sortNeeded = true; \
                 break; \
             } \
         } \
         break;

    // Check if array is sorted on the index column
    bool sortNeeded  = false;           // Do we need to sort?
    switch (indexVector->type.type) {
        CHECK_SORT_CASE(U8);
        CHECK_SORT_CASE(U16);
        CHECK_SORT_CASE(U32);
        CHECK_SORT_CASE(U64);
        CHECK_SORT_CASE(S8);
        CHECK_SORT_CASE(S16);
        CHECK_SORT_CASE(S32);
        CHECK_SORT_CASE(S64);
        CHECK_SORT_CASE(F32);
        CHECK_SORT_CASE(F64);
      default:
        psAbort("Should never get here: bad type.");
    }

    if (sortNeeded) {
        psVector *sortedIndex = psVectorSortIndex(NULL, indexVector); // Indices for sorting
        psArray *newValues = psArrayAlloc(numCols);
        for (long i = 0; i < numCols; i++) {
            psVector *oldVector = vectors->data[i]; // Vector of interest
            psVector *newVector = psVectorAlloc(numRows, oldVector->type.type);
            newValues->data[i] = newVector;

#define REARRANGE_CASE(TYPE) \
    case PS_TYPE_##TYPE: \
        for (long j = 0; j < numRows; j++) { \
            newVector->data.TYPE[j] = oldVector->data.TYPE[sortedIndex->data.S32[j]]; \
        } \
        break;

            switch (oldVector->type.type) {
                REARRANGE_CASE(U8);
                REARRANGE_CASE(U16);
                REARRANGE_CASE(U32);
                REARRANGE_CASE(U64);
                REARRANGE_CASE(S8);
                REARRANGE_CASE(S16);
                REARRANGE_CASE(S32);
                REARRANGE_CASE(S64);
                REARRANGE_CASE(F32);
                REARRANGE_CASE(F64);
              default:
                psAbort("Should never get here: bad type.");
            }
        }

        psFree(sortedIndex);
        table->values = newValues;
    } else {
        table->values = psMemIncrRefCounter(vectors);
    }

    table->index = psMemIncrRefCounter(table->values->data[indexCol]);
    table->indexCol = indexCol;

#define SET_VALID_CASE(TYPE) \
    case PS_TYPE_##TYPE: \
        *(double*)&table->validFrom = table->index->data.TYPE[0]; \
        *(double*)&table->validTo   = table->index->data.TYPE[numRows - 1]; \
        break;

    switch (table->index->type.type) {
        SET_VALID_CASE(U8);
        SET_VALID_CASE(U16);
        SET_VALID_CASE(U32);
        SET_VALID_CASE(U64);
        SET_VALID_CASE(S8);
        SET_VALID_CASE(S16);
        SET_VALID_CASE(S32);
        SET_VALID_CASE(S64);
        SET_VALID_CASE(F32);
        SET_VALID_CASE(F64);
      default:
        psAbort("Should never get here: bad type.");
    }

    return true;
}

long psLookupTableRead(psLookupTable* table)
{
    PS_ASSERT_LOOKUPTABLE_NON_NULL(table, 0);

    // Read vectors from file specified in table
    psArray *vectors = psVectorsReadFromFile(table->filename, table->format); // Vectors in file
    if (!vectors) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read vectors from %s", table->filename);
        return 0;
    }

    // Import vector into table
    if (!psLookupTableImport(table, vectors, table->indexCol)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to import vectors from %s into lookup table.",
                table->filename);
        psFree(vectors);
        return 0;
    }
    psFree(vectors);                // Drop reference

    return table->index->n;
}

#define CONVERT_VALUE_TO_F64(VECTOR,INDEX,RESULT)     \
switch(VECTOR->type.type) {                           \
case PS_TYPE_U8:                                      \
    RESULT = (psF64)VECTOR->data.U8[INDEX];           \
    break;                                            \
case PS_TYPE_U16:                                     \
    RESULT = (psF64)VECTOR->data.U16[INDEX];          \
    break;                                            \
case PS_TYPE_U32:                                     \
    RESULT = (psF64)VECTOR->data.U32[INDEX];          \
    break;                                            \
case PS_TYPE_U64:                                     \
    RESULT = (psF64)VECTOR->data.U64[INDEX];          \
    break;                                            \
case PS_TYPE_S8:                                      \
    RESULT = (psF64)VECTOR->data.S8[INDEX];           \
    break;                                            \
case PS_TYPE_S16:                                     \
    RESULT = (psF64)VECTOR->data.S16[INDEX];          \
    break;                                            \
case PS_TYPE_S32:                                     \
    RESULT = (psF64)VECTOR->data.S32[INDEX];          \
    break;                                            \
case PS_TYPE_S64:                                     \
    RESULT = (psF64)VECTOR->data.S64[INDEX];          \
    break;                                            \
case PS_TYPE_F32:                                     \
    RESULT = (psF64)VECTOR->data.F32[INDEX];          \
    break;                                            \
case PS_TYPE_F64:                                     \
    RESULT = VECTOR->data.F64[INDEX];                 \
    break;                                            \
default:                                              \
    RESULT = NAN;                                     \
    break;                                            \
}

#define CHECK_LOWER_UPPER_BOUND(TABLE,INDEX,COLUMN)                     \
switch (TABLE->index->type.type) {                                      \
case PS_TYPE_S32:                                                       \
    if ( (psS32)index < TABLE->index->data.S32[0] ) {                    \
        return NAN;                                                     \
    }                                                                   \
    if ( (psS32)index > TABLE->index->data.S32[numRows-1] ) {            \
        return NAN;                                                     \
    }                                                                   \
    break;                                                              \
case PS_TYPE_S64:                                                       \
    if ( (psS64)index < TABLE->index->data.S64[0] ) {                    \
        return NAN;                                                     \
    }                                                                   \
    if ( (psS64)index > TABLE->index->data.S64[numRows-1] ) {            \
        return NAN;                                                     \
    }                                                                   \
    break;                                                              \
case PS_TYPE_F32:                                                       \
    if ( (psF32)index < TABLE->index->data.F32[0] ) {                    \
        return NAN;                                                     \
    }                                                                   \
    if ( (psF32)index > TABLE->index->data.F32[numRows-1] ) {            \
        return NAN;                                                     \
    }                                                                   \
    break;                                                              \
case PS_TYPE_F64:                                                       \
    if ( index < TABLE->index->data.F64[0] ) {                           \
        return NAN;                                                     \
    }                                                                   \
    if ( index > TABLE->index->data.F64[numRows-1] ) {                   \
        return NAN;                                                     \
    }                                                                   \
    break;                                                              \
default:                                                                \
    return NAN;                                                         \
    break;                                                              \
}

double psLookupTableInterpolate(const psLookupTable *table,
                                double index,
                                long column)
{
    PS_ASSERT_LOOKUPTABLE_NON_NULL(table, NAN);
    // Ensuring information exists
    PS_ASSERT_VECTOR_NON_NULL(table->index, NAN);
    PS_ASSERT_ARRAY_NON_NULL(table->values, NAN);
    PS_ASSERT_INT_WITHIN_RANGE(column, 0, (int)table->values->n - 1, NAN);
    PS_ASSERT_VECTOR_NON_NULL((psVector*)table->values->data[column], NAN);

    psU64 hiIdx = 0;
    psU64 loIdx = 0;
    long  numRows = 0;
    psF64 out = 0.0;
    psF64 denom = 0.0;
    psF64 convertVal = 0.0;
    psF64 tempVal = 0.0;
    psVector *indexVec = NULL;
    psVector *valuesVec = NULL;
    psArray *values = NULL;

    indexVec = table->index;
    values = table->values;
    numRows = table->index->n;
    valuesVec = (psVector*)values->data[column];

    // Verify the index is within the bounds of the table
    CHECK_LOWER_UPPER_BOUND(table,index,column);

    // Find location in table where specified index is between to entries
    CONVERT_VALUE_TO_F64(indexVec, 0, convertVal)
    while (index > convertVal ) {
        hiIdx++;
        /*  XXX:  following is unreachable.
                if (hiIdx >= numRows) {
                    psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                            _("High index too big, %" PRIu64 "."), hiIdx);
                    return NAN;
                }
        */
        CONVERT_VALUE_TO_F64(indexVec, hiIdx, convertVal)
    }

    // Check for negative low index and generate error
    loIdx = hiIdx--;
    if (loIdx < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Low index too small, %" PRIu64 "."), loIdx);
        return NAN;
    }

    // Perform linear interpolation to calculate return value
    CONVERT_VALUE_TO_F64(indexVec, hiIdx, denom)
    CONVERT_VALUE_TO_F64(indexVec, loIdx, convertVal);
    denom -= convertVal;
    if (fabs(denom) < FLT_EPSILON) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Divide by zero error during interpolation."));
        return NAN;
    } else {
        CONVERT_VALUE_TO_F64(valuesVec,hiIdx,tempVal)
        CONVERT_VALUE_TO_F64(valuesVec,loIdx,convertVal)
        tempVal -= convertVal;
        CONVERT_VALUE_TO_F64(indexVec,loIdx,convertVal)
        out = tempVal*(index-convertVal)/denom;
        CONVERT_VALUE_TO_F64(valuesVec,loIdx,convertVal)
        out += convertVal;
    }

    return out;
}

psVector* psLookupTableInterpolateAll(const psLookupTable *table,
                                      double index)
{
    PS_ASSERT_LOOKUPTABLE_NON_NULL(table, NULL);
    // Ensuring information exists
    PS_ASSERT_VECTOR_NON_NULL(table->index, NULL);
    PS_ASSERT_ARRAY_NON_NULL(table->values, NULL);
    long numCols = table->values->n;
    if (numCols == 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "No columns in lookup table from which to interpolate.");
        return NULL;
    }

    psVector *outVector = psVectorAlloc(numCols, PS_TYPE_F64);

    // Fill vectors with results and status of results
    // XXX: This algorithm must be changed to something more efficient!
    for (long i = 0; i < numCols; i++) {
        outVector->data.F64[i] = psLookupTableInterpolate(table, index, i);
        if (isnan(outVector->data.F64[i])) {
            // Free allocated vector
            psFree(outVector);
            outVector = NULL;
            // Break out of loop since error detected
            break;
        }
    }

    return outVector;
}

