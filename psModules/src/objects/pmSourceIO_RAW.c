/** @file  pmSourceIO.c
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.20 $ $Name: not supported by cvs2svn $
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

/***** Text Output Methods *****/
bool pmSourcesWriteRAW (psArray *sources, char *filename)
{

    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_PTR_NON_NULL(filename, false);

    char *name = (char *) psAlloc (strlen(filename) + 10);

    sprintf (name, "%s.psf.dat", filename);
    pmSourcesWritePSFs (sources, name);

    sprintf (name, "%s.ext.dat", filename);
    pmSourcesWriteEXTs (sources, name, true);

    sprintf (name, "%s.nul.dat", filename);
    pmSourcesWriteNULLs (sources, name);

    sprintf (name, "%s.mnt.dat", filename);
    pmMomentsWriteText (sources, name);

    psFree (name);
    return true;
}

// write the PSF sources to an output file
bool pmSourcesWritePSFs (psArray *sources, char *filename)
{
    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_PTR_NON_NULL(filename, false);

    double dPos;
    int i, j;
    FILE *f;
    psF32 *PAR, *dPAR;
    pmModel  *model;

    f = fopen (filename, "w");
    if (f == NULL) {
        psLogMsg (__func__, 3, "can't open output file for PSF sources: %s\n", filename);
        return false;
    }

    // write sources with models first
    for (i = 0; i < sources->n; i++) {
        pmSource *source = (pmSource *) sources->data[i];
        if (source->type != PM_SOURCE_TYPE_STAR)
            continue;
        model = source->modelPSF;
        if (model == NULL)
            continue;

        PAR  = model->params->data.F32;
        dPAR = model->dparams->data.F32;

        // dPos is positional error, dMag is mag error
        dPos = hypot (dPAR[PM_PAR_XPOS], dPAR[PM_PAR_YPOS]);

        fprintf (f, "%7.1f %7.1f  %7.1f %8.4f  %7.4f %7.4f  ",
                 PAR[PM_PAR_XPOS], PAR[PM_PAR_YPOS], source->sky,
                 source->psfMag, source->psfMagErr, dPos);

        for (j = 4; j < model->params->n; j++) {
            fprintf (f, "%9.6f ", PAR[j]);
        }
        fprintf (f, " : ");
        for (j = 4; j < model->params->n; j++) {
            fprintf (f, "%9.6f ", dPAR[j]);
        }

        float logChi = ((model[0].chisq == 0.0) || (model[0].nDOF == 0)) ? NAN : log10(model[0].chisq/model[0].nDOF);
        float logChiNorm = ((model[0].chisqNorm == 0.0) || (model[0].nDOF == 0)) ? NAN : log10(model[0].chisqNorm/model[0].nDOF);

        fprintf (f, ": %8.4f %2d %#5x %7.3f %7.3f  %7.1f %7.2f %4.2f %4d %2d\n",
                 source[0].apMag, source[0].type, source[0].mode,
                 logChi, logChiNorm,
                 source[0].peak->rawFlux,
                 source[0].apRadius,
                 source[0].pixWeightNotBad,
                 model[0].nDOF,
                 model[0].nIter);
    }
    fclose (f);
    return true;
}

// dump the sources to an output file
bool pmSourcesWriteEXTs (psArray *sources, char *filename, bool requireEXT)
{
    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_PTR_NON_NULL(filename, false);

    double dPos;
    int i, j;
    FILE *f;
    psF32 *PAR, *dPAR;
    pmModel  *model;

    f = fopen (filename, "w");
    if (f == NULL) {
        psLogMsg ("pmModelWriteEXTs", 3, "can't open output file for EXT sources: %s\n", filename);
        return false;
    }

    // write sources with models first
    for (i = 0; i < sources->n; i++) {
        pmSource *source = (pmSource *) sources->data[i];

        if (requireEXT && (source->type != PM_SOURCE_TYPE_EXTENDED))
            continue;

        model = source->modelEXT;
        if (model == NULL)
            continue;

        PAR  = model->params->data.F32;
        dPAR = model->dparams->data.F32;

        // dPos is shape error
        // XXX these are hardwired for SGAUSS
        dPos = hypot ((dPAR[PM_PAR_SXX] / PAR[PM_PAR_SXX]), (dPAR[PM_PAR_SYY] / PAR[PM_PAR_SYY]));

        fprintf (f, "%7.1f %7.1f  %7.1f %8.4f  %7.4f %7.4f  ",
                 PAR[PM_PAR_XPOS], PAR[PM_PAR_YPOS], source->sky,
                 source->extMag, source->psfMagErr, dPos);

        for (j = 4; j < model->params->n; j++) {
            fprintf (f, "%9.6f ", PAR[j]);
        }
        fprintf (f, " : ");
        for (j = 4; j < model->params->n; j++) {
            fprintf (f, "%9.6f ", dPAR[j]);
        }
        fprintf (f, ": %7.4f  %2d %#5x %7.3f %7.3f  %7.1f %7.2f %4.2f %4d %2d\n",
                 source->apMag,
                 source[0].type, source[0].mode,
                 log10(model[0].chisq/model[0].nDOF),
                 log10(model[0].chisqNorm/model[0].nDOF),
                 source[0].peak->rawFlux,
                 source[0].apRadius,
                 source[0].pixWeightNotBad,
                 model[0].nDOF,
                 model[0].nIter);
    }
    fclose (f);
    return true;
}

