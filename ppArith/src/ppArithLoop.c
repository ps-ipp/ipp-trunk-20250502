/** @file ppArithLoop.c
 *
 *  @brief
 *
 *  @ingroup ppArith
 *
 *  @author IfA
 *  @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 19:45:30 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include <ppStats.h>
#include <psphot.h>

#include "ppArith.h"

bool ppArithLoop(pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(config, false);

    bool mdok;                          // Status of MD lookup
    const char *statsName = psMetadataLookupStr(&mdok, config->arguments, "STATS"); // Filename for statistics
    psMetadata *stats = NULL;           // Container for statistics
    FILE *statsFile = NULL;             // File stream for statistics
    if (statsName && strlen(statsName) > 0) {
        psString resolved = pmConfigConvertFilename(statsName, config, true, true); // Resolved filename
        statsFile = fopen(resolved, "w");
        if (!statsFile) {
            psError(PS_ERR_IO, true, "Unable to open statistics file %s for writing.\n", resolved);
            psFree(resolved);
            return false;
        } else {
            stats = psMetadataAlloc();
        }
        psFree(resolved);
    }

    const char *outName = psMetadataLookupStr(NULL, config->arguments, "FILERULE.OUTPUT"); // Output filerule
    pmFPAfile *output = psMetadataLookupPtr(NULL, config->files, outName); // Output file
    assert(output);                     // We added it earlier

    const char *inName = psMetadataLookupStr(NULL, config->arguments, "FILERULE.INPUT"); ///< Input filerule
    pmFPAfile *input1 = NULL, *input2 = NULL; // Input files
    psString fileRegex = NULL;   // Regular expression to find input files
    psStringAppend(&fileRegex, "^%s$", inName);
    psMetadataIterator *iter = psMetadataIteratorAlloc(config->files, PS_LIST_HEAD, fileRegex); // Iterator
    psMetadataItem *item = psMetadataGetAndIncrement(iter); // Item from iteration
    input1 = item->data.V;
    assert(input1);                     // It should be there!
    if ((item = psMetadataGetAndIncrement(iter))) {
        input2 = item->data.V;
        assert(input2);
    }
    psFree(iter);

    float const2 = psMetadataLookupF32(&mdok, config->arguments, "PPARITH.CONST");

    pmFPAview *view = pmFPAviewAlloc(0); // Pointer into FPA hierarchy
    pmHDU *lastHDU = NULL;              // Last HDU that was updated

    // Iterate over the FPA hierarchy
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        return false;
    }

    pmChip *outChip;                    // Output chip of interest
    while ((outChip = pmFPAviewNextChip(view, output->fpa, 1)) != NULL) {
        pmChip *inChip1 = pmFPAviewThisChip(view, input1->fpa); // Input chip of interest
        pmChip *inChip2 = input2 ? pmFPAviewThisChip(view, input2->fpa) : NULL; // Input chip of interest
        if (inChip2 && ((!inChip1->file_exists && inChip2->file_exists) ||
                        (inChip1->file_exists && !inChip2->file_exists))) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "FPA format discrepency between inputs");
            psFree(view);
            return false;
        }

        if (!inChip1->file_exists) {
            continue;
        }

        if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
            return false;
        }

        pmCell *outCell;                // Cell of interest
        while ((outCell = pmFPAviewNextCell(view, output->fpa, 1)) != NULL) {
            pmCell *inCell1 = pmFPAviewThisCell(view, input1->fpa); // Input cell of interest
            pmCell *inCell2 = input2 ? pmFPAviewThisCell(view, input2->fpa) : NULL; // Input cell of interest
            if (inCell2 && ((!inCell1->file_exists && inCell2->file_exists) ||
                            (inCell1->file_exists && !inCell2->file_exists))) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "FPA format discrepency between inputs");
                psFree(view);
                return false;
            }
            if (!inCell1->file_exists) {
                continue;
            }
            if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                return false;
            }

            // Put version information into the header
            pmHDU *hdu = pmHDUFromCell(outCell);
            if (hdu && hdu != lastHDU) {
                if (!hdu->header) {
                    hdu->header = psMetadataAlloc();
                }
                ppArithVersionHeader(hdu->header);
                lastHDU = hdu;
            }

            pmReadout *outRO;           // Readout of interest
            while ((outRO = pmFPAviewNextReadout(view, output->fpa, 1))) {
                if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                    return false;
                }
                pmReadout *inRO1 = pmFPAviewThisReadout(view, input1->fpa);// Input readout of interest
                pmReadout *inRO2 = input2 ? pmFPAviewThisReadout(view, input2->fpa) :
                    NULL;// Input readout of interest

                if (inRO2 && ((!inRO1->data_exists && inRO2->data_exists) ||
                              (inRO1->data_exists && !inRO2->data_exists))) {
                    psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                            "FPA format discrepency between inputs");
                    psFree(view);
                    return false;
                }
                if (!inRO1->data_exists) {
                    continue;
                }

                // Perform the analysis
                if (!ppArithReadout(outRO, inRO1, inRO2, const2, config, view)) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to perform arithmetic.\n");
                    return false;
                }

                if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                    return false;
                }
            }

            // Perform statistics on the cell
            if (stats) {
                ppStatsFPA(stats, output->fpa, view, 0, config);
            }

            if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                return false;
            }
        }

        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            return false;
        }
    }

    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        return false;
    }

    psFree(view);

    // Write out summary statistics
    if (stats) {
        const char *statsMDC = psMetadataConfigFormat(stats);
        if (!statsMDC || strlen(statsMDC) == 0) {
            psWarning("Unable to get statistics MDC file.\n");
        } else {
            fprintf(statsFile, "%s", statsMDC);
        }
        psFree(statsMDC);
        fclose(statsFile);

        psFree(stats);
    }

    return true;
}
