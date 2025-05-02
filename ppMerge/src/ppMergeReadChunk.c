/** @file ppMergeReadChunk.c
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 02:44:31 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "ppMerge.h"

#define THREAD_WAIT 10000               ///< Microseconds to wait if thread is not available

ppMergeFileGroup *ppMergeReadChunk(bool *status, psArray *fileGroups, pmConfig *config, int numChunk)
{
    *status = true;

    bool mdok;
    bool haveMasks = psMetadataLookupBool(&mdok, config->arguments, "INPUTS.MASKS"); // Do we have masks?
    bool haveVariances = psMetadataLookupBool(&mdok, config->arguments,
                                              "INPUTS.VARIANCES");// Do we have variances?
    int rows = psMetadataLookupS32(NULL, config->arguments, "ROWS"); // Number of rows to read per chunk
    int rejectedInputs = 0;
    // select an available fileGroup
    while (1) {
        // check for any fileGroups which can read data
        for (int j = 0; j < fileGroups->n; j++) {
            ppMergeFileGroup *fileGroup = fileGroups->data[j];
            if (fileGroup->read) {
                continue;
            }

            // find max last scan so far
            int lastScan = 0;
            for (int i = 0; i < fileGroups->n; i++) {
                ppMergeFileGroup *fileGroup = fileGroups->data[i];
                lastScan = PS_MAX(fileGroup->lastScan, lastScan);
            }
            fileGroup->firstScan = lastScan;
            fileGroup->lastScan = lastScan + rows;

            psArray *readouts = fileGroup->readouts;

            psTimerStart ("ppMergeReadChunk");

            psTrace("ppStack", 2, "Reading data for chunk %d into fileGroup %d....n", numChunk, j);
            for (int i = 0; i < readouts->n; i++) {
                pmReadout *inRO = readouts->data[i]; ///< Input readout
		if (!inRO->process) continue;

                // override the recorded last scan
                inRO->thisImageScan  = fileGroup->firstScan;
                inRO->thisVarianceScan = fileGroup->firstScan;
                inRO->thisMaskScan   = fileGroup->firstScan;
                inRO->forceScan      = true;
                // inRO->process        = true;

		// char *cellname = psMetadataLookupStr(&mdok, inRO->parent->concepts, "CELL.NAME");
		// fprintf (stderr, "cell: %s, file %d, process: %d, image, mask, var: %lx, %lx, %lx -> ", 
		// 	 cellname, i, inRO->process, (long int) inRO->image, (long int) inRO->mask, (long int) inRO->variance);

                // Read a chunk from a file
                pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT", i);

		int zMax = 0;
                bool keepReading = false;
                if (pmReadoutMore(inRO, file->fits, 0, &zMax, rows, config)) {
		    // skip video cells
		    if (zMax > 1) {
			psWarning ("skipping video cell (1: %ld)", (long) pthread_self());
			inRO->process = false;
			rejectedInputs++;
			continue;
		    }
                    keepReading = true;
                    if (!pmReadoutReadChunk(inRO, file->fits, 0, &zMax, rows, 0, config)) {
                        psError(PS_ERR_IO, false, "Unable to read chunk %d for file PPMERGE.INPUT %d",
                                numChunk, i);
                        *status = false;
                        return NULL;
                    }
		    // skip video cells
		    if (zMax > 1) {
			psWarning ("skipping video cell (2: %ld)", (long) pthread_self());
			inRO->process = false;
			rejectedInputs++;
			continue;
		    }
                }
		// skip video cells
		// XXX this could be more efficient if we identified the cells to skip before calling the function...
		if (zMax > 1) {
		    psWarning ("skipping video cell (3: %ld)", (long) pthread_self());
		    inRO->process = false;
		    rejectedInputs++;
		    continue;
		}

                if (haveMasks && pmReadoutMoreMask(inRO, file->fits, 0, &zMax, rows, config)) {
                    keepReading = true;
                    pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT.MASK", i);
                    if (!pmReadoutReadChunkMask(inRO, file->fits, 0, &zMax, rows, 0, config)) {
                        psError(PS_ERR_IO, false, "Unable to read chunk %d for file PPMERGE.INPUT.MASK %d",
                                numChunk, i);
                        *status = false;
                        return NULL;
                    }
                }

                if (haveVariances && pmReadoutMoreVariance(inRO, file->fits, 0, &zMax, rows, config)) {
                    keepReading = true;
                    pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPMERGE.INPUT.VARIANCE", i);
                    if (!pmReadoutReadChunkVariance(inRO, file->fits, 0, &zMax, rows, 0, config)) {
                        psError(PS_ERR_IO, false,
                                "Unable to read chunk %d for file PPMERGE.INPUT.VARIANCE %d",
                                numChunk, i);
                        *status = false;
                        return NULL;
                    }
                }
		
		// fprintf (stderr, "%lx, %lx, %lx\n", (long int) inRO->image, (long int) inRO->mask, (long int) inRO->variance);
		if (!keepReading) {
		  return NULL;
		}
		
		
            }
	    psLogMsg("ppMerge",PS_LOG_INFO,"%d %ld %d %d %d",
		     j,readouts->n,rejectedInputs,fileGroup->read,fileGroup->busy);
	    if (rejectedInputs >= readouts->n) {
	      fileGroup->read = false;
	      fileGroup->busy = false;
	      return NULL;
	    }
	    
            fileGroup->read = fileGroup->busy = true;
            return fileGroup;
        }

        // Check for threads that are ready to read
        bool wait = true;
        for (int j = 0; j < fileGroups->n; j++) {
            ppMergeFileGroup *fileGroup = fileGroups->data[j];
            if (fileGroup->busy) {
                continue;
            }
            fileGroup->read = false;
            wait = false;
        }
        if (wait) {
            // No threads currently available
            usleep(THREAD_WAIT);
        }
    }
    return NULL;
}
