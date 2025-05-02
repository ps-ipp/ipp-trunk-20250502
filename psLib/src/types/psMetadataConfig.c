/** @file  psMetadataConfig.c
*
*  @brief Contains metadata input/output functions.
*
*  This file defines functions to read and write metadata to/from an external file.
*
*  @ingroup Metadata
*
*  @author Ross Harman, MHPCC
*  @author Eric Van Alst, MHPCC
*  @author Joshua Hoblitt, University of Hawaii 2006-2007
*
*  @version $Revision: 1.144 $ $Name: not supported by cvs2svn $
*  @date $Date: 2009-02-06 23:35:09 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <fitsio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <strings.h>
#include "psSlurp.h"

#include "psMemory.h"
#include "psMetadataConfig.h"
#include "psAssert.h"
#include "psTrace.h"

/******************************************************************************/
/*  DEFINE STATEMENTS                                                         */
/******************************************************************************/

/** Maximum size of a string */
#define MAX_STRING_LENGTH 1024
#define INITIAL_LENGTH 10               // Initial length for arrays

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

static psMetadata *genTypeTemplate(char *linePtr);
static psMetadata *parseTypeValues(psMetadata *template, char *linePtr)
;
static bool parseLine(psArray *levelArray,
                      char *linePtr,
                      bool overwrite,
                      bool *notBlank);
static bool parseMetadataItem(char *keyName, psArray *levelArray,
                              char *linePtr, psMetadataFlags flags);
static psArray *p_psMetadataKeyArray(psMetadata *md);
static bool parseGeneric(char *keyName,
                         psArray *levelArray,
                         char *linePtr,
                         psMetadataFlags flags);
static bool parseType(char *keyName,
                      psArray *levelArray,
                      char *linePtr,
                      psMetadataFlags flags);
static bool parseMetadataEnd(char *keyName,
                             psArray *levelArray,
                             char *linePtr,
                             psMetadataFlags flags);


// A metadata data structure used in parsing arrays.
// Contains array information and the metadata storage location.
typedef struct
{
    psArray *   nonUniqueKeyArray;      ///< non-unique key names
    psHash *    typeTemplates;          ///< hash of user type templates
    psMetadata *metadata;               ///< metadata container
}
p_psParseLevelInfo;

static void parseLevelInfoFree(p_psParseLevelInfo *info)
{
    psFree(info->nonUniqueKeyArray);
    psFree(info->typeTemplates);
    psFree(info->metadata);
}

static p_psParseLevelInfo *p_psParseLevelInfoAlloc(void)
{
    // Allocate memory for parse level info
    p_psParseLevelInfo *info = (p_psParseLevelInfo*)psAlloc(sizeof(p_psParseLevelInfo));
    psMemSetDeallocator(info,(psFreeFunc)parseLevelInfoFree);

    info->nonUniqueKeyArray = psArrayAllocEmpty(INITIAL_LENGTH);
    info->typeTemplates = psHashAlloc(10);
    info->metadata = NULL;

    return info;
}

// Determines if a line is blank (whitespace only) or a commentline. It returns true if so. The input string
// must be null terminated.
static bool ignoreLine(char *inString)
{
    while(*inString!='\0' && *inString!='#') {
        if(!isspace(*inString)) {
            return false;
        }
        inString++;
    }

    return true;
}


//  Removes leading and trailing whitespace and # characters from a string. The cleaned string is a new null
//  terminated copy of the original input string.
static char *cleanString(char *inString,
                         psS32 sLen,
                         bool ignoreComment)
{
    char *ptrB = NULL;
    char *ptrE = NULL;
    char *cleaned = NULL;

    // Initialize begining of string pointer
    ptrB = inString;

    // Skip over leading # or whitespace
    if(ignoreComment) {
        while (isspace(*ptrB) || *ptrB=='#') {
            ptrB++;
        }
    } else {
        while (isspace(*ptrB)) {
            ptrB++;
        }
    }

    // Skip over trailing whitespace, null terminators, and # characters
    ptrE = inString + sLen;
    if(ignoreComment) {
        while(isspace(*ptrE) || *ptrE=='\0' || *ptrE=='#') {
            ptrE--;
        }
    } else {
        while(isspace(*ptrE) || *ptrE=='\0') {
            ptrE--;
        }
    }

    // Length, sLen, does not include '\0'
    sLen = ptrE - ptrB + 1;

    // Adds '\0' to end of string and +1 to sLen
    if(sLen < 0 ) {
        cleaned = NULL;
    } else {
        cleaned = psStringNCopy(ptrB, sLen);
    }

    return cleaned;
}

// Returns cleaned token based on delimiter, but not including delimiter. Also changes the pointer location
// the beginning of the string. Tokens are newly allocated null terminated strings.
// XXX EAM : not sure this API is well-thought-out:
// *status must be set to 0 going in.
// status is 1 if delimeter is found, but no valid token
// returned string is NULL if no valid token is found
static char *getToken(char **inString,
                      char *delimiter,
                      psS32 *status,
                      bool ignoreComment)
{
    char *cleanToken = NULL;
    char *convertChar = NULL;
    psS32 sLen = 0;

    // Convert tab characters to white space
    while((convertChar=strchr(*inString,'\t')) != NULL ) {
        *convertChar = ' ';
    }

    // Skip over leading whitespace
    while(isspace(**inString)) {
        (*inString)++;
    }

    // Length of token, not including delimiter
    sLen = strcspn(*inString, delimiter);
    if(sLen) {

        // Create new, cleaned, and null terminated token
        cleanToken = cleanString(*inString, sLen,ignoreComment);

        // Move to end of token
        (*inString) += sLen;
    } else if(**inString!='\0' && sLen==0) {
        *status = 1;
    }

    return cleanToken;
}

// Returns single parsed value as a double precision number. The input string must be cleaned and null
// terminated.
static double parseDouble(char *inString,
                          psS32 *status)
{
    char *end = NULL;
    double value = 0.0;

    value = strtod(inString, &end);
    if(*end != '\0') {
        *status = 1;
    } else if(inString==end) {
        *status = 1;
    }

    return value;
}

// Returns single parsed value as a long signed int. The input string must be cleaned and null
// terminated.
static psS64 parseSignedInt(char *inString,
                            psS32 *status)
{
    char *end = NULL;
    psS64 value = 0;

    value = strtol(inString, &end, 0);
    if(*end != '\0') {
        *status = 1;
    } else if(inString==end) {
        *status = 1;
    }

    return value;
}

// Returns single parsed value as a double precision number. The input string must be cleaned and null
// terminated.
static psU64 parseUnsignedInt(char *inString,
                              psS32 *status)
{
    char *end = NULL;
    psU64 value = 0;

    value = strtoul(inString, &end, 0);
    if(*end != '\0') {
        *status = 1;
    } else if(inString==end) {
        *status = 1;
    }

    return value;
}

