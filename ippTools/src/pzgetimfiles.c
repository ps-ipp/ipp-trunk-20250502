/*
 * pzgetimfiles.c
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

#include "pxtag.h"
#include "pxtools.h"
#include "pzgetimfiles.h"

#define FILESET_LS_CMD "dsfilesetls"

static bool go (pxConfig *config);
static psArray *parseFiles(pxConfig *config, const char *str);

int main(int argc, char **argv)
{
    psLibInit(NULL);

    pxConfig *config = pzgetimfilesConfig(NULL, argc, argv);
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
    PXOPT_LOOKUP_STR(uri, config->args, "-uri", true, false);
    PXOPT_LOOKUP_STR(filesetid, config->args, "-filesetid", true, false);
    PXOPT_LOOKUP_STR(camera, config->args, "-inst", true, false);
    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", true, false);

    // optional
    PXOPT_LOOKUP_S32(timeout, config->args, "-timeout", false, false);

    // invoke dsfilesetls
    psString cmd = NULL;
    psStringAppend(&cmd, "%s --uri %s", FILESET_LS_CMD, uri);
    if (timeout) {
        psStringAppend(&cmd, " --timeout %d", timeout);
    }

    psTrace("pzgetimfiles", PS_LOG_INFO, "cmd is: %s\n", cmd);

    FILE *output = popen(cmd, "r");
    psFree(cmd);

    if (!output) {
        psError(PS_ERR_UNKNOWN, true, "popen() failed");
        return false;
    }

    psString cmdOutput = psSlurpFile(output);
    int status = pclose(output);

    // We have an id value now, let's get it and use it
    char *id_query = "SELECT summit_id FROM summitExp WHERE exp_name = '%s' AND camera = '%s' AND telescope = '%s'";

    if (!p_psDBRunQueryF(config->dbh, id_query, filesetid, camera, telescope)) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      return false;
    }
    
    psArray *id_output = p_psDBFetchResult(config->dbh);
    if (!id_output) {
      psError(PS_ERR_UNKNOWN, false, "database error");
      return false;
    }
    if (psArrayLength(id_output) != 1) {
      psError(PS_ERR_UNKNOWN, false, "database error: incorrect number of results");
      return false;
    }

    psMetadata *id_result = id_output->data[0];

    psS64 summit_id = psMetadataLookupS64(NULL, id_result, "summit_id");
    psFree(id_output);
    // End of summit_id block.
    
    if (status != 0) {
        // mark the summitExp row as faulted
        if (!p_psDBRunQueryF(config->dbh, "UPDATE summitExp SET fault = %d WHERE summit_id = %lld", WEXITSTATUS(status), (long long) summit_id)) {
            psError(PS_ERR_UNKNOWN, false, "database error");
        }

        psError(PS_ERR_UNKNOWN, true, "%s failed with exit status %d",
            FILESET_LS_CMD, WEXITSTATUS(status));
        psFree(cmdOutput);
        return false;
    }

    // prase output of dsfilesetls
    psArray *newImfiles = parseFiles(config, cmdOutput);
    if (!newImfiles) {
        // XXX not nessicarily an error but we don't want to keep trying to
        // download an "empty" fileset.
        // mark the summitExp row as faulted
        if (!p_psDBRunQueryF(config->dbh, "UPDATE summitExp SET fault = %d WHERE summit_id = %lld", 250, (long long) summit_id)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
        }
        psError(PS_ERR_UNKNOWN, true, "no new files/imfiles");
        psFree(cmdOutput);
        return false;
    }
    psFree(cmdOutput);
    
    // save the number of new Imfiles;
    long imfiles = psArrayLength(newImfiles);

    // start a transaction so it's all rows or nothing
    if (!psDBTransaction(config->dbh)) {
        psError(PS_ERR_UNKNOWN, false, "database error");
        return false;
    }

    // if the fileset was empty (no files) then we can bail out early
    if (imfiles == 0) {
        psFree(newImfiles);

        char *query = 
            "UPDATE summitExp"
            " SET imfiles = %d"
	    " WHERE summit_id = %ld";
        if (!p_psDBRunQueryF(config->dbh, query, imfiles, summit_id)) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }

        // remove the pzDownloadExp entry for this exp (fileset)
        {
            char *query = 
                "DELETE FROM pzDownloadExp"
                " WHERE"
	        " summit_id = %ld";
            if (!p_psDBRunQueryF(config->dbh, query, summit_id)) {
                // rollback
                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error");
                }
                psError(PS_ERR_UNKNOWN, false, "database error");
                return false;
            }

            // sanity check: we should have removed only one row
            psU64 affected = psDBAffectedRows(config->dbh);
            if (psDBAffectedRows(config->dbh) != 1) {
                // rollback
                if (!psDBRollback(config->dbh)) {
                    psError(PS_ERR_UNKNOWN, false, "database error");
                }
                psError(PS_ERR_UNKNOWN, false, "should have affected 1 row but %" PRIu64 " rows were modified", affected);
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

    // create a temp table
    {
        char *query = 
            "CREATE TEMPORARY TABLE incoming (exp_name VARCHAR(64), camera VARCHAR(64), telescope VARCHAR(64), file_id VARCHAR(64), bytes INT, md5sum VARCHAR(32), class VARCHAR(64), class_id VARCHAR(64), uri VARCHAR(255), PRIMARY KEY(exp_name, camera, telescope, class, class_id)) ENGINE=MEMORY";

        if (!p_psDBRunQuery(config->dbh, query)) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(newImfiles);
            return false;
        }
    }

    // load the imfiles (files) into the temp table
    {
        char *query = "INSERT INTO incoming (exp_name, camera, telescope, file_id, bytes, md5sum, class, class_id, uri) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

        long inserted = p_psDBRunQueryPrepared(config->dbh, newImfiles, query);
        if (inserted < 0) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            psFree(newImfiles);
            return false;
        }
        // sanity check that we actually inserted something
        if (inserted == 0) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error -- we should have inserted at least one row");
            psFree(newImfiles);
            return false;
        }
    }

    psFree(newImfiles);

    // copy imfiles (files) from the temp table into summitImfiles
    {
        char *query = 
            "INSERT IGNORE INTO summitImfile" 
            "   SELECT"
	    "       %ld," // summit_id
            "       incoming.exp_name,"
            "       incoming.camera,"
            "       incoming.telescope,"
            "       incoming.file_id,"
            "       incoming.bytes,"
            "       incoming.md5sum,"
            "       incoming.class,"
            "       incoming.class_id,"
            "       incoming.uri,"
            "       NULL"       // epoch
            "   FROM incoming";
        if (!p_psDBRunQueryF(config->dbh, query,summit_id)) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }
    }

    // update summitExp.imfiles (should have been NULL)
    {
        char *query = 
            "UPDATE summitExp"
            " SET imfiles = (SELECT COUNT(*) FROM summitImfile"
            "   WHERE"
	    "   summit_id = %ld"
            ")" 
            " WHERE"
	    "   summit_id = %ld"
            "   AND imfiles IS NULL";
        if (!p_psDBRunQueryF(config->dbh, query, summit_id, summit_id)) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "database error");
            return false;
        }

        // sanity check: we should have updated only one row
        psU64 affected = psDBAffectedRows(config->dbh);
        if (psDBAffectedRows(config->dbh) != 1) {
            // rollback
            if (!psDBRollback(config->dbh)) {
                psError(PS_ERR_UNKNOWN, false, "database error");
            }
            psError(PS_ERR_UNKNOWN, false, "should have affected 1 row but %" PRIu64 " rows were modified", affected);
            return false;
        }
    }

    // add new exps to pzDownloadExp -- must be done before the new exps are
    // added to summitExp because of the SQL logic
    {
        char *query = 
            "INSERT IGNORE INTO pzDownloadExp" 
            "   SELECT"
  	    "       %ld,"    // summit_id
            "       incoming.exp_name,"
            "       incoming.camera,"
            "       incoming.telescope,"
            "       \"run\","    // state
            "       NULL"
            "   FROM incoming"
            "   GROUP BY"
            "       incoming.exp_name,"
            "       incoming.camera,"
            "       incoming.telescope";

        if (!p_psDBRunQueryF(config->dbh, query,summit_id)) {
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

static psArray *parseFiles(pxConfig *config, const char *str)
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_PTR_NON_NULL(str, NULL);

    // these are constants for all records parsed -- look them up before we do
    // any work
    PXOPT_LOOKUP_STR(exp_name, config->args, "-filesetid", true, false);
    PXOPT_LOOKUP_STR(camera, config->args, "-inst", true, false);
    PXOPT_LOOKUP_STR(telescope, config->args, "-telescope", true, false);

    // split the string into lines
    psList *doc = psStringSplit(str, "\n", false);

    psListIterator *lineCursor = psListIteratorAlloc(doc, 0, false);

    psArray *pzPendingImfiles = psArrayAllocEmpty(psListLength(doc));
    psString line;
    while ((line = psListGetAndIncrement(lineCursor))) {
        psTrace("pzgetimfiles", PS_LOG_INFO, "parsing line: %s\n", line);

        // split line into tokens
        psList *tokens = psStringSplit(line, " ", false);

        // check to see if this line is a comment (or if the first token is
        // NULL)
        if (!psListGet(tokens, 0) || *((char *)psListGet(tokens, 0)) == '#') {
            psFree(tokens);
            continue;
        }

        // check that we have the right number of tokens
        // print "# uri fileid bytes md5sum type \n";
        if (psListLength(tokens) < 6) {
            psError(PS_ERR_UNKNOWN, true, "invalid line format: %s", line);
            psFree(tokens);
            psFree(pzPendingImfiles);                
            psFree(lineCursor);
            psFree(doc);
            return false;
        }

        // find the values of interest
        psListIterator *tokenCursor = psListIteratorAlloc(tokens, 0, false);
        char *uri       = psListGetAndIncrement(tokenCursor);
        char *file_id   = psListGetAndIncrement(tokenCursor); // fileid
        char *bytes     = psListGetAndIncrement(tokenCursor); // bytes
        char *md5sum    = psListGetAndIncrement(tokenCursor); // md5sum
        char *class     = psListGetAndIncrement(tokenCursor); // type
        char *class_id  = psListGetAndIncrement(tokenCursor); // chipname

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
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "file_id", 0, NULL, file_id)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item bytes");
            psFree(md);
            psFree(tokenCursor);
            psFree(tokens);
            return NULL;
        }
        if (!psMetadataAddS64(md, PS_LIST_TAIL, "bytes", 0, NULL, (psS64)atoll(bytes))) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item bytes");
            psFree(md);
            psFree(tokenCursor);
            psFree(tokens);
            return NULL;
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "md5sum", 0, NULL, md5sum)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item md5sum");
            psFree(md);
            psFree(tokenCursor);
            psFree(tokens);
            return NULL;
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class", 0, NULL, class)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item class");
            psFree(md);
            psFree(tokenCursor);
            psFree(tokens);
            return NULL;
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, class_id)) {
            psError(PS_ERR_UNKNOWN, false, "failed to add item class_id");
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

        // debugging
        if (psTraceGetLevel("pzgetimfiles") >= PS_LOG_INFO) {
            psString doc = psMetadataConfigFormat(md);
            psTrace("pzgetimfiles", PS_LOG_INFO, "parsed line as:\n %s\n", doc);
            psFree(doc);
        }

        psArrayAdd(pzPendingImfiles, 0, md);

        psFree(md);
    }

    psFree(lineCursor);
    psFree(doc);

    if (!psArrayLength(pzPendingImfiles)) {
        psError(PS_ERR_UNKNOWN, false, "string contained no valid rows");
        psFree(pzPendingImfiles);
        return false;
    }

    return pzPendingImfiles;
}
