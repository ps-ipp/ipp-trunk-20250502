/** @file  psArguments.h
 *
 *  @brief Contains operations for parsing command line input arguments.
 *
 *  @ingroup Arguments
 *
 *  @author David Robbins, MHPCC
 *
 *  @version $Revision: 1.35 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-05-05 00:09:04 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>

#include "psArguments.h"
#include "fitsio.h"
#include "psMemory.h"
#include "psError.h"
#include "psAbort.h"

#include "psLogMsg.h"
#include "psTrace.h"
#include "psAssert.h"

#define NUM_SPACES 2   // Number of spaces between

// Set verbosity level
int psArgumentVerbosity(int *argc,
                        char **argv)
{
    int logLevel = psLogGetLevel();     // Current logging level
    PS_ASSERT_PTR_NON_NULL(argv, 2);
    PS_ASSERT_PTR_NON_NULL(argc, 2);
    PS_ASSERT_INT_POSITIVE(*argc, 2);
    int argnum = 0;   // Argument number

    // set in order, so that -vvv overrides -vv overrides -v
    if ( (argnum = psArgumentGet(*argc, argv, "-v")) ) {
        psArgumentRemove(argnum, argc, argv);
        logLevel = 3;
        psLogSetLevel(logLevel);
    }
    if ( (argnum = psArgumentGet(*argc, argv, "-vv")) ) {
        psArgumentRemove(argnum, argc, argv);
        logLevel = 4;
        psLogSetLevel(logLevel);
    }
    if ( (argnum = psArgumentGet(*argc, argv, "-vvv")) ) {
        psArgumentRemove(argnum, argc, argv);
        logLevel = 5;
        psLogSetLevel(logLevel);
    }

    if ( (argnum = psArgumentGet(*argc, argv, "-logfmt")) ) {
        if (*argc < argnum + 2) {
            psError(PS_ERR_IO, true, "-logfmt switch specified without a format.");
        } else {
            psArgumentRemove(argnum, argc, argv);
            psLogSetFormat(argv[argnum]); // XXX EAM : this function should return an error if the log format is invalid
            psArgumentRemove(argnum, argc, argv);
        }
    }

    // Now the trace stuff
    // argument format is: -trace (facil) (level)
    while ( (argnum = psArgumentGet(*argc, argv, "-trace")) ) {
        if ( (*argc < argnum + 3) ) {
            psError(PS_ERR_IO, true, "-trace switch specified without facility and level.");
            return logLevel;
        }
        psArgumentRemove(argnum, argc, argv);
        // psTraceSetLevel is cast to void to avoid a warning in the case where
        // PS_NO_TRACE is set and psTraceSetLevel is a macro returning an
        // untyped value
        (void)psTraceSetLevel(argv[argnum], atoi(argv[argnum+1])); // XXX: This function should return an error if the trace level is invalid
        psArgumentRemove(argnum, argc, argv);
        psArgumentRemove(argnum, argc, argv);
    }
    if ((argnum = psArgumentGet(*argc, argv, "-trace-levels"))) {
        psTracePrintLevels();
        return logLevel;
    }

    return logLevel;
}

// Find the location of the specified argument
int psArgumentGet(int argc,
                  char **argv,
                  const char *arg)
{
    PS_ASSERT_INT_POSITIVE(argc, 0);
    PS_ASSERT_PTR_NON_NULL(argv, 0);
    PS_ASSERT_STRING_NON_EMPTY(arg, 0);

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], arg))
            return i;
    }

    return 0;
}

// Remove the specified argument (by location)
bool psArgumentRemove(int argnum,
                      int *argc,
                      char **argv)
{
    PS_ASSERT_INT_POSITIVE(argnum, false);
    PS_ASSERT_PTR_NON_NULL(argc, false);
    PS_ASSERT_INT_LESS_THAN(argnum, *argc, false);
    PS_ASSERT_PTR_NON_NULL(argv, false);

    (*argc)--;
    for (int i = argnum; i < *argc; i++) {
        argv[i] = argv[i+1];
    }
    argv[*argc] = NULL;

    return true;
}


#define ARG_READ_CASE_INT(TYPE,FUNC) \
case PS_TYPE_##TYPE: { \
    char *end; \
    item->data.TYPE = FUNC(argv[argnum], &end, 0); \
    if (end == argv[argnum]) { \
        psError(PS_ERR_IO, true, "Unable to read argument value for %s", item->name); \
        return false; \
    } \
    return psArgumentRemove(argnum, argc, argv); \
}

#define ARG_READ_CASE_FLOAT(TYPE,FUNC) \
case PS_TYPE_##TYPE: { \
    char *end; \
    item->data.TYPE = FUNC(argv[argnum], &end); \
    if (end == argv[argnum]) { \
        psError(PS_ERR_IO, true, "Unable to read argument value for %s", item->name); \
        return false; \
    } \
    return psArgumentRemove(argnum, argc, argv); \
}

static bool argumentRead(psMetadataItem *item, // Item to read into
                         int argnum, // Argument number
                         int *argc, // Number of arguments in total
                         char **argv) // The arguments

{
    switch(item->type)
    {
        ARG_READ_CASE_INT(U8,strtol);
        ARG_READ_CASE_INT(U16,strtol);
        ARG_READ_CASE_INT(U32,strtol);
        ARG_READ_CASE_INT(U64,strtoll);
        ARG_READ_CASE_INT(S8,strtol);
        ARG_READ_CASE_INT(S16,strtol);
        ARG_READ_CASE_INT(S32,strtol);
        ARG_READ_CASE_INT(S64,strtoll);
        ARG_READ_CASE_FLOAT(F32,strtof);
        ARG_READ_CASE_FLOAT(F64,strtod);
    case PS_DATA_BOOL:
        // Turn option on; no optional argument to remove
        item->data.B = true;
        return true;
    case PS_DATA_STRING:
        psFree(item->data.V);
        item->data.V = psStringCopy(argv[argnum]);
        return psArgumentRemove(argnum, argc, argv);
    case PS_DATA_TIME:
    {
        psTime *time = psTimeFromString(argv[argnum], PS_TIME_UTC);
        if (!time) {
            psError(PS_ERR_IO, true, "Unable to read argument value for %s", item->name);
            return false;
        }
        item->data.V = time; 
        return psArgumentRemove(argnum, argc, argv);
    }
    default:
        psError(PS_ERR_IO, true, "Argument type (%x) is not supported --- argument %s (%s) ignored\n",
                item->type, item->name, item->comment);
        return false;
    }

    return true;
}


bool psArgumentParse(psMetadata *arguments,
                     int *argc,
                     char **argv)
{
    PS_ASSERT_METADATA_NON_NULL(arguments, false);
    PS_ASSERT_PTR_NON_NULL(argc, false);
    PS_ASSERT_INT_POSITIVE(*argc, false);
    PS_ASSERT_PTR_NON_NULL(argv, false);

    // We need to do a bit of mucking around in order to preserve the arguments metadata until the last
    // minute --- if there is a bad argument, we need to return the old "arguments", since they contain
    // the default values, which we probably want to output in a "help" message (we don't want to print
    // the changed values and have the user think that they are default values).
    psMetadata *oldArgs = psMetadataCopy(NULL, arguments); // Copy of old arguments, in event of an error
    psMetadata *multiFlag = psMetadataAlloc(); // Flag for which MULTI values have been read

    for (int i = 1; i < *argc; i++) {
        psTrace("psLib.types", 7, "Looking at %s\n", argv[i]);
        psMetadataItem *argItem = psMetadataLookup(arguments, argv[i]);
        if (argItem) {
	    psStringAppend (&argItem->comment, " (found)");
            psArgumentRemove(i, argc, argv); // Remove the switch

            switch (argItem->type) {
            case PS_DATA_METADATA: {
                    // -arg 1 2 3
                    psMetadata *params = argItem->data.V; // The list of parameters
                    if (*argc < i + params->list->n) {
                        psError(PS_ERR_IO, true, "Not enough arguments for %s.\n", argItem->name);
                        // Remove the arguments --- they will be ignored
                        for (int j = i; j < *argc; j++) {
                            psArgumentRemove(i, argc, argv);
                        }
                        goto failed;
                    }

                    psMetadataIterator *paramsIter = psMetadataIteratorAlloc(params,
                                                     PS_LIST_HEAD, NULL);// Iter
                    psMetadataItem *param = NULL; // Parameter from iteration
                    while ((param = psMetadataGetAndIncrement(paramsIter))) {
			if (!argumentRead(param, i, argc, argv)) {
			    psFree(paramsIter);
			    psError(PS_ERR_IO, false, "error parsing argument %s\n", argItem->name);
			    goto failed;
			}
                    }
                    psFree(paramsIter);
                    break;
                }
            case PS_DATA_METADATA_MULTI: {
                    // -arg 1 -arg 2 -arg 3 ....
                    psList *multi = argItem->data.V; // The MULTI list
                    psMetadataItem *template = psListGet(multi, PS_LIST_HEAD)
                                               ; // The template item
                    psMetadataItem *newItem = psMetadataItemCopy(template)
                                              ; // New item to add
                    if (!argumentRead(newItem, i, argc, argv)) {
			psFree(newItem);
			psError(PS_ERR_IO, false, "error parsing argument %s\n", argItem->name);
			goto failed;
		    }

                    psMetadataAddItem(arguments, newItem, PS_LIST_TAIL, PS_META_DUPLICATE_OK);
                    psFree(newItem);      // Drop reference

                    // Remove the template
                    bool search = false;  // Result of search
                    if (!psMetadataLookupBool(&search, multiFlag, argItem->name) || !search) {
                        // Chop the template off
                        psListRemove(multi, PS_LIST_HEAD); // Remove from MULTI (hash side)
                        psListRemoveData(arguments->list, template)
                        ; // Remove from list side
                        psMetadataAddBool(multiFlag, PS_LIST_HEAD, argItem->name, PS_META_REPLACE,
                                          NULL, true);
                    }
                    break;
                }
            default:
                // -arg 1
                if (argItem->type != PS_DATA_BOOL && *argc < i + 1) {
                    psError(PS_ERR_IO, true, "Required argument for %s is missing.\n", argItem->name);
                    goto failed;
                }
                if (!argumentRead(argItem, i, argc, argv)) {
		    psError(PS_ERR_IO, false, "error parsing argument %s\n", argItem->name);
		    goto failed;
		}
                break;
            }
            i--;                        // We removed stuff
        } else if (strncmp(argv[i], "-", 1) == 0 || strncmp(argv[i], "+", 1) == 0) {
            // Someone's specified a bad option
            psError(PS_ERR_IO, true, "Unknown option: %s\n", argv[i]);
            goto failed;
        }
    }

    psFree(multiFlag);
    psFree(oldArgs);                    // Didn't need these
    return true;

failed:
    // We need to copy everything back
    while (psListLength(arguments->list) > 0) {
        psMetadataRemoveIndex(arguments, PS_LIST_TAIL);
    }
    psMetadataCopy(arguments, oldArgs);
    psFree(multiFlag);
    psFree(oldArgs);
    return false;
}


#define LENGTH_CASE(TYPE) \
case PS_TYPE_##TYPE: \
return arg->data.TYPE == 0 ? 1 : \
       (arg->data.TYPE > 0 ? (int)log10f((float)arg->data.TYPE) + 1 : \
        (int)log10f(-(float)arg->data.TYPE) + 2);

static int argLength(psMetadataItem *arg)
{
    switch (arg->type) {
        // Only doing a representative set of types
        LENGTH_CASE(U8);
        LENGTH_CASE(U16);
        LENGTH_CASE(U32);
        LENGTH_CASE(U64);
        LENGTH_CASE(S8);
        LENGTH_CASE(S16);
        LENGTH_CASE(S32);
        LENGTH_CASE(S64);
    case PS_DATA_F32:
        return isnan(arg->data.F32) ? 3 : (arg->data.F32 >= 0 ? 12 : 13); // -d.dddddde?dd
    case PS_DATA_F64:
        return isnan(arg->data.F64) ? 3 : (arg->data.F64 >= 0 ? 12 : 13); // -d.dddddde?dd
    case PS_DATA_BOOL:
        return arg->data.B ? 4 : 5;
    case PS_DATA_STRING:
        return arg->data.V ? strlen(arg->data.V) : 0;
    case PS_DATA_TIME:
    {
        if (arg->data.V) {
            psString str = psTimeToISO(arg->data.V);
            int len = strlen(str);
            psFree(str);
            return len;
        }
        return 0;
    }
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                "Argument type (%x) is not supported.\n", arg->type);
        return 0;
        //        psAbort("Argument type (%x) is not supported.\n", arg->type);
    }

    return 0;
}


#define PRINT_CASE(TYPE,FORMAT) \
case PS_TYPE_##TYPE: \
printf(FORMAT, item->data.TYPE); \
break;

// Print the value and comment for an argument item
static void printValueComment(psMetadataItem *item, // Argument item for which to print
                              const char *comment, // Comment to print
                              unsigned int numSpaces) // Number of spaces

{
    int valueLength = 0;                // Length of the value portion
    printf("(");
    if (item) {
        // Print the value
        switch (item->type) {
            PRINT_CASE(U8, "%u");
            PRINT_CASE(U16, "%u");
            PRINT_CASE(U32, "%u");
            PRINT_CASE(U64, "%" PRIu64);
            PRINT_CASE(S8, "%d");
            PRINT_CASE(S16, "%d");
            PRINT_CASE(S32, "%d");
            PRINT_CASE(S64, "%" PRId64);
            PRINT_CASE(F32, "%.6e");
            PRINT_CASE(F64, "%.6e");
        case PS_DATA_BOOL:
            if (item->data.B) {
                printf("TRUE");
            } else {
                printf("FALSE");
            }
            break;
        case PS_DATA_STRING:
            if (item->data.V) {
                printf("%s", item->data.str);
            }
            break;
        default:
            //            psAbort("Argument type (%x) for %s is not supported.\n",
            //                    item->type, item->name);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    "Argument type (%x) for %s is not supported.\n", item->type, item->name);
        }
        valueLength = argLength(item);
    }
    printf(")");

    // Spaces for formatting
    for (int i = valueLength; i < numSpaces; i++) {
        printf(" ");
    }

    // Print the comment
    if (comment) {
        printf("%s", comment);
    }
    printf("\n");

    return;
}

// Return the maximum length of the values
static void maxLength(int *maxName,     // Maximum length of the name
                      int *maxValue,    // Maximum length of the value
                      psMetadata *md)    // Metadata for which to check the length

{
    psMetadataIterator *iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, NULL); // Iterator
    psMetadataItem *item = NULL;        // Item from iteration
    while ((item = psMetadataGetAndIncrement(iter)))
    {
        if (item->type == PS_DATA_METADATA) {
            maxLength(maxName, maxValue, item->data.V);
        } else {
            int nameLength = strlen(item->name); // Length of the name
            if (nameLength > *maxName) {
                *maxName = nameLength;
            }
            int valueLength = argLength(item); // Length of the value
            if (valueLength > *maxValue) {
                *maxValue = valueLength;
            }
        }
    }
    psFree(iter);
    return;
}


void psArgumentHelp(psMetadata *arguments)
{
    if (arguments == NULL)
        return;
    printf("Optional arguments, with default values:\n");
    int maxName = 4;   // Maximum length of a name
    int maxValue = 4;   // Maximum length of a value

    // First pass to get the sizes
    maxLength(&maxName, &maxValue, arguments);

    // Second pass to print
    psMetadataIterator *argIter = psMetadataIteratorAlloc(arguments, PS_LIST_HEAD, NULL);
    psMetadataItem *argItem = NULL; // Item from iterator
    while ((argItem = psMetadataGetAndIncrement(argIter))) {
        // Initial indent
        for (int i = 0; i < NUM_SPACES; i++) {
            printf(" ");
        }

        // Print the name if required
        printf("%s", argItem->name);
        int position = strlen(argItem->name); // Number of spaces in
        for (int i = position; i < maxName + NUM_SPACES; i++) {
            printf(" ");
        }

        // Check to see if it's a MULTI --- there are no default values for a MULTI
        // -arg 1 -arg 2 -arg 3 ...
        psMetadataItem *multiCheck = psMetadataLookup(arguments, argItem->name); // Item to check for MULTI
        if (multiCheck->type == PS_DATA_METADATA_MULTI) {
            bool first = true;          // Is this the first one?
            psListIterator* iter = psListIteratorAlloc(multiCheck->data.list,PS_LIST_HEAD,true);
            psMetadataItem* listItem;
            int count = 0;
            //            while ((listItem=(psMetadataItem*)psListGetAndIncrement(iter)) != NULL) {
            while ((listItem=(psMetadataItem*)psListGetAndIncrement(iter)) != NULL) {
                //                for (int i = 0; i < NUM_SPACES; i++) {
                //                    printf(" ");
                //                }
                //                printf("%s", listItem->name);
                //                int position = strlen(listItem->name); // Number of spaces in
                //                for (int i = position; i < maxName + NUM_SPACES; i++) {
                //                    printf(" ");
                //                }
                if (!first) {
                    for (int i = 0; i < maxName + 2*NUM_SPACES; i++) {
                        printf(" ");
                    }
                }
                printValueComment(listItem, listItem->comment, maxValue + NUM_SPACES);
                first = false;
                count++;
                //                continue;
            }
            psFree(iter);
            while (count > 1) {
                argItem = psMetadataGetAndIncrement(argIter);
                count--;
            }
            continue;
        }

        // -arg 1 2 3
        if (argItem->type == PS_DATA_METADATA) {
            psMetadata *params = argItem->data.V; // The list of parameters
            psMetadataIterator *paramsIter = psMetadataIteratorAlloc(params, PS_LIST_HEAD, NULL); // Iterator
            psMetadataItem *paramItem = NULL; // Parameter, from iteration
            bool first = true;          // Is this the first one?
            while ((paramItem = psMetadataGetAndIncrement(paramsIter))) {
                if (!first) {
                    for (int i = 0; i < maxName + 2*NUM_SPACES; i++) {
                        printf(" ");
                    }
                }
                printValueComment(paramItem, paramItem->comment, maxValue + NUM_SPACES);
                first = false;
            }
            psFree(paramsIter);
            continue;
        }

        // -arg 1
        printValueComment(argItem, argItem->comment, maxValue + NUM_SPACES);
    }

    psFree(argIter);
}

void psArgumentHelpSimple(FILE *stream, psMetadata *arguments)
{
    PS_ASSERT_PTR_NON_NULL(arguments, );

    int maxName = 4;   // Maximum length of a name
    int maxValue = 4;   // Maximum length of a value

    // First pass to get the sizes
    maxLength(&maxName, &maxValue, arguments);

    // Second pass to print
    psMetadataIterator *iter = psMetadataIteratorAlloc(arguments, PS_LIST_HEAD, NULL);
    psMetadataItem *item = NULL; // Item from iterator
    while ((item = psMetadataGetAndIncrement(iter))) {
        // Initial indent + name + indent + comment
        fprintf(stream, "%*s" "%-*s" "%*s" "%s\n",
                NUM_SPACES, " ",
                maxName, item->name,
                NUM_SPACES, " ",
                item->comment);
    }

    psFree(iter);
}