/** Returns true or false. 'T', 't', '1', 'F', 'f', and '0' are acceptable, parsable variations. */
static bool parseBool(char *inString,
                      psS32 *status)
{
    // if inString is NULL return flalse, status = 0
    if (!inString) {
        if (status) {
            *status = 0;
        }
        return false;
    }

    if (!strncasecmp(inString, "T", 2) ||
            !strncmp(inString, "1", 2) ||
            !strncasecmp(inString, "true", 5)) {
        if (status != NULL) {
            *status = 0;
        }
        return true;
    }
    if ( !strncasecmp(inString, "F", 2) ||
            !strncmp(inString, "0", 2) ||
            !strncasecmp(inString, "false", 6)) {
        if (status != NULL) {
            *status = 0;
        }
        return false;
    }

    if (status != NULL) {
        *status = 1;
    }
    return false;
}

/** Returns a psTime structure */
static psTime *parseTime(char *inString,
                         psTimeType tt,
                         psS32 *status)
{
    PS_ASSERT_PTR_NON_NULL(status, NULL);
    if (!inString) {
        *status = 0;
        return NULL;
    }

    // handle "NULL" as the time value
    if (strncmp(inString, "NULL", 5) == 0) {
        *status = 0;
        return NULL;
    }

    psTime *out = psTimeFromISO(inString, tt);
    if (!out) {
        *status = 1;
        return NULL;
    }

    *status = 0;

    return out;
}

/** Returns parsed vector filled with with data. The input string must be null terminated. */
static psVector *parseVector(char *inString,
                             psElemType elemType,
                             psS32 *status)
{
    PS_ASSERT_PTR_NON_NULL(inString, NULL);

    // split string into multiple values on " " and ","
    psList *tokens = psStringSplit(inString, ", ", false);

    // allocate a large enough vector to hold all of the tokens
    psVector *vec = psVectorAlloc(psListLength(tokens), elemType);

    // iterate through the list of tokens and concert them to numeric values
    // one by one
    for (long i = 0; i < psListLength(tokens); i ++) {
        psString tok = psListGet(tokens, i);
        char *ptr = NULL;
        double value = strtod(tok, &ptr);

        // ptr will be set to tok if the parse failed
        if (tok == ptr) {
            *status = 1;
            psError(PS_ERR_IO, true,
                    _("failed to parse %s as a vector element."), tok);
            psFree(vec);
            psFree(tokens);
            return NULL;
        }

        // XXX this switch statement and cases should be turned inside out but
        // this optimization isn't a priority
        switch(elemType) {
        case PS_TYPE_U8:
            vec->data.U8[i] = (psU8)value;
            break;
        case PS_TYPE_U16:
            vec->data.U16[i] = (psU16)value;
            break;
        case PS_TYPE_U32:
            vec->data.U32[i] = (psU32)value;
            break;
        case PS_TYPE_U64:
            vec->data.U64[i] = (psU64)value;
            break;
        case PS_TYPE_S8:
            vec->data.S8[i] = (psS8)value;
            break;
        case PS_TYPE_S16:
            vec->data.S16[i] = (psS16)value;
            break;
        case PS_TYPE_S32:
            vec->data.S32[i] = (psS32)value;
            break;
        case PS_TYPE_S64:
            vec->data.S64[i] = (psS64)value;
            break;
        case PS_TYPE_F32:
            vec->data.F32[i] = (psF32)value;
            break;
        case PS_TYPE_F64:
            vec->data.F64[i] = (psF64)value;
            break;
        default:
            *status = 1;
            psError(PS_ERR_BAD_PARAMETER_VALUE,true,
                    _("Specified type, %d, is not supported."), elemType);
            psFree(vec);
            psFree(tokens);
            return NULL;
            break;                          // unreachable
        }
    }

    psFree(tokens);

    return vec;
}


/*****************************************************************************/
/* FUNCTION IMPLEMENTATION - PUBLIC                                          */
/*****************************************************************************/

bool psMetadataItemPrint(FILE  *fd,
                         const char *format,
                         const psMetadataItem* item)
{
    psDataType type;
    bool success = true;

    PS_ASSERT_PTR_NON_NULL(fd, false);
    PS_ASSERT_STRING_NON_EMPTY(format, false);
    PS_ASSERT_METADATA_ITEM_NON_NULL(item, false);

    type = item->type;

    // determining the format type
    char *fType = strchr(format,'%');
    if (fType == NULL) {
        // well, the format contains no reference to the metadataItem's data:
        // that is truly trival to do!
        //        fprintf(fd,format);
        return false;
    }

    // skip over any format modifiers
    const char *formatEnd = format+strlen(format);
    while ( (fType < formatEnd) &&
        (strchr(" +-01234567890.$#, hlL",*(++fType)) != NULL) ) {}

    #define METADATAITEM_NUMERIC_CAST(FORMAT_TYPE) { \
        switch(type) { \
        case PS_DATA_BOOL: \
            fprintf(fd, format, (FORMAT_TYPE) item->data.B); \
            break; \
        case PS_DATA_S8: \
            fprintf(fd,format,(FORMAT_TYPE)  item->data.S8); \
            break; \
        case PS_DATA_S16: \
            fprintf(fd,format,(FORMAT_TYPE)  item->data.S16); \
            break; \
        case PS_DATA_S32: \
            fprintf(fd,format,(FORMAT_TYPE)  item->data.S32); \
            break; \
        case PS_DATA_S64: \
            fprintf(fd,format,(FORMAT_TYPE)  item->data.S64); \
            break; \
        case PS_DATA_U8: \
            fprintf(fd,format,(FORMAT_TYPE)  item->data.U8); \
            break; \
        case PS_DATA_U16: \
            fprintf(fd,format,(FORMAT_TYPE)  item->data.U16); \
            break; \
        case PS_DATA_U32: \
            fprintf(fd,format,(FORMAT_TYPE)  item->data.U32); \
            break; \
        case PS_DATA_U64: \
            fprintf(fd,format,(FORMAT_TYPE)  item->data.U64); \
            break; \
        case PS_DATA_F32: \
            fprintf(fd, format,(FORMAT_TYPE)  item->data.F32); \
            break; \
        case PS_DATA_F64: \
            fprintf(fd, format,(FORMAT_TYPE) item->data.F64); \
            break; \
        default: \
            psError(PS_ERR_BAD_PARAMETER_TYPE,true, \
                    _("Specified psDataType, %d, is not supported."), (int)type); \
            success = false; \
        } \
    }

    switch(*fType) {
    case 'd':
    case 'i':
    case 'c':
        METADATAITEM_NUMERIC_CAST(int)
        break;
    case 'o':
    case 'u':
    case 'x':
    case 'X':
        METADATAITEM_NUMERIC_CAST(unsigned int)
        break;
    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
    case 'a':
    case 'A':
        METADATAITEM_NUMERIC_CAST(double)
        break;
    case 's':
        if (type == PS_DATA_STRING) {
            fprintf(fd,format, item->data.str);
        } else {
            psError(PS_ERR_BAD_PARAMETER_TYPE,true,
                    _("Specified psDataType, %d, is not supported."), (int)type);
            success = false;
        }
        break;
    case 'p':
        fprintf(fd,format,item->data.V);
        break;
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Specified print format, %%%c, is not supported."), *fType);
        success = false;
        break;
    }

    return success;
}

