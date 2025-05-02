/** @file  psXML.c
*
*  @brief Contains basic XML definitions and operations
*
*  This file defines the basic type for an XML struct and functions useful
*  in creation and usage of XML documents/files.
*
*  @ingroup XML
*
*  @author David Robbins, MHPCC
*
*  @version $Revision: 1.49 $ $Name: not supported by cvs2svn $
*  @date $Date: 2008-11-05 11:12:40 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#include "psXML.h"
#include "psAssert.h"
#include <unistd.h>
#include <fcntl.h>

/******************************************************************************/
/*  DEFINE STATEMENTS                                                         */
/******************************************************************************/

/** Maximum size of a string */
#define MAXSTR 256
#define MAXVEC 2048
#define MAXBUF 10000


static void XMLFree(psXMLDoc *XML)
{
    xmlFreeDoc(*XML);
}

psXMLDoc *psXMLDocAlloc(void)
{
    psXMLDoc *XML;
    XML = psAlloc(sizeof(psXMLDoc));
    psMemSetDeallocator(XML, (psFreeFunc) XMLFree);
    return(XML);
}

psXMLDoc *psMetadataToXMLDoc(const psMetadata *md)
{
    PS_ASSERT_PTR_NON_NULL(md, NULL);

    psXMLDoc *XML = psXMLDocAlloc();
    xmlNode *cur_node = NULL;
    xmlNode *root = NULL;
    xmlAttr *prop = NULL;
    psDataType type;
    char content[MAXSTR];
    char vec[MAXVEC];
    char timeVal[MAXSTR];
    int i;
    psMetadataIterator *iter = psMetadataIteratorAlloc(md, PS_LIST_HEAD, NULL);
    psMetadataItem *item;

    //setup root element
    if( root == NULL ) {
        *XML = xmlNewDoc((const xmlChar*)"1.0");
        root = xmlNewDocNode(*XML, NULL, (const xmlChar*)"metadata", (const xmlChar*)"\n");
        xmlDocSetRootElement(*XML, root);
    }
    item = psMetadataGetAndIncrement(iter);
    while ( item != NULL ) {
        type = item->type;
        switch (type) {
        case PS_DATA_BOOL:
            cur_node = xmlNewChild(root, NULL, (const xmlChar*)"item", (const xmlChar*)"\n");
            prop = xmlNewProp(cur_node, (const xmlChar*)"name", (const xmlChar*)item->name);
            prop = xmlNewProp(cur_node, (const xmlChar*)"psType", (const xmlChar*)"BOOL");
            if ( item->data.B )
                strncpy(content, "TRUE", MAXSTR);
            else
                strncpy(content, "FALSE", MAXSTR);
            prop = xmlNewProp(cur_node, (const xmlChar*)"value", (const xmlChar*)content);
            break;
        case PS_DATA_S32:
            snprintf(content, MAXSTR, "%d", item->data.S32);
            cur_node = xmlNewChild(root, NULL, (const xmlChar*)"item", (const xmlChar*)"\n");
            prop = xmlNewProp(cur_node, (const xmlChar*)"name", (const xmlChar*)item->name);
            prop = xmlNewProp(cur_node, (const xmlChar*)"psType", (const xmlChar*)"S32");
            prop = xmlNewProp(cur_node, (const xmlChar*)"value", (const xmlChar*)content);
            break;
        case PS_DATA_F32:
            snprintf(content, MAXSTR, "%f", item->data.F32);
            cur_node = xmlNewChild(root, NULL, (const xmlChar*)"item", (const xmlChar*)"\n");
            prop = xmlNewProp(cur_node, (const xmlChar*)"name", (const xmlChar*)item->name);
            prop = xmlNewProp(cur_node, (const xmlChar*)"psType", (const xmlChar*)"F32");
            prop = xmlNewProp(cur_node, (const xmlChar*)"value", (const xmlChar*)content);
            break;
        case PS_DATA_F64:
            snprintf(content, MAXSTR, "%lf", item->data.F64);
            cur_node = xmlNewChild(root, NULL, (const xmlChar*)"item", (const xmlChar*)"\n");
            prop = xmlNewProp(cur_node, (const xmlChar*)"name", (const xmlChar*)item->name);
            prop = xmlNewProp(cur_node, (const xmlChar*)"psType", (const xmlChar*)"F64");
            prop = xmlNewProp(cur_node, (const xmlChar*)"value", (const xmlChar*)content);
            break;
        case PS_DATA_STRING:
            cur_node = xmlNewChild(root, NULL, (const xmlChar*)"item", (const xmlChar*)"\n");
            prop = xmlNewProp(cur_node, (const xmlChar*)"name", (const xmlChar*)item->name);
            prop = xmlNewProp(cur_node, (const xmlChar*)"psType", (const xmlChar*)"STR");
            strncpy(content, item->data.str, MAXSTR);
            prop = xmlNewProp(cur_node, (const xmlChar*)"value", (const xmlChar*)content);
            break;
        case PS_DATA_METADATA:
            i = 0;
            psXMLDoc *newDoc;
            xmlNode *new_root = NULL;
            xmlNode *new_node = NULL;
            newDoc = psMetadataToXMLDoc(item->data.md);
            new_root = xmlDocGetRootElement(*newDoc);
            prop = xmlNewProp(new_root, (const xmlChar*)"name", (const xmlChar*)item->name);
            prop = xmlNewProp(new_root, (const xmlChar*)"psType", (const xmlChar*)"METADATA");
            new_node = xmlAddSibling(cur_node, new_root);
            psFree(newDoc);
            break;
        case PS_DATA_TIME:
            cur_node = xmlNewChild(root, NULL, (const xmlChar*)"time", (const xmlChar*)"\n");
            prop = xmlNewProp(cur_node, (const xmlChar*)"name", (const xmlChar*)item->name);
            if ( ((psTime*)(item->data.V))->type == PS_TIME_UTC )
                prop = xmlNewProp(cur_node, (const xmlChar*)"psType", (const xmlChar*)"PS_TIME_UTC");
            else if ( ((psTime*)(item->data.V))->type == PS_TIME_TAI )
                prop = xmlNewProp(cur_node, (const xmlChar*)"psType", (const xmlChar*)"PS_TIME_TAI");
            else if ( ((psTime*)(item->data.V))->type == PS_TIME_UT1 )
                prop = xmlNewProp(cur_node, (const xmlChar*)"psType", (const xmlChar*)"PS_TIME_UT1");
            else if ( ((psTime*)(item->data.V))->type == PS_TIME_TT )
                prop = xmlNewProp(cur_node, (const xmlChar*)"psType", (const xmlChar*)"PS_TIME_TT");
            else {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                        _("Failed to recognize XML content.  Invalid syntax."));
                return NULL;
            }
            snprintf(content, MAXSTR, "%ld, ", (long)((psTime*)(item->data.V))->sec);
            strncpy(timeVal, content, MAXSTR);
            snprintf(content, MAXSTR, "%u, ", ((psTime*)(item->data.V))->nsec);
            strncat(timeVal, content, MAXSTR);
            if ( ((psTime*)(item->data.V))->leapsecond )
                strncat(timeVal, "T", MAXSTR);
            else
                strncat(timeVal, "F", MAXSTR);
            prop = xmlNewProp(cur_node, (const xmlChar*)"value", (const xmlChar*)timeVal);
            break;
        case PS_DATA_VECTOR:
            cur_node = xmlNewChild(root, NULL, (const xmlChar*)"vector", (const xmlChar*)"\n");
            prop = xmlNewProp(cur_node, (const xmlChar*)"name", (const xmlChar*)item->name);
            type = ((psVector*)(item->data.V))->type.type;
            strncpy(vec, "", MAXSTR);
            switch (type) {
            case PS_DATA_S32:
                prop = xmlNewProp(cur_node, (const xmlChar*)"psType", (const xmlChar*)"S32");
                for (i = 0; ((psVector*)(item->data.V))->data.S32[i] != 0; i++) {
                    snprintf(content, MAXSTR, "%d",
                             ((psVector*)(item->data.V))->data.S32[i]);
                    if ( ((psVector*)(item->data.V))->data.S32[i+1] != 0 ) {
                        strncat(content, ", ", 2);
                    }
                    strncat(vec, content, MAXSTR);
                }
                break;
            case PS_DATA_F32:
                prop = xmlNewProp(cur_node, (const xmlChar*)"psType", (const xmlChar*)"F32");
                for (i = 0; ((psVector*)(item->data.V))->data.F32[i] != 0; i++) {
                    snprintf(content, MAXSTR, "%f",
                             ((psVector*)(item->data.V))->data.F32[i]);
                    if ( ((psVector*)(item->data.V))->data.F32[i+1] != 0 ) {
                        strncat(content, ", ", 2);
                    }
                    strncat(vec, content, MAXSTR);
                }
                break;
            case PS_DATA_F64:
                prop = xmlNewProp(cur_node, (const xmlChar*)"psType", (const xmlChar*)"F64");
                for (i = 0; ((psVector*)(item->data.V))->data.F64[i] != 0; i++) {
                    snprintf(content, MAXSTR, "%lf",
                             ((psVector*)(item->data.V))->data.F64[i]);
                    if ( ((psVector*)(item->data.V))->data.F64[i+1] != 0 ) {
                        strncat(content, ", ", 2);
                    }
                    strncat(vec, content, MAXSTR);
                }
                break;
            case PS_DATA_UNKNOWN:
            default:
                psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                        _("Failed to recognize datatype from/for XML file."));
                psFree(XML);
                psFree(iter);
                return NULL;
            }
            prop = xmlNewProp(cur_node, (const xmlChar*)"value", (const xmlChar*)vec);
            break;
        case PS_DATA_UNKNOWN:
        default:
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Failed to recognize datatype from/for XML file."));
            psFree(XML);
            psFree(iter);
            return NULL;
        }
        item = psMetadataGetAndIncrement(iter);
    }
    //    xmlSaveFormatFile("xmlfname", *XML, 1);
    psFree(iter);
    return XML;
}