// dump the sources to an output file
bool pmSourcesWriteNULLs (psArray *sources, char *filename)
{
    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_PTR_NON_NULL(filename, false);

    int i;
    FILE *f;
    pmMoments *moment = NULL;
    pmSource *source = NULL;

    f = fopen (filename, "w");
    if (f == NULL) {
        psLogMsg ("DumpObjects", 3, "can't open output file for NULL sources: %s\n", filename);
        return false;
    }

    pmMoments *empty = pmMomentsAlloc ();

    // write sources with models first
    for (i = 0; i < sources->n; i++) {
        source = sources->data[i];

        // skip these sources (in PSF or EXT)
        if (source->type == PM_SOURCE_TYPE_STAR)
            continue;
        if (source->type == PM_SOURCE_TYPE_EXTENDED)
            continue;

        if (source->moments == NULL) {
            moment = empty;
        } else {
            moment = source->moments;
        }

        fprintf (f, "%5d %5d  %7.1f  %7.1f %7.1f  %6.3f %6.3f  %8.1f %7.1f %7.1f %7.1f  %4d %2d\n",
                 source->peak->x, source->peak->y, source->peak->detValue,
                 moment->Mx, moment->My,
                 moment->Mxx, moment->Myy,
                 moment->Sum, moment->Peak,
                 moment->Sky, moment->SN,
                 moment->nPixels, source->type);
    }
    fclose (f);
    psFree (empty);
    return true;
}

// write the moments to an output file
bool pmMomentsWriteText (psArray *sources, char *filename)
{
    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_PTR_NON_NULL(filename, false);

    int i;
    FILE *f;
    pmSource *source = NULL;

    f = fopen (filename, "w");
    if (f == NULL) {
        psLogMsg ("pmMomentsWriteText", 3, "can't open output file for moments: %s\n", filename);
        return false;
    }

    fprintf (f, "# %5s %5s  %8s  %7s %7s  %6s %6s  %10s %7s %7s %7s  %4s %4s %5s\n",
	     "x", "y", "peak", "Mx", "My", "Mxx", "Myy", "Sum", "Peak", "Sky", "SN", "nPix", "type", "mode");

    for (i = 0; i < sources->n; i++) {
        source = sources->data[i];
        if (source->moments == NULL)
            continue;
        fprintf (f, "%5d %5d  %8.1f  %7.1f %7.1f  %6.3f %6.3f  %10.1f %7.1f %7.1f %7.1f  %4d %2d %#5x\n",
                 source->peak->x, source->peak->y, source->peak->detValue,
                 source->moments->Mx, source->moments->My,
                 source->moments->Mxx, source->moments->Myy,
                 source->moments->Sum, source->moments->Peak,
                 source->moments->Sky, source->moments->SN,
                 source->moments->nPixels, source->type, source->mode);
    }
    fclose (f);
    return true;
}

// write the peaks to an output file
bool pmPeaksWriteText (psArray *peaks, char *filename)
{

    int i;
    FILE *f;

    f = fopen (filename, "w");
    if (f == NULL) {
        psLogMsg ("pmPeaksWriteText", 3, "can't open output file for peaks%s\n", filename);
        return false;
    }

    for (i = 0; i < peaks->n; i++) {
        pmPeak *peak = peaks->data[i];
        if (peak == NULL)
            continue;
        fprintf (f, "%5d %5d  %7.1f %7.2f %7.2f %7.2f\n",
                 peak->x, peak->y, peak->detValue, peak->rawFlux, peak->xf, peak->yf);
    }
    fclose (f);
    return true;
}

