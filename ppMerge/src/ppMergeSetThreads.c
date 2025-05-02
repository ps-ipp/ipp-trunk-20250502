/** @file ppMergeSetThreads.c
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-01 21:43:05 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "ppMerge.h"

// "PPMERGE_READOUT_COMBINE", 5
bool ppMergeThread_pmReadoutCombine(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    pmReadout *output           = job->args->data[0];
    ppMergeFileGroup *fileGroup = job->args->data[1];
    psVector *zero              = job->args->data[2];
    psVector *scale             = job->args->data[3];
    pmCombineParams *params     = job->args->data[4];

    bool status = pmReadoutCombine (output, fileGroup->readouts, zero, scale, params);

    // after we are done, tell the I/O system that this file group is done
    fileGroup->busy = false;
    return status;
}

bool ppMergeThread_pmDarkCombine(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    pmCell *outCell             = job->args->data[0];
    ppMergeFileGroup *fileGroup = job->args->data[1];
    psScalar *iter              = job->args->data[2];
    psScalar *rej               = job->args->data[3];
    psScalar *maskVal           = job->args->data[4];

    bool status = pmDarkCombine(outCell, fileGroup->readouts, iter->data.S32, rej->data.F32, maskVal->data.PS_TYPE_IMAGE_MASK_DATA);

    // after we are done, tell the I/O system that this file group is done
    fileGroup->busy = false;
    return status;
}

bool ppMergeThread_pmShutterCorrectionGenerate(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    pmReadout *output             = job->args->data[0];
    pmReadout *pattern            = job->args->data[1];
    ppMergeFileGroup *fileGroup   = job->args->data[2];
    psScalar *shutterRef          = job->args->data[3];
    pmShutterCorrectionData *data = job->args->data[4];
    psScalar *iter                = job->args->data[5];
    psScalar *rej                 = job->args->data[6];
    psScalar *maskVal             = job->args->data[7];

    bool status = pmShutterCorrectionGenerate(output, pattern, fileGroup->readouts, shutterRef->data.F32, data, iter->data.S32, rej->data.F32, maskVal->data.PS_TYPE_IMAGE_MASK_DATA);

    // after we are done, tell the I/O system that this file group is done
    fileGroup->busy = false;
    return status;
}

bool ppMergeSetThreads(void)
{

    {
        psThreadTask *task = psThreadTaskAlloc("PPMERGE_READOUT_COMBINE", 5);
        task->function = &ppMergeThread_pmReadoutCombine;
        psThreadTaskAdd(task);
        psFree(task);
    }

    {
        psThreadTask *task = psThreadTaskAlloc("PPMERGE_DARK_COMBINE", 5);
        task->function = &ppMergeThread_pmDarkCombine;
        psThreadTaskAdd(task);
        psFree(task);
    }

    {
        psThreadTask *task = psThreadTaskAlloc("PPMERGE_SHUTTER_CORRECTION", 8);
        task->function = &ppMergeThread_pmShutterCorrectionGenerate;
        psThreadTaskAdd(task);
        psFree(task);
    }

    return true;
}
