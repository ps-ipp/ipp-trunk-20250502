/**
 *  C Implementation: tap_psLookupTable_all
 *
 * Description:  Tests for psLookupTableAlloc, psMemCheckLookupTable,
 *               psVectorsReadFromFile, psLookupTableImport, psLookupTableRead,
 *               psLookupTableInterpolate, psLookupTableInterpolateAll
 *
 * Author: dRob <David.Robbins@mhpcc.hpc.mil>, (C) 2006
 *
 * XXX: Must add correct build directories.  This file reads a file in the
 * types/ subdirectory and will not execute directly if not run from psLib/test
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#include <pslib.h>
#include <string.h>
#include "tap.h"
#include "pstap.h"


int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(32);

    // Tests for psLookupTable Functions


    // testLookupTableAlloc()
    // psLookupTableAlloc & psMemCheckLookupTable Fxns
    {
        psMemId id = psMemGetId();
        psLookupTable *lt = NULL;
        //Tests for psLookupTableAlloc
        //Return NULL for NULL filename input
        {
            lt = psLookupTableAlloc(NULL, "\%f \%lf \%d \%ld", 10);
            ok( lt == NULL,
                "psLookupTableAlloc:               return NULL for NULL filename input.");
        }
        //Return NULL for NULL format input
        {
            lt = psLookupTableAlloc("table.dat", NULL, 10);
            ok( lt == NULL,
                "psLookupTableAlloc:               return NULL for NULL format input.");
        }
        //Return properly allocated lookupTable for valid inputs
        {
            lt = psLookupTableAlloc("table.dat", "\%f \%lf \%d \%ld", 10);
            ok( lt != NULL && psMemCheckLookupTable(lt),
                "psLookupTableAlloc:               "
                "return properly allocated lookupTable for valid inputs.");
        }

        //Tests for psMemCheckArray
        //Make sure psMemCheckArray works correctly - return false
        if (0) {
            int j = 2;
            ok( !psMemCheckLookupTable(&j),
                "psMemCheckLookupTable:            return false for non-LookupTable input.");
        }

        psFree(lt);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testLookupTableReadImport()
    // psVectorsReadFromFile, psLookupTableImport, psLookupTableRead Fxns");
    {
        psMemId id = psMemGetId();
        psArray *outVec = NULL;
        psArray *vectors = NULL;
        psLookupTable*  table1  = NULL;
        long           numRows = 0;

        //Tests for psVectorReadFromFile
        // Attempt to read from NULL filename input
        {
            outVec = psVectorsReadFromFile(NULL,
                                           "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf");
            ok( outVec == NULL,
                "psVectorsReadFromFile:            return NULL for NULL filename input.");
        }
        // Attempt to read from NULL format input
        {
            outVec = psVectorsReadFromFile("table.dat", NULL);
            ok( outVec == NULL,
                "psVectorsReadFromFile:            return NULL for NULL format input.");
        }
        // Attempt to read from invalid filename input
        {
            outVec = psVectorsReadFromFile("tableS32.dat",
                                           "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf");
            ok( outVec == NULL,
                "psVectorsReadFromFile:            return NULL for invalid filename input.");
        }
        // Attempt to read from invalid format input
        {
            outVec = psVectorsReadFromFile("table.dat", "\%s");
            ok( outVec == NULL,
                "psVectorsReadFromFile:            return NULL for invalid format input.");
        }
        // Attempt to read from table containing invalid entry - F32
        {
            outVec = psVectorsReadFromFile("tableF32_err.dat",
                                           "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf");
            ok( outVec == NULL,
                "psVectorsReadFromFile:            return NULL for table containing invalid entry.");
        }
        // Attempt to read from table containing invalid entry - F64
        {
            outVec = psVectorsReadFromFile("tableF64_err.dat",
                                           "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf");
            ok( outVec == NULL,
                "psVectorsReadFromFile:           return NULL for table containing invalid entry.");
        }
        // Attempt to read from table containing invalid entry - S32
        {
            outVec = psVectorsReadFromFile("tableS32_err.dat",
                                           "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf");
            ok( outVec == NULL,
                "psVectorsReadFromFile:           return NULL for table containing invalid entry.");
        }
        // Attempt to read from table containing invalid entry - S64
        {
            outVec = psVectorsReadFromFile("tableS64_err.dat",
                                           "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf");
            ok( outVec == NULL,
                "psVectorsReadFromFile:           return NULL for table containing invalid entry.");
        }
        // Attempt to read from empty table
        {
            outVec = psVectorsReadFromFile("tableF32_2.dat",
                                           "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf");
            ok( outVec == NULL,
                "psVectorsReadFromFile:           return NULL for empty table.");
        }
        // Attempt to read from file with partially constructed row
        {
            outVec = psVectorsReadFromFile("table3.dat",
                                           "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf");
            ok( outVec == NULL,
                "psVectorsReadFromFile:           return NULL for table with partially"
                " constructed row.");
        }
        // Attempt to read with valid inputs
        {
            outVec = psVectorsReadFromFile("table.dat",
                                           "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf");
            ok(outVec, "psVectorsReadFromFile:           read vectors from file");

	    skip_start (!outVec, 2, "skipping tests using the invalid null array");
	    ok(outVec->n == 9, "psVectorsReadFromFile:           read correct number of vectors");

	    psVector *tmpVec = outVec->data[6];
	    ok(tmpVec->data.S32[1] == -8, "psVectorsReadFromFile:           selected data element has correct value.");
	    skip_end();
        }

        //Tests for psLookupTableRead
        // Attempt to read table with NULL input table specified
        numRows = psLookupTableRead(table1);
        {
            ok( numRows == 0,
                "psLookupTableRead:               return NULL for NULL filename input.");
        }
        // Attempt to read table with bad filename specified
        table1 = psLookupTableAlloc("psTable.dat", "\%f \%lf \%d \%ld", 10);
        numRows = psLookupTableRead(table1);
        {
            ok( numRows == 0,
                "psLookupTableRead:               return NULL for table with invalid filename.");
        }

        // Attempt to read valid table with wrong indexCol
        psFree(table1);
        table1 = psLookupTableAlloc("table.dat",
                                "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf", 10);
        numRows = psLookupTableRead(table1);
        {
            ok( numRows == 0,
                "psLookupTableRead:               return correct number of rows for valid inputs.");
        }
        // Attempt to read valid table
        psFree(table1);
        table1 = psLookupTableAlloc("table.dat",
                                    "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf", 0);
        numRows = psLookupTableRead(table1);
        {
            ok( numRows == 4,
                "psLookupTableRead:               return correct number of rows for valid inputs.");
        }

        //Tests for psLookupTableImport (remaining cases)
        // Attempt to import table with negative indexCol
        vectors = psVectorsReadFromFile(table1->filename, table1->format);
        {
            ok( !psLookupTableImport(table1, vectors, -1 ),
                "psLookupTableImport:             return false for negative indexCol input.");
        }
        //Attempt to import table with unsorted array column
        psFree(table1);
        table1 = psLookupTableAlloc("table.dat",
                                    "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf", 0);
        {
            ok( psLookupTableImport(table1, vectors, 7 ),
                "psLookupTableImport:             return true for array with unsorted column");
        }

        psFree(vectors);
        psFree(outVec);
        psFree(table1);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testLookupTableInterpolates()
    // psLookupTableInterpolate & psLookupTableInterpolateAll Fxns
    {
        psMemId id = psMemGetId();
        psLookupTable *table1 = NULL;
        psVector *vec = NULL;
        // XXX: Remove the "types" path here
        table1 = psLookupTableAlloc("table.dat",
                                    "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf", 0);
        psLookupTable *table2 = NULL;
        // XXX: Remove the "types" path here
        table2 = psLookupTableAlloc("table.dat",
                                    "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf", 0);
        psLookupTableRead(table2);
    
        //Tests for psLookupTableInterpolateAll
        //Return NULL for NULL table input
        {
            vec = psLookupTableInterpolateAll(NULL, 0);
            ok( vec == NULL,
                "psLookupTableInterpolateAll:     return NULL for NULL table input.");
        }
        //Return NULL for NULL table values
        {
            vec = psLookupTableInterpolateAll(table1, 0);
            ok( vec == NULL,
                "psLookupTableInterpolateAll:     return NULL for NULL table values.");
        }
    
        ok(table2->values != NULL, "table->values not NULL");
	skip_start (!table2->values, 3, "skipping for failed table load");
        //Return NULL for table with table->values->n == 0
        {
            long n = table2->values->n;
            table2->values->n = 0;
            vec = psLookupTableInterpolateAll(table2, 0);
            ok( vec == NULL,
                "psLookupTableInterpolateAll:     return NULL for table with no columns.");
            table2->values->n = n;
        }
        //Return NULL for invalid index input
        {
            vec = psLookupTableInterpolateAll(table2, -10.5);
            ok( vec == NULL,
                "psLookupTableInterpolateAll:     return NULL for invalid index input.");
        }
        //Return correct vector output for valid inputs
        {
            vec = psLookupTableInterpolateAll(table2, 1);
            skip_start(  vec == NULL, 1,
                         "Skipping 1 tests because psLookupTableInterpolateAll failed");
            is_double(vec->data.F64[0], 1.0,
                      "psLookupTableInterpolateAll:     return correct output vector for valid inputs.");
            skip_end();
        }
	skip_end();
    
        //Remaining tests for psLookupTableInterpolate
        //Return NAN for invalid index - divide by zero error
        psLookupTable *table3 = NULL;
        table3 = psLookupTableAlloc("table2.dat",
                                    "\%f \%d \%d \%ld \%d \%d \%d \%ld \%*d \%lf", 0);
        psLookupTableRead(table3);
        {
            double retVal;
            retVal = psLookupTableInterpolate(table3, 1.5e-20, 0);
            ok( isnan(retVal),
                "psLookupTableInterpolateAll:     return NAN for invalid index input.");
        }
    
        psFree(table3);
        psFree(vec);
        psFree(table2);
        psFree(table1);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}
