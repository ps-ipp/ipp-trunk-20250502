/** @file psDB.c
 *
 * Copyright (C) 2005-2007  Joshua Hoblitt, University of Hawaii
 * Copyright (C) 2005  Aaron Culliney
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * program; see the file COPYING. If not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * @brief database functions
 *
 * This file contains functions that perform basic database operations.  MySQL
 * 4.1.2 or newer is required.
 *
 * $Id: psDB.c,v 1.171 2009-02-07 01:11:24 eugene Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_PSDB

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>
#undef __STRICT_ANSI__
#include <stdlib.h>
#define __STRICT_ANSI__
#include <math.h>
#include <inttypes.h>
#include <sys/types.h>
#include <regex.h>
#include <mysql.h>
// #include <mysql_com.h> // enum_field_types

#include "psDB.h"
#include "psMemory.h"
#include "psAssert.h"
#include "psAbort.h"
#include "psError.h"
#include "psString.h"
#include "psTrace.h"
#include "psMetadataConfig.h"

# if (MYSQL_VERSION_ID >= 80001) && !defined MARIADB_VERSION_ID 
    # define MYSQL_BOOL bool
# else
    # define MYSQL_BOOL my_bool
# endif

// set the pointer to NULL if we are actually freeing the memory
#define PSDB_NULL_FREE(ptr) \
if (psMemGetRefCounter(ptr) == 1) { \
    psFree(ptr); \
    ptr = NULL; \
} else { \
    psFree(ptr); \
}

typedef struct
{
    enum enum_field_types type;
    bool            isUnsigned;
}
mysqlType;

typedef struct
{
    MYSQL_BIND      *bind;
    psU32           n;
}
psDBMysqlRow;

// cache of prepared query statements
static pthread_mutex_t preparedQueryMutex = PTHREAD_MUTEX_INITIALIZER;
static psHash *preparedQuery = NULL;

static pthread_mutex_t lookupTableMutex = PTHREAD_MUTEX_INITIALIZER;
static psHash   *pTypeToSQLlookupTable = NULL;
static psHash   *sqlToPTypeLookupTable = NULL;
static psHash   *mysqlToSqlLookupTable = NULL;
static psHash   *pTypeToMysqlLookupTable = NULL;

// free func
static void psDBFree(psDB *dbh);

// database utility functions
static inline bool psDBPackMySQLRow(psDBMysqlRow *mysqlRow, const psMetadata *values);
static psDBMysqlRow *psDBMysqlRowAlloc(psU32 paramCount);
static void psDBMysqlRowFree(psDBMysqlRow *row);
static void psDBMysqlRowRecycle(psDBMysqlRow *mysqlRow);

// SQL generation functions
static psString psDBGenerateCreateTableSQL(const char *tableName, const psMetadata *where);
static psString psDBGenerateSelectRowSQL(const char *tableName, const char *col,
        const psMetadata *where, psU64 limit);
static psString psDBGenerateInsertRowSQL(const char *tableName, const psMetadata *row);
static psString psDBGenerateUpdateRowSQL(const char *tableName, const psMetadata *where,
        const psMetadata *values);
static psString psDBGenerateDeleteRowSQL(const char *tableName, const psMetadata *where,
        unsigned long long limit);
static psString psDBGenerateSetSQL(const psMetadata *set
                                  );
static char *psDBGenerateConditionalSQL(const psMetadataItem *item, const char *tableName);

// lookup table functions
static psElemType psDBMySQLToPType(enum enum_field_types type, unsigned int flags);
static psString psDBPTypeToSQL(psElemType pType);
static mysqlType *psDBPTypeToMySQL(psElemType pType);

static psHash  *psDBPTypeToSQLTableSetup(void);
static psHash  *psDBPTypeToSQLTableGet(void);
static void     psDBPTypeToSQLTableCleanup(void);

static psHash  *psDBSQLToPTypeTableSetup(void);
static psHash  *psDBSQLToPTypeTableGet(void);
static void     psDBSQLToPTypeTableCleanup(void);

static psHash  *psDBMySQLToSQLTableSetup(void);
static psHash  *psDBMySQLToSQLTableGet(void);
static void     psDBMySQLToSQLTableCleanup(void);

static psHash  *psDBPTypeToMySQLTableSetup(void);
static psHash  *psDBPTypeToMySQLTableGet(void);
static void     psDBPTypeToMySQLTableCleanup(void);

static psPtr    psDBMySQLTypeAlloc(enum enum_field_types type, bool isUnsigned);
static void     psDBAddToLookupTable(psHash *lookupTable, psU32 type, const char *string);
static void     psDBAddVoidToLookupTable(psHash *lookupTable, psU32 type, psPtr value);
static psErrorCode mysqlTopsErr(MYSQL *mysql);

// pType utility functions
static psPtr      psDBGetPTypeNaN(psElemType pType);
static MYSQL_BOOL psDBIsPTypeNaN(psElemType pType, psPtr data);

// public functions
/*****************************************************************************/

psDB *psDBAlloc(const char *host,
                const char *user,
                const char *passwd,
                const char *dbname,
                unsigned int port)
{
    MYSQL           *mysql;
    psDB            *dbh;

    mysql = mysql_init(NULL);
    if (!mysql) {
        psAbort("mysql_init(), out of memory.");
    }

    // without this call we won't pick up anything from my.cnf
    mysql_options(mysql, MYSQL_READ_DEFAULT_GROUP, "client");

    // Connect to host and mySql server with specified database
    if (!mysql_real_connect(mysql, host, user, passwd, dbname, port, NULL, 0)) {
        psError(mysqlTopsErr(mysql), true,
                _("Failed to connect to database.  Error: %s"),mysql_error(mysql));

        mysql_close(mysql);

        return NULL;
    }

    dbh = psAlloc(sizeof(psDB));

    dbh->mysql = mysql;

    // explicit transactions default to false
    if (!psDBExplicitTrans(dbh, false)) {
        psError(mysqlTopsErr(dbh->mysql), true,
                "failed to set transaction type. Error: %s", mysql_error(mysql));

        mysql_close(mysql);
        psFree(dbh);

        return NULL;
    }

    if (psMemGetThreadSafety()) {
        pthread_mutex_lock(&lookupTableMutex);

        // psDBPTypeToSQLTableSetu must be run first
        psDBPTypeToSQLTableSetup();
        psDBSQLToPTypeTableSetup();
        psDBMySQLToSQLTableSetup();
        psDBPTypeToMySQLTableSetup();

        pthread_mutex_unlock(&lookupTableMutex);
    } else {
        // psDBPTypeToSQLTableSetu must be run first
        psDBPTypeToSQLTableSetup();
        psDBSQLToPTypeTableSetup();
        psDBMySQLToSQLTableSetup();
        psDBPTypeToMySQLTableSetup();
    }

    // don't set the deallocator func until after we've setup the lookup tables
    // so an alloc error doesn't try to free potentionally uncreated tables
    psMemSetDeallocator(dbh, (psFreeFunc) psDBFree);

    psTrace("psLib.db", PS_LOG_INFO, "connected to database %s", dbname);

    return dbh;
}

static void psDBFree(psDB *dbh)
{
    // quietly handle NULLs
    if (!dbh) {
        return;
    }

    // Attempt to close specified database connection
    mysql_close(dbh->mysql);

    // ASC WARNING NOTE: the psDBSQLToPTypeTableCleanup cleanup routine
    // needs to be called first because it refers to
    // psDBPTypeToSQLTableGet ...
    //
    // The *cleanup functions should be thread safe as they just call psFree()
    // but we don't want to be cleaning up & setting up at the same time
    if (psMemGetThreadSafety()) {
        pthread_mutex_lock(&lookupTableMutex);

        psDBSQLToPTypeTableCleanup();
        psDBMySQLToSQLTableCleanup();
        psDBPTypeToSQLTableCleanup();
        psDBPTypeToMySQLTableCleanup();

        pthread_mutex_unlock(&lookupTableMutex);
    } else {
        psDBSQLToPTypeTableCleanup();
        psDBMySQLToSQLTableCleanup();
        psDBPTypeToSQLTableCleanup();
        psDBPTypeToMySQLTableCleanup();
    }

    psTrace("psLib.db", PS_LOG_INFO, "disconnected");
}

bool psDBCreate(psDB *dbh,
                const char *dbname)
{
    PS_ASSERT_PTR_NON_NULL(dbh, false);
    PS_ASSERT_PTR_NON_NULL(dbname, false);

    // the MySQL C API notes that mysql_create_db() is deprecated
    bool status = p_psDBRunQueryF(dbh, "CREATE DATABASE %s", dbname);
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "Failed to create new database.");
    }

    psTrace("psLib.db", PS_LOG_INFO, "created a database named %s", dbname);

    return status;
}

bool psDBChange(psDB *dbh,
                const char *dbname)
{
    PS_ASSERT_PTR_NON_NULL(dbh, false);
    PS_ASSERT_PTR_NON_NULL(dbname, false);

    // Attempt to select new database
    if (mysql_select_db(dbh->mysql, dbname) != 0) {
        psError(mysqlTopsErr(dbh->mysql), true, _("Failed to change database.  Error: %s"),
                mysql_error(dbh->mysql));

        return false;
    }

    psTrace("psLib.db", PS_LOG_INFO, "changed to using database %s", dbname);

    return true;
}

bool psDBDrop(psDB *dbh,
              const char *dbname)
{
    PS_ASSERT_PTR_NON_NULL(dbh, false);
    PS_ASSERT_PTR_NON_NULL(dbname, false);

    // the MySQL C API notes that mysql_drop_db() is deprecated
    bool status = p_psDBRunQueryF(dbh, "DROP DATABASE %s", dbname);
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "Failed to drop database.");
    }

    psTrace("psLib.db", PS_LOG_INFO, "dropped database %s", dbname);

    return status;
}

bool psDBCreateTable(psDB *dbh,
                     const char *tableName,
                     const psMetadata *md)
{
    PS_ASSERT_PTR_NON_NULL(dbh, false);
    PS_ASSERT_PTR_NON_NULL(tableName, false);
    PS_ASSERT_PTR_NON_NULL(md, false);

    // Generate SQL query string
    psString query = psDBGenerateCreateTableSQL(tableName, md);
    if (!query) {
        psError(PS_ERR_UNEXPECTED_NULL, false, _("Query generation failed."));
        return false;
    }

    // Run SQL query to create table
    bool status = p_psDBRunQuery(dbh, query);
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, _("Failed to create table."));
    }

    psFree(query);

    psTrace("psLib.db", PS_LOG_INFO, "created table %s", tableName);

    return status;
}

bool psDBDropTable(psDB *dbh,
                   const char *tableName)
{
    PS_ASSERT_PTR_NON_NULL(dbh, false);
    PS_ASSERT_PTR_NON_NULL(tableName, false);

    // Create SQL command string to drop table
    bool status = p_psDBRunQueryF(dbh, "DROP TABLE %s", tableName);
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, _("Failed to drop table."));
    }

    psTrace("psLib.db", PS_LOG_INFO, "dropped table %s", tableName);

    return status;
}