//compares strings and returns the correct data type//
static psDataType getType(const char *in)
{
    //Returns types S32, F32, F64, STRING, BOOL, or UNKNOWN on ERROR
    if ( !strncmp(in, "S32", MAXSTR) ) {
        return(PS_DATA_S32);
    } else if ( !strncmp(in, "F32", MAXSTR) ) {
        return(PS_DATA_F32);
    } else if ( !strncmp(in, "F64", MAXSTR) ) {
        return(PS_DATA_F64);
    } else if ( !strncmp(in, "STR", MAXSTR) ) {
        return(PS_DATA_STRING);
    } else if ( !strncmp(in, "BOOL", MAXSTR) ) {
        return(PS_DATA_BOOL);
    }

    return(PS_DATA_UNKNOWN);
}

static void storeValues(psVector *inVector, char *in, psDataType type)
{
    int i;
    char *endp;

    switch (type) {
    case PS_DATA_S32:
        i = 0;
        int intValue = 0;
        while (i < MAXSTR) {
            intValue = (int)strtol(in, &endp, 10);
            if ( intValue != 0 ) {
                inVector->data.S32[i] = intValue;
                i++;
                while (!strncmp(endp, ",", 1))
                    endp++;
                in = endp;
            } else
                i = MAXSTR;
        }
        break;
    case PS_DATA_F32:
        i = 0;
        float floatValue = 0.0;
        while (i < MAXSTR) {
            floatValue = strtof(in, &endp);
            if ( floatValue != 0.0 ) {
                inVector->data.F32[i] = floatValue;
                i++;
                while (!strncmp(endp, ",", 1))
                    endp++;
                in = endp;
            } else
                i = MAXSTR;
        }

        break;
    case PS_DATA_F64:
        i = 0;
        double doubleValue = 0.0;
        while (i < MAXSTR) {
            doubleValue = strtod(in, &endp);
            if ( doubleValue != 0.0 ) {
                inVector->data.F64[i] = doubleValue;
                i++;
                while (!strncmp(endp, ",", 1))
                    endp++;
                in = endp;
            } else
                i = MAXSTR;
        }
        break;
    default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Failed to recognize datatype from/for XML file."));
    }
}

