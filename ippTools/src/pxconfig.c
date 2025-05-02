/*
 * pxconfig.c
 *
 * Copyright (C) 2006  Joshua Hoblitt
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

#include <string.h>
#include <stdio.h>

#include <pslib.h>
#include <psmodules.h>

#include "pxtools.h"

static void pxConfigFree(pxConfig *ptr);

pxConfig *pxConfigAlloc(void)
{
    pxConfig *config;

    config = psAlloc(sizeof(pxConfig));
    psMemSetDeallocator(config, (psFreeFunc)pxConfigFree);

    config->modeName        = NULL;
    config->mode            = 0;
    config->dbh             = NULL;
    config->modules         = NULL;
    // XXX config->where           = NULL;
    config->args            = NULL;

    return config;
}

static void pxConfigFree(pxConfig *config)
{
    psFree(config->modeName);
    psFree(config->dbh);
    psFree(config->modules);
    // XXX psFree(config->where);
    psFree(config->args);
}

void pxUsage(FILE *stream, int argc, char **argv, const char *modeName, psMetadata *argSet) 
{
    fprintf(stream, "Usage: %s %s [<options>]\n\n", argv[0], modeName);
    fprintf(stream, "%s:\n", modeName);

    psArgumentHelpSimple(stream, argSet);
}

bool pxGetOptions(FILE *stream, int argc, char **argv, pxConfig *config, psMetadata *modes, psMetadata *argSets)
{
    // figure out what mode we're running in
    psMetadataIterator *iter = psMetadataIteratorAlloc(modes, PS_LIST_HEAD, NULL);
    psMetadataItem *item = NULL;
    while ((item = psMetadataGetAndIncrement(iter))) {
        int N = 0;
        if ((N = psArgumentGet(argc, argv, item->name))) {
            psArgumentRemove(N, &argc, argv);
            // check for duplicate mode specification
            if (config->mode) {
                psError(PS_ERR_UNKNOWN, true, "only one mode selection is allowed");
                fprintf(stream, "only one mode selection is allowed\n");
                pxUsage(stream, argc, argv, "<mode>", modes);
                psFree(iter);
                return NULL;
            }

            config->mode = item->data.U32;
            config->modeName = psStringCopy(item->name);

            bool status = false;
            psMetadata *argset = psMetadataLookupMetadata(&status, argSets, item->name);
            // make sure we can find the argSet for the sepcified mode
            if (!status) {
                psError(PS_ERR_UNKNOWN, true, "can not find arguments for mode");
                fprintf(stderr, "can not find arugments for mode");
                psFree(iter);
                return NULL;
            }

            config->args = psMemIncrRefCounter(argset);
        }

    }

    psFree(iter);

    // make sure we found a mode
    if (config->mode == PXTOOL_MODE_NONE) {
        psError(PS_ERR_UNKNOWN, true, "mode argument is required");
        fprintf(stderr, "mode argument is required\n");
        pxUsage(stream, argc, argv, "<mode>", modes);
        return NULL;
    }

    // actually parse the command line
    if (!psArgumentParse(config->args, &argc, argv)) {
        psError(PS_ERR_UNKNOWN, false, "error parsing arguments");
        fprintf(stderr, "error parsing arguments\n");
        pxUsage(stream, argc, argv, config->modeName, config->args);
        return NULL;
    }

    // look for left overs on the command line
    if (argc != 1) {
        psError(PS_ERR_UNKNOWN, false, "extra arguments: ");
        fprintf(stderr, "extra arguments: ");
        for (int i = 1; i < argc; i++) {
            fprintf (stderr, "%s ", argv[i]);
        }
        fprintf(stderr, "\n");
        pxUsage(stream, argc, argv, config->modeName, config->args);
        return NULL;
    }

    // make sure that all required parameters have been specified
    iter = psMetadataIteratorAlloc(config->args, PS_LIST_HEAD, NULL);
    item = NULL; // Item from iterator
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (strstr(item->comment, "require") == NULL) {
            continue;
        }
        if (strstr(item->comment, "(found)") != NULL) {
            continue;
        }

        switch (item->type) {
            case PS_DATA_BOOL:
                psError(PS_ERR_UNKNOWN, false, "boolean type can not be required");
                fprintf(stderr, "boolean type can not be required\n");
                psFree(iter);
                return NULL;
                break;
            case PS_DATA_S8:
                if (item->data.S8 != INT8_MAX) {
                    break;
                }
                goto ARG_REQUIRED;
            case PS_DATA_S16:
                if (item->data.S16 != INT16_MAX) {
                    break;
                }
                goto ARG_REQUIRED;
            case PS_DATA_S32:
                if (item->data.S32 != INT32_MAX) {
                    break;
                }
                goto ARG_REQUIRED;
            case PS_DATA_S64:
                if (item->data.S64 != INT64_MAX) {
                    break;
                }
                goto ARG_REQUIRED;
            case PS_DATA_U8:
                if (item->data.U8 != UINT8_MAX) {
                    break;
                }
                goto ARG_REQUIRED;
            case PS_DATA_U16:
                if (item->data.U16 != UINT16_MAX) {
                    break;
                }
                goto ARG_REQUIRED;
            case PS_DATA_U32:
                if (item->data.U32 != UINT32_MAX) {
                    break;
                }
                goto ARG_REQUIRED;
            case PS_DATA_U64:
                if (item->data.U64 != UINT64_MAX) {
                    break;
                }
                goto ARG_REQUIRED;
            case PS_DATA_F32:
            case PS_DATA_F64:
                if (!isnan(item->data.F64)) {
                    break;
                }
                goto ARG_REQUIRED;
            case PS_DATA_STRING:
            case PS_DATA_TIME:
            case PS_DATA_METADATA:
                if (item->data.V) {
                    break;
                }
                goto ARG_REQUIRED;
            default:
                psError(PS_ERR_UNKNOWN, false, "unknown argument type");
                fprintf(stream, "unknown argument type\n");
                psFree(iter);
                return NULL;
            ARG_REQUIRED:                
                psError(PS_ERR_UNKNOWN, false, "argument %s is required", item->name);
                fprintf(stream, "argument %s is required\n", item->name);
                pxUsage(stream, argc, argv, config->modeName, config->args);
                psFree(iter);
                return NULL;
        }

    }

    psFree(iter);

    // save argv/argc 
    config->argv = argv;
    config->argc = argc;

    return true;
}