psArray *psDBSelectColumn(psDB *dbh,
                          const char *tableName,
                          const char *col,
                          unsigned long long limit)
{
    PS_ASSERT_PTR_NON_NULL(dbh, NULL);
    PS_ASSERT_PTR_NON_NULL(tableName, NULL);
    PS_ASSERT_PTR_NON_NULL(col, NULL);

    MYSQL_RES       *result;            // complete db result set
    MYSQL_ROW       row;                // single row of db result set
    my_ulonglong    rowCount;           // number of rows in db result set
    unsigned long   dataSize;           // size of field
    unsigned int    fieldCount;         // number of fields in db result set
    psArray         *column = NULL;     // return array
    psPtr           data;               // copy of result field

    // Generate SQL query string
    psString query = psDBGenerateSelectRowSQL(tableName, col, NULL, limit);
    if (!query) {
        psError(PS_ERR_UNEXPECTED_NULL, false, _("Query generation failed."));
        return NULL;
    }

    // Execut SQL query string
    if (!p_psDBRunQuery(dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, _("Failed to select column."));
        psFree(query);
        return NULL;
    }
    psFree(query);

    // Obtain query result and check for no data condition
    result = mysql_store_result(dbh->mysql);

    if (!result) {
        // no result set
        fieldCount = mysql_field_count(dbh->mysql);

        // if field count is zero the query returned no data.  If it's non-zero
        // then something bad has happened.
        if (fieldCount != 0) {
            psError(mysqlTopsErr(dbh->mysql), true, _("Query returned no data.  Error: %s"),
                    mysql_error(dbh->mysql));
            return NULL;
        }
    }

    // Get number of rows returned in result
    rowCount = mysql_num_rows(result);
    // XXX has mysql's semantics changed?  If this is zero then why was a
    // result set returned?
    if (!rowCount) {
        mysql_free_result(result);
        return NULL;
    }

    // pre-allocate enough elements to hold the complete result set
    // then reset n to 0 so elements are added from the beginning of
    // the array
    column = psArrayAllocEmpty(rowCount);

    // Fetch each result
    while ((row = mysql_fetch_row(result))) {
        // get the first element of lengths array that is part of the
        // result set
        dataSize = *(mysql_fetch_lengths(result));

        // represent NULL as a NULL string
        if (row[0] == NULL) {
            psArrayAdd(column, 0, NULL);
            continue;
        }

        // make a copy of the data
        data = psAlloc(dataSize+1);
        memcpy(data, row[0], dataSize);
        ((char*)data)[dataSize] = '\0';

        // add field to return array
        psArrayAdd(column, 0, data);
        psFree(data);
    }

    // Clean up mysql memory
    mysql_free_result(result);

    return column;
}

// dest = assign to, source = source string psArray, conv = conversion function,
// type = type to cast to, pType = psElemType
#define PS_STR_ARRAY_TO_PTYPE(dest, source, conv, type, pType) \
{ \
    psPtr           myNaN; \
    \
    for (long i = 0; i < source->n; i++) { \
        if (source->data[i]) { \
            dest[i] = (type)conv(source->data[i]); \
        } else { \
            myNaN = psDBGetPTypeNaN(pType); \
            dest[i] = *(type *)myNaN; \
            psFree(myNaN); \
        } \
    } \
}

psVector *psDBSelectColumnNum(psDB *dbh,
                              const char *tableName,
                              const char *col,
                              psElemType type,
                              unsigned long long limit)
{
    PS_ASSERT_PTR_NON_NULL(dbh, NULL);
    PS_ASSERT_PTR_NON_NULL(tableName, NULL);
    PS_ASSERT_PTR_NON_NULL(col, NULL);

    psArray *stringColumn = psDBSelectColumn(dbh, tableName, col, limit);
    if (!stringColumn) {
        // could be an error or the result set was just empty
        return NULL;
    }

    psVector *column = psVectorAlloc(stringColumn->n, type);

    // conversion functions are a portability issue
    switch (type) {
    case PS_DATA_S8:
        PS_STR_ARRAY_TO_PTYPE(column->data.S8, stringColumn, atoi, psS8, PS_DATA_S8);
        break;
    case PS_DATA_S16:
        PS_STR_ARRAY_TO_PTYPE(column->data.S16, stringColumn, atoi, psS16, PS_DATA_S16);
        break;
    case PS_DATA_S32:
        PS_STR_ARRAY_TO_PTYPE(column->data.S32, stringColumn, atoi, psS32, PS_DATA_S32);
        break;
    case PS_DATA_S64:
        PS_STR_ARRAY_TO_PTYPE(column->data.S64, stringColumn, atoll, psS64, PS_DATA_S64);
        break;
    case PS_DATA_U8:
        PS_STR_ARRAY_TO_PTYPE(column->data.U8, stringColumn, atoi, psU8, PS_DATA_U8);
        break;
    case PS_DATA_U16:
        PS_STR_ARRAY_TO_PTYPE(column->data.U16, stringColumn, atoi, psU16, PS_DATA_U16);
        break;
    case PS_DATA_U32:
        PS_STR_ARRAY_TO_PTYPE(column->data.U32, stringColumn, atoi, psU32, PS_DATA_U32);
        break;
    case PS_DATA_U64:
        PS_STR_ARRAY_TO_PTYPE(column->data.U64, stringColumn, atoll, psU64, PS_DATA_U64);
        break;
    case PS_DATA_F32:
        PS_STR_ARRAY_TO_PTYPE(column->data.F32, stringColumn, atof, psF32, PS_DATA_F32);
        break;
    case PS_DATA_F64:
        PS_STR_ARRAY_TO_PTYPE(column->data.F64, stringColumn, atof, psF64, PS_DATA_F64);
        break;
    case PS_DATA_BOOL:
        // valid for psVector?
        PS_STR_ARRAY_TO_PTYPE(column->data.U8, stringColumn, atoi, psU8, PS_DATA_U8);
        break;
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                "Invalid type specified in psDBSelectColumnNum.\n");
        psFree(stringColumn);
        psFree(column);
        return NULL;
        break;
    }

    psFree(stringColumn);

    return column;
}

psArray *psDBSelectRows(psDB *dbh,
                        const char *tableName,
                        const psMetadata *where,
                        unsigned long long limit)
{
    PS_ASSERT_PTR_NON_NULL(dbh, NULL);
    PS_ASSERT_PTR_NON_NULL(tableName, NULL);

    // Create select row query
    psString query = psDBGenerateSelectRowSQL(tableName, NULL, where, limit);
    if (!query) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Query generation failed.");
        return NULL;
    }

    // Run SQL query
    if (!p_psDBRunQuery(dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "Query execution failed.");
        psFree(query);
        return NULL;
    }
    psFree(query);

    // return the result
    return p_psDBFetchResult(dbh);
}

bool psDBInsertOneRow(psDB *dbh,
                      const char *tableName,
                      const psMetadata *row)
{
    PS_ASSERT_PTR_NON_NULL(dbh, false);
    PS_ASSERT_PTR_NON_NULL(tableName, false);
    PS_ASSERT_PTR_NON_NULL(row, false);

    // Create array to store single row
    psArray *rowSet = psArrayAllocEmpty(1);
    psArrayAdd(rowSet, 0, (psPtr)row);

    // Execute function to insert rows
    if (!psDBInsertRows(dbh, tableName, rowSet)) {
        psError(PS_ERR_UNKNOWN, false, _("Failed to insert row."));
        psFree(rowSet);
        return false;
    }

    psFree(rowSet);

    return true;
}

bool psDBInsertRows(psDB *dbh,
                    const char *tableName,
                    const psArray *rowSet)
{
    PS_ASSERT_PTR_NON_NULL(dbh, false);
    PS_ASSERT_PTR_NON_NULL(tableName, false);
    PS_ASSERT_PTR_NON_NULL(rowSet, false);

    // we are assuming that all rows in the set have an identical with reguard
    // to field count and type
    psMetadata *row = rowSet->data[0];

    // Generate SQL query string
    psString query = psDBGenerateInsertRowSQL(tableName, row);
    if (!query) {
        psError(PS_ERR_UNEXPECTED_NULL, false, _("Query generation failed."));
        return false;
    }

    if (p_psDBRunQueryPrepared(dbh, rowSet, query) < 0) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "insert failed");
        psFree(query);
        return false;
    }

    psFree(query);

    return true;
}

psArray *psDBDumpRows(psDB *dbh,
                      const char *tableName)
{
    PS_ASSERT_PTR_NON_NULL(dbh, false);
    PS_ASSERT_PTR_NON_NULL(tableName, false);

    return psDBSelectRows(dbh, tableName, NULL, 0);
}

psMetadata *psDBDumpCols(psDB *dbh,
                         const char *tableName)
{
    PS_ASSERT_PTR_NON_NULL(dbh, false);
    PS_ASSERT_PTR_NON_NULL(tableName, false);

    MYSQL_RES       *result;
    MYSQL_FIELD     *field;
    unsigned int    fieldCount;
    psU32           pType;
    unsigned int    i;
    psPtr           column;

    // find column types
    result = mysql_list_fields(dbh->mysql, tableName, NULL);
    if (!result) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Failed to retrieve column types.");
    }

    field = mysql_fetch_fields(result);
    fieldCount = mysql_num_fields(result);

    // this metadata is returned
    psMetadata *table = psMetadataAlloc();

    // fetch each column and load into psMetadata
    for (i =0; i < fieldCount; i++) {
        // find ptype of column
        pType = psDBMySQLToPType(field[i].type, field[i].flags);
        if (!pType) {
            psError(PS_ERR_UNKNOWN, false, "Failed to lookup type.");
            psFree(table);
            mysql_free_result(result);
            return NULL;
        }
        //psLogMsg( __func__, PS_LOG_INFO, "pType=[%ld]\n", pType );

        // if the ptype is PS_DATA_PTR assume that it's a string and fetch the
        // column as an psArray of strings; otherwise fetch the column as a
        // psVector.
        if (pType == PS_DATA_STRING) {
            // PS_DATA_UNKNOWN -> PS_DATA_ARRAY ?
            column = psDBSelectColumn(dbh, tableName, field[i].name, 0);
            psMetadataAddArray(table, PS_LIST_TAIL, field[i].name, 0, "", column);
            //            psMetadataAddStr(table, PS_LIST_TAIL, field[i].name, "", column);
            psFree(column);
        } else {
            column = psDBSelectColumnNum(dbh, tableName, field[i].name, pType, 0);
            psMetadataAddVector(table, PS_LIST_TAIL, field[i].name, 0, "", column);
            psFree(column);
        }
    }

    // Clean up mysql memory
    mysql_free_result(result);

    return table;
}

long psDBUpdateRows(psDB *dbh,
                    const char *tableName,
                    const psMetadata *where,
                    psMetadata *values)
{
    PS_ASSERT_PTR_NON_NULL(dbh, -1);
    PS_ASSERT_PTR_NON_NULL(tableName, -1);
    PS_ASSERT_PTR_NON_NULL(values, -1);

    // Generate SQL query to update row
    psString query = psDBGenerateUpdateRowSQL(tableName, where, values);
    if (!query) {
        psError(PS_ERR_UNEXPECTED_NULL, false, _("Query generation failed."));
        return -1;
    }

    psArray *rowData = psArrayAllocEmpty(1);
    psArrayAdd(rowData, 0, values);
    long rowsAffected = p_psDBRunQueryPrepared(dbh, rowData, query);
    psFree(rowData);
    psFree(query);
    if (rowsAffected < 0) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "update failed");
        return -1;
    }

    return rowsAffected;
}

long psDBDeleteRows(psDB *dbh,
                    const char *tableName,
                    const psMetadata *where,
                    unsigned long long limit)
{
    PS_ASSERT_PTR_NON_NULL(dbh, -1);
    PS_ASSERT_PTR_NON_NULL(tableName, -1);

    // Create SQL statement string
    psString query = psDBGenerateDeleteRowSQL(tableName, where,limit);
    if (!query) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Query generation failed.");
        return -1;
    }

    // Run SQL query
    if (!p_psDBRunQuery(dbh, query)) {
        psError(PS_ERR_UNKNOWN, false, "Delete failed.");
        mysql_rollback(dbh->mysql);
        psFree(query);
        return -1;
    }
    psFree(query);

    // Get the number of affected row for the delete command
    psS64 rows = (psS64)mysql_affected_rows(dbh->mysql);

    return rows;
}

long long psDBLastInsertID(psDB *dbh)
{
    PS_ASSERT_PTR_NON_NULL(dbh, -1);

    psTrace("psLib.db", PS_LOG_INFO, "calling myql_insert_id()");

    // XXX return type is actually my_ulonglong - should the return be psU64?
    long long id = (long long)mysql_insert_id(dbh->mysql);

    psTrace("psLib.db", PS_LOG_INFO, "LAST_INSERT_ID == %lld", id);

    return id;
}

