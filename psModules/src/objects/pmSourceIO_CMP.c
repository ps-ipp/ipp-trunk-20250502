/** @file  pmSourceIO.c
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.34 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-12-08 02:51:14 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pslib.h>

#include "pmConfig.h"
#include "pmDetrendDB.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmPSFtry.h"

#include "pmSourceIO.h"

// XXX make sure in and out have consistent zero-point adjustments
// XXX make sure the angle in correctly translated to/from degrees
// XXX we lose all information from the 'type' field

// XXX update this file is we convert to PAR[4] : SigmaX*sqrt(2) (not 1/SigmaX)

// elixir-style pseudo FITS table (header + ascii list)
bool pmSourcesWriteCMP (psArray *sources, char *filename, psMetadata *header)
{
    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_PTR_NON_NULL(filename, false);
    PS_ASSERT_PTR_NON_NULL(header, false);

    int i, type;
    // psMetadataItem *mdi;
    psF32 *PAR;
    float lsky = 0;
    bool status;
    psEllipseAxes axes;

    // find config information for output header
    float ZERO_POINT = psMetadataLookupF32 (&status, header, "ZERO_PT");
    if (!status) {
        ZERO_POINT = 25.0;
    }

    // MEF elements have XTENSION, not SIMPLE: remove this (replace with SIMPLE)
    psMetadataLookupStr (&status, header, "XTENSION");
    if (status) {
        psMetadataRemoveKey (header, "XTENSION");
    }

    // create file, write-out header
    psMetadataAddS32 (header, PS_LIST_HEAD, "NAXIS", PS_META_REPLACE, "head data only", 0);
    psMetadataAddBool (header, PS_LIST_HEAD, "SIMPLE", PS_META_REPLACE, "CMP file, not simple", false);

    psFits *fits = psFitsOpen (filename, "w");
    if (fits == NULL) {
        psError(PS_ERR_IO, false, "can't open output file for write %s\n", filename);
        return false;
    }
    // XXX what is the EXTNAME??
    if (!psFitsWriteBlank(fits, header, "")) {
        psError(PS_ERR_IO, false, "Writing header to %s\n", filename);
        (void)psFitsClose(fits);
        return false;
    }
    if (!psFitsClose(fits)) {
        const psErrorCode code = psErrorCodeLast();

        if (code == PS_ERR_BAD_FITS) {
            psErrorClear();
        } else {
            psError(PS_ERR_IO, false, "Closing %s\n", filename);
            return false;
        }
    }

    // re-open, add data to end of file
    FILE *f = fopen (filename, "a+");
    if (f == NULL) {
        psLogMsg ("WriteSourceOBJ", 3, "can't reopen output file for append %s\n", filename);
        psError(PS_ERR_IO, false, "can't open output file for output %s\n", filename);
        return false;
    }

    fseeko(f, 0, SEEK_END);

    psLine *line = psLineAlloc (67);  // 66 is imclean-defined line length

    // write sources with models first
    for (i = 0; i < sources->n; i++) {
        pmSource *source = (pmSource *) sources->data[i];

        // no difference between PSF and non-PSF model
        pmModel *model = pmSourceGetModel (NULL, source);
        if (model == NULL)
            continue;

        PAR = model->params->data.F32;

        type = pmSourceGetDophotType (source);
        lsky = (source->sky < 1.0) ? 0.0 : log10(source->sky);

        axes = pmPSF_ModelToAxes (PAR, model->class->useReff);

        float psfMagErr = isfinite(source->psfMagErr) ? source->psfMagErr : 999;

        psLineInit (line);
        psLineAdd (line, "%6.1f ",  PAR[PM_PAR_XPOS]);
        psLineAdd (line, "%6.1f ",  PAR[PM_PAR_YPOS]);
        psLineAdd (line, "%6.3f ",  PS_MIN (99.0, source->psfMag + ZERO_POINT));
        psLineAdd (line, "%03d ",   PS_MIN (999, (int)psfMagErr));
        psLineAdd (line, "%2d ",    type);
        psLineAdd (line, "%3.1f ",  lsky);
        psLineAdd (line, "%6.3f ",  PS_MIN (99.0, source->extMag + ZERO_POINT));
        psLineAdd (line, "%6.3f ",  PS_MIN (99.0, source->apMag  + ZERO_POINT));
        psLineAdd (line, "%6.2f ",  axes.major);
        psLineAdd (line, "%6.2f ",  axes.minor);
        psLineAdd (line, "%5.1f\n", axes.theta);
        if (fwrite(line->line, 1, line->Nline, f) < line->Nline) {
            psError(PS_ERR_IO, true, "Unable to write CMP sources file (%s)", filename);
            fclose(f);
            psFree(line);
            return false;
        }
    }
    fclose (f);
    psFree (line);
    return true;
}

# define BYTES_STAR 66
# define BLOCK 1000

// elixir-style pseudo FITS table (header + ascii list)
psArray *pmSourcesReadCMP (char *filename, psMetadata *header)
{
    PS_ASSERT_PTR_NON_NULL(filename, false);
    PS_ASSERT_PTR_NON_NULL(header, false);

    bool status;
    int Ninstar;
    psF32 *PAR;
    psEllipseAxes axes;

    // define PSF model type
    int modelType = pmModelClassGetType ("PS_MODEL_GAUSS");

    char *PSF_NAME = psMetadataLookupStr (&status, header, "PSF_NAME");
    if (PSF_NAME != NULL) {
        modelType = pmModelClassGetType (PSF_NAME);
    }

    // XXX unused // find config information for output header
    // XXX unused float ZERO_POINT = psMetadataLookupF32 (&status, header, "ZERO_PT");
    // XXX unused if (!status)
    // XXX unused     ZERO_POINT = 25.0;

    // how many lines in the header?
    long nLines = header->list->n;
    off_t nBytes = nLines * 80;
    if (nBytes % 2880) {
        off_t nBlock = 1 + (off_t)(nBytes / 2880);
        nBytes = nBlock * 2880;
    }

    // re-open, seek to end of header
    FILE *f = fopen (filename, "r");
    if (f == NULL) {
        psLogMsg ("pmSourcesReadCMP", 3, "can't open output file for input %s\n", filename);
        return NULL;
    }

    fseeko(f, nBytes, SEEK_SET);

    // prepare array to store data
    int nStars = psMetadataLookupS32 (&status, header, "NSTARS");
    psArray *sources = psArrayAlloc (nStars);
    sources->n = 0;

    // we have fixed bytes / line : use that info
    // XXX use the min of nStars and BLOCK?
    char *buffer = psAlloc (BYTES_STAR*PS_MIN(nStars, BLOCK));

    int Nextra = 0;
    while (true) {
        /* load next data block */
        // XXX fix the use of two vars with different case -JH
        off_t Nbytes = BYTES_STAR * BLOCK - Nextra;
        off_t nbytes = fread (&buffer[Nextra], 1, Nbytes, f);
        if (nbytes == 0) {
            goto done_load;
        }
        nbytes += Nextra;

        /* check line-by-line integrity */
        char *c  = buffer;
        char *c2 = NULL;
        while (c < buffer + nbytes) {
            for (c2 = c; *c2 == '\n'; c2++)
                ;
            if (c2 > c) { /* extra return chars */
                memmove (c, c2, (int)(buffer + nbytes - c2));
                int Nskip = c2 - c;
                nbytes -= Nskip;
                memset(buffer + nbytes, '\0', Nskip);
                psLogMsg (__func__, 4, "deleted %d extra return chars\n", Nskip);
            }
            c2 = strchr (c, '\n');
            if (c2 == (char *) NULL) {
                goto done_check;
            }
            c2++;
            if ((c2 - c) != BYTES_STAR) { /* bad line, delete it */
                memmove (c, c2, (int)(buffer + nbytes - c2));
                int Nskip = c2 - c;
                nbytes -= Nskip;
                memset(buffer + nbytes, '\0', Nskip);
                psLogMsg (__func__, 4, "deleted line, %d extra chars\n", Nskip);
            } else {
                c = c2;
            }
        }
