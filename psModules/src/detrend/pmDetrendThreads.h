/* @file pmDetrendThreads.h
 * @brief theading functions related to detrends
 * @author Eugene Magnier, IfA
 *
 * @version $Revision: 1.1 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-08-05 01:24:47 $
 * Copyright 2004-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_DETREND_THREADS_H
#define PM_DETREND_THREADS_H

/// @addtogroup detrend Detrend Creation and Application
/// @{

/// init the thread handler tasks for detrending
bool pmDetrendSetThreadTasks (int newScanRows);

/// get the requested number of scan rows per thread
int pmDetrendGetScanRows(void);

/// @}
#endif