static void storeTime(psTime *out, char *in)
{
    char *endp;
    long sec = 0;
    unsigned int nsec = 0;

    sec = strtol(in, &endp, 10);
    out->sec = sec;
    if ( !strncmp(endp, ",", 1) )
        endp++;
    else
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Failed to recognize XML content.  Invalid syntax."));
    in = endp;
    nsec = (unsigned int)strtol(in, &endp, 10);
    out->nsec = nsec;
    if ( !strncmp(endp, ",", 1) )
        endp++;
    else
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Failed to recognize XML content.  Invalid syntax."));
    in = endp;
    if ( !strncmp(in, "false", 10) || !strncmp(in, "f", 10) ||
            !strncmp(in, "FALSE", 10) || !strncmp(in, " F", 10) )
        out->leapsecond = false;
    else if ( !strncmp(in, "true", 10) || !strncmp(in, "t", 10) ||
              !strncmp(in, "TRUE", 10) || !strncmp(in, "T", 10) )
        out->leapsecond = true;
    else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Failed to recognize XML content.  Invalid syntax."));
    }
}

//Check the xml item for errors.  Returns the data type or PS_DATA_UNKNOWN for errors.//
static psDataType chkType(xmlNode *node)
{
    //NEED TO CHECK CONTENTS OF EACH ITEM/NODE //
    //if item then,
    if(!strncmp((const char*)node->name, "item", MAXSTR) || !strncmp((const char*)node->name, "ITEM", MAXSTR)) {
        if (node->properties != NULL && node->properties->name != NULL
                && node->properties->next != NULL && node->properties->next->name != NULL
                && node->properties->next->next != NULL
                && node->properties->next->next->name != NULL) {
            psDataType type = getType((const char*)node->properties->next->children->content);
            return type;
        } else {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Failed to recognize XML content.  Invalid syntax."));
            return(PS_DATA_UNKNOWN);
        }
    }
    //if vector then,
    if(!strncmp((const char*)node->name, "vector", MAXSTR) || !strncmp((const char*)node->name, "VECTOR", MAXSTR)) {
        if (node->properties != NULL && node->properties->name != NULL
                && node->properties->next != NULL && node->properties->next->name != NULL
                && node->properties->next->next != NULL
                && node->properties->next->next->name != NULL) {
            return(PS_DATA_VECTOR);
        } else {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Failed to recognize XML content.  Invalid syntax."));
            return(PS_DATA_UNKNOWN);
        }

    }
    //if metadata then,
    if(!strncmp((const char*)node->name, "metadata", MAXSTR)) {
        if (node->properties != NULL && node->properties->name
                && node->properties->next != NULL && node->properties->next->name != NULL) {
            return(PS_DATA_METADATA);
        } else {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Failed to recognize XML content.  Invalid syntax."));
            return(PS_DATA_UNKNOWN);
        }
    }
    if(!strncmp((const char*)node->name, "time", MAXSTR) || !strncmp((const char*)node->name, "TIME", MAXSTR) ) {
        if (node->properties != NULL && node->properties->name
                && node->properties->next != NULL && node->properties->next->name != NULL) {
            return(PS_DATA_TIME);
        } else {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Failed to recognize XML content.  Invalid syntax."));
            return(PS_DATA_UNKNOWN);
        }
    }
    return(PS_DATA_UNKNOWN);
}

