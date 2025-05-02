/** @file pswarpSetThreads.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-05 20:44:04 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

/** each thread runs this function, starting a new job when it finished with an old one
 * it is called with a (void *) pointer to its own thread pointer
 */
bool pswarpThread_pswarpTransformTile(psThreadJob *job)
{
    pswarpTransformTileArgs *args = job->args->data[0];
    return pswarpTransformTile(args);
}

bool pswarpSetThreads(void)
{
    psThreadTask *task = psThreadTaskAlloc("PSWARP_TRANSFORM_TILE", 1);
    task->function = &pswarpThread_pswarpTransformTile;
    psThreadTaskAdd(task);
    psFree(task);

    return true;
}