bool psDBExplicitTrans(psDB *dbh, bool mode)
{
    PS_ASSERT_PTR_NON_NULL(dbh, -1);

    psTrace("psLib.db", PS_LOG_INFO, "calling mysql_autocommit(): %u", !mode);

    // mode needs to be inverted as autocommits are the opposide of explicit
    // transactions.
    // the return value also needs to be inverted for the same reason.
    // is it safe to assume my_bool always safely casts to bool?
    // XXX EAM 2022.03.15 : at least in mysql 8.0.28, my_bool is gone and just bool is used
    // in 5.0 and 5.6, my_bool is char
    // 
    return !(bool)mysql_autocommit(dbh->mysql, !mode);
}

bool psDBTransaction(psDB *dbh)
{
    PS_ASSERT_PTR_NON_NULL(dbh, -1);

    bool status = p_psDBRunQuery(dbh, "START TRANSACTION");
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "Failed to start a new transaction.");
    }

    return status;
}

bool psDBCommit(psDB *dbh)
{
    PS_ASSERT_PTR_NON_NULL(dbh, -1);

    psTrace("psLib.db", PS_LOG_INFO, "calling myql_commit()");

    // is it safe to assume my_bool always safely casts to bool?
    // mysql_commit - Zero if successful. Non-zero if an error occurred.
    return !(bool)mysql_commit(dbh->mysql);
}

bool psDBRollback(psDB *dbh)
{
    PS_ASSERT_PTR_NON_NULL(dbh, -1);

    psTrace("psLib.db", PS_LOG_INFO, "calling myql_rollback()");

    // is it safe to assume my_bool always safely casts to bool?
    // mysql_rollback - Zero if successful. Non-zero if an error occurred.
    return !(bool)mysql_rollback(dbh->mysql);
}

psU64 psDBAffectedRows(psDB *dbh)
{
    PS_ASSERT_PTR_NON_NULL(dbh, (psU64)-1);

    // mysql_affected_rows() returns (my_ulonglong)-1 on error
    return (psU64)mysql_affected_rows(dbh->mysql);
}

// database utility functions
/*****************************************************************************/

bool p_psDBRunQuery(psDB *dbh,
                    const char *query)
{
    PS_ASSERT_PTR_NON_NULL(dbh, false);
    PS_ASSERT_PTR_NON_NULL(query, false);

    psTrace("psLib.db", PS_LOG_INFO, "Executing SQL:\n%s", query);

    if (mysql_real_query(dbh->mysql, query, (unsigned long)strlen(query)) !=0) {
        psError(mysqlTopsErr(dbh->mysql), true, _("Failed to execute SQL query.  Error: %s"), mysql_error(dbh->mysql));
        return false;
    }

    return true;
}

bool p_psDBRunQueryF(psDB *dbh,
                    const char *format,
                    ...)
{
    PS_ASSERT_PTR_NON_NULL(dbh, false);
    PS_ASSERT_PTR_NON_NULL(format, false);

    psString query = NULL;

    // Run query
    va_list ap;
    va_start(ap, format);
    psStringAppendV(&query, format, ap);
    va_end(ap);

    bool status = p_psDBRunQuery (dbh, query);

    psFree(query);

    return status;
}

long p_psDBRunQueryPreparedF(psDB *dbh,
                            const psArray *rowSet,
                            const char *format,
                            ...)
{
    PS_ASSERT_PTR_NON_NULL(dbh, -1);
    PS_ASSERT_PTR_NON_NULL(rowSet, -1);
    PS_ASSERT_PTR_NON_NULL(format, -1);

    psString query = NULL;

    // generate query string
    va_list ap;
    va_start(ap, format);
    psStringAppendV(&query, format, ap);
    va_end(ap);

    long status = p_psDBRunQueryPrepared(dbh, rowSet, query);

    psFree(query);

    return status;
}

long p_psDBRunQueryPrepared(psDB *dbh,
                            const psArray *rowSet,
                            const char *query
    )
{
    PS_ASSERT_PTR_NON_NULL(dbh, -1);
    PS_ASSERT_PTR_NON_NULL(rowSet, -1);
    PS_ASSERT_PTR_NON_NULL(query, -1);

    // start lock on query cache
    if (psMemGetThreadSafety()) {
        pthread_mutex_lock(&preparedQueryMutex);
    }

    // initalize the prepared query cache
    if (!preparedQuery) {
        preparedQuery = psHashAlloc(10);
        psMemSetPersistent(preparedQuery, true);
    }

    psTrace("psLib.db", PS_LOG_INFO, "Preparing SQL:\n%s", query);

    // check the query cache
    MYSQL_STMT **stmt = psHashLookup(preparedQuery, query);
    if (!stmt) {
        psTrace("psLib.db", PS_LOG_INFO, "statment is not in query cache");

        // Prepare SQL statement
        stmt = psAlloc(sizeof(MYSQL_STMT *));
        *stmt = mysql_stmt_init(dbh->mysql);
        if (!stmt) {
            psAbort("mysql_stmt_init(), out of memory.");
        }
        if (mysql_stmt_prepare(*stmt, query, (unsigned long)strlen(query))) {
            psError(PS_ERR_UNKNOWN, true, "Failed to prepare query.  Error: %s", mysql_stmt_error(*stmt));
            mysql_stmt_close(*stmt);

            // end lock on query cache
            if (psMemGetThreadSafety()) {
                pthread_mutex_unlock(&preparedQueryMutex);
            }

            return -1;
        }

        // add this statement to the cache
        psHashAdd(preparedQuery, query, stmt);
    } else {
        psTrace("psLib.db", PS_LOG_INFO, "found statment in query cache");
    }

    // end lock on query cache
    if (psMemGetThreadSafety()) {
        pthread_mutex_unlock(&preparedQueryMutex);
    }

    // how many place holders are in our query
    psS32 paramCount = mysql_stmt_param_count(*stmt);

    // structure large enough to hold one field of data per place holder
    psDBMysqlRow *mysqlRow = psDBMysqlRowAlloc(paramCount);

    // loop over rows
    for (long j = 0; j < rowSet->n; j++) {
        psMetadata *rowData = rowSet->data[j];

        if (!psDBPackMySQLRow(mysqlRow, rowData)) {
            psError(PS_ERR_UNKNOWN, false, "Failed to pack params into bind structure.");

            psFree(mysqlRow);
//            mysql_stmt_close(stmt);

            return -1;
        }

        if (psTraceGetLevel("psLib.db") >= PS_LOG_INFO) {
            psString binding = psMetadataConfigFormat(rowData);
            psTrace("psLib.db", PS_LOG_INFO, "Binding:\n%s", binding);
            psFree(binding);
        }

        if (mysql_stmt_bind_param(*stmt, mysqlRow->bind)) {
            psError(PS_ERR_UNKNOWN, true, "Failed to bind params.  Error: %s", mysql_stmt_error(*stmt));

            psFree(mysqlRow);
//           mysql_stmt_close(stmt);

            return -1;
        }

        if (mysql_stmt_execute(*stmt)) {
            psError(PS_ERR_UNKNOWN, true, "Failed to execute prepared statement.  Error: %s", mysql_stmt_error(*stmt));

            psFree(mysqlRow);
//            mysql_stmt_close(stmt);

            return -1;
        }

        // free temporary buffers
        psDBMysqlRowRecycle(mysqlRow);

    } // end loop over rows

    psFree(mysqlRow);

    // FYI mysql_stmt_affected_rows() must be called before a commit
    long rowsAffected = mysql_stmt_affected_rows(*stmt);

//    mysql_stmt_close(stmt);

    return rowsAffected;
}

