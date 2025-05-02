/* @file pmVisual.h
 * @brief functions to create visual diagnostics with the help of 'kapa'
 * @author Chris Beaumont, IfA
 *
 * Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_THREAD_TOOLS_H
#define PM_THREAD_TOOLS_H

psArray *pmReadoutAssignSourcesToCells (int Cx, int Cy, psArray *sources);
bool     pmReadoutChooseCellSizes (int *Cx, int *Cy, pmReadout *readout, int nThreads);

#endif //ndef PM_THREAD_TOOLS_H
