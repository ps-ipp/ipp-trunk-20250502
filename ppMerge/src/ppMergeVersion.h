/** @file ppMergeVersion.h
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-01 21:43:05 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifndef PP_MERGE_VERSION_H
#define PP_MERGE_VERSION_H

/**
 * Return short version information
 */
psString ppMergeVersion(void);

/**
 * Return software source
 */
psString ppMergeSource(void);

/**
 * Return long version information
 */
psString ppMergeVersionLong(void);

/**
 * Update the metadata with version information for all dependencies
 */
void ppMergeVersionMetadata(psMetadata *metadata ///< Metadata to update with version information
    );

#endif