psArray *p_psDBFetchResult(psDB *dbh)
{
    PS_ASSERT_PTR_NON_NULL(dbh, NULL);

    MYSQL_RES       *result;            // complete db result set
    MYSQL_ROW       row;                // single row of db result set
    MYSQL_FIELD     *field;             // field type info
    my_ulonglong    rowCount;           // number of rows in db result set
    unsigned int    fieldCount;         // number of fields in db result set
    unsigned long   *fieldLength;       // field sizes
    long            len;                // field length
    psArray         *resultSet;         // return array
    int             i;                  // field index
    psMetadata      *md;                // a row
    psU32           pType;              // psElemType of a field
    psPtr           data;               // copy of result field

    result = mysql_store_result(dbh->mysql);
    if (!result) {
        // no result set
        fieldCount = mysql_field_count(dbh->mysql);

        // we're going to exit no matter what after this point
        mysql_free_result(result);

        // if field count is zero the query should have returned no data.  If
        // it's non-zero then something bad has happened.
        if (fieldCount != 0) {
            psError(mysqlTopsErr(dbh->mysql), true, "Query returned no data.  Error: %s", mysql_error(dbh->mysql));

            return NULL;
        }

        return psArrayAlloc(0);
    }

    rowCount = mysql_num_rows(result);
    // XXX has mysql's semantics changed?  If this is zero then why was a
    // result set returned?
    if (rowCount == 0) {
        mysql_free_result(result);
        // occording to popular opinion a query that succeeds but returns no
        // data should return an empty array -- this is somewhat inefficent -JH
        return psArrayAlloc(0);
    }

    // pre-allocate enough elements to hold the complete result set
    // then reset n to 0 so elements are added from the beginning of
    // the array
    resultSet = psArrayAllocEmpty(rowCount);

    field = mysql_fetch_fields(result);
    fieldCount = mysql_num_fields(result);

    psTrace("psLib.db", PS_LOG_INFO, "query returned %lu rows with %d fields", (long unsigned int) rowCount, fieldCount);

    while ((row = mysql_fetch_row(result))) {
      // allocate new psMetadata to represent a row
      md = psMetadataAlloc();

      fieldLength = mysql_fetch_lengths(result);

      for (i = 0; i < fieldCount; i++) {
        // lookup MySQL column type
        pType = psDBMySQLToPType(field[i].type, field[i].flags);
        if (!pType) {
          psError(PS_ERR_UNKNOWN, false, "Failed to lookup type. %d %d %d",i,field[i].type, field[i].flags);
          psFree(md);
          mysql_free_result(result);
          psFree(resultSet);
          return NULL;
        }

        len = fieldLength[i];
        if (len) {
          // copy the data out of the result set struct
          data = psAlloc(len + 1);
          memcpy(data, row[i], len);
          ((char*)data)[len] = '\0';
        } else {
          // if len is zero then the value is NULL
          psTrace("psLib.db", PS_LOG_INFO, "database field name %s contains a NULL", field[i].name);
          data = psDBGetPTypeNaN(pType);

          // the NULL case must be handled for each type differently
          switch (pType) {
            case PS_DATA_STRING:
              if (!psMetadataAddStr(md, PS_LIST_TAIL, field[i].name, 0, "", data)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;
            case PS_DATA_U8:
              if (!psMetadataAddU8(md, PS_LIST_TAIL, field[i].name, 0, "",*(psU8 *)data)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;
            case PS_DATA_U16:
              if (!psMetadataAddU16(md, PS_LIST_TAIL, field[i].name, 0, "",*(psU16 *)data)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;
            case PS_DATA_U32:
              if (!psMetadataAddU32(md, PS_LIST_TAIL, field[i].name, 0, "",*(psU32 *)data)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;
            case PS_DATA_U64:
              if (!psMetadataAddU64(md, PS_LIST_TAIL, field[i].name, 0, "",*(psU64 *)data)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;
            case PS_DATA_S8:
              if (!psMetadataAddS8(md, PS_LIST_TAIL, field[i].name, 0, "", *(psS8 *)data)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;
            case PS_DATA_S16:
              if (!psMetadataAddS16(md, PS_LIST_TAIL, field[i].name, 0, "", *(psS16 *)data)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;
            case PS_DATA_S32:
              if (!psMetadataAddS32(md, PS_LIST_TAIL, field[i].name, 0, "", *(psS32 *)data)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;
            case PS_DATA_S64:
              if (!psMetadataAddS64(md, PS_LIST_TAIL, field[i].name, 0, "", *(psS64 *)data)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;
            case PS_DATA_F32:
              if (!psMetadataAddF32(md, PS_LIST_TAIL, field[i].name, 0, "", *(psF32 *)data)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;
            case PS_DATA_F64:
              if (!psMetadataAddF64(md, PS_LIST_TAIL, field[i].name, 0, "", *(psF64 *)data)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;
            case PS_DATA_BOOL:
              if (!psMetadataAddBool(md, PS_LIST_TAIL, field[i].name, 0, "", *(bool *)data)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;
            case PS_DATA_TIME:
              // just pass NULL values through as there is currently no
              // concept of a psTime with a NULL value
              if (!psMetadataAddTime(md, PS_LIST_TAIL, field[i].name, 0, "", NULL)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;

            default:
              // just pass NULL values for all other types
              if (!psMetadataAddUnknown(md, PS_LIST_TAIL, field[i].name, 0, "", NULL)) {
                psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
              }
              break;
          }
          psFree(data);
          continue;
        }

            switch (pType) {
            case PS_DATA_STRING:
                if (!psMetadataAddStr(md, PS_LIST_TAIL, field[i].name, 0, "", data)) {
                    psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                    psFree(data);
                    psFree(md);
                    mysql_free_result(result);
                    psFree(resultSet);
                    return NULL;
                }
                break;
            case PS_DATA_U8:
                if (!psMetadataAddU8(md, PS_LIST_TAIL, field[i].name, 0, "",(psU8)atoll(data))) {
                    psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                    psFree(data);
                    psFree(md);
                    mysql_free_result(result);
                    psFree(resultSet);
                    return NULL;
                }
                break;
            case PS_DATA_U16:
                if (!psMetadataAddU16(md, PS_LIST_TAIL, field[i].name, 0, "",(psU16)atoll(data))) {
                    psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                    psFree(data);
                    psFree(md);
                    mysql_free_result(result);
                    psFree(resultSet);
                    return NULL;
                }
                break;
            case PS_DATA_U32:
                if (!psMetadataAddU32(md, PS_LIST_TAIL, field[i].name, 0, "",(psU32)atoll(data))) {
                    psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                    psFree(data);
                    psFree(md);
                    mysql_free_result(result);
                    psFree(resultSet);
                    return NULL;
                }
                break;
            case PS_DATA_U64:
                if (!psMetadataAddU64(md, PS_LIST_TAIL, field[i].name, 0, "",(psU64)atoll(data))) {
                    psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                    psFree(data);
                    psFree(md);
                    mysql_free_result(result);
                    psFree(resultSet);
                    return NULL;
                }
                break;
            case PS_DATA_S8:
                if (!psMetadataAddS8(md, PS_LIST_TAIL, field[i].name, 0, "", (psS8)atoll(data))) {
                    psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                    psFree(data);
                    psFree(md);
                    mysql_free_result(result);
                    psFree(resultSet);
                    return NULL;
                }
                break;
            case PS_DATA_S16:
                if (!psMetadataAddS16(md, PS_LIST_TAIL, field[i].name, 0, "", (psS16)atoll(data))) {
                    psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                    psFree(data);
                    psFree(md);
                    mysql_free_result(result);
                    psFree(resultSet);
                    return NULL;
                }
                break;
            case PS_DATA_S32:
                if (!psMetadataAddS32(md, PS_LIST_TAIL, field[i].name, 0, "", (psS32)atoll(data))) {
                    psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                    psFree(data);
                    psFree(md);
                    mysql_free_result(result);
                    psFree(resultSet);
                    return NULL;
                }
                break;
            case PS_DATA_S64:
                if (!psMetadataAddS64(md, PS_LIST_TAIL, field[i].name, 0, "", (psS64)atoll(data))) {
                    psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                    psFree(data);
                    psFree(md);
                    mysql_free_result(result);
                    psFree(resultSet);
                    return NULL;
                }
                break;
            case PS_DATA_F32:
                if (!psMetadataAddF32(md, PS_LIST_TAIL, field[i].name, 0, "", atof(data))) {
                    psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                    psFree(data);
                    psFree(md);
                    mysql_free_result(result);
                    psFree(resultSet);
                    return NULL;
                }
                break;
            case PS_DATA_F64:
                if (!psMetadataAddF64(md, PS_LIST_TAIL, field[i].name, 0, "", atof(data))) {
                    psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                    psFree(data);
                    psFree(md);
                    mysql_free_result(result);
                    psFree(resultSet);
                    return NULL;
                }
                break;
            case PS_DATA_BOOL:
                if (!psMetadataAddBool(md, PS_LIST_TAIL, field[i].name, 0, "", (bool)atoi(data))) {
                    psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                    psFree(data);
                    psFree(md);
                    mysql_free_result(result);
                    psFree(resultSet);
                    return NULL;
                }
                break;
            case PS_DATA_TIME:
                // just pass NULL values through as there is currently no
                // concept of a psTime with a NULL value
                if (!(char *)data) {
                    if (!psMetadataAddTime(md, PS_LIST_TAIL, field[i].name, 0, "", NULL)) {
                        psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                        psFree(data);
                        psFree(md);
                        mysql_free_result(result);
                        psFree(resultSet);
                        return NULL;
                    }
                } else if (psStrcasestr(data, "0000-00-00 00:00:00")) {
                    // look for 0000-00-00 00:00:00, which can't be parsed by
                    // psTimeStrptime as the month/day are bogus
                    if (!psMetadataAddTime(md, PS_LIST_TAIL, field[i].name, 0, "", NULL)) {
                        psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                        psFree(data);
                        psFree(md);
                        mysql_free_result(result);
                        psFree(resultSet);
                        return NULL;
                    }
                } else {
                    psTime *time = psTimeStrptime((char *)data, "%EY-%m-%d%t%T");
                    if (!time) {
                        psError(PS_ERR_UNKNOWN, true, "error parsing MySQL DateTime string");
                        psFree(md);
                        mysql_free_result(result);
                        psFree(resultSet);
                        return NULL;
                    }
                    if (!psMetadataAddTime(md, PS_LIST_TAIL, field[i].name, 0, "", time)) {
                        psError(PS_ERR_UNKNOWN, false, "Failed to add item %s", field[i].name);
                        psFree(data);
                        psFree(md);
                        mysql_free_result(result);
                        psFree(resultSet);
                        return NULL;
                    }
                    psFree(time);
                }
                break;
                #if 0
                // this is the procedure needed for stmt
                MYSQL_TIME *myTime = (MYSQL_TIME *)&data;
                // convert MYSQL_TIME to struct tm
                struct tm tmTime;
                tmTime.tm_year  = (int)myTime->year - 1900;
                tmTime.tm_mon   = (int)myTime->month - 1;
                tmTime.tm_mday  = (int)myTime->day;
                tmTime.tm_hour  = (int)myTime->hour;
                tmTime.tm_min   = (int)myTime->minute;
                tmTime.tm_sec   = (int)myTime->second;
                // assume for the time being that we don't have negative time
                //(bool)myTime->neg
                // currently unused by mysql nor does struct tm support it
                // (unsigned long)myTime->second_part;
                psTime *time = psTimeFromTM(&tmTime);
                psMetadataAddTime(md, PS_LIST_TAIL, field[i].name, 0, "",
                                  time);
                psFree(time);
                #endif

            default:
                psError(PS_ERR_BAD_PARAMETER_TYPE , true,
                        "field name: %s FIXME: Only type of "
                        "PS_DATA_U8 (PS_DATA_U8), "
                        "PS_DATA_U16 (PS_DATA_U16), "
                        "PS_DATA_U32 (PS_DATA_U32), "
                        "PS_DATA_U64 (PS_DATA_U64), "
                        "PS_DATA_S8 (PS_DATA_S8), "
                        "PS_DATA_S16 (PS_DATA_S16), "
                        "PS_DATA_S32 (PS_DATA_S32), "
                        "PS_DATA_S64 (PS_DATA_S64), "
                        "PS_DATA_F32 (PS_DATA_F32), "
                        "PS_DATA_F64 (PS_DATA_F64), "
                        "PS_DATA_BOOL (PS_DATA_BOOL), "
                        "PS_DATA_STRING "
                        "and PS_DATA_TIME are supported.", field[i].name);
                psFree(data);
                psFree(md);
                mysql_free_result(result);
                psFree(resultSet);
                return NULL;
            }

            psFree(data);
        }

        if (psTraceGetLevel("psLib.db") >= PS_LOG_INFO) {
            psString rowString = psMetadataConfigFormat(md);
            psTrace("psLib.db", PS_LOG_INFO, "adding row to result set:\n %s", rowString);
            psFree(rowString);
        }

        // add row to result set
        psArrayAdd(resultSet, 0, md);
        psFree(md);
    }

    mysql_free_result(result);


    return resultSet;
}

static inline bool psDBPackMySQLRow(psDBMysqlRow *mysqlRow,
                                    const psMetadata *values)
{
    PS_ASSERT_PTR_NON_NULL(mysqlRow, false);
    PS_ASSERT_PTR_NON_NULL(values, false);

    mysqlType       *mType;             // type tmp variable

    // XXX EAM 2022.03.15 : at least in mysql 8.0.28, my_bool is gone and just bool is used
    // in 5.0 and 5.6, my_bool is char
    static MYSQL_BOOL isNull = true;       // used in a MYSQL_BIND to indicate NULL

    MYSQL_BIND *bind = mysqlRow->bind;

    // row iterator
    // check size of values == paramCount ?
    psListIterator *cursor = psListIteratorAlloc(values->list, 0, false);

    // loop over fields
    psMetadataItem *item = NULL;              // field in row
    for (psU32 i = 0; (item = psListGetAndIncrement(cursor)); i++) {
        // lookup pType -> mysql type
        mType = psDBPTypeToMySQL(item->type);

        bind[i].buffer_type = mType->type;
        bind[i].is_unsigned = mType->isUnsigned;

        psFree(mType);

        // input data length is determined by the MYSQL_TYPE_* unless it's a
        // string
        switch (item->type) {
        case PS_DATA_U8: {
                bind[i].length  = 0;
                bind[i].buffer  = &item->data.U8;
                bind[i].is_null = psDBIsPTypeNaN(item->type, &item->data.U8) ? &isNull : NULL;
                break;
            }
        case PS_DATA_U16: {
                bind[i].length  = 0;
                bind[i].buffer  = &item->data.U16;
                bind[i].is_null = psDBIsPTypeNaN(item->type, &item->data.U16) ? &isNull : NULL;
                break;
            }
        case PS_DATA_U32: {
                bind[i].length  = 0;
                bind[i].buffer  = &item->data.U32;
                bind[i].is_null = psDBIsPTypeNaN(item->type, &item->data.U32) ? &isNull : NULL;
                break;
            }
        case PS_DATA_U64: {
                bind[i].length  = 0;
                bind[i].buffer  = &item->data.U64;
                bind[i].is_null = psDBIsPTypeNaN(item->type, &item->data.U64) ? &isNull : NULL;
                break;
            }
        case PS_DATA_S8: {
                bind[i].length  = 0;
                bind[i].buffer  = &item->data.S8;
                bind[i].is_null = psDBIsPTypeNaN(item->type, &item->data.S8) ? &isNull : NULL;
                break;
            }
        case PS_DATA_S16: {
                bind[i].length  = 0;
                bind[i].buffer  = &item->data.S16;
                bind[i].is_null = psDBIsPTypeNaN(item->type, &item->data.S16) ? &isNull : NULL;
                break;
            }
        case PS_DATA_S32: {
                bind[i].length  = 0;
                bind[i].buffer  = &item->data.S32;
                bind[i].is_null = psDBIsPTypeNaN(item->type, &item->data.S32) ? &isNull : NULL;
                break;
            }
        case PS_DATA_S64: {
                bind[i].length  = 0;
                bind[i].buffer  = &item->data.S64;
                bind[i].is_null = psDBIsPTypeNaN(item->type, &item->data.S64) ? &isNull : NULL;
                break;
            }
        case PS_DATA_F32: {
                bind[i].length  = 0;
                bind[i].buffer  = &item->data.F32;
                bind[i].is_null = psDBIsPTypeNaN(item->type, &item->data.F32) ? &isNull : NULL;
                break;
            }
        case PS_DATA_F64: {
            // This hack is to work around a MySQL bug, where values of DBL_MAX are dumped (as text) with
            // insufficient digits, causing the value to be rounded outside the bounds of DBL_MAX
            // (specifically, -1.7976931348623157e+308 gets dumped as -1.79769313486232e+308 which is less
            // than -DBL_MAX), which cannot then be loaded by MySQL.  We assume that we're using doubles for
            // additional precision compared to floats, and not for additional size, so limiting to the
            // maximum value of a float is not damaging.
            if (item->data.F64 < -FLT_MAX) {
                psWarning("Saturating double value at -FLT_MAX to work around MySQL bug: %lf --> %lf",
                          item->data.F64, -FLT_MAX);
                item->data.F64 = -FLT_MAX;
            } else if (item->data.F64 > FLT_MAX) {
                psWarning("Saturating double value at FLT_MAX to work around MySQL bug: %lf --> %lf",
                          item->data.F64, FLT_MAX);
                item->data.F64 = FLT_MAX;
            }
            bind[i].length  = 0;
            bind[i].buffer  = &item->data.F64;
            bind[i].is_null = psDBIsPTypeNaN(item->type, &item->data.F64) ? &isNull : NULL;
            break;
        }
        case PS_DATA_BOOL: {
                // XXX: ASC HACK NOTE (2005/06/03): set extreme bytes to the
                // boolean character value.  sizeof(bool)==4 which triggers
                // an endianess issue in the MySQL conversion (reading only 1
                // byte), on Macintosh hardware (and maybe others?)
                unsigned int c  = (unsigned int)item->data.B;
                item->data.S32  = (unsigned int)((c<<24) | c);
                bind[i].length  = 0;
                bind[i].buffer  = &item->data.B;
                bind[i].is_null = psDBIsPTypeNaN(item->type, &item->data.B) ? &isNull : NULL;
                break;
            }
        case PS_DATA_STRING: {
                // convert NaNs to NULL and set the buffer_length for strings

                if (item->data.str) {
                    // will handle the case of "" as a NULL database value
                    bind[i].buffer_length = (unsigned long)strlen(item->data.str);
                    bind[i].length  = &bind[i].buffer_length;
                    bind[i].buffer  = psStringCopy(item->data.V);
                    bind[i].is_null = *item->data.str == '\0' ? &isNull : NULL;
                } else {
                    // handles the case of NULL as a NULL database value
                    bind[i].buffer_length = 0;
                    bind[i].length  = &bind[i].buffer_length;
                    bind[i].buffer  = NULL;
                    bind[i].is_null = &isNull;
                }
                break;
            }
        case PS_DATA_TIME: {
                // XXX we're abusing the comment field of the metadata item
                // here as we need to have a buffer that exists outside this
                // functions scope without leaking memory make a copy of the
                // psTime so we don't modify user data when we try to do the
                // conversion
                if (item->data.V) {
                    psTime *time = (psTime *)item->data.V;
                    struct tm *tmTime = psTimeToTM(time);
                    if (!tmTime) {
                        psError(PS_ERR_UNKNOWN, false, _("failed to convert psTime to struct tm"));
                        psFree(cursor);

                        return false;
                    }

                    // XXX it wouldn't hurt to make this conversion it's own
                    // function myTime is used as the 'buffer' so it doesn't
                    // have to be free'd
                    MYSQL_TIME *myTime = psAlloc(sizeof(MYSQL_TIME));
                    myTime->year    = (unsigned int)tmTime->tm_year + 1900;
                    myTime->month   = (unsigned int)tmTime->tm_mon + 1;
                    myTime->day     = (unsigned int)tmTime->tm_mday;
                    myTime->hour    = (unsigned int)tmTime->tm_hour;
                    myTime->minute  = (unsigned int)tmTime->tm_min;
                    myTime->second  = (unsigned int)tmTime->tm_sec;
                    // assume for the time being that we don't have negative
                    // time as ISO8601 doesn't support dates prior to 0
		    // XXX EAM 2022.03.15 : at least in mysql 8.0.28, my_bool is gone and just bool is used
		    // in 5.0 and 5.6, my_bool is char
                    myTime->neg     = (bool)false;
                    // currently unused by mysql
                    myTime->second_part  = (unsigned long)time->nsec;
                    psFree(tmTime);

                    bind[i].buffer  = myTime;
                    bind[i].buffer_length = sizeof(MYSQL_TIME);
                    bind[i].length  = &bind[i].buffer_length;
                    bind[i].is_null = NULL;
                } else {
                    // handles the case of NULL as a NULL database value
                    bind[i].buffer_length = 0;
                    bind[i].length  = &bind[i].buffer_length;
                    bind[i].buffer  = NULL;
                    bind[i].is_null = &isNull;
                }
                break;
            }
        default: {
                psError(PS_ERR_BAD_PARAMETER_TYPE , true,
                        "FIXME: Unsupported data type");
                psFree(cursor);

                return false;
                break;                  // unreachable
            }
        }
    }

    psFree(cursor);

    return true;
}

static psDBMysqlRow *psDBMysqlRowAlloc(psU32 paramCount)
{
    psDBMysqlRow *row = psAlloc(sizeof(psDBMysqlRow));
    psMemSetDeallocator(row, (psFreeFunc)psDBMysqlRowFree);

    row->bind = psAlloc(sizeof(MYSQL_BIND) * paramCount);
    memset(row->bind, 0, sizeof(MYSQL_BIND) * paramCount);
    row->n = paramCount;

    return row;
}

static void psDBMysqlRowFree(psDBMysqlRow *mysqlRow)
{
    PS_ASSERT_PTR_NON_NULL(mysqlRow, );

    MYSQL_BIND *bind = mysqlRow->bind;

    psDBMysqlRowRecycle(mysqlRow);

    psFree(bind);
}

static void psDBMysqlRowRecycle(psDBMysqlRow *mysqlRow)
{
    PS_ASSERT_PTR_NON_NULL(mysqlRow, );

    MYSQL_BIND *bind = mysqlRow->bind;

    for (psU32 i = 0; i < mysqlRow->n; i++) {
        // buffer_length is only defined for pointers to character buffers
        // primitive types will have this value set to zero.
        if (bind[i].buffer_length) {
            psFree(bind[i].buffer);
        }
    }

    memset(bind, '\0', sizeof(MYSQL_BIND) * mysqlRow->n);
}


// SQL generation functions
/*****************************************************************************/

static psString psDBGenerateCreateTableSQL(const char *tableName,
        const psMetadata *table)
{
    PS_ASSERT_PTR_NON_NULL(tableName, NULL);
    PS_ASSERT_PTR_NON_NULL(table, NULL);

    char            *query = NULL;      // complete query
    psMetadataItem  *item;              // column description
    psListIterator  *cursor;            // column iterator
    char            *colType;           // type lookup table

    // Begin to create SQL string to create table
    psStringAppend(&query, "CREATE TABLE %s (", tableName);

    // Set list iterator at head of list
    cursor = psListIteratorAlloc(table->list, 0, false);

    // find column name and type
    while ((item = psListGetAndIncrement(cursor))) {
        switch (item->type) {
        case PS_DATA_U8:
        case PS_DATA_U16:
        case PS_DATA_U32:
        case PS_DATA_U64:
        case PS_DATA_S8:
        case PS_DATA_S16:
        case PS_DATA_S32:
        case PS_DATA_S64:
        case PS_DATA_F32:
        case PS_DATA_F64:
        case PS_DATA_BOOL:
        case PS_DATA_TIME: {
                // + column name + _ + column type
                colType = psDBPTypeToSQL(item->type);
                psStringAppend(&query, "%s %s", item->name, colType);
                psFree(colType);
                break;
            }
        case PS_DATA_STRING: {
                // + column name + _ + varchar( + length + )
                psStringAppend(&query, "%s VARCHAR(%s)", item->name, item->data.str);
                break;
            }
        default: {
                psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                        "FIXME: Unsupported data type %d", item->type);

                psFree(query);
                psFree(cursor);

                return NULL;
                break;                  // unreachable
            }
        }

        if (psStrcasestr(item->comment, "AUTO_INCREMENT")) {
            psStringAppend(&query, " %s", "AUTO_INCREMENT");
        }
        if (psStrcasestr(item->comment, "Unique")) {
            psStringAppend(&query, " %s", "UNIQUE");
        }
        if (psStrcasestr(item->comment, "NOT NULL")) {
            psStringAppend(&query, " %s", "NOT NULL");
        }

        // add a , after every column declaration except the last one
        if (!cursor->offEnd) {
            psStringAppend(&query, ", ");
        }
    }


    // find database indexes
    // check for Primary Keys first as they have to be part of the same Primary
    // Key stmt
    // Reset iterator to head of list
    psListIteratorSet(cursor, 0);
    psArray *pKeys = psArrayAllocEmpty(1);
    while ((item = psListGetAndIncrement(cursor))) {
        if (psStrcasestr(item->comment, "Primary Key")) {
            psArrayAdd(pKeys, 0, item->name);
        }
    }
    if (psArrayLength(pKeys)) {
        psStringAppend(&query, ", PRIMARY KEY(");
        for (int i = 0; i < psArrayLength(pKeys); i++) {
            if (i < 1) {
                psStringAppend(&query, "%s", (char *)pKeys->data[i]);
            } else {
                psStringAppend(&query, ", %s", (char *)pKeys->data[i]);
            }
        }
        psStringAppend(&query, ")");
    }
    psFree(pKeys);

    // Reset iterator to head of list
    psListIteratorSet(cursor, 0);
    // look for regular keys
    while ((item = psListGetAndIncrement(cursor))) {
        // it's just a regular key if it matchs "Key" but not "Primary Key"
        if (psStrcasestr(item->comment, "Key")
                && (psStrcasestr(item->comment, "Primary Key") == NULL)) {
            psStringAppend(&query, ", KEY(%s)", item->name);
        } else if (psStrcasestr(item->comment, "AUTO_INCREMENT")) {
            // this needs to be recognized as a key if it wasn't already
            psStringAppend(&query, ", KEY(%s)", item->name);
        }
    }


    // Reset iterator to head of list
    psListIteratorSet(cursor, 0);
    // look for indexes
    while ((item = psListGetAndIncrement(cursor))) {
        // don't compile a regex unless we have too

        if (psStrcasestr(item->comment, "UINDEX")) {
            // look for UINDEXs before INDEXs
            regex_t         myregex;
            regmatch_t      mymatch[2];     // match + 1 sub strings
            int             errbuf_size = 1024;
            char            errbuf[errbuf_size];
            char            *pattern = "UINDEX[:space:]*[(]([^)]*)";

            int status = regcomp(&myregex, pattern, REG_EXTENDED|REG_ICASE);
            if (status != 0) {
                regerror(status, &myregex, errbuf, errbuf_size);
                psError(PS_ERR_UNKNOWN, true, "regcomp() failed: %s", errbuf);
                psFree(query);
                psFree(cursor);
                return NULL;
            }

            psTrace("psLib.db", 10, "trying regex pattern: %s against string: %s", pattern, item->comment);
            status = regexec(&myregex, item->comment, 2, mymatch, 0);
            if (status != 0) {
                regerror(status, &myregex, errbuf, errbuf_size);
                psError(PS_ERR_UNKNOWN, true, "regexec() failed: %s", errbuf);
                psFree(query);
                psFree(cursor);
                return NULL;
            }

            regfree(&myregex);

            // sub string 1: uindex(.*)
            size_t matchStart = (size_t)mymatch[1].rm_so;
            size_t matchEnd   = (size_t)mymatch[1].rm_eo;
            size_t matchLength = matchEnd - matchStart;

            if (matchStart == -1) {
                psError(PS_ERR_UNKNOWN, true, "substring 1 failed to match");
                psFree(query);
                psFree(cursor);
                return NULL;
            }

            psString index = psStringNCopy(item->comment + matchStart, matchLength);
            psTrace("psLib.db", 10, "regex $1 matched: %s", index);
            psStringAppend(&query, ", UNIQUE KEY(%s)", index);
            psFree(index);
        } else if (psStrcasestr(item->comment, "INDEX")) {
            regex_t         myregex;
            regmatch_t      mymatch[2];     // match + 1 sub strings
            int             errbuf_size = 1024;
            char            errbuf[errbuf_size];
            // don't accidentally match uindex
            char            *pattern = "[:space:]*INDEX[:space:]*[(]([^)]*)";

            int status = regcomp(&myregex, pattern, REG_EXTENDED|REG_ICASE);
            if (status != 0) {
                regerror(status, &myregex, errbuf, errbuf_size);
                psError(PS_ERR_UNKNOWN, true, "regcomp() failed: %s", errbuf);
                psFree(query);
                psFree(cursor);
                return NULL;
            }

            psTrace("psLib.db", 10, "trying regex pattern: %s against string: %s", pattern, item->comment);
            status = regexec(&myregex, item->comment, 2, mymatch, 0);
            if (status != 0) {
                regerror(status, &myregex, errbuf, errbuf_size);
                psError(PS_ERR_UNKNOWN, true, "regexec() failed: %s", errbuf);
                psFree(query);
                psFree(cursor);
                return NULL;
            }

            regfree(&myregex);

            // sub string 1: index(.*)
            size_t matchStart = (size_t)mymatch[1].rm_so;
            size_t matchEnd   = (size_t)mymatch[1].rm_eo;
            size_t matchLength = matchEnd - matchStart;

            if (matchStart == -1) {
                psError(PS_ERR_UNKNOWN, true, "substring 1 failed to match");
                psFree(query);
                psFree(cursor);
                return NULL;
            }

            psString index = psStringNCopy(item->comment + matchStart, matchLength);
            psTrace("psLib.db", 10, "regex $1 matched: %s", index);
            psStringAppend(&query, ", INDEX(%s)", index);
            psFree(index);
        }
    }

    // Reset iterator to head of list
    psListIteratorSet(cursor, 0);
    // look for foreign keys after all other key types
    while ((item = psListGetAndIncrement(cursor))) {
        // don't compile a regex unless we have too
        if (psStrcasestr(item->comment, "FKEY") == NULL) {
            continue;
        }
        // find foreign key and references
        regex_t         myregex;
        regmatch_t      mymatch[3];     // match + 2 sub strings
        int             errbuf_size = 1024;
        char            errbuf[errbuf_size];
        char            *pattern = "FKEY(.*)[:space:]*REF(.*)";

        int status = regcomp(&myregex, pattern, REG_EXTENDED|REG_ICASE);
        if (status != 0) {
            regerror(status, &myregex, errbuf, errbuf_size);
            psError(PS_ERR_UNKNOWN, true, "regcomp() failed: %s", errbuf);
            psFree(query);
            psFree(cursor);
            return NULL;
        }

        psTrace("psLib.db", 10, "trying regex pattern: %s against string: %s", pattern, item->comment);
        status = regexec(&myregex, item->comment, 3, mymatch, 0);
        if (status != 0) {
            regerror(status, &myregex, errbuf, errbuf_size);
            psError(PS_ERR_UNKNOWN, true, "regexec() failed: %s", errbuf);
            psFree(query);
            psFree(cursor);
            return NULL;
        }

        regfree(&myregex);

        // sub string 1: foreign(.*)
        size_t matchStart = (size_t)mymatch[1].rm_so;
        size_t matchEnd   = (size_t)mymatch[1].rm_eo;
        size_t matchLength = matchEnd - matchStart;

        if (matchStart == -1) {
            psError(PS_ERR_UNKNOWN, true, "substring 1 failed to match");
            psFree(query);
            psFree(cursor);
            return NULL;
        }

        psString fkey = psStringNCopy(item->comment + matchStart, matchLength);
        psTrace("psLib.db", 10, "regex $1 matched: %s", fkey);
        psStringAppend(&query, ", FOREIGN KEY %s", fkey);
        psFree(fkey);

        // sub string 2: references(.*)
        matchStart = (size_t)mymatch[2].rm_so;
        matchEnd   = (size_t)mymatch[2].rm_eo;
        matchLength = matchEnd - matchStart;

        if (matchStart == -1) {
            psError(PS_ERR_UNKNOWN, true, "substring 2 failed to match");
            psFree(query);
            psFree(cursor);
            return NULL;
        }

        psString refs = psStringNCopy(item->comment + matchStart, matchLength);
        psTrace("psLib.db", 10, "regex $2 matched: %s", refs);
        psStringAppend(&query, " REFERENCES %s", refs);
        psFree(refs);
    }

    psFree(cursor);

    // end column types + table type
    psStringAppend(&query, ") ENGINE=innodb");

    return query;
}

static psString psDBGenerateSelectRowSQL(const char *tableName,
        const char *col,
        const psMetadata *where,
        psU64 limit)
{
    PS_ASSERT_PTR_NON_NULL(tableName, NULL);

    char            *query = NULL;
    char            *whereSQL;
    char            *limitString;

    // select all columns if col is NULL
    if (col) {
        psStringAppend(&query, "SELECT %s FROM %s", col, tableName);
    } else {
        psStringAppend(&query, "SELECT * FROM %s", tableName);
    }

    // select all rows if where is NULL
    if (where) {
        whereSQL = psDBGenerateWhereSQL(where, tableName);
        if (!whereSQL) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "SQL substring generation failed.");

            psFree(query);

            return NULL;
        }
        psStringAppend(&query, " %s", whereSQL);
        psFree(whereSQL);
    }

    // treat limit == 0 as "no limit"
    if (limit) {
        limitString = psDBGenerateLimitSQL(limit);
        psStringAppend(&query, " %s", limitString);
        psFree(limitString);
    }

    return query;
}

psString psDBGenerateLimitSQL(psU64 limit)
{
    psString query = NULL;

    psString limitString = psDBIntToString(limit);
    psStringAppend(&query, " LIMIT %s", limitString);
    psFree(limitString);

    return query;
}

static psString psDBGenerateInsertRowSQL(const char *tableName,
        const psMetadata *row)
{
    PS_ASSERT_PTR_NON_NULL(tableName, NULL);
    PS_ASSERT_PTR_NON_NULL(row, NULL);

    char            *query = NULL;
    psListIterator  *cursor;
    psMetadataItem  *item;

    // Start building query string
    psStringAppend(&query, "INSERT INTO %s (", tableName);

    cursor = psListIteratorAlloc(row->list, 0, false);

    // get field names
    while ((item = psListGetAndIncrement(cursor))) {
        psStringAppend(&query, "%s", item->name);

        // + , + _ between every field name
        if (!cursor->offEnd) {
            psStringAppend(&query, ", ");
        }
    }

    // end of field names
    psStringAppend(&query, ") VALUES (");

    psListIteratorSet(cursor, 0);

    // create value place holders
    while ((item = psListGetAndIncrement(cursor))) {
        psStringAppend(&query, "?");

        // + ", " between every place holder
        if (!cursor->offEnd) {
            psStringAppend(&query, ", ");
        }
    }

    psFree(cursor);

    // end of values
    psStringAppend(&query, ")");

    return query;
}

static psString psDBGenerateUpdateRowSQL(const char *tableName,
        const psMetadata *where,
        const psMetadata *values)
{
    PS_ASSERT_PTR_NON_NULL(tableName, NULL);
    PS_ASSERT_PTR_NON_NULL(values, NULL);

    char            *query = NULL;
    char            *setSQL;
    char            *whereSQL;

    // Create set SQL substring
    setSQL = psDBGenerateSetSQL(values);
    if (!setSQL) {
        psError(PS_ERR_UNEXPECTED_NULL, false, _("SQL substring generation failed."));
        return NULL;
    }

    // Create where SQL substring
    whereSQL = psDBGenerateWhereSQL(where, tableName);
    if (!whereSQL) {
        psError(PS_ERR_UNEXPECTED_NULL, false, _("SQL substring generation failed."));
        return NULL;
    }

    // Append substring to SQL update string
    psStringAppend(&query, "UPDATE %s %s %s", tableName, setSQL, whereSQL);
    psFree(setSQL);
    psFree(whereSQL);

    return query;
}

static psString psDBGenerateDeleteRowSQL(const char *tableName,
        const psMetadata *where,
        unsigned long long limit)
{
    PS_ASSERT_PTR_NON_NULL(tableName, NULL);

    char            *query = NULL;
    char            *whereSQL;
    char            *limitString;

    // delete all rows if where is NULL
    if (!where) {
        psStringAppend(&query, "TRUNCATE TABLE %s", tableName);
        return query;
    }

    // Generate where SQL substring
    whereSQL = psDBGenerateWhereSQL(where, tableName);
    if (!whereSQL) {
        psError(PS_ERR_UNEXPECTED_NULL, false, _("SQL substring generation failed."));
        return NULL;
    }

    psStringAppend(&query, "DELETE FROM %s %s", tableName, whereSQL);

    // Complete delete SQL command string
    // treat limit == 0 as "no limit"
    if (limit) {
        limitString = psDBIntToString(limit);
        psStringAppend(&query, " LIMIT %s", limitString);
        psFree(limitString);
    }
    psFree(whereSQL);

    return query;
}

psString psDBGenerateWhereSQL(const psMetadata *where, const char *tableName)
{
    PS_ASSERT_PTR_NON_NULL(where, NULL);

    psString search = psDBGenerateWhereConditionSQL(where, tableName);
    if (search) {
        psStringPrepend(&search, "WHERE ");
    }

    return search;
}

psString psDBGenerateWhereConditionSQL(const psMetadata *where, const char *tableName)
{
    PS_ASSERT_PTR_NON_NULL(where, NULL);

    psString query = NULL;

    // we need to know if an item is 'MULTI' so we have to march through the
    // list by key name and not by items
    psList *keys = psHashKeyList(where->hash);
    psListIterator *cursor = psListIteratorAlloc(keys, 0, false);

    // find column name and match pattern
    psString itemName = NULL;
    while ((itemName = psListGetAndIncrement(cursor))) {
        psMetadataItem *item = psMetadataLookup(where, itemName);

        if (item->type == PS_DATA_METADATA_MULTI) {
            char *logicalOp = "AND";

            // scan through the list and change the logicalOp joining
            // conditionals together to "OR" if a comment of "==" or "LIKE" is found
            psListIterator *mCursor = psListIteratorAlloc(item->data.list, 0,
                                      false);
            psMetadataItem *mItem = NULL;
            while ((mItem = psListGetAndIncrement(mCursor))) {
                if (mItem->comment && psStrcasestr(mItem->comment, "==")) {
                    logicalOp = "OR";
                    break;
                }
                if (mItem->comment && psStrcasestr(mItem->comment, "LIKE")) {
                    logicalOp = "OR";
                    break;
                }
            }

            // "(" + conditional [ + " AND/OR " + conditional ] + ")"
            // reset the iterator
            psListIteratorSet(mCursor, 0);
            psStringAppend(&query, "(");
            while ((mItem = psListGetAndIncrement(mCursor))) {
                psString conditional = psDBGenerateConditionalSQL(mItem, tableName);
                if (!conditional) {
                    psError(PS_ERR_UNKNOWN, false,
                            "SQL conditional generation failed.");
                    psFree(mCursor);
                    psFree(cursor);
                    psFree(keys);
                    psFree(query);
                    return NULL;
                }

                psStringAppend(&query, "%s", conditional);
                psFree(conditional);

                // + " AND/OR "
                if (!mCursor->offEnd) {
                    psStringAppend(&query, " %s ", logicalOp);
                }

            }
            psFree(mCursor);

            psStringAppend(&query, ")");
        } else {
            psString conditional = psDBGenerateConditionalSQL(item, tableName);
            if (!conditional) {
                psError(PS_ERR_UNKNOWN, false,
                        "SQL conditional generation failed.");
                psFree(cursor);
                psFree(keys);
                psFree(query);
                return NULL;
            }

            psStringAppend(&query, "%s", conditional);
            psFree(conditional);
        }

        // + " and " after every column declaration except the last one
        if (!cursor->offEnd) {
            psStringAppend(&query, " AND ");
        }
    }

    //    psFree(cursor);
    psFree(keys);

    return query;
}

static psString psDBGenerateSetSQL(const psMetadata *set
                                  )
{
    PS_ASSERT_PTR_NON_NULL(set
                           , NULL);

    psString query = psStringCopy("SET ");

    psListIterator *cursor = psListIteratorAlloc(set
                             ->list, 0, false);

    // find column name
    psMetadataItem *item = NULL;
    while ((item = psListGetAndIncrement(cursor))) {
        // + column name + _ + = + _ + ?
        psStringAppend(&query, "%s = ?", item->name);

        // + ", " after every column declaration except the last one
        if (!cursor->offEnd) {
            psStringAppend(&query, ",  ");
        }
    }

    psFree(cursor);

    return query;
}

typedef enum {
    PS_DB_OP_EQ,
    PS_DB_OP_LT,
    PS_DB_OP_GT,
    PS_DB_OP_LE,
    PS_DB_OP_GE,
    PS_DB_OP_NE,
} psDBOpValue;

# define PS_DB_FLT_PAD (FLT_EPSILON * 10)
# define PS_DB_DBL_PAD (DBL_EPSILON * 10)

static psString psDBGenerateConditionalSQL(const psMetadataItem *item, const char *tableName)
{
    PS_ASSERT_PTR_NON_NULL(item, NULL);

    psString query = NULL;

    // stringify the psMetadataItem into a SQL search specification
    // XXX we're making a big assumption here that the MySQL server handles
    // floating point in the exact same way as the client.  This is a bit scary
    // as MySQL uses native types on the server end.  In theory all IEEE754
    // math is the same but know that isn't always the case.  At least forcing
    // the comparison to be done on the server provides some consistency
    // between clients on different archs.

    // if tableName is specified prepend it to the item name
    psString itemName = NULL;
    if (tableName) {
        psStringAppend(&itemName, "%s.%s", tableName, item->name);
    } else {
        psStringAppend(&itemName, "%s", item->name);
    }

    // select the by of comparasion to use.  currently the comparision
    // op value is only used by PS_DATA_S32 & PS_DATA_TIME

    // default to exact match ('=')
    char *opStr = "=";
    psDBOpValue op = PS_DB_OP_EQ;
    if (item->comment) {
        // arbitrary semantic, precedence is: >=, <=, >, <, = (default)
        if (strstr(item->comment, ">=")) {
            opStr = ">=";
            op = PS_DB_OP_GE;
        } else if (strstr(item->comment, "<=")) {
            opStr = "<=";
            op = PS_DB_OP_LE;
        } else if (strstr(item->comment, ">")) {
            opStr = ">";
            op = PS_DB_OP_GT;
        } else if (strstr(item->comment, "<")) {
            opStr = "<";
            op = PS_DB_OP_LT;
        } else if (strstr(item->comment, "!=")) {
            opStr = "!=";
            op = PS_DB_OP_NE;
        }
    }


    // XXX why are >, < searches not supported here????
    // fix this immediately!!
    // for datetime comparisons, we need to use single-quotes around the string value:
    // where dateobs < '2002-09-06' and dateobs > '2002-09-05,15:40:22'

    switch (item->type) {
    case PS_DATA_S8:
    case PS_DATA_S16:
    case PS_DATA_S32:
        // the raw opStr is good enough in the query
        psStringAppend(&query, "%s %s %d", itemName, opStr, (int)(item->data.S32));
        break;
    case PS_DATA_S64:
        // the raw opStr is good enough in the query
        psStringAppend(&query, "%s %s %" PRId64, itemName, opStr, item->data.S64);
        break;
    case PS_DATA_U8:
    case PS_DATA_U16:
    case PS_DATA_U32:
        psStringAppend(&query, "%s %s %u", itemName, opStr, (unsigned int)(item->data.U32));
        break;
    case PS_DATA_U64:
        // the raw opStr is good enough in the query
        psStringAppend(&query, "%s %s %" PRIu64, itemName, opStr, item->data.U64);
        break;
    case PS_DATA_F32:
        // need to handle floating-point round-off issues (use a padding of 10*FLT_EPSILON)
        switch (op) {
        case PS_DB_OP_EQ:
            psStringAppend(&query, "(ABS(%s - %.8f) < %.8f)", itemName, (float)(item->data.F32), PS_DB_FLT_PAD);
            break;
        case PS_DB_OP_NE:
            psStringAppend(&query, "(ABS(%s - %.8f) >= %.8f)", itemName, (float)(item->data.F32), PS_DB_FLT_PAD);
            break;
        case PS_DB_OP_LE:
        case PS_DB_OP_LT:
            // A < B becomes A < B + epsilon
            psStringAppend(&query, "(%s < %.8f + %.8f)", itemName, (float)(item->data.F32), PS_DB_FLT_PAD);
            break;
        case PS_DB_OP_GE:
        case PS_DB_OP_GT:
            // A > B becomes A > B - epsilon
            psStringAppend(&query, "(%s > %.8f - %.8f)", itemName, (float)(item->data.F32), PS_DB_FLT_PAD);
            break;
        }
        break;
    case PS_DATA_F64:
        // need to handle floating-point round-off issues (use a padding of 10*FLT_EPSILON)
        switch (op) {
        case PS_DB_OP_EQ:
            psStringAppend(&query, "(ABS(%s - %.17f) < %.17f)", itemName, (float)(item->data.F64), PS_DB_DBL_PAD);
            break;
        case PS_DB_OP_NE:
            psStringAppend(&query, "(ABS(%s - %.17f) >= %.17f)", itemName, (float)(item->data.F64), PS_DB_DBL_PAD);
            break;
        case PS_DB_OP_LE:
        case PS_DB_OP_LT:
            // A < B becomes A < B + epsilon
            psStringAppend(&query, "(%s < %.17f + %.17f)", itemName, (float)(item->data.F64), PS_DB_DBL_PAD);
            break;
        case PS_DB_OP_GE:
        case PS_DB_OP_GT:
            // A > B becomes A > B - epsilon
            psStringAppend(&query, "(%s > %.17f - %.17f)", itemName, (float)(item->data.F64), PS_DB_DBL_PAD);
            break;
        }
        break;
    case PS_DATA_BOOL:
        switch (op) {
        case PS_DB_OP_EQ:
            psStringAppend(&query, "%s = %d", itemName, (int)(item->data.B));
            break;
        case PS_DB_OP_NE:
            psStringAppend(&query, "%s != %d", itemName, (int)(item->data.B));
            break;
        default:
            psError(PS_ERR_UNKNOWN, true, "NULL bool can't be compared with any operator other than '=='");
            psFree(itemName);
            return NULL;
        }
        break;
    case PS_DATA_STRING:
        // XXX EAM : probably need to add some regex-like operations here
        // + column name + _ + like + _ + ' + value + '
        // check for NULL and empty ("") strings
        if (item->data.V == NULL || *item->data.str == '\0') {
            psStringAppend(&query, "%s IS NULL", itemName);
        } else {
            if (item->comment && psStrcasestr(item->comment, "LIKE")) {
                // XXX ASC NOTE: we should have a better match for
                // char & varchar columns than this.  LIKE is OK for
                // very large TEXT columns that really shouldn't be
                // used in a where clause...
                psStringAppend(&query, "%s LIKE '%s'", itemName, item->data.str);
            } else if (item->comment && psStrcasestr(item->comment, "NOTLKE")) {
                // XXX ASC NOTE: we should have a better match for
                // char & varchar columns than this.  LIKE is OK for
                // very large TEXT columns that really shouldn't be
                // used in a where clause...
                psStringAppend(&query, "%s NOT LIKE '%s'", itemName, item->data.str);
            } else {
                psStringAppend(&query, "%s %s '%s'", itemName, opStr, item->data.str);
            }
        }
        break;
    case PS_DATA_TIME: {
            if (item->data.V) {
                psString timeStr = psTimeToString(item->data.V, 6); // MySQL only handles microseconds
                psStringAppend(&query, "%s %s '%s'", itemName, opStr, timeStr);
                psFree(timeStr);
            } else if (strstr(opStr, "=")) {
                psStringAppend(&query, "%s IS NULL", itemName);
            } else {
                psError(PS_ERR_UNKNOWN, true, "psTime comparison value is NULL: A NULL psTime value can't be compared with any operator other than '=='");
                psFree(itemName);
                return NULL;
            }

            break;
        }
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                "Fixme: Unsupported data type");
        psFree(itemName);

        return NULL;
    }

    psFree(itemName);

    return query;
}


// lookup table functions
/*****************************************************************************/

static psElemType psDBMySQLToPType(enum enum_field_types type,
                                   unsigned int flags)
{
    psHash          *mysqlToSQLTable;   // type lookup table
    psHash          *sqlToPSTable;      // type lookup table
    char            *key;               // hash tmp value
    char            *value;             // hash tmp value
    psString        sqlType;            // copy of lookup table result
    psU32           pType;              // psElemType of a field

    mysqlToSQLTable = psDBMySQLToSQLTableGet();

    // lookup MySQL column type
    key     = psDBIntToString((psU64)type);
    sqlType = psMemIncrRefCounter(psHashLookup(mysqlToSQLTable, key));
    psFree(key);

    if (!sqlType) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "type lookup failed.");
        return 0;
    }

    // MySQL column types can not be directly translated to PS
    // types as the the mysql types do not tell you if the value is
    // signed or unsigned.  The result is this ugly conversion from
    // mysql -> ascii -> ptype
    // XXX it appears that TIMESTAMP fields get marked as being unsigned...
    if ((flags & UNSIGNED_FLAG) && strncmp(sqlType, "DATETIME", 9)) {
        psString new = NULL;
        psStringAppend(&new, "%s UNSIGNED", sqlType);
        psFree(sqlType);
        sqlType = new;
    }

    //psTrace("psLib.db", 9, "sqlType=[%s]\n", sqlType );

    // convert MySQL type to PS type
    sqlToPSTable = psDBSQLToPTypeTableGet();
    value = psHashLookup(sqlToPSTable, sqlType);

    if (!value) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "type lookup of %s failed.", sqlType);
        psFree(sqlType);
        return 0;
    }
    psFree(sqlType);

    pType = (psU32)atol(value);

    return pType;
}

static psString psDBPTypeToSQL(psElemType pType)
{
    psHash          *pTypeToSQLTable;   // type lookup table
    char            *key;               // hash tmp value
    char            *sqlType;             // hash tmp value

    pTypeToSQLTable = psDBPTypeToSQLTableGet();

    key = psDBIntToString((psU64)pType);
    sqlType = psHashLookup(pTypeToSQLTable, key);
    psFree(key);

    if (!sqlType) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "type lookup failed.");

        return NULL;
    }

    psMemIncrRefCounter(sqlType);

    return sqlType;
}

static mysqlType *psDBPTypeToMySQL(psElemType pType)
{
    psHash          *pTypeToMySQLTable; // type lookup table
    char            *key;               // hash tmp value
    mysqlType       *mType;             // mysqlType struct to return

    pTypeToMySQLTable = psDBPTypeToMySQLTableGet();

    key = psDBIntToString((psU64)pType);
    mType = psHashLookup(pTypeToMySQLTable, key);
    psFree(key);

    if (!mType) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "type lookup failed.");

        return NULL;
    }

    psMemIncrRefCounter(mType);

    return mType;
}

// PTypeToSQLTable

static psHash *psDBPTypeToSQLTableSetup(void)
{
    if (!pTypeToSQLlookupTable) {
        pTypeToSQLlookupTable = psHashAlloc(14);

        // no support for CHAR, TEXT or GLOB
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_S8,  "TINYINT");
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_S16, "SMALLINT");
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_S32, "INT");
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_S64, "BIGINT");
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_U8,  "TINYINT UNSIGNED");
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_U16, "SMALLINT UNSIGNED");
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_U32, "INT UNSIGNED" );
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_U64, "BIGINT UNSIGNED");
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_F32, "FLOAT");
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_F64, "DOUBLE");
        // XXX Since BOOL is added after S8 all "TINYINT" data will appear in
        // the database as boolean data.  There does not seem to be any way to
        // work around this with MySQL < 5.0.3.
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_BOOL,"TINYINT");
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_STRING, "VARCHAR");
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_TIME, "DATETIME");
        psDBAddToLookupTable(pTypeToSQLlookupTable, PS_DATA_UNKNOWN, "NULL");
    } else {
        // increment the ref count by one for every psDB
        psMemIncrRefCounter(pTypeToSQLlookupTable);
    }

    return pTypeToSQLlookupTable;
}