static psMetadata *genTypeTemplate(char *linePtr)
{
    psMetadata*     metadataTemplate = NULL;
    psS32           status           = 0;
    char*           token            = NULL;
    psMetadataItem* tempItem         = NULL;

    // Loop through line and generate metadata items for each token found
    while ((token = getToken(&linePtr, " ", &status, false)) != NULL) {

        // If not allocated then allocate new metadata
        if (metadataTemplate == NULL) {
            metadataTemplate = psMetadataAlloc();
        }

        // Look for comment indicator #
        if (strstr(token, "#") != 0) {
            psFree(metadataTemplate);
            psFree(token);
            return NULL;
        }

        // Create metadata item to represent token read
        tempItem = psMetadataItemAllocStr(token, "", "");
        if (tempItem == NULL) {
            psFree(metadataTemplate);
            psFree(token);
            return NULL;
        }
        psFree(token);

        // Add item to template
        if (!psMetadataAddItem(metadataTemplate, tempItem, PS_LIST_TAIL, PS_META_DEFAULT)) {
            psFree(metadataTemplate);
            psFree(tempItem);
            return NULL;
        }
        psFree(tempItem);
    }

    return metadataTemplate;
}


static psMetadata *parseTypeValues(psMetadata *template,
                                   char *linePtr)
{
    psMetadata*      md           = NULL;
    char*            token        = NULL;
    psS32            status       = 0;
    psMetadataItem*  mdItem       = NULL;
    psMetadataItem*  templateItem = NULL;
    psListIterator*  iter         = NULL;

    // Allocate metadata to return
    md = psMetadataAlloc();

    // Determine the number of items in template
    long items = psListLength(template->list);

    if (items > 0 ) {
        // Point to first item in template
        iter = psListIteratorAlloc(template->list, PS_LIST_HEAD, true);
        // For each item in template parse line string for values
        for (long i = 0; i < items; i++) {
            // Get template item
            templateItem = psListGetAndIncrement(iter);
            if (templateItem == NULL) {
                psFree(md);
                psFree(iter);
                md = NULL;
                break;
            }

            // Get the next token on the line
            token = getToken(&linePtr, " ", &status, false);
            if (token != NULL) {
                // Allocate metadata item
                mdItem = psMetadataItemAllocStr(templateItem->name,
                                                templateItem->comment, token);
                if (mdItem == NULL) {
                    psFree(md);
                    md = NULL;
                    psFree(token);
                    psFree(iter);
                    break;
                }
                psFree(token);

                // Add item to metadata
                if (!psMetadataAddItem(md, mdItem, PS_LIST_TAIL, PS_META_DEFAULT)) {
                    psFree(md);
                    md = NULL;
                    psFree(mdItem);
                    psFree(iter);
                    break;
                }
                psFree(mdItem);
            } else {
                // Missing items
                psFree(md);
                psFree(iter);
                md = NULL;
                break;
            }
        }
    }

    return md;
}