//Sorts the xmlDocPtr data into a usable Metadata structure.  Transverses an xml tree.//
static psMetadata *xml2metadata(xmlNode *nodePtr, int nodeNum)
{
    psMetadata *meta = NULL;
    xmlNode *cur_node = NULL;
    char name[MAXSTR];
    char content[MAXSTR];
    //    char *name = NULL;
    //    char *content = NULL;
    psDataType type = PS_DATA_UNKNOWN;
    psDataType elemType = PS_DATA_UNKNOWN;

    meta = psMetadataAlloc();
    cur_node = nodePtr;

    //    for ( cur_node = nodePtr; cur_node != NULL; cur_node = cur_node->next )
    while (cur_node != NULL) {
        if ( cur_node->type == XML_ELEMENT_NODE ) {
            //if the root node isn't a metadata//
            if ( nodeNum == 0 && strncmp((const char*)cur_node->name, "metadata", MAXSTR) ) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                        _("Failed to recognize XML content.  Invalid syntax."));
                psFree(meta);
                return NULL;
            }

            if ( nodeNum != 0 ) {
                type = chkType(cur_node);
                //name = NULL;
                //content = NULL;
                switch ( type ) {
                case PS_DATA_S32:
                    strncpy(name, (char*)cur_node->properties->children->content, MAXSTR);
                    strncpy(content,
                            (char*)cur_node->properties->next->next->children->content, MAXSTR);
                    psMetadataAddS32(meta, PS_LIST_TAIL, name, 0, "",
                                     (int)strtol(content, NULL, 10));
                    break;
                case PS_DATA_F32:
                    strncpy(name, (const char*)cur_node->properties->children->content, MAXSTR);
                    strncpy(content,
                            (const char*)cur_node->properties->next->next->children->content, MAXSTR);
                    psMetadataAddF32(meta, PS_LIST_TAIL, name, 0, "", strtof(content, NULL));
                    break;
                case PS_DATA_F64:
                    strncpy(name, (const char*)cur_node->properties->children->content, MAXSTR);
                    strncpy(content,
                            (const char*)cur_node->properties->next->next->children->content, MAXSTR);
                    psMetadataAddF64(meta, PS_LIST_TAIL, name, 0, "", strtod(content, NULL));
                    break;
                case PS_DATA_STRING:
                    strncpy(name, (const char*)cur_node->properties->children->content, MAXSTR);
                    psMetadataAddStr(meta, PS_LIST_TAIL, name, 0, "",
                                     (const char*)cur_node->properties->next->next->children->content);
                    break;
                case PS_DATA_VECTOR:
                    strncpy(name, (const char*)cur_node->properties->children->content, MAXSTR);
                    strncpy(content,
                            (const char*)cur_node->properties->next->next->children->content, MAXSTR);
                    elemType = getType((const char*)cur_node->properties->next->children->content);
                    if (elemType == PS_DATA_UNKNOWN || elemType == PS_DATA_STRING
                            || elemType == PS_DATA_BOOL) {
                        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                                _("Failed to recognize datatype from/for XML file."));
                        psFree(meta);
                        return NULL;
                    }

                    psVector *vec = psVectorAlloc(strlen(content), elemType);
                    storeValues(vec, content, elemType);
                    psMetadataAddVector(meta, PS_LIST_TAIL, name, 0, "", vec);
                    psFree(vec);
                    break;
                case PS_DATA_TIME:
                    strncpy(name, (const char*)cur_node->properties->children->content, MAXSTR);
                    strncpy(content,
                            (const char*)cur_node->properties->next->next->children->content, MAXSTR);
                    psTimeType timeType;
                    char type[MAXSTR];
                    strncpy(type, (const char*)cur_node->properties->next->children->content, MAXSTR);
                    if ( !strncmp(type, "PS_TIME_UTC", MAXSTR) )
                        timeType = PS_TIME_UTC;
                    else if ( !strncmp(type, "PS_TIME_TAI", MAXSTR) )
                        timeType = PS_TIME_TAI;
                    else if ( !strncmp(type, "PS_TIME_UT1", MAXSTR) )
                        timeType = PS_TIME_UT1;
                    else if ( !strncmp(type, "PS_TIME_TT", MAXSTR) )
                        timeType = PS_TIME_TT;
                    else {
                        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                                _("Failed to recognize XML content.  Invalid syntax."));
                        psFree(meta);
                        return NULL;
                    }

                    psTime *time;
                    time = psTimeAlloc(timeType);
                    storeTime(time, content);
                    psMetadataAddTime(meta, PS_LIST_TAIL, name, 0, "", time);
                    psFree(time);
                    break;
                case PS_DATA_METADATA:
                    strncpy(name, (const char*)cur_node->properties->children->content, MAXSTR);
                    psMetadata *temp = NULL;
                    temp = xml2metadata(cur_node->children, nodeNum);
                    psMetadataAddMetadata(meta, PS_LIST_TAIL, name, 0, "", temp);
                    psFree(temp);
                    break;
                case PS_DATA_BOOL:
                    strncpy(name, (const char*)cur_node->properties->children->content, MAXSTR);
                    strncpy(content,
                            (const char*)cur_node->properties->next->next->children->content, MAXSTR);
                    if ( !strncmp(content, "true", MAXSTR) ||
                            !strncmp(content, "TRUE", MAXSTR) ||
                            !strncmp(content, "T", MAXSTR) ||
                            !strncmp(content, "t", MAXSTR) ) {
                        psMetadataAddBool(meta, PS_LIST_TAIL, name, 0, "", true);
                    } else if ( !strncmp(content, "false", MAXSTR) ||
                                !strncmp(content, "FALSE", MAXSTR) ||
                                !strncmp(content, "F", MAXSTR) ||
                                !strncmp(content, "f", MAXSTR) ) {
                        psMetadataAddBool(meta, PS_LIST_TAIL, name, 0, "", false);
                    } else {
                        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                                _("Failed to recognize XML content.  Invalid syntax."));
                        psFree(meta);
                        return NULL;
                    }
                    break;
                case PS_DATA_UNKNOWN:
                default:
                    psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                            _("Failed to recognize datatype from/for XML file."));
                    psFree(meta);   //XXX: Do I really need this?
                    return NULL;
                }
                nodeNum++;
            } else //if root node, then increment nodeNum and go to children//
            {
                nodeNum++;
                cur_node = cur_node->children;
            }
        }
        cur_node = cur_node->next;
    }
    psFree(cur_node);
    return meta;
}

