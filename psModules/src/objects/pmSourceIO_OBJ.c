/** @file  pmSourceIO.c
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.19 $ $Name: not supported by cvs2svn $
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

// dophot-style output list with fixed line width
bool pmSourcesWriteOBJ (psArray *sources, char *filename)
{
    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_PTR_NON_NULL(filename, false);

    int type;
    psF32 *PAR, *dPAR;
    float dmag, apResid;
    psEllipseAxes axes;

    psTimerStart ("string");

    psLine *line = psLineAlloc (104);  // 104 is dophot-defined line length

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
        dPAR = model->dparams->data.F32;

        dmag = dPAR[PM_PAR_I0] / PAR[PM_PAR_I0];
        type = pmSourceGetDophotType (source);
        if ((source->apMag < 99.0) && (source->psfMag < 99.0)) {
            apResid = source->apMag - source->psfMag;
        } else {
            apResid = 0.0;
        }

        axes = pmPSF_ModelToAxes (PAR, model->class->useReff);

        psLineInit (line);
        psLineAdd (line, "%3d",   type);
        psLineAdd (line, "%8.2f", PAR[PM_PAR_XPOS]);
        psLineAdd (line, "%8.2f", PAR[PM_PAR_YPOS]);
        psLineAdd (line, "%8.3f", source->psfMag);
        psLineAdd (line, "%6.3f", dmag);
        psLineAdd (line, "%9.2f", source->sky);
        psLineAdd (line, "%9.3f", axes.major);
        psLineAdd (line, "%9.3f", axes.minor);
        psLineAdd (line, "%7.2f", axes.theta);
        psLineAdd (line, "%8.3f", source->extMag);
        psLineAdd (line, "%8.3f", source->apMag);
        psLineAdd (line, "%8.2f\n", apResid);
        if (fwrite (line->line, 1, line->Nline, f) < line->Nline) {
            psError(PS_ERR_IO, true, "Unable to write OBJ sources file (%s)", filename);
            fclose(f);
            psFree(line);
            return false;
        }
    }
    fclose (f);
    psFree (line);
    fprintf (stderr, "%f seconds for %d objects\n", psTimerMark ("string"), (int)sources->n);
    return true;
}
