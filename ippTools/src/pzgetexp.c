/*
 * pzgetexp.c
 *
 * Copyright (C) 2006-2008  Joshua Hoblitt
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * program; see the file COPYING. If not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pxtools.h"
#include "pzgetexp.h"

#define PRODUCT_LS_CMD "dsproductls"

static bool go (pxConfig *config);
static psArray *parseFileSets(pxConfig *config, const char *str);

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = pzgetexpConfig(NULL, argc, argv);
    if (!config) {
        psError(PXTOOLS_ERR_CONFIG, false, "failed to configure");
        goto FAIL;
    }

    if (!go(config)) {
        goto FAIL;
    }

    psFree(config);
    pmConfigDone();
    psLibFinalize();

    exit(EXIT_SUCCESS);

FAIL:
    psErrorStackPrint(stderr, "\n");
    int exit_status = pxerrorGetExitStatus();
    
    psFree(config);
    pmConfigDone();
    psLibFinalize();

    exit(exit_status);
}

static bool go(pxConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    // required
    PXOPT_LOOKUP_STR(camera, config->args, "-inst", true, false);
    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", true, false);
    PXOPT_LOOKUP_STR(uri, config->args, "-uri", true, false);

    //optional
    PXOPT_LOOKUP_S32(timeout, config->args, "-timeout", false, false);
    PXOPT_LOOKUP_BOOL(all, config->args, "-all", false);
    PXOPT_LOOKUP_BOOL(ignore_errors, config->args, "-ignore-errors", false);

    // find last fileset/exp_name (if we have one)
    bool haveLastFileSet = false;
    psString lastFileSet = NULL;

    // -all means "request all known filesets"
    if (!all) {
        char *query = "SELECT * from summitExp WHERE camera = \"%s\" and TELESCOPE = \"%s\" ORDER BY dateobs DESC LIMIT 1";
        if (!p_psDBRunQueryF(config->dbh, query, camera, telescope)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }

        psArray *output = p_psDBFetchResult(config->dbh);
        if (!output) {
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        if (!psArrayLength(output)) {
            psError(PS_ERR_UNKNOWN, false, "no summitExp rows found");
            haveLastFileSet = false;
            psFree(output);
        } else {
            haveLastFileSet = true;
            bool status = false;
            lastFileSet = psStringCopy(psMetadataLookupStr(&status, output->data[0], "exp_name"));
            psFree(output);
        }
    }

    // invoke dsproductls
    // dsproductls --uri <> --last_fileset <>
    psString cmd = NULL;
    if (haveLastFileSet) {
        psStringAppend(&cmd, "%s --uri %s --last_fileset %s",
            PRODUCT_LS_CMD, uri, lastFileSet);
        psFree(lastFileSet);
    } else {
        psStringAppend(&cmd, "%s --uri %s", PRODUCT_LS_CMD, uri);
    }
    if (timeout) {
        psStringAppend(&cmd, " --timeout %d", timeout);
    }

    psTrace("pzgetexp", PS_LOG_INFO, "cmd is: %s\n", cmd);

    FILE *output = popen(cmd, "r");
    psFree(cmd);

    if (!output) {
        psError(PS_ERR_UNKNOWN, true, "popen() failed");
        return false;
    }
    psString cmdOutput = psSlurpFile(output);
    pclose(output);

    psArray *newSummitExps = parseFileSets(config, cmdOutput);
    psFree(cmdOutput);
    if (!newSummitExps) {
        // XXX not necessarily an error?
        psError(PS_ERR_UNKNOWN, true, "no new fileSet/exp IDs");
        return false;
    }
    if (!psArrayLength(newSummitExps)) {
        psTrace("pzgetexp", PS_LOG_INFO, "no new fileSet/exp IDs");
        psFree(newSummitExps);
        return true;
    }
    // If we supply an unknown last_fileset (exposure name) to the summit datastore,
    // it will return all filesets it knows about.  This can cause problems (table overflow)
    // if this is too large a number.  Raise an error and let the user fix the underlying
    // problem (last exposure not known to the summit datastore)
    // Do we need to be able to configure this number?
    if (psArrayLength(newSummitExps) > 10000) {
        psError(PS_ERR_UNKNOWN, true, "too many new fileSet/exp IDs? unknown exposure? problem with summitExp table?");
        return false;
    }

    // start a transaction so it's all rows or nothing
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        psFree(newSummitExps);
        return false;
    }

    // increase memory table size limits; default is 16MB, but our current (2022) config
    // in /etc/mysql/my.cnf sets to default to 1024M.
    // This command sets the max_heap_table_size within the transaction, and does not
    // apparently stick.  But this is not really needed, so we are deactivating it.
    if (0) {
        // 512MB
        char *query = "SET max_heap_table_size = 1024*1024*512";
        if (!p_psDBRunQuery(config->dbh, query)) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(newSummitExps);
            return false;
        }
    }

    // XXX for test, make this not a temporary table:
    // create a temporry table
    {
        char *query = "CREATE TEMPORARY TABLE incoming" 
            " (exp_name VARCHAR(64), camera VARCHAR(64), telescope VARCHAR(64), dateobs DATETIME, exp_type VARCHAR(64), uri VARCHAR(255), PRIMARY KEY(exp_name, camera, telescope))"
           " ENGINE=MEMORY";
        if (!p_psDBRunQuery(config->dbh, query)) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(newSummitExps);
            return false;
        }
    }

    {
      char *query_notice = "INSERT INTO incoming (exp_name, camera, telescope, dateobs, exp_type, uri) VALUES (?, ?, ?, ?, ?, ?)";
      char *query_ignore = "INSERT IGNORE INTO incoming (exp_name, camera, telescope, dateobs, exp_type, uri) VALUES (?, ?, ?, ?, ?, ?)";

      char *query = ignore_errors ? query_ignore : query_notice;

        long inserted = p_psDBRunQueryPrepared(config->dbh, newSummitExps, query);
        if (inserted < 0) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(newSummitExps);
            return false;
        }
	fprintf (stderr, "inserted: %d\n", (int) inserted);
        // sanity check that we actually inserted something
        if (inserted == 0) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error -- we should have inserted at least one row");
            psFree(newSummitExps);
            return false;
        }
    }

    psFree(newSummitExps);

    // TEST
    if (0) {
	char *query = "select * from incomingTest";
	p_psDBRunQuery(config->dbh, query);
        psArray *output = p_psDBFetchResult(config->dbh);
        if (!output) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
	    return false;
        }
	
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "test", TRUE)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
	exit (2);
    }

    // debug this query
    if (0) {
        char *query = 
            "DESCRIBE INSERT INTO summitExp" 
            "   SELECT"
  	    "       NULL," // summit_id
            "       incomingTest.*,"
            "       NULL,"  // imfiles
            "       0,"     // fault
            "       NULL"   // epoch
            "   FROM incomingTest"
            "   LEFT JOIN summitExp"
            "       USING(exp_name, camera, telescope)"
            "   WHERE"
            "       summitExp.exp_name is NULL"
            "       AND summitExp.camera is NULL"
            "       AND summitExp.telescope is NULL";

        if (!p_psDBRunQuery(config->dbh, query)) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
        psArray *output = p_psDBFetchResult(config->dbh);
        if (!output) {
	    psError(PS_ERR_UNKNOWN, false, "database error");
	    return false;
        }
	
        // negative simple so the default is true
        if (!ippdbPrintMetadatas(stdout, output, "test", TRUE)) {
            psError(PS_ERR_UNKNOWN, false, "failed to print array");
            psFree(output);
            return false;
        }
        psFree(output);
    }

    // exit  (1);

    // add new exps to summitExp
    {
        char *query = 
            "INSERT INTO summitExp" 
            "   SELECT"
  	    "       NULL," // summit_id
            "       incoming.*,"
            "       NULL,"  // imfiles
            "       0,"     // fault
            "       NULL"   // epoch
            "   FROM incoming"
            "   LEFT JOIN summitExp"
            "       USING(exp_name, camera, telescope)"
            "   WHERE"
            "       summitExp.exp_name is NULL"
            "       AND summitExp.camera is NULL"
	    "       AND summitExp.telescope is NULL";

	// MAJOR hack to avoid excess long inserts
	// include this line to limit the incomping entries
	// "       AND incoming.dateobs > '2023-01-01'";

        if (!p_psDBRunQuery(config->dbh, query)) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
    }
    
    // point of no return
    if (!psDBCommit(config->dbh)) {
        // rollback
        if (!psDBRollback(config->dbh)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    return true;
}

static psArray *parseFileSets(pxConfig *config, const char *str)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_PTR_NON_NULL(str, NULL);
    
    // these are constants for all records parsed -- look them up before we do
    // any work
    // required
    PXOPT_LOOKUP_STR(camera, config->args, "-inst", true, false);
    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", true, false);

    // split the string into lines
    psList *doc = psStringSplit(str, "\n", false);

    psListIterator *lineCursor = psListIteratorAlloc(doc, 0, false);

    psArray *summitExps = psArrayAllocEmpty(psListLength(doc));
    psString line;
    while ((line = psListGetAndIncrement(lineCursor))) {
        psTrace("pzgetimfile", PS_LOG_INFO, "parsing line: %s\n", line);

        // split line into tokens
        psList *tokens = psStringSplit(line, " ", false);

        // check to see if this line is a comment (or if the first token is
        // NULL)
        if (!psListGet(tokens, 0) || *((char *)psListGet(tokens, 0)) == '#') {
            psFree(tokens);
            continue;
        }

        // check that we have the right number of tokens
        // print "# uri fileset datetime type\n";
        if (psListLength(tokens) != 4) {
            // error
            return false;
        }

        // find the values of interest
        psListIterator *tokenCursor = psListIteratorAlloc(tokens, 0, false);
        char *uri       = psListGetAndIncrement(tokenCursor);
        char *exp_name    = psListGetAndIncrement(tokenCursor); // fileset
        char *dateobsStr= psListGetAndIncrement(tokenCursor); // datetime
        char *exp_type  = psListGetAndIncrement(tokenCursor); // type

        // create a new metadata to represent this line and it's values
        psMetadata *md = psMetadataAlloc();
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "exp_name", 0, NULL, exp_name)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item exp_name");
            psFree(md);
            psFree(tokenCursor);
            psFree(tokens);
            return NULL;
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "camera", 0, NULL, camera)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item camera");
            psFree(md);
            psFree(tokenCursor);
            psFree(tokens);
            return NULL;
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "telescope", 0, NULL, telescope)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item telescope");
            psFree(md);
            psFree(tokenCursor);
            psFree(tokens);
            return NULL;
        }
        psTime *dateobs = psTimeFromISO(dateobsStr, PS_TIME_UTC);
        if (!psMetadataAddTime(md, PS_LIST_TAIL, "dateobs", 0, NULL, dateobs)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item telescope");
            psFree(dateobs);
            psFree(md);
            psFree(tokenCursor);
            psFree(tokens);
            return NULL;
        }
        psFree(dateobs);
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "exp_type", 0, NULL, exp_type)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item exp_type");
            psFree(md);
            psFree(tokenCursor);
            psFree(tokens);
            return NULL;
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, uri)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item uri");
            psFree(md);
            psFree(tokenCursor);
            psFree(tokens);
            return NULL;
        }

        // must be freed after the new metadata is built -- holds the strings
        psFree(tokenCursor);
        psFree(tokens);

        // add the new metadata to the result set
        psArrayAdd(summitExps, 0, md);

        // debugging
        if (psTraceGetLevel("pzgetexp") == PS_LOG_INFO) {
            psString doc = psMetadataConfigFormat(md);
            psTrace("pzgetexp", PS_LOG_INFO, "parsed line as:\n %s\n", doc);
            psFree(doc);
        }

        psFree(md);

    }

    psFree(lineCursor);
    psFree(doc);

    return summitExps;
}