psMetadata *psXMLDocToMetadata(const psXMLDoc *doc)
{
    PS_ASSERT_PTR_NON_NULL(doc, NULL);
    //    psMetadata *md = psMetadataAlloc();
    psMetadata *md = NULL;
    xmlNode *root_element = NULL;
    //XXX: doc changed to *doc
    root_element = xmlDocGetRootElement(*doc);
    md = xml2metadata(root_element, 0);
    return md;
}

psXMLDoc *psXMLParseFile(const char *filename)
{

    PS_ASSERT_PTR_NON_NULL(filename, NULL);
    psXMLDoc *XML;
    XML = psXMLDocAlloc();
    *XML = xmlReadFile(filename, NULL, 0);

    xmlCleanupParser();
    xmlMemoryDump();
    return XML;
}

bool psXMLDocToFile(const psXMLDoc *doc,
                    const char *filename)
{
    PS_ASSERT_PTR_NON_NULL(doc, 0);
    PS_ASSERT_PTR_NON_NULL(filename, 0);
    FILE *file;
    if( (file = fopen(filename, "w")) == NULL ) {
        psError(PS_ERR_IO, true, _("Failed to open file '%s'. Check if it exists and it has the proper permissions."), filename);
        return false;
    }

    xmlDocDump(file, *doc);
    fclose(file);
    return true;
}

