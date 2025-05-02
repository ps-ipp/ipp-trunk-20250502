/** @file  pmSourceIO.c
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.16 $ $Name: not supported by cvs2svn $
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

// elixir-mode / sextractor-style output list with fixed line width
bool pmSourcesWriteSX (psArray *sources, char *filename)
{
    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_PTR_NON_NULL(filename, false);

    psF32 *PAR;
    psEllipseAxes axes;

    psLine *line = psLineAlloc (110);  // 110 is sextractor line length

    FILE *f = fopen (filename, "w");
    if (f == NULL) {
        psLogMsg (__func__, 3, "can't open output file for output %s\n", filename);
        return false;
    }

    // write sources with models
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = (pmSource *) sources->data[i];

        // no difference between PSF and non-PSF model
        pmModel *model = pmSourceGetModel (NULL, source);
        if (model == NULL)
            continue;

        PAR = model->params->data.F32;

        // pmSourceSextractType (source, &type, &flags);

        axes = pmPSF_ModelToAxes (PAR, model->class->useReff);

        psLineInit (line);
        psLineAdd (line, "%5.2f",  0.0); // should be type
        psLineAdd (line, "%11.3f", PAR[PM_PAR_XPOS]);
        psLineAdd (line, "%11.3f", PAR[PM_PAR_YPOS]);
        psLineAdd (line, "%9.4f",  source->psfMag);
        psLineAdd (line, "%9.4f",  source->psfMagErr);
        psLineAdd (line, "%13.4f", source->sky);
        psLineAdd (line, "%9.2f",  axes.major);
        psLineAdd (line, "%9.2f",  axes.minor);
        psLineAdd (line, "%6.1f",  axes.theta);
        psLineAdd (line, "%9.4f",  source->extMag);
        psLineAdd (line, "%9.4f",  source->apMag);
        psLineAdd (line, "%4d\n",  0); // should be flags
        if (fwrite(line->line, 1, line->Nline, f) < line->Nline) {
            psError(PS_ERR_IO, true, "Unable to write SX sources file (%s)", filename);
            fclose(f);
            psFree(line);
            return false;
        }
    }
    fclose (f);
    psFree (line);
    return true;
}

// XXX need to fix the FWHM / shape stuff,
// XXX make sure we are using the correct mags, etc