bool parseMetadataItem(char *keyName,
                       psArray *levelArray,
                       char *linePtr,
                       psMetadataFlags flags)
{
    // XXX This function is a monstrous abomination.  I have been slowly
    // refactoring it as I fix bugs but it really needs to be split up into
    // managable bits.
    bool               returnValue   = true;
    bool               addStatus     = false;
    psDataType           mdType        = PS_DATA_UNKNOWN;
    psElemType           vectorType    = PS_TYPE_S8;
    char*                strValue      = NULL;
    char*                strComment    = NULL;
    psS32                status        = 0;
    psMetadata*          md            = NULL;
    psArray*             nonUniqueKeys = NULL;
    psMetadata*          tempMeta      = NULL;
    p_psParseLevelInfo*  nextLevelInfo = NULL;

    long level = psArrayLength(levelArray) - 1;

    // Get the metadata item type
    char *strType = getToken(&linePtr, " ", &status,true);

    // Check for no type
    if (strType == NULL) {
        psError(PS_ERR_IO, true, _("Failed to read a metadata type."));
        return false;
    }

    // Set metadata type based on type token
    // Check if the keyName specifies a vector and if so use strType token to
    // find vector type
    if (*keyName == '@') {
        mdType = PS_DATA_VECTOR;
        // Get the type of vector
        if(!strncmp(strType, "U8", 2)) {
            vectorType = PS_TYPE_U8;
        } else if (!strncmp(strType,"U16",3)) {
            vectorType = PS_TYPE_U16;
        } else if (!strncmp(strType,"U32",3)) {
            vectorType = PS_TYPE_U32;
        } else if (!strncmp(strType,"U64",3)) {
            vectorType = PS_TYPE_U64;
        } else if (!strncmp(strType,"S8",2)) {
            vectorType = PS_TYPE_S8;
        } else if (!strncmp(strType,"S16",3)) {
            vectorType = PS_TYPE_S16;
        } else if (!strncmp(strType,"S32",3)) {
            vectorType = PS_TYPE_S32;
        } else if (!strncmp(strType,"S64",3)) {
            vectorType = PS_TYPE_S64;
        } else if (!strncmp(strType,"F32",3)) {
            vectorType = PS_TYPE_F32;
        } else if (!strncmp(strType,"F64",3)) {
            vectorType = PS_TYPE_F64;
        } else {
            psError(PS_ERR_IO, true,
                    _("Failed to parse the value '%s' of metadata item %s, type %s, "),
                      "", keyName,  strType);
            psFree(strType);
            return false;
        }
    } else if(!strncmp(strType, "STR", 3)) {
        mdType = PS_DATA_STRING;
    } else if(!strncmp(strType, "BOOL", 4)) {
        mdType = PS_DATA_BOOL;
    } else if(!strncmp(strType, "S8", 2)) {
        mdType = PS_DATA_S8;
    } else if(!strncmp(strType, "S16", 3)) {
        mdType = PS_DATA_S16;
    } else if(!strncmp(strType, "S32", 3)) {
        mdType = PS_DATA_S32;
    } else if(!strncmp(strType, "S64", 3)) {
        mdType = PS_DATA_S64;
    } else if(!strncmp(strType, "U8", 2)) {
        mdType = PS_DATA_U8;
    } else if(!strncmp(strType, "U16", 3)) {
        mdType = PS_DATA_U16;
    } else if(!strncmp(strType, "U32", 3)) {
        mdType = PS_DATA_U32;
    } else if(!strncmp(strType, "U64", 3)) {
        mdType = PS_DATA_U64;
    } else if(!strncmp(strType, "F32", 3)) {
        mdType = PS_DATA_F32;
    } else if(!strncmp(strType, "F64", 3)) {
        mdType = PS_DATA_F64;
    } else if(!strncmp(strType, "MULTI", 5)) {
        mdType = PS_DATA_METADATA_MULTI;
    } else if(!strncmp(strType, "METADATA", 8)) {
        mdType = PS_DATA_METADATA;
    } else if( !strncmp(strType, "UTC", 3) || !strncmp(strType, "TAI", 3)
               || !strncmp(strType, "UT1", 3) || !strncmp(strType, "TT", 3)) {
        mdType = PS_DATA_TIME;
    } else {
        // Search through user types
        // Check if type already exists in typeTempaltes
        psHash *typeTemplates = ((p_psParseLevelInfo*)(levelArray->data[level]))->typeTemplates;

        psMetadata *template = psHashLookup(typeTemplates, strType);
        if (!template) {
            psError(PS_ERR_IO, true,
                    _("Metadata TYPE '%s' is invalid."), strType);
            psFree(strType);
            return false;
        }

        // covert the template, and the rest of the line, into a metadata
        tempMeta = parseTypeValues(template, linePtr);
        if (!tempMeta) {
            // Metadata type read error
            psError(PS_ERR_IO,true,
                    _("Failed to read Metadata TYPE %s."), keyName);
            psFree(strType);
            return false;
        }

        // get the current level's metadata
        md = ((p_psParseLevelInfo*)(levelArray->data[level]))->metadata;

        // add the parsed TYPE to it
        addStatus = psMetadataAdd(md,PS_LIST_TAIL,keyName,
                PS_DATA_METADATA | flags,"",tempMeta);
        psFree(tempMeta);

        // Check for add failure
        if (! addStatus) {
            psError(PS_ERR_IO, true,
                    _("Duplicate Metadata item, %s, found.  "
                     "Overwrite not allowed."), keyName);
            psFree(strType);
            return false;
        }

        psFree(strComment);
        psFree(strValue);
        psFree(strType);

        return true;
    }

    // If type is MULTI or META then check for the (optional) directives UPDATE or RESET;
    // otherwise, get the value and comment.
    // line may have the following forms:
    // NAME METADATA
    // NAME METADATA # Comment
    // NAME METADATA#Comment
    // NAME METADATA UPDATE # Comment
    // NAME METADATA RESET # Comment
    // NAME METADATA UPDATE
    // NAME METADATA RESET
    if((mdType == PS_DATA_METADATA_MULTI) || (mdType == PS_DATA_METADATA)) {

        // Get the metadata directive if there is one.
        status = 0;
        strValue = getToken (&linePtr, "#", &status, true);

        if (!status && strValue) {
            // found a directive, what does it say?
            if (strcasecmp (strValue, "UPDATE") && strcasecmp (strValue, "RESET")) {
                psError(PS_ERR_IO, true, _("Invalid directive %s for METADATA or MULTI."), strValue);
                psFree(strType);
                psFree(strValue);
                return false;
            }

            // found a directive, what does it say?
            if (!strcasecmp (strValue, "UPDATE")) {
                // this folder or group is merged with an existing one of the same name
                flags |= PS_META_UPDATE_FOLDER;
            }
            if (!strcasecmp (strValue, "RESET")) {
                // this folder or group replaces an existing one of the same name
                flags |= PS_META_REPLACE;
            }
            psFree(strValue);
            strValue = NULL;
        }

        // Not all lines will have comments, so NULL is ok.
        status = 0;

        // XXX this is a very ugly way of finding from the current position to
        // the end of the line
        strComment = getToken(&linePtr, "", &status, true);

        if (status) {
            psError(PS_ERR_IO, true, _("Error reading metadata line"));
            psFree(strType);
            psFree(strComment);
            return false;
        }
    } else {
        // Get the metadata item value if there is one.
        status = 0;
        strValue = getToken(&linePtr, "#", &status,true);

        if(status) {
            psError(PS_ERR_IO, true, _("Failed to read a metadata value."));
            psFree(strType);
            psFree(strValue);
            return false;
        }

        if(strValue==NULL) {
            psError(PS_ERR_IO, true, _("Failed to read a metadata value."));
            psFree(strType);
            psFree(strValue);
            return false;
        }
        // Not all lines will have comments, so NULL is ok.
        status = 0;
        // XXX this is a very ugly way of finding from the current position to
        // the end of the line
        strComment = getToken(&linePtr, "", &status, true);
        if(status) {
            psError(PS_ERR_IO, true, _("Failed to read a metadata comment"));
            psFree(strType);
            psFree(strValue);
            psFree(strComment);
            return false;
        }
    }

#define PARSE_ADD_CASE(NAME, TYPE, PARSEFUNC) \
  case PS_DATA_##NAME: { \
      ps##TYPE value = PARSEFUNC(strValue, &status); \
      if (!status) { \
          addStatus = psMetadataAdd##TYPE(md, PS_LIST_TAIL, keyName, flags, strComment, value); \
      } else { \
          psError(PS_ERR_IO, true, \
                  _("Failed to parse the value '%s' of metadata item %s, type %s."), \
                  strValue, keyName, strType); \
          returnValue = false; \
      } \
      break; \
  }

    // Need to add item to metadata so get pointer to metadata
    status = 0;
    md = ((p_psParseLevelInfo*)(levelArray->data[level]))->metadata;
    nonUniqueKeys = ((p_psParseLevelInfo*)(levelArray->data[level]))->nonUniqueKeyArray;
    switch(mdType) {
        PARSE_ADD_CASE(BOOL,   Bool,   parseBool);
        PARSE_ADD_CASE(F32,    F32,    parseDouble);
        PARSE_ADD_CASE(F64,    F64,    parseDouble);
        PARSE_ADD_CASE(S8,     S8,     parseSignedInt);
        PARSE_ADD_CASE(S16,    S16,    parseSignedInt);
        PARSE_ADD_CASE(S32,    S32,    parseSignedInt);
        PARSE_ADD_CASE(S64,    S64,    parseSignedInt);
        PARSE_ADD_CASE(U8,     U8,     parseUnsignedInt);
        PARSE_ADD_CASE(U16,    U16,    parseUnsignedInt);
        PARSE_ADD_CASE(U32,    U32,    parseUnsignedInt);
        PARSE_ADD_CASE(U64,    U64,    parseUnsignedInt);
      case PS_DATA_STRING:
        // map "NULL" strings to NULL
        if (strcasecmp(strValue, "null") == 0) {
            addStatus = psMetadataAddStr(md, PS_LIST_TAIL, keyName, flags, strComment, NULL);
        } else {
            addStatus = psMetadataAddStr(md, PS_LIST_TAIL, keyName, flags, strComment, strValue);
        }
        break;
      case PS_DATA_TIME: {
          psTimeType timeType = PS_TIME_TAI;
          if(!strncmp(strType, "UTC", 3)) {
              timeType = PS_TIME_UTC;
          } else if(!strncmp(strType, "TAI", 3)) {
              timeType = PS_TIME_TAI;
          } else if(!strncmp(strType, "UT1", 3)) {
              timeType = PS_TIME_UT1;
          } else if(!strncmp(strType, "TT", 3)) {
              timeType = PS_TIME_TT;
          }

          psTime *mTime = parseTime(strValue, timeType, &status);
          if(!status) {
              addStatus = psMetadataAdd(md, PS_LIST_TAIL, keyName,
                                        mdType | flags,
                                        strComment, mTime);
          } else {
              psError(PS_ERR_IO, true,
                      _("Failed to parse the value '%s' of metadata item %s, type %s."),
                      strValue, keyName, strType);
              returnValue = false;
          }
          psFree(mTime);
          break;
      }
      case PS_DATA_VECTOR: {
          psVector *tempVec = parseVector(strValue, vectorType, &status);
          if(!status) {
              addStatus = psMetadataAdd(md, PS_LIST_TAIL, keyName+1,
                                        mdType | flags,
                                        strComment, tempVec);
          } else {
              psError(PS_ERR_IO, true,
                      _("Failed to parse the value '%s' of metadata item %s, type %s."),
                      strValue, keyName, strType);
              returnValue = false;
          }
          psFree(tempVec);
          break;
      }
    case PS_DATA_METADATA_MULTI:
        // Add key to non-unique array of keys
        // Check for duplicate MULTI lines

      // XXX currently, we only place the name of the MULTI on this list.  we thus lose
      // the associated comment (if any) and the flags (if any) we could probably fix this
      // behavior by allowing psMetadataAddItem to be passed an empty MULTI, which would
      // have a null data pointer until an element is added (in other words, treat MULTI
      // as another type of folder, but without a link of its own on the metadata->list

        addStatus = true;
        for(psS32 k=0; k < nonUniqueKeys->n; k++) {
            if(strcmp(keyName,(char*)nonUniqueKeys->data[k]) == 0) {
                psError(PS_ERR_IO,true,_("Duplicate MULTI specifier."));
                psFree(strType);
                return false;
            }
        }
        psString tempStr = psStringCopy(keyName);
        nonUniqueKeys = psArrayAdd(nonUniqueKeys,0,tempStr);
        addStatus = true;
        psFree(tempStr);
        break;
    case PS_DATA_METADATA: {
            // check to see if this keyname already exists and is allowed as a
            // MULTI.  If we don't do this check first, it's possible that we
            // can create a new "scope" yet fail to add the new metdata.
            psMetadataItem *item = psMetadataLookup (md, keyName);
            if ((item != NULL) &&
                    ((item->type != PS_DATA_METADATA_MULTI) &&
                    ((flags & PS_META_REPLACE) == 0))
            ) {
                psError(PS_ERR_IO, true, _("Duplicate Metadata declaration: %s is not allowed without 'overwrite' or MULTI specifier."), keyName);
                break;
            }

            // create the nested metadata
            psMetadata *newScope = psMetadataAlloc();

            // Create next level info
            nextLevelInfo = p_psParseLevelInfoAlloc();


            // try to add the new metdata to the current one
            addStatus = psMetadataAdd(md, PS_LIST_TAIL, keyName,
                                      mdType | flags,
                                      strComment, newScope);

            if (addStatus) {
                // switch to the scope to the new metadata
                nextLevelInfo->metadata = psMemIncrRefCounter(newScope);

                // Add next level to levelArray
                levelArray = psArrayAdd(levelArray, 0, nextLevelInfo);
            }

            psFree(nextLevelInfo);
            psFree(newScope);

            break;
        }
    default:
        psError(PS_ERR_IO, true,
                _("Metadata of unknown type found."));
        break;
    }

    // Check if the add status was successful
    if (! addStatus) {
        returnValue = false;
    }

    psFree(strComment);
    psFree(strValue);
    psFree(strType);

    return returnValue;
}