psXMLDoc *psXMLParseMem(const char *buffer,
                        int size)
{
    PS_ASSERT_PTR_NON_NULL(buffer, NULL);
    PS_ASSERT_INT_NONZERO(size, NULL);
    char *URL = "new.xml";
    psXMLDoc *XML;
    XML = psXMLDocAlloc();
    *XML = xmlReadMemory(buffer, size, URL, NULL, 0);

    xmlCleanupParser();
    xmlMemoryDump();

    return XML;
}

bool psXMLDocToMem(const psXMLDoc *doc,
                   char *buffer)
{
    PS_ASSERT_PTR_NON_NULL(doc, 0);
    int bufferSize;
    xmlChar *buff;

    xmlDocDumpMemory(*doc, &buff, &bufferSize);
    if ( MAXVEC < strlen((char *)buff) ) {
        psError(PS_ERR_LOCATION_INVALID, true, _("Buffer to small to store XML doc."));
        //        xmlFree(buff);
        return false;
    }
    strncpy(buffer, (const char*)buff, MAXVEC);
    xmlFree(buff);

    return true;
}

psXMLDoc *psXMLParseFD(int fd)
{
    PS_ASSERT_INT_NONNEGATIVE(fd, NULL);
    char *URL = "new.xml";
    psXMLDoc *XML;
    XML = psXMLDocAlloc();
    *XML = xmlReadFd(fd, URL, NULL, 0);

    xmlCleanupParser();
    xmlMemoryDump();

    return XML;
}

bool psXMLDocToFD(const psXMLDoc *doc,
                  int fd)
{
    PS_ASSERT_PTR_NON_NULL(doc, 0);
    PS_ASSERT_INT_NONNEGATIVE(fd, 0);

    char buf[MAXBUF];
    int n;
    if ( psXMLDocToMem(doc, buf) ) {
        n = strlen(buf);
        if (write(fd, buf, n)) {;} //ignore return value
    } else {
        psError(PS_ERR_LOCATION_INVALID, true, _("Buffer to small to store XML doc."));
        return false;
    }
    return true;
}