static psHash *psDBPTypeToSQLTableGet(void)
{
    return pTypeToSQLlookupTable;
}

static void psDBPTypeToSQLTableCleanup(void)
{
    PSDB_NULL_FREE(pTypeToSQLlookupTable);
}


// SQLToPTypeTable

static psHash *psDBSQLToPTypeTableSetup(void)
{
    psHash          *psToSQLTable;
    psList          *list;
    psListIterator  *cursor;
    char            *key;
    char            *value;

    if (!sqlToPTypeLookupTable) {
        // invert the PSToSQL table
        psToSQLTable = psDBPTypeToSQLTableGet();
        sqlToPTypeLookupTable = psHashAlloc(psToSQLTable->n);

        list = psHashKeyList(psToSQLTable);
        cursor = psListIteratorAlloc(list, 0, false);

        while ((key = psListGetAndIncrement(cursor))) {
            value = psHashLookup(psToSQLTable, key);
            // switch key and value
            psHashAdd(sqlToPTypeLookupTable, value, key);
        }

        // Add BLOB & TEXT reverse mappings
        value = psDBIntToString((psU64)PS_DATA_STRING);
        psHashAdd(sqlToPTypeLookupTable, "BLOB",    value);
        psHashAdd(sqlToPTypeLookupTable, "TEXT",    value);
        psFree(value);

        // DECIMAL does not exist in the pType to SQL table
        value = psDBIntToString(0);
        psHashAdd(sqlToPTypeLookupTable, "DECIMAL", value);
        psFree(value);

        psFree(cursor);
        psFree(list);
    } else {
        // increment the ref count by one for every psDB
        psMemIncrRefCounter(sqlToPTypeLookupTable);
    }

    return sqlToPTypeLookupTable;
}