static bool parseLine(psArray *levelArray,
                      char *linePtr,
                      bool overwrite,
                      bool *notBlank)
{
    psMetadataFlags     flags  = PS_META_DEFAULT;

    // Set flags if overwrite specified
    if (overwrite) {
        flags = PS_META_REPLACE;
    }

    // If line is a comment or blank, then extract data
    if (ignoreLine(linePtr)) {
        // do nothing and return
        if (notBlank) {
            *notBlank = false;
        }
        return true;
    }

    // even if it's a "bad line" we know it can't be "blank" after this point
    *notBlank = true;

    // Get metadata item name
    psS32               status = 0;
    char *keyName = getToken(&linePtr, " ", &status, true);
    if (status) {
        psError(PS_ERR_IO, true, _("Failed to read item key name on line"));
        psFree(keyName);
        return false;
    }

    // Check for special keyName values "TYPE", "END"
    if (strcmp(keyName, "END") == 0) {
        if (!parseMetadataEnd(keyName, levelArray, linePtr, flags)) {
            goto FAIL;
        }
    } else if (strcmp(keyName,"TYPE") == 0 ) {
        if (!parseType(keyName, levelArray, linePtr, flags)) {
            goto FAIL;
        }
    } else {
        if (!parseGeneric(keyName, levelArray, linePtr, flags)) {
            goto FAIL;
        }
    }

    psFree(keyName);

    return true;

FAIL:
    psError(PS_ERR_UNKNOWN, false, _("Failed to parse line"));
    psFree(keyName);
    return false;
}

