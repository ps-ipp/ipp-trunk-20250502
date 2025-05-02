/* @file  psPipe.h
 * @brief 3-stream pipe
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-01-24 02:54:15 $
 * Copyright 2004-2005 Institute for Astronomy, University of Hawaii
 */

#ifndef PS_PIPE_H
#define PS_PIPE_H

/// @addtogroup Extras Miscellaneous Funtions
/// @{

// move these to pslib??
typedef struct
{
    int pid;
    int fd_stdin;
    int fd_stdout;
    int fd_stderr;
}
psPipe;

// psPipe functions
psPipe *psPipeAlloc (void);
psPipe *psPipeOpen (char *command);
int     psPipeClose (psPipe *pipe);

/// @}
# endif