static psHash *psDBSQLToPTypeTableGet(void)
{
    return sqlToPTypeLookupTable;
}

static void psDBSQLToPTypeTableCleanup(void)
{
    PSDB_NULL_FREE(sqlToPTypeLookupTable);
}


// MySQLToSQLTable

static psHash *psDBMySQLToSQLTableSetup(void)
{
    if (!mysqlToSqlLookupTable) {
        mysqlToSqlLookupTable = psHashAlloc(20);

        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_TINY,      "TINYINT");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_SHORT,     "SMALLINT");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_LONG,      "INT");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_INT24,     "MEDIUMINT");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_LONGLONG,  "BIGINT");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_DECIMAL,   "DECIMAL");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_FLOAT,     "FLOAT");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_DOUBLE,    "DOUBLE");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_TIMESTAMP, "DATETIME");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_DATE,      "DATE");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_TIME,      "TIME");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_DATETIME,  "DATETIME");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_YEAR,      "YEAR");
        // strictly speaking this should CHAR but there is no equivilent ps type
        // for char as psString is already mapped to varchar
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_STRING,    "VARCHAR");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_VAR_STRING,"VARCHAR");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_BLOB,      "BLOB");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_SET,       "SET");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_ENUM,      "ENUM");
        psDBAddToLookupTable(mysqlToSqlLookupTable, MYSQL_TYPE_NULL,      "NULL");
    } else {
        // increment the ref count by one for every psDB
        psMemIncrRefCounter(mysqlToSqlLookupTable);
    }

    return mysqlToSqlLookupTable;
}

