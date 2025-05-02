/** @file  psMetadataConfig.h
 *
 *  @brief Contains metadata input/output functions.
 *
 *  This file defines functions to read and write metadata to/from an external file.
 *
 *  @author EAM, IfA
 *  @author PAP, IfA
 *  @author JH, IfA
 *  @author Ross Harman, MHPCC
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.26 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-01-23 22:47:23 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */
#ifndef PS_METADATACONFIG_H
#define PS_METADATACONFIG_H

#include "psMetadata.h"

/// @addtogroup DataContainer Data Containers
/// @{

/** Print metadata item to file.
 *
 *  Metadata items may be printed to an open file descriptor based on a
 *  provided format. The format is a sprintf format statement with exactly
 *  one % formatting command. If the metadata item type is a numeric type,
 *  this formatting command must also be numeric, and the type conversion
 *  performed to the value to match the format type. If the metadata type is
 *  a string, the formatting command must also be for a string. If the
 *  metadata type is any other data type, printing is not allowed.
 *  Currently, this function does not compress the output file
 *
 * @return psMetadataItem* :    Pointer metadata item.
 */
bool psMetadataItemPrint(
    FILE * fd,                         ///< Pointer to file to write metadata item.
    const char *format,                ///< Format to print metadata item.
    const psMetadataItem* item         ///< Metadata item to print.
);

/** Read metadata configuration file.
 *
 *  Loads pre-defined settings by parsing a configuration file into a psMetadata structure.
 *
 *  @return psMetadata* : Resulting metadata from read.
 */
psMetadata* psMetadataConfigRead(
    psMetadata* md,                    ///< Resulting metadata from read.
    unsigned int *nFail,               ///< Number of failed lines.
    const char *filename,              ///< Name of file to read.
    bool overwrite                     ///< Allow overwrite of duplicate specifications.
);

/** Parse metadata configuration string.
 *
 *  Loads pre-defined settings by parsing a string into a psMetadata structure.
 *
 *  @return psMetadata* : Resulting metadata from parse.
 */
psMetadata* psMetadataConfigParse(
    psMetadata* md,                    ///< Resulting metadata from read.
    unsigned int *nFail,               ///< Number of failed lines.
    const char *str,                   ///< String to process.
    bool overwrite                     ///< Allow overwrite of duplicate specifications.
);

/** Converts a psMetadata structure (including any nested psMetadata) into a
 *  configuration file formatted string.
 *
 *  A NULL shall be returned on error.
 *  @return psString:       a Configuration File formatted string.
 */
psString psMetadataConfigFormat(
    psMetadata *md                     ///< The metadata to convert
);

/** format metadata item.
 *
 *  Metadata Item is formatted to a string consistent with the MDC file formats
 *
 *  @return char:           allocated formatted string
*/
psString psMetadataItemFormat(psMetadataItem *item);

/** Converts a psMetadata structure (including any nested psMetadata) into a
 *  configuration file formatted string that is written out to filename.
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool psMetadataConfigWrite(
    psMetadata *md,                    ///< The metadata to convert
    const char *filename,	       ///< Name of file to write
    const char *compress	       ///< Output compression options 
);

/** Converts a psMetadata structure (including any nested psMetadata) into a
 *  configuration file formatted string that is written a file stream.
 *  Currently, this function does not compress the output file
 *
 *  @return bool:       True if successful, otherwise false.
 */
bool psMetadataConfigPrint(
    FILE *stream,                       ///< file stream to write to
    psMetadata *md                      ///< The metadata to convert
);

/// @}
#endif // #ifndef PS_METADATAIO_H