static bool parseGeneric(char *keyName,
                         psArray *levelArray,
                         char *linePtr,
                         psMetadataFlags flags)
{
    PS_ASSERT_STRING_NON_EMPTY(keyName, false);
    PS_ASSERT_ARRAY_NON_NULL(levelArray, false);
    PS_ASSERT_PTR_NON_NULL(linePtr, false);

    long level = psArrayLength(levelArray) - 1;

    // Check if key name present in array of non-unique key names
    psS32 limit = ((p_psParseLevelInfo*)(levelArray->data[level]))->nonUniqueKeyArray->n;
    for (psS32 k = 0; k < limit; k++) {
        char *name = (char*)((p_psParseLevelInfo*)
                             (levelArray->data[level]))->nonUniqueKeyArray->data[k];
        if (strcmp(name, keyName) == 0) {
            flags = PS_META_DUPLICATE_OK;
        }
    }

    // Parse metadataItem
    if (!parseMetadataItem(keyName, levelArray, linePtr, flags)) {
        return false;
    }

    return true;
}

static bool parseType(char *keyName,
                      psArray *levelArray,
                      char *linePtr,
                      psMetadataFlags flags)
{
    PS_ASSERT_STRING_NON_EMPTY(keyName, false);
    PS_ASSERT_ARRAY_NON_NULL(levelArray, false);
    PS_ASSERT_PTR_NON_NULL(linePtr, false);

    long level = psArrayLength(levelArray) - 1;

    // Get the type name
    psS32 status = 0;
    char *strType = getToken(&linePtr, " ", &status, true);
    if(!strType) {
        psError(PS_ERR_IO,true, _("Failed to read item type."));
        return false;
    }
    if (status) {
        psError(PS_ERR_IO, true, _("Failed to read item type."));
        psFree(strType);
        return false;
    }

    // Access typeTypes for this level/scope
    psHash *typeTemplates = ((p_psParseLevelInfo*)(levelArray->data[level]))->typeTemplates;

    // Check if type already exists in typeTempaltes
    if (psHashLookup(typeTemplates, strType)) {
        psError(PS_ERR_IO, true,
            _("Specified type %s is already defined."), strType);
        psFree(strType);
        return false;
    }

    // attempt to parse the TYPE line into a template metadata
    psMetadata *tempTemplate = genTypeTemplate(linePtr);
    if (!tempTemplate) {
        psError(PS_ERR_IO, true, _("Metadata type '%s' is invalid."), strType);
        psFree(strType);
        return false;
    }

    // Add key name to array of type
    // Add template to hash of templates
    if (!psHashAdd(typeTemplates, strType, tempTemplate)) {
        psError(PS_ERR_UNKNOWN, false, _("failed to add template for '%s' to hash"), strType);
        psFree(tempTemplate);
        psFree(strType);
        return false;
    }

    psFree(tempTemplate);
    psFree(strType);

    return true;
}

static bool parseMetadataEnd(char *keyName,
                             psArray *levelArray,
                             char *linePtr,
                             psMetadataFlags flags)
{
    PS_ASSERT_STRING_NON_EMPTY(keyName, false);
    PS_ASSERT_ARRAY_NON_NULL(levelArray, false);
    PS_ASSERT_PTR_NON_NULL(linePtr, false);

    long level = psArrayLength(levelArray) - 1;

    if ((level) < 1) {
        psError(PS_ERR_UNKNOWN, false, "can not END the top level metadata");
        return false;
    }

    // the END just moved us up one nesting level
    // Remove lower info level
    if(!psArrayRemoveIndex(levelArray, level)) {
        psError(PS_ERR_UNKNOWN, false, "failed to remove array item");
        return false;
    }

    return true;
}

psMetadata *psMetadataConfigRead(psMetadata *md,
                                 unsigned int *nFail,
                                 const char *filename,
                                 bool overwrite)
{
    // Check for NULL file name
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);

    psString file = psSlurpFilename(filename);
    if (!file) {
        psError(PS_ERR_IO, true, _("failed to read file '%s'"), filename);
        return NULL;
    }

    md = psMetadataConfigParse(md, nFail, (char *)file, overwrite);
    if (!md) {
        psError(PS_ERR_IO, true, _("failed to parse file '%s'"), filename);
        psFree(md);
        psFree(file);
        return NULL;
    }

    psFree(file);
    return md;
}

psMetadata* psMetadataConfigParse(psMetadata* md,
                                  unsigned int *nFail,
                                  const char *str,
                                  bool overwrite)
{
    bool allocedMD = false;

    PS_ASSERT_STRING_NON_EMPTY(str, NULL);

    // Initialise nFail, if provided
    if (nFail) {
        *nFail = 0;
    }

    // Allocate metadata if necessary
    if (md == NULL) {
        allocedMD = true;
        md = psMetadataAlloc();
    }

    // split the input string into an array of lines
    psList *doc = psStringSplit(str, "\n", true);
    if (!doc) {
        psError(PS_ERR_UNKNOWN, false, "failed to split string: %s", str);
        if (allocedMD) {
            psFree(md);
        }
        return NULL;
    }

    // accept completely empty strings
    // nFail == 0 / nPass == 0 - OK
    if (psListLength(doc) == 0) {
        psTrace("psLib.types", PS_LOG_INFO, "string contained no lines");
        psFree(doc);
        return md;
    }

    // Allocate array to store parse level information
    psArray *parseLevelInfoArray = psArrayAllocEmpty(INITIAL_LENGTH);

    // Set parse level info for the top level
    p_psParseLevelInfo *topLevelInfo = p_psParseLevelInfoAlloc();
    topLevelInfo->metadata = psMemIncrRefCounter(md);
    psArrayAdd(parseLevelInfoArray, 0, topLevelInfo);
    psFree(topLevelInfo);

    // clear the error stack so we can call psError(..., false, ...) and let
    // errors from the parse loop accumulate
    psErrorClear();

    // line type counts
    long nLines = 0;                // all lines
    long nGood  = 0;                // valid lines excluding blank/comment
    long nBad   = 0;                // invalid lines

    // While loop to parse the file
    char *line = NULL;
    psListIterator *iter = psListIteratorAlloc(doc, 0, false);
    while ((line = psListGetAndIncrement(iter))) {
        nLines++; // indexed from 1

        bool notBlank = false;
        if (!parseLine(parseLevelInfoArray, line, overwrite, &notBlank)) {
            nBad++;
            psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Error parsing line #%lu : %s", nLines, line);
        } else if (notBlank) {
        // do not count blank/comment lines as "good" lines
            nGood++;
        }
    }

    psFree(iter);
    psFree(doc);

    // Free parse array and line buffer
    psFree(parseLevelInfoArray);

    // nFail > 0 / nPass == 0 - NULL
    if ((nBad > 0) && (nGood == 0)) {
        psError(PS_ERR_UNKNOWN, false, "string contained no data lines and %ld bad lines", nBad);
        psFree(md);
        return NULL;
    }

    // pass back the number of failed lines
    if (nFail) {
        *nFail = nBad;
    }

    // nFail == 0 / nPass > 0
    // nFail > 0 / nPass > 0
    return md;
}

