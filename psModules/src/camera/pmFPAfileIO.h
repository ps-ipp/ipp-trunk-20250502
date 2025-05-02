/* @file  pmFPAview.h
 * @brief Tools to manipulate the FPA structure elements.
 *
 * @author EAM, IfA
 * @author PAP, IfA
 *
 * @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-06-30 00:53:45 $
 * Copyright 2004-2005 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_FILE_IO_H
#define PM_FPA_FILE_IO_H

/// @addtogroup Camera Camera Layout
/// @{

// Determine appropriate file name
psString pmFPAfileName(const pmFPAfile *file, const pmFPAview *view, pmConfig *config);

// open the real file corresponding to the given pmFPAfile appropriate to the current view
bool pmFPAfileOpen (pmFPAfile *file, const pmFPAview *view, pmConfig *config);

// read from the real file corresponding to the given pmFPAfile for the current view
bool pmFPAfileRead (pmFPAfile *file, const pmFPAview *view, pmConfig *config);

bool pmFPAfileCreate (pmFPAfile *file, const pmFPAview *view, const pmConfig *config);

// write to the real file corresponding to the given pmFPAfile for the current view
bool pmFPAfileWrite (pmFPAfile *file, const pmFPAview *view, pmConfig *config);

// close the real file corresponding to the given pmFPAfile appropriate to the current view
bool pmFPAfileClose (pmFPAfile *file, const pmFPAview *view);

// free the data at this level
bool pmFPAfileFreeData(pmFPAfile *file, const pmFPAview *view);

// set the state of the specified pmFPAfile to active (state == true) or inactive
// if name is NULL, set the state for all pmFPAfiles
bool pmFPAfileActivate (psMetadata *files, bool state, const char *name);

/// Set the state of a single pmFPAfile (in the case of multiple files with the same name)
///
/// Returns file activated
pmFPAfile *pmFPAfileActivateSingle(psMetadata *files, ///< Files to activate
                                   bool state, ///< State to set
                                   const char *name, ///< Name of file to activate
                                   int num    ///< Sequence numbner of file to activate
    );

// examine all pmFPAfiles listed in the files and perform the needed I/O operations (open,read,write,close)
bool pmFPAfileIOChecks (pmConfig *config, const pmFPAview *view, pmFPAfilePlace place);

bool pmFPAfileWritePHU(pmFPAfile *file, const pmFPAview *view, pmConfig *config);
bool pmFPAfileReadPHU (pmFPAfile *file, const pmFPAview *view, pmConfig *config);

bool pmFPAfileIOList (pmConfig *config);

/// @}
# endif
