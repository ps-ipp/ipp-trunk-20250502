/** @file psDB.h
 *
 * Copyright (C) 2007  Joshua Hoblitt, University of Hawaii
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
 * @brief database types and functions
 *
 * This file defines the abstract database type and functions that
 * perform basic database operations.
 *
 * $Id: psDB.h,v 1.38 2009-02-07 01:11:24 eugene Exp $
 */

#ifndef PS_DB_H
#define PS_DB_H 1
#ifdef HAVE_PSDB

/// @addtogroup FileIO Input/Output
/// @{

#include "psType.h"
#include "psMetadata.h"

/** Database handle
 *
 *  An opaque object representing a database connection.
 *
 */
typedef struct
{
    void* mysql;                       ///< MySQL database handle
}
psDB;

/** Opens a new database connection
 *
 *  @return psDB*:      A new psDB object if the database connection is
 *  successful or NULL on failure.
 */
psDB *psDBAlloc(
    const char *host,                  ///< Database server hostname
    const char *user,                  ///< Database username
    const char *passwd,                ///< Database password
    const char *dbname,                ///< Database namespace
    unsigned int port                  ///< Database port number
) PS_ATTR_MALLOC;

/** Opens a new database connection
 *
 *  This function is deprecated favor of psDBAlloc()
 *
 *  @return psDB*:      A new psDB object if the database connection is
 *  successful or NULL on failure.
 */
#ifdef DOXYGEN
psDB *psDBInit(
    const char *host,                  ///< Database server hostname
    const char *user,                  ///< Database username
    const char *passwd,                ///< Database password
    const char *dbname,                ///< Database namespace
    unsigned int port                  ///< Database port number
);
#else // DOXYGEN
#define psDBInit(host, user, passwd, dbname, port) \
psDBAlloc(host, user, passwd, dbname, port)
#endif

#ifdef DOXYGEN
/** Closes a database connection
 *
 *  This function is deprecated favor of psFree()
 */
void psDBCleanup(
    psDB *dbh                          ///< Database handle
);
#else // DOXYGEN
#define psDBCleanup(dbh) \
psFree(dbh)
#endif

/** Creates a new database namespace
 *
 * @return bool:    true on success
 */
bool psDBCreate(
    psDB *dbh,                         ///< Database handle
    const char *dbname                 ///< New database namespace
);

/** Changes the current database namespace
 *
 * @return bool:    true on success
 */
bool psDBChange(
    psDB *dbh,                         ///< Database handle
    const char *dbname                 ///< Database namespace
);

/** Drops a database namespace
 *
 * @return bool:    true on success
 */
bool psDBDrop(
    psDB *dbh,                         ///< Database handle
    const char *dbname                 ///< Database namespace
);

/** Formats and Executes a SQL query
 *
 * This function will execute a string as a raw SQL query.  No additional
 * processing of the string or abstraction of the underlying database's SQL
 * dialect is provided.  Caveat emptor.
 *
 * @return bool:    true on success
 */
bool p_psDBRunQueryF(
    psDB *dbh,                         ///< Database handle
    const char *format,                ///< SQL string to execute
    ...                                ///< Arguments for name formatting and metadata item data.
) PS_ATTR_FORMAT(printf, 2, 3);

/** Executes a SQL query
 *
 * This function will execute a string as a raw SQL query.  No additional
 * processing of the string or abstraction of the underlying database's SQL
 * dialect is provided.  Caveat emptor.
 *
 * @return bool:    true on success
 */
bool p_psDBRunQuery(
    psDB *dbh,                         ///< Database handle
    const char *query                ///< SQL string to execute
    );

/** Formats and Executes a SQL query as a prepared statement
 *
 * This function will execute a string as a raw SQL query.  No additional
 * processing of the string or abstraction of the underlying database's SQL
 * dialect is provided.  Caveat emptor.
 *
 * @return long:    the number of database rows affected
 */
long p_psDBRunQueryPreparedF(
    psDB *dbh,                          ///< Database handle
    const psArray *rowSet,              ///< row data as psArray of psMetadata
    const char *format,                 ///< SQL string to execute
    ...
) PS_ATTR_FORMAT(printf, 3, 4);

/** Executes a SQL query as a prepared statement
 *
 * This function will execute a string as a raw SQL query.  No additional
 * processing of the string or abstraction of the underlying database's SQL
 * dialect is provided.  Caveat emptor.
 *
 * @return long:    the number of database rows affected
 */
long p_psDBRunQueryPrepared(
    psDB *dbh,                          ///< Database handle
    const psArray *rowSet,              ///< row data as psArray of psMetadata
    const char *query			///< SQL string to execute
);

/** Fetches the result of a SQL query
 *
 * This function returns the result of the most recent SQL query as a psArray
 * of psMetadata.  Caveat emptor.
 *
 * @return psArray*:    A psArray of psMetadata or NULL on failure
 */