static psHash *psDBMySQLToSQLTableGet(void)
{
    return mysqlToSqlLookupTable;
}

static void psDBMySQLToSQLTableCleanup(void)
{
    PSDB_NULL_FREE(mysqlToSqlLookupTable);
}


// PTypeToMySQLTable

static psHash *psDBPTypeToMySQLTableSetup(void)
{
    if (!pTypeToMysqlLookupTable) {
        pTypeToMysqlLookupTable = psHashAlloc(14);

        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_S8,     psDBMySQLTypeAlloc(MYSQL_TYPE_TINY,       false));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_S16,    psDBMySQLTypeAlloc(MYSQL_TYPE_SHORT,      false));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_S32,    psDBMySQLTypeAlloc(MYSQL_TYPE_LONG,       false));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_S64,    psDBMySQLTypeAlloc(MYSQL_TYPE_LONGLONG,   false));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_U8,     psDBMySQLTypeAlloc(MYSQL_TYPE_TINY,       true));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_U16,    psDBMySQLTypeAlloc(MYSQL_TYPE_SHORT,      true));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_U32,    psDBMySQLTypeAlloc(MYSQL_TYPE_LONG,       true));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_U64,    psDBMySQLTypeAlloc(MYSQL_TYPE_LONGLONG,   true));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_F32,    psDBMySQLTypeAlloc(MYSQL_TYPE_FLOAT,      false));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_F64,    psDBMySQLTypeAlloc(MYSQL_TYPE_DOUBLE,     false));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_BOOL,   psDBMySQLTypeAlloc(MYSQL_TYPE_TINY,       true));
        // XXX: removed PS_DATA_PTR, can this be removed too?
        // psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_PTR,    psDBMySQLTypeAlloc(MYSQL_TYPE_VAR_STRING, false));

        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_STRING,    psDBMySQLTypeAlloc(MYSQL_TYPE_VAR_STRING, false));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_TIME,      psDBMySQLTypeAlloc(MYSQL_TYPE_DATETIME, false));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_VECTOR,    psDBMySQLTypeAlloc(MYSQL_TYPE_VAR_STRING, false));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_IMAGE,    psDBMySQLTypeAlloc(MYSQL_TYPE_VAR_STRING, false));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_HASH,   psDBMySQLTypeAlloc(MYSQL_TYPE_VAR_STRING, false));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_LOOKUPTABLE,
                                 psDBMySQLTypeAlloc(MYSQL_TYPE_VAR_STRING, false));
        //        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_JPEG,   psDBMySQLTypeAlloc(MYSQL_TYPE_VAR_STRING, false));
        //        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_PNG,    psDBMySQLTypeAlloc(MYSQL_TYPE_VAR_STRING, false));
        //        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_ASTROM, psDBMySQLTypeAlloc(MYSQL_TYPE_VAR_STRING, false));
        psDBAddVoidToLookupTable(pTypeToMysqlLookupTable, PS_DATA_UNKNOWN,psDBMySQLTypeAlloc(MYSQL_TYPE_NULL, false));
    } else {
        // increment the ref count by one for every psDB
        psMemIncrRefCounter(pTypeToMysqlLookupTable);
    }

    return pTypeToMysqlLookupTable;
}