psString psMetadataConfigFormat(psMetadata *md)
{
    PS_ASSERT_METADATA_NON_NULL(md, NULL);

    psString format = psStringCopy("");

    psArray *keys = p_psMetadataKeyArray(md);
    for (long i = 0; i < psArrayLength(keys); i++) {
        psMetadataItem *item = psMetadataLookup(md, keys->data[i]);
        if (!item) {
            // XXX : this is probably not the right error value
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Specified psDataType, %d, is not supported."), PS_DATA_UNKNOWN);
            psTrace("psLib.types", 5, "failed to find key %s\n", (char *) keys->data[i]);
            psFree(keys);
            psFree(format);
            return NULL;
        }
        psString str = psMetadataItemFormat(item);
        if (!str) {
            psError(PS_ERR_UNKNOWN, false, "failed to format psMetadataItem");
            psTrace("psLib.types", 5, "failed to format %s\n", (char *) keys->data[i]);
            psFree(keys);
            psFree(format);
            psFree(str);
            return NULL;
        }
        psStringAppend(&format, "%s", str);
        psFree(str);
    }

    psFree(keys);

    return format;
}

// format a single metadata item for output (consistent with config dump I/O, includes return char)
psString psMetadataItemFormat(psMetadataItem *item)
{
    PS_ASSERT_METADATA_ITEM_NON_NULL(item, NULL);

    psString content = NULL;

    #define FORMAT_PRIMITIVE_METADATAITEM(type, dataformat) \
    psStringAppend(&content, "%-15s  %-8s  %-15" dataformat, \
                   item->name, #type, item->data.type); \
    if (item->comment && strncmp(item->comment, "", 2)) { \
        psStringAppend(&content, "  # %s", item->comment); \
    } \
    psStringAppend(&content, "\n");

    // In this block, the single item is used to build 'content'
    switch (item->type) {
    case PS_DATA_METADATA_MULTI: {
            psStringAppend(&content, "%s MULTI\n", item->name);

            // a MULTI is a list of items so we need to recurse through the
            // list
            psListIterator *iter = psListIteratorAlloc(item->data.list, 0, false);
            psMetadataItem *multiItem = NULL;
            while ((multiItem = psListGetAndIncrement(iter))) {
                psString str = psMetadataItemFormat(multiItem);
                psStringAppend(&content, "%s", str);
                psFree(str);
            }
            psFree(iter);
        }
        break;
    case PS_DATA_BOOL:
        psStringAppend (&content, "%-15s  %-8s  %-15s",
                        item->name, "BOOL", item->data.B ? "T" : "F");
        if (item->comment && strncmp(item->comment, "", 2)) {
            psStringAppend(&content, "  # %s", item->comment);
        }
        psStringAppend(&content, "\n");
        break;
    case PS_DATA_S8:
        FORMAT_PRIMITIVE_METADATAITEM(S8, "d");
        break;
    case PS_DATA_S16:
        FORMAT_PRIMITIVE_METADATAITEM(S16, "d");
        break;
    case PS_DATA_S32:
        FORMAT_PRIMITIVE_METADATAITEM(S32, "d");
        break;
    case PS_DATA_S64:
        FORMAT_PRIMITIVE_METADATAITEM(S64, PRId64);
        break;
    case PS_DATA_U8:
        FORMAT_PRIMITIVE_METADATAITEM(U8, "u");
        break;
    case PS_DATA_U16:
        FORMAT_PRIMITIVE_METADATAITEM(U16, "u");
        break;
    case PS_DATA_U32:
        FORMAT_PRIMITIVE_METADATAITEM(U32, "u");
        break;
    case PS_DATA_U64:
        FORMAT_PRIMITIVE_METADATAITEM(U64, PRIu64);
        break;
    case PS_DATA_F32:
        FORMAT_PRIMITIVE_METADATAITEM(F32, ".7g");
        break;
    case PS_DATA_F64:
        FORMAT_PRIMITIVE_METADATAITEM(F64, ".15g");
        break;
      case PS_DATA_STRING: {
          bool valid = false;
          if (item->data.str && strlen(item->data.str) > 0) {
              char *p = item->data.str;
              while (*p && isblank(*p)) p++;
              if (*p) valid = true;
          }
          if (valid) {
              psStringAppend(&content, "%-15s  %-8s  %-15s",
                             item->name, "STR", item->data.str);
          } else {
              psStringAppend(&content, "%-15s  %-8s  %-15s",
                             item->name, "STR", "NULL");
          }
          if (item->comment && strncmp(item->comment,"",2)) {
              psStringAppend(&content, "  # %s", item->comment);
          }
          psStringAppend(&content, " \n");
          break;
      }
    case PS_DATA_METADATA: {
            if (item->comment && strncmp(item->comment,"",2)) {
                psStringAppend(&content, "\n%s  METADATA  # %s", item->name, item->comment);
            } else {
                psStringAppend(&content, "\n%s  METADATA  ", item->name);
            }

            psMetadata *md = item->data.md; // Metadata at new level
            if (md) {
                psString newStr = psMetadataConfigFormat(item->data.md);
                if (!newStr) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to format metadata %s", item->name);
                    psFree(content);
                    return NULL;
                }

                // add 3 extra spaces to each metadata folder item
                char *buf = strtok(newStr, "\n");
                while (buf != NULL) {
                    psStringAppend(&content, "\n   %s", buf);
                    buf = strtok(NULL, "\n");
                }
                psFree(newStr);
            }
            psStringAppend(&content, "\nEND\n");
            break;
        }
    case PS_DATA_TIME:
        psStringAppend(&content, "%-15s  ", item->name);
        psTime *time = item->data.V;
        if (time) {
            switch (time->type) {
            case PS_TIME_UTC:
                psStringAppend(&content, "%-8s  ", "UTC");
                break;
            case PS_TIME_TAI:
                psStringAppend(&content, "%-8s  ", "TAI");
                break;
            case PS_TIME_UT1:
                psStringAppend(&content, "%-8s  ", "UT1");
                break;
            case PS_TIME_TT:
                psStringAppend(&content, "%-8s  ", "TT");
                break;
            default:
                psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                        _("Specified psTime type, %d, is not supported."),
                        time->type);
                psFree(content);
                return NULL;
            }
            psString timeStr = psTimeToISO(time);
            psStringAppend(&content, "%s", timeStr);
            psFree(timeStr);
        } else {
            // psTime is a NULL pointer
            psStringAppend(&content, "%-8s  %-15s", "TAI", "NULL");
        }

        if (item->comment && strncmp(item->comment,"",2)) {
            psStringAppend(&content, "  # %s", item->comment);
        }
        psStringAppend(&content, "\n");
        break;
    case PS_DATA_VECTOR:
        psStringAppend(&content, "@%s ", item->name);
        psVector *vector = item->data.V;

        switch (vector->type.type) {
        case PS_DATA_U8:
            psStringAppend(&content, "U8 ");
            for (int i = 0; i < vector->n; i++) {
                psStringAppend(&content, "%u ", vector->data.U8[i]);
            }
            break;
        case PS_DATA_U16:
            psStringAppend(&content, "U16 ");
            for (int i = 0; i < vector->n; i++) {
                psStringAppend(&content, "%u ", vector->data.U16[i]);
            }
            break;
        case PS_DATA_U32:
            psStringAppend(&content, "U32 ");
            for (int i = 0; i < vector->n; i++) {
                psStringAppend(&content, "%u ", vector->data.U32[i]);
            }
            break;
        case PS_DATA_U64:
            psStringAppend(&content, "U64 ");
            for (int i = 0; i < vector->n; i++) {
                psStringAppend(&content, "%" PRIu64 " ", vector->data.U64[i]);
            }
            break;
        case PS_DATA_S8:
            psStringAppend(&content, "S8 ");
            for (int i = 0; i < vector->n; i++) {
                psStringAppend(&content, "%d ", vector->data.S8[i]);
            }
            break;
        case PS_DATA_S16:
            psStringAppend(&content, "S16 ");
            for (int i = 0; i < vector->n; i++) {
                psStringAppend(&content, "%d ", vector->data.S16[i]);
            }
            break;
        case PS_DATA_S32:
            psStringAppend(&content, "S32 ");
            for (int i = 0; i < vector->n; i++) {
                psStringAppend(&content, "%d ", vector->data.S32[i]);
            }
            break;
        case PS_DATA_S64:
            psStringAppend(&content, "S64 ");
            for (int i = 0; i < vector->n; i++) {
                psStringAppend(&content, "%" PRId64 " ", vector->data.S64[i]);
            }
            break;
        case PS_DATA_F32:
            psStringAppend(&content, "F32 ");
            for (int i = 0; i < vector->n; i++) {
                psStringAppend(&content, "%.7g ", vector->data.F32[i]);
            }
            break;
        case PS_DATA_F64:
            psStringAppend(&content, "F64 ");
            for (int i = 0; i < vector->n; i++) {
                psStringAppend(&content, "%.15g ", vector->data.F64[i]);
            }
            break;
        default:
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Specified psDataType, %d, is not supported."), vector->type.type);
            psFree(content);
            return NULL;
        }
        if (item->comment && strncmp(item->comment, "", 2)) {
            psStringAppend(&content, "  # %s", item->comment);
        }
        psStringAppend(&content, "\n");
        break;
    case PS_DATA_UNKNOWN:
    default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified psDataType, %d, is not supported."), item->type);
        psFree(content);
        return NULL;
    }

    return content;
}