psArray *p_psDBFetchResult(
    psDB *dbh                          ///< Database handle
);

/** Creates a new database table
 *
 * This function generates and executes the SQL needed to create a table named
 * "tableName", with the column names and data types as described in "md".  Each
 * data item in the psMetadata collection represents a single table field.  The
 * name of the field is given by the name of the psMetadataItem and the data
 * type is give by the psMetadataItem.type and psMetadataItem.ptype entries.  A
 * lookup table should be used to convert from PSLib types into MySQL
 * compatible SQL data types.  For example, a PS_DATA_STRING would map to an SQL99
 * varchar.  If the value of type is PS_DATA_STRING then the psMetadataItem.data
 * element is set to a string with the length for the field written as a text
 * string.  The value of the psMetadataItem.data element is unused for the
 * PS_META_PRIMITIVE types.  Other psMetadata types beyond PS_DATA_STRING and
 * PS_META_PRIMITIVE are not allowed in a table definition.
 *
 * Database indexes can be specified setting the "comment" field to "Primary
 * Key" or "Key".  Comments are otherwise ignored.
 *
 * @return bool:    true on success
 */
bool psDBCreateTable(
    psDB *dbh,                         ///< Database handle
    const char *tableName,             ///< Table name
    const psMetadata *md               ///< Column names, types, and indexes
);

/** Deletes a database table
 *
 * @return bool:    true on success
*/
bool psDBDropTable(
    psDB *dbh,                          ///< Database handle
    const char *tableName               ///< Table name
);

/** Selects a column from a table
 *
 * This function generates and executes the SQL needed to select an entire
 * column from a table or up to "limit" rows from it.  If "limit" is 0, the
 * entire range is returned.
 *
 * @return psArray*:    A psArray of strings or NULL on failure
 */
psArray *psDBSelectColumn(
    psDB *dbh,                         ///< Database handle
    const char *tableName,             ///< Table name
    const char *col,                   ///< Column name
    unsigned long long limit           ///< Maximum number of elements to return
);

/** Selects a column from a table and casts it to a given type
 *
 * This function generates and executes the SQL needed to select an entire
 * column from a table or up to "limit" rows from it.  If "limit" is 0, the
 * entire range is returned.  The data in the column is cast to to "pType".
 *
 * @return psVector*:   A psVector or NULL on failure
 */
psVector *psDBSelectColumnNum(
    psDB *dbh,                         ///< Database handle
    const char *tableName,             ///< Table name
    const char *col,                   ///< Column name
    psElemType type,                   ///< Resulting psVector type
    unsigned long long limit           ///< Maximum number of elements to return
);

/** Selects a set of rows from a table
 *
 * This function returns rows from the specified table which match the
 * restrictions given by "where".  The restrictions are specified as field /
 * value pairs.  The psMetadata collection "where" must consist of valid
 * database fields.  The selected rows are returned as a psArray of psMetadata
 * values, one per row.
 *
 * Currently, the "where" specification only supports the PS_DATA_STRING type.
 * The string value can be a SQL match pattern, e.g. "%foo%", or an empty
 * string, e.g. "", to match NULL field values.
 *
 * @return psArray*:    A psArray of psMetadata or NULL on failure
 */
psArray *psDBSelectRows(
    psDB *dbh,                         ///< Database handle
    const char *tableName,             ///< Table name
    const psMetadata *where,           ///< Row match criteria
    unsigned long long limit           ///< Maximum number of elements to return
);

/** Insert a single row into a table
 *
 * This function inserts the data from "row" into "tableName".
 *
 * The "row" specification uses the psMetadataItem name as the column name.
 * The field values may be specified in any order.  psMetadata types beyond
 * PS_DATA_STRING and PS_META_PRIMITIVE are not supported.  If fields are
 * specified in "row" that do not exist in "tableName", the insert will fail.
 *
 * @return bool:    true on success
 */
bool psDBInsertOneRow(
    psDB *dbh,                         ///< Database handle
    const char *tableName,             ///< Table name
    const psMetadata *row              ///< Row description
);

/** Insert a set of rows into a table
 *
 * This function inserts the data from "rowSet" into "tableName".
 *
 * "rowSet" is a psArray of psMetadata containing row specifications identical to
 * those used in psDBInsertOneRow().
 *
 * @return bool:    true on success
 */
bool psDBInsertRows(
    psDB *dbh,                         ///< Database handle
    const char *tableName,             ///< Table name
    const psArray *rowSet              ///< Set of rows to insert
);

/** Retrieves all rows from a table
 *
 * This function fetches all rows as an psArray of psMetadata.  The rows are in
 * the same psMetadata format as used in psDBInsertOneRow() & psDBInsertRows().
 *
 * @return psArray*:    A psArray of psMetadata or NULL on failure
 */