done_check:

        /* extract data for stars */
        Ninstar = nbytes / BYTES_STAR;
        Nextra = nbytes % BYTES_STAR;
        for (int j = 0; j < Ninstar; j++) {
            psString line = psStringNCopy (&buffer[j*BYTES_STAR], BYTES_STAR);

            psArray *array = psStringSplitArray (line, " ", false);

            // XXX this is a bit cheap: I don't even attempt to interpret the
            // sextractor / dophot analysis to distinguish stars and galaxies
            // your milage may vary...
            pmSource *source = pmSourceAlloc ();
            source->modelPSF = pmModelAlloc (modelType);
            source->type = PM_SOURCE_TYPE_STAR;

            PAR = source->modelPSF->params->data.F32;

            PAR[PM_PAR_SKY] = pow (atof (array->data[5]), 10.0);
            PAR[PM_PAR_XPOS] = atof (array->data[0]);
            PAR[PM_PAR_YPOS] = atof (array->data[1]);
            source->psfMag = atof (array->data[2]);
            source->extMag = atof (array->data[6]);
            source->psfMagErr = atof (array->data[3]) / 1000.0;
            source->apMag  = atof (array->data[7]);
            axes.major     = atof (array->data[8]);
            axes.minor     = atof (array->data[9]);
            axes.theta  = atof (array->data[10]);

            if (!isfinite(axes.major))
                goto skip_source;
            if (!isfinite(axes.minor))
                goto skip_source;
            if (!isfinite(axes.theta))
                goto skip_source;

            pmPSF_AxesToModel (PAR, axes, source->modelPSF->class->useReff);

            psArrayAdd (sources, 100, source);

skip_source:
            psFree (line);
            psFree (array);
            psFree (source);

        }
    }
done_load:

    // XXX if sources->n != nStars, give an error?
    psFree (buffer);

    fclose (f);
    return (sources);
}
