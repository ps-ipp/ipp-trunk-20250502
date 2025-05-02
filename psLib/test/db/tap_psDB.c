/** @file  tap_psDB.c
 *
 *  @brief Contains the tests for psDB.[ch]
 *
 *  @author Aaron Culliney, MHPCC
 *  @author Joshua Hoblitt, University of Hawaii
 *
 *  @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-03-27 22:52:02 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#include <string.h>

#include "tap.h"
#include "pstap.h"

#include "pslib.h"

#define HOST    "localhost"
#define USER    "test"
#define PASSWD  ""
#define DBNAME  "test"
#define PORT    0

int main(int argc, char* argv[])
{
#if DEBUG
    psTraceSetLevel("psLib.db", 10);
    psTraceSetLevel("err", 10);
#endif

    plan_tests(34 + 1);

    // see if we can open a db connection at all
    {
        psDB *dbh = psDBInit(HOST, USER, PASSWD, DBNAME, PORT);
        ok(dbh, "open a database handle");
        if (!dbh) {
            done();
        }
        psFree(dbh);
    }

    // test new database creation
    // the next 3 tests will fail as the MySQL test account has insufficent
    // permissions
    {
        psDB *dbh = psDBInit(HOST, USER, PASSWD, DBNAME, PORT);

        skip_start(!psDBCreate(dbh, "foobar"), 4,
                "you probably don't have proper database permissions");
        ok(psDBCreate(dbh, "foobar"), "psDBCreate()");
        ok(psDBChange(dbh, "foobar"), "psDBChange()");
        ok(psDBDrop(dbh, "foobar"), "psDBDrop()");
        // change back to the test dbname
        ok(psDBChange(dbh, DBNAME), "psDBChange()");
        skip_end();

        psFree(dbh);
    }

    // psDBCreateTable() & psDBDropTable()
    {
        psDB *dbh = psDBInit(HOST, USER, PASSWD, DBNAME, PORT);
        psMetadata *md = psMetadataAlloc();
        psMetadataAdd(md, PS_LIST_TAIL, "foo", PS_DATA_S32, "Primary Key", 0);
        psMetadataAdd(md, PS_LIST_TAIL, "bar", PS_DATA_BOOL, "Key", false);
        psMetadataAdd(md, PS_LIST_TAIL, "baz", PS_DATA_F64,  NULL, 0.0);
        psMetadataAdd(md, PS_LIST_TAIL, "boing", PS_DATA_STRING, NULL, "60");

        ok(psDBCreateTable(dbh, "bar", md), "psDBCreateTable() - create table");
        psFree(md);

        ok(psDBDropTable(dbh, "bar"), "psDBDropTable() - drop table");

        psFree(dbh);
    }

    {
        psDB *dbh = psDBInit(HOST, USER, PASSWD, DBNAME, PORT);
        psMetadata *md = psMetadataAlloc();
        psMetadataAdd(md, PS_LIST_TAIL, "foo", PS_TYPE_S32, "Primary Key", 0);
        psDBCreateTable(dbh, "bar", md);

        ok(!psDBCreateTable(dbh, "bar", md), "psDBCreateTable() - table already exists");

        psFree(md);

        psDBDropTable(dbh, "bar");

        ok(!psDBDropTable(dbh, "fubar"), "psDBDropTable() - non-existant table");
        psFree(dbh);
    }

    {
        psDB *dbh = psDBInit(HOST, USER, PASSWD, DBNAME, PORT);

        // setup yak table for later tests
        {
            psMetadata *md = psMetadataAlloc();
            psMetadataAdd(md, PS_LIST_TAIL, "hair", PS_TYPE_F32, NULL, 0.0);
            psDBCreateTable(dbh, "yak", md);
            psFree(md);
        }

        // setup horse table for later tests
        {
            psMetadata *md = psMetadataAlloc();
            psMetadataAdd(md, PS_LIST_TAIL, "color", PS_DATA_STRING, NULL, "100");
            psDBCreateTable(dbh, "horse", md);
            psFree(md);
        }

        // psDBInsertOneRow()
        {
            psMetadata *md = psMetadataAlloc();
            psMetadataAdd(md, PS_LIST_TAIL, "hair", PS_TYPE_F32, NULL, 10e3);

            ok(psDBInsertOneRow(dbh, "yak", md),"psDBInsertOneRow() - number");

            psFree(md);
        }

        {
            psMetadata *md = psMetadataAlloc();
            psMetadataAdd(md, PS_LIST_TAIL, "hair", PS_TYPE_F32, NULL, NAN);
            ok(psDBInsertOneRow(dbh, "yak", md),"psDBInsertOneRow() - nan");

            psFree(md);
        }

        // psDBInsertRows()
        {
            psArray *rowSet = psArrayAllocEmpty(3);
            psMetadata *row = psMetadataAlloc();
            psMetadataAdd(row, PS_LIST_TAIL, "color", PS_DATA_STRING, NULL, "brown");
            psArrayAdd(rowSet, 0, row);
            psFree(row);

            row = psMetadataAlloc();
            psMetadataAdd(row, PS_LIST_TAIL, "color", PS_DATA_STRING, NULL, NULL);
            psArrayAdd(rowSet, 0, row);
            psFree(row);

            row = psMetadataAlloc();
            psMetadataAdd(row, PS_LIST_TAIL, "color", PS_DATA_STRING, NULL, "pink");
            psArrayAdd(rowSet, 0, row);
            psFree(row);

            ok(psDBInsertRows(dbh, "horse", rowSet), "psDBInsertRows() - multi-row insert");

            psFree(rowSet);
        }

        // psDBSelectColumn()
        {
            psArray *column = psDBSelectColumn(dbh, "horse", "color", 42);
            ok(column, "psDBSelectColumn() - select");
            is_long(psArrayLength(column), 3, "psDBSelectColumn() - number of elements");

            // XXX this test is depending on the order the rows come out it...
            // this shouldn't be depended upon
            is_str((char *)column->data[0], "brown", "horse.color == 'brown'");
            ok((char *)column->data[1] == NULL,    "horse.color == NULL");
            is_str((char *)column->data[2], "pink",  "horse.color == 'pink'");

            psFree(column);
        }

        // psDBSelectColumnNum()
        {
            psVector *column = psDBSelectColumnNum(dbh, "yak", "hair", PS_TYPE_F32, 42);
            ok(column, "psDBSelectColumnNum() - select");
            is_long(psVectorLength(column), 2, "psDBSelectColumnNum() - number of elements");

            is_float(column->data.F32[0], 10e3, "hair == 10e3")
            is_float(column->data.F32[1], NAN, "hair == NAN")

            psFree(column);
        }

        // psDBSelectRows()
        {
            psMetadata *md = psMetadataAlloc();
            psMetadataAdd(md, PS_LIST_TAIL, "color", PS_DATA_STRING, NULL, "brown");

            psArray *resultSet = psDBSelectRows(dbh, "horse", md, 0);
            ok(resultSet, "psDBSelectRows() - select");
            is_long(psArrayLength(resultSet), 1, "psDBSelectRows() - number of rows");

            psFree(md);
            for (long i = 0; i < resultSet->n; i++) {
                md = (psMetadata *)resultSet->data[0];

                psMetadataItem *item = psListGet(md->list, 0);
                is_str(item->name, "color", "column name");
                is_str((char *)item->data.V, "brown", "column value");
            }
            psFree(resultSet);

        }

        // psDBDumpRows()
        {
            psArray *resultSet = psDBDumpRows(dbh, "horse");
            ok(resultSet, "psDBDumpRows() - dump");
            ok(psArrayLength(resultSet) == 3, "psDBDumpRows() - number of rows");
            psFree(resultSet);
        }

        // psDBDumpCols()
        {
            psMetadata *columns = psDBDumpCols(dbh, "horse");
            ok(columns, "psDBDumpCols() - dump");
            ok(psListLength(columns->list) == 1, "psDBDumpCols() - number of columns");

            psMetadataItem *item = psListGet(columns->list, 0);
            is_str(item->name, "color", "column name");
            is_long(psArrayLength((psArray*)item->data.V), 3, "column array length");

            psFree(columns);
        }

        // psDBUpdateRows()
        {
            psMetadata *where = psMetadataAlloc();
            psMetadataAdd(where, PS_LIST_TAIL, "color", PS_DATA_STRING, NULL, "pink");

            psMetadata *value = psMetadataAlloc();
            psMetadataAdd(value, PS_LIST_TAIL, "color", PS_DATA_STRING, NULL, "HOT pink");

            psU64 rowsAffected = psDBUpdateRows(dbh, "horse", where, value);
            psFree(where);

            ok(rowsAffected, "psDBUpdateRows() - pink -> HOT pink");

            psArray *resultSet = psDBSelectRows(dbh, "horse", value, 0);
            psFree(value);

            for (long i = 0; i < resultSet->n; i++) {
                psMetadata *md = (psMetadata *)resultSet->data[0];

                psMetadataItem *item = psListGet(md->list, 0);
                is_str((char *)item->data.V, "HOT pink", "psDBUpdateRows() - psDBSelectRows() found HOT pink");

            }

            psFree(resultSet);
        }

        // psDBDeleteRows()
        ok(psDBDeleteRows(dbh, "yak", NULL, 0), "psDBDeleteRows()");

        // cleanup other tests
        psDBDropTable(dbh, "horse");
        psDBDropTable(dbh, "yak");

        psFree(dbh);
    }


    done();
}
