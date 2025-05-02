/** @file ppMergeFileGroup.c
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-01 21:43:05 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "ppMerge.h"

static void mergeFileGroupFree(ppMergeFileGroup *group)
{
    psFree(group->readouts);
    return;
}

ppMergeFileGroup *ppMergeFileGroupAlloc(void)
{
    ppMergeFileGroup *group = psAlloc(sizeof(ppMergeFileGroup));
    psMemSetDeallocator(group, (psFreeFunc)mergeFileGroupFree);

    group->readouts = NULL;
    group->read = false;
    group->busy = false;
    return group;
}
