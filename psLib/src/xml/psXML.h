/* @file  psXML.h
 * @brief Contains basic XML definitions and operations
 *
 * This file defines the basic type for an XML struct and functions useful
 * in creation and usage of XML documents/files.
 *
 * @author David Robbins, MHPCC
 *
 * @version $Revision: 1.21 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-01-23 22:47:23 $
 * Copyright 2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_XML_H
#define PS_XML_H

/// @addtogroup FileIO Input/Output
/// @{

#include <libxml/parser.h>
//#include <libxml/tree.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "psMetadata.h"
#include "psMetadataConfig.h"
#include "psMemory.h"
#include "psError.h"
#include "psString.h"
#include "psConstants.h"
#include "psTime.h"

/** XML wrapper pointing to an XML document in memory */
typedef xmlDocPtr psXMLDoc;


/*****************************************************************************/

/* FUNCTION PROTOTYPES                                                       */

/*****************************************************************************/

/** Allocates a new psXMLDoc
 *
 *  @return psXMLDoc*         a new psXMLDoc object
*/
psXMLDoc *psXMLDocAlloc(void);

/** Converts a psMetadata data structure to a complete psXMLDoc (in memory)
 *
 *  @return psXMLDoc*           Pointer to XML document from read
*/
psXMLDoc *psMetadataToXMLDoc(
    const psMetadata *md               ///< psMetadata structure to read
);

/** Converts a complete psXMLDoc (in memory) to a psMetadata data structure
 *
 *  @return psMetadata*         Pointer to psMetadata data structure from read
*/
psMetadata *psXMLDocToMetadata(
    const psXMLDoc *doc                ///< psXMLDoc to read
);

/** Loads the data in a named file into a complete psXMLDoc (in memory)
 *
 *  @return psXMLDoc*       Pointer to resulting XML document from read
*/
psXMLDoc *psXMLParseFile(
    const char *filename               ///< Filename of file to be read
);

/** Writes out a complete psXMLDoc (in memory) to a named file
 *
 *  @return bool:       True on success or false otherwise.
*/
bool psXMLDocToFile(
    const psXMLDoc *doc,               ///< psXMLDoc to read
    const char *filename               ///< Filename of file to be written
);

/** Accepts a block of memory and parses it into a complete psXMLDoc (also in memory)
 *
 *  @return psXMLDoc*       Pointer to resulting XML document from read
*/
psXMLDoc *psXMLParseMem(
    const char *buffer,                ///< Buffer to read
    int size                           ///< Size of buffer
);

/** Accepts a complete psXMLDoc (in memory) and parses it into a block of memory
 *
 *  @return bool:       True on success or false otherwise.
*/
bool psXMLDocToMem(
    const psXMLDoc *doc,               ///< psXMLDoc to read
    char *buffer                       ///< Buffer to write
);

/** Reads from a file descriptor and converts the incoming data to a complete
 *  psXMLDoc (in memory)
 *
 *  @return psXMLDoc*      Pointer to resulting XML document from read
*/
psXMLDoc *psXMLParseFD(
    int fd                             ///< File descriptor to read
);

/** Reads from a complete psXMLDoc (in memory) and writes it to a file descriptor
 *
 *  @return bool:       True on success or false otherwise.
*/
bool psXMLDocToFD(
    const psXMLDoc *doc,               ///< psXMLDoc to read
    int fd                             ///< File descriptor to write
);

/// @}
#endif // #ifndef PS_XML_H