psArray *psDBDumpRows(
    psDB *dbh,                         ///< Database handle
    const char *tableName              ///< Table name
);

/** Retrieves all columns from a table
 *
 * This function fetches all columns, as either a psVector or a psArray
 * depending on whether or not the column is numeric, and return them in a
 * psMetadata structure where psMetadataItem.name contains the column's name.
 *
 * @return psMetadata*:     A psMetadata containing either a psArrays or psVector per column
 */
psMetadata *psDBDumpCols(
    psDB *dbh,                         ///< Database handle
    const char *tableName              ///< Table name
);

/** Updates the field values, as specified, in a table
 *
 * This function updates the fields contained in "values" in the row(s) that
 * have a field with the value indicated by "where".  Where "where" is in the
 * same format as used in psDBSelectRows().
 *
 * The "values" specification uses the same format as the row specification
 * used in psDBInsertOneRow(), etc.
 *
 * @return long:    The number of rows modified or a negative value on error
 */
long psDBUpdateRows(
    psDB *dbh,                         ///< Database handle
    const char *tableName,             ///< Table name
    const psMetadata *where,           ///< Row match criteria
    psMetadata *values                 ///< new field values
);

/** Deletes rows, as specified, in a table
 *
 * Delete the rows that are matched by "where" using the same semantics for
 * "where" as in psDBUpdateRow().
 *
 * If "where" is NULL, all rows in the table will be removed and regardless of
 * the number of rows that were dropped, only 1 will be returned on success.
 *
 * @return long:    The number of rows removed or a negative value on error
 */
long psDBDeleteRows(
    psDB *dbh,                         ///< Database handle
    const char *tableName,             ///< Table name
    const psMetadata *where,           ///< Row match criteria
    unsigned long long limit           ///< Maximum number of rows to delete
);

/** Get the last insert ID
 *
 * Returns the value of MySQLs 'LAST_INSERT_ID()' function
 *
 * @return long:    The last insert ID
 */
long long psDBLastInsertID(
    psDB *dbh                          ///< Database handle
);

/** Enable/Disable explicit database transactions
 *
 * This function is used to enable explicit transaction support.  It is off by
 * default.
 *
 * @return bool:    true if transactions are enabled
 */
bool psDBExplicitTrans(
    psDB *dbh,                          ///< Database handle
    bool mode                           ///< transactions enable/disable
);

/** Start a new transaction set.
 *
 * This is only a meaningful action if explict transactions are disabled.
 *
 * @return bool:    true on success
 */
bool psDBTransaction(
    psDB *dbh                           ///< Database handle
);

/** Commits the current transaction
 *
 * This function will commit the current transaction set (a rollback is not
 * possible after this function is successfully executed).  A commit also
 * effectively starts a new transaction if explict transactions are enabled.
 *
 * @return bool:    true on success
 */
bool psDBCommit(
    psDB *dbh                           ///< Database handle
);

/** Rollback the current transaction
 *
 * This function will rollback the current transaction set.
 *
 * @return bool:    true on success
 */
bool psDBRollback(
    psDB *dbh                           ///< Database handle
);

/** Generates an SQL "Where" fragment
 *
 * This function generates an SQL fragment (not a whole usable query) based on
 * the standard "where" metadata format.
 *
 * @return psString:   A psString or NULL on failure
 */
psString psDBGenerateWhereSQL(
    const psMetadata *where,           ///< Row match criteria
    const char *tableName              ///< Table name
);

/** Generates an SQL "where conditon" statement
 *
 * This function generates a "Where" fragment but omits the "Where" keyword.
 * This function generates an SQL fragment (not a whole usable query) based on
 * the standard "where" metadata format.
 *
 * @return psString:   A psString or NULL on failure
 */
psString psDBGenerateWhereConditionSQL(
    const psMetadata *where,           ///< Row match criteria
    const char *tableName              ///< Table name
);

/** Generates an SQL "limit" statement
 *
 * This function generates an SQL fragment (not a whole usable query).
 *
 * @return psString:   A psString or NULL on failure
 */
psString psDBGenerateLimitSQL(
    psU64 limit                         ///< result set row limit
);

/** converts an integer into a psString
 *
 * Note that this function takes an unsigned value.
 *
 * @return psString:   A psString or NULL on failure
 */
psString psDBIntToString(
    psU64 value                         // integer value to convert
);

/** The number of rows modified or inserted by the last query
 *
 *  This function returns ((psU64) - 1) on error
 *
 * @return psU64
 */
psU64 psDBAffectedRows(
    psDB *dbh                           ///< Database handle
);

/// @}
#else
typedef void psDB;
#endif // HAVE_PSDB
#endif // PS_DB_H