static psArray *p_psMetadataKeyArray(psMetadata *md)
{
    PS_ASSERT_METADATA_NON_NULL(md, NULL);

    psArray *keys = psArrayAllocEmpty(psListLength(md->list));

    // since we want to preserve the order of the keys in the metadata we can't
    // get a list of key names from the metadata's hash table.  Instead we have
    // to iterate through the metadata's list of items and reject duplicate key
    // names.

    psMetadataItem *item = NULL;
    psMetadataIterator *iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, NULL);
OUTSIDE:
    while ((item = psMetadataGetAndIncrement(iter))) {
        for (long i = 0; i < psArrayLength(keys); i++) {
            // does this element have a value
            psString elem = keys->data[i];
            if (elem) {
                if(strcmp(elem, item->name) == 0) {
                    goto OUTSIDE;
                }
            }
        }
        psString string = psStringCopy(item->name);
        psArrayAdd(keys, 0, string);
        psFree(string);
    }
    psFree(iter);
    return keys;
}

bool psMetadataConfigWrite(psMetadata *md, const char *filename, const char *compress)
{
  PS_ASSERT_METADATA_NON_NULL(md, NULL);
  PS_ASSERT_STRING_NON_EMPTY(filename, NULL);

  psString fileString = NULL;
  fileString = psMetadataConfigFormat(md);
  if (fileString == NULL) {
    psError(PS_ERR_BAD_PARAMETER_NULL, false, "psMetadataConfigFormat returned NULL.\n");
    return false;
  }

  if (compress) {
    if (strlen(compress) > 2) {
      psError(PS_ERR_BAD_PARAMETER_VALUE, true, "invalid compression options %s", compress);
      psFree(fileString);
      return false;
    }
    char modeString[4];
    snprintf (modeString, 4, "w%s", compress);

    gzFile file = gzopen(filename, modeString);
    if (file == Z_NULL) {
      psError(PS_ERR_IO, true, "Failed to open specified file, %s\n", filename);
      psFree(fileString);
      return false;
    }

    int nbytes = gzwrite (file, fileString, strlen(fileString));
    if (nbytes != strlen(fileString)) {
      psError(PS_ERR_IO, true, "Failed to write contents of configuration file %s", filename);
      psFree(fileString);
      gzclose(file);
      return false;
    }
    psFree(fileString);
    if (gzclose(file) != Z_OK) {
      psError(PS_ERR_IO, true, "Failed to close file, %s\n", filename);
      return false;
    }
  } else {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
      psError(PS_ERR_IO, true, "Failed to open specified file, %s\n", filename);
      psFree(fileString);
      return false;
    }

    int nbytes = fwrite(fileString, 1, strlen(fileString), file);
    if (nbytes != strlen(fileString)) {
      psError(PS_ERR_IO, true, "Failed to write contents of configuration file %s", filename);
      psFree(fileString);
      fclose(file);
      return false;
    }
    psFree(fileString);
    if (fclose(file) == EOF) {
      psError(PS_ERR_IO, true, "Failed to close file, %s\n", filename);
      return false;
    }
  }
  return true;
}

bool psMetadataConfigPrint(FILE *stream,
                           psMetadata *md)
{
    PS_ASSERT_METADATA_NON_NULL(md, false);
    PS_ASSERT_PTR_NON_NULL(stream, false);
    if (fprintf(stream, "\n") <= 0) {
        psError(PS_ERR_IO, false,
                "Unable to write to specified file.");
        return false;
    }

    psString str = psMetadataConfigFormat(md);
    if (!str) {
        psError(PS_ERR_UNKNOWN, false,
                _("failed to format metadata into a string."));
        return false;
    }

    fprintf(stream, "%s", str);
    psFree(str);

    return true;
}