static psHash *psDBPTypeToMySQLTableGet(void)
{
    return pTypeToMysqlLookupTable;
}

static void psDBPTypeToMySQLTableCleanup(void)
{
    PSDB_NULL_FREE(pTypeToMysqlLookupTable);
}

static psPtr psDBMySQLTypeAlloc(enum enum_field_types type,
                                bool isUnsigned)
{
    mysqlType *mType = psAlloc(sizeof(mysqlType));
    mType->type       = type;
    mType->isUnsigned = isUnsigned;

    return mType;
}

static void psDBAddToLookupTable(psHash *lookupTable,
                                 psU32 type,
                                 const char *string)
{
    PS_ASSERT_PTR_NON_NULL(lookupTable, );

    psString key = psDBIntToString((psU64)type);
    psString value = psStringCopy(string);

    psHashAdd(lookupTable, key, value);

    psFree(key);
    psFree(value);
}

static void psDBAddVoidToLookupTable(psHash *lookupTable,
                                     psU32 type,
                                     psPtr value)
{
    PS_ASSERT_PTR_NON_NULL(lookupTable, );

    psString key = psDBIntToString((psU64)type);

    psHashAdd(lookupTable, key, value);

    // destructive of value parameter
    psFree(value);
    psFree(key);
}

static psErrorCode mysqlTopsErr(MYSQL *mysql)
{
    unsigned int myerrno = mysql_errno(mysql);
    if ((myerrno >= 1000) && (myerrno < 2000)) {
        return PS_ERR_DB_SERVER;
    } else if ((myerrno >= 2000) && (myerrno < 3000)) {
        return PS_ERR_DB_CLIENT;
    }

    return PS_ERR_UNKNOWN;
}


// pType utility functions
/*****************************************************************************/

#define PS_NAN_ALLOC(dest, type, nan) \
dest = psAlloc(sizeof(type)); \
*(type *)dest = nan;

static psPtr psDBGetPTypeNaN(psElemType pType)
{
    psPtr           myNaN = NULL;

    switch (pType) {
    case PS_DATA_S8:
        PS_NAN_ALLOC(myNaN, psS8, PS_MAX_S8);
        break;
    case PS_DATA_S16:
        PS_NAN_ALLOC(myNaN, psS16, PS_MAX_S16);
        break;
    case PS_DATA_S32:
        PS_NAN_ALLOC(myNaN, psS32, PS_MAX_S32);
        break;
    case PS_DATA_S64:
        PS_NAN_ALLOC(myNaN, psS64, PS_MAX_S64);
        break;
    case PS_DATA_U8:
        PS_NAN_ALLOC(myNaN, psU8, PS_MAX_U8);
        break;
    case PS_DATA_U16:
        PS_NAN_ALLOC(myNaN, psU16, PS_MAX_U16);
        break;
    case PS_DATA_U32:
        PS_NAN_ALLOC(myNaN, psU32, PS_MAX_U32);
        break;
    case PS_DATA_U64:
        PS_NAN_ALLOC(myNaN, psU64, PS_MAX_U64);
        break;
    case PS_DATA_F32:
        PS_NAN_ALLOC(myNaN, psF32, NAN);
        break;
    case PS_DATA_F64:
        PS_NAN_ALLOC(myNaN, psF64, NAN);
        break;
    case PS_DATA_BOOL:
        // XXX: what is NaN for a bool?
        PS_NAN_ALLOC(myNaN, psU8, PS_MAX_U8);
        break;
    }

    return myNaN;
}

#define PS_IS_NAN(type, data, nan) *(type *)data == nan

static MYSQL_BOOL psDBIsPTypeNaN(psElemType pType, psPtr data)

{
    bool    isNaN = NULL;

    switch (pType) {
      case PS_DATA_S8:
        isNaN = PS_IS_NAN(psS8, data, PS_MAX_S8);
        break;
      case PS_DATA_S16:
        isNaN = PS_IS_NAN(psS16, data, PS_MAX_S16);
        break;
      case PS_DATA_S32:
        isNaN = PS_IS_NAN(psS32, data, PS_MAX_S32);
        break;
      case PS_DATA_S64:
        isNaN = PS_IS_NAN(psS64, data, PS_MAX_S64);
        break;
      case PS_DATA_U8:
        isNaN = PS_IS_NAN(psU8, data, PS_MAX_U8);
        break;
      case PS_DATA_U16:
        isNaN = PS_IS_NAN(psU16, data, PS_MAX_U16);
        break;
      case PS_DATA_U32:
        isNaN = PS_IS_NAN(psU32, data, PS_MAX_U32);
        break;
      case PS_DATA_U64:
        isNaN = PS_IS_NAN(psU64, data, PS_MAX_U64);
        break;
      case PS_DATA_F32:
	isNaN = !isfinite(*((psF32 *) data)); // trap nan, +inf, -inf
	break;
      case PS_DATA_F64:
	isNaN = !isfinite(*((psF64 *) data)); // trap nan, +inf, -inf
	break;
      case PS_DATA_BOOL:
        isNaN = PS_IS_NAN(psU8, data, PS_MAX_U8); // probably meaningless
        break;
    }

    return isNaN;
}

// string utility functions
/*****************************************************************************/

psString psDBIntToString(psU64 value)
{
    // length of string (log10 + 1) + \0
    // if value is 0, length is 1 char + \0
    size_t length = value ? (size_t)log10((double)value) + 1 + 1
                    : 2;
    psString string = psStringAlloc(length);
    snprintf(string, length, "%li", (long int)value);

    return string;
}

#endif // HAVE_PSDB
