#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/*
    STATUS:
	All functions are tested.  However, some of the pmGrowthCurveCorrect()
	tests that were supplied by IfA ae failing.
*/

#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(70);

    // ----------------------------------------------------------------------
    // pmGrowthCurveAlloc() tests
    // call pmGrowthCurveAlloc() with acceptable input parameters.
    {
        psMemId id = psMemGetId();
        pmGrowthCurve *growthCurve = pmGrowthCurveAlloc(1.0, 2.0, 3.0);
        ok(growthCurve && psMemCheckGrowthCurve(growthCurve), "pmGrowthCurveAlloc() allocated a pmGrowthCurve correctly");
        ok(growthCurve->radius && psMemCheckVector(growthCurve->radius), "pmGrowthCurveAlloc() allocated the radius psVector correctly");
        ok(growthCurve->apMag  && psMemCheckVector(growthCurve->apMag) &&
           growthCurve->apMag->n == growthCurve->radius->n, "pmGrowthCurveAlloc() allocated the apMag psVector correctly");
        ok(growthCurve->refRadius == 3.0, "pmGrowthCurveAlloc() set growthCurve->refRadius correctly");
        ok(growthCurve->maxRadius == 2.0, "pmGrowthCurveAlloc() set growthCurve->maxRadius correctly");
        ok(growthCurve->apLoss == 0.0, "pmGrowthCurveAlloc() set growthCurve->apLoss correctly");
        ok(growthCurve->fitMag == 0.0, "pmGrowthCurveAlloc() set growthCurve->fitMag correctly");
        psFree(growthCurve);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmGrowthCurveCorrect() tests
    // Call pmGrowthCurveCorrect() with NULL input pmGrowthCurve
    {
        psMemId id = psMemGetId();
        pmGrowthCurve *growthCurve = pmGrowthCurveAlloc(1.0, 2.0, 3.0);
        ok(isnan(pmGrowthCurveCorrect(NULL, 0.0)), "pmGrowthCurveCorrect() returned NAN with NULL input pmGrowthCurve");
        psFree(growthCurve);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmGrowthCurveCorrect() with acceptable input parameters.
    {
        #define RADIUS 0.5
        psMemId id = psMemGetId();
        pmGrowthCurve *growthCurve = pmGrowthCurveAlloc(1.0, 2.0, 3.0);
        float testCor = pmGrowthCurveCorrect(growthCurve, RADIUS);
        float actRad = psVectorInterpolate (growthCurve->radius, growthCurve->apMag, RADIUS);
        float actCor = growthCurve->apRef - actRad;

        ok(!isnan(testCor), "pmGrowthCurveCorrect() call was successful");
        ok(actCor == testCor, "pmGrowthCurveCorrect() calculated the correction correctly");

        psFree(growthCurve);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // EM test 01: test allocation
    // offset of 0.0,0.0 wrt growth ref source
    {
        psMemId id = psMemGetId();

        pmGrowthCurve *growth = pmGrowthCurveAlloc (2.0, 100.0, 15.0);

        ok(growth != NULL, "growth curve allocated");
        skip_start(growth == NULL, 0, "Skipping tests because pmShutterCorrParsAlloc() failed");

        ok(growth->radius->n == (100.0 - 2.0), "correct number of growth radii");
        ok(growth->apMag->n == (100.0 - 2.0), "correct number of growth apMags");

        ok_float(growth->refRadius, 15.0, "correct refRadius");
        ok_float(growth->maxRadius, 100.0, "correct maxRadius");

        // does the growth curve correctly fix aperture mags?

        // generate a simple readout
        pmReadout *readout = pmReadoutAlloc (NULL);
        readout->image = psImageAlloc (64, 64, PS_TYPE_F32);
        readout->mask  = psImageAlloc (64, 64, PS_TYPE_U8);

        // create an empty reference image
        psImageInit (readout->image, 0.0);
        psImageInit (readout->mask, 0);

        // generate a simple psf
        pmModelClassInit();
        pmPSF *psf = pmPSFBuildSimple("PS_MODEL_GAUSS", 1.5, 1.5, 0.0);
        psf->growth = growth;

        pmGrowthCurveGenerate(readout, psf, false, 0, 0);

        // check ap mags for a few radii set by hand
        ok_float_tol(growth->apMag->data.F32[0],   -9.7805, 0.0001, "apMag at radius 0: %f", growth->apMag->data.F32[0]);
        ok_float_tol(growth->apMag->data.F32[3],  -10.3722, 0.0001, "apMag at radius 3: %f", growth->apMag->data.F32[3]);
        ok_float_tol(growth->apMag->data.F32[10], -10.3759, 0.0001, "apMag at radius 10: %f", growth->apMag->data.F32[10]);
        ok_float_tol(growth->apMag->data.F32[30], -10.3759, 0.0001, "apMag at radius 30: %f", growth->apMag->data.F32[30]);

        ok_float_tol(growth->apRef,  -10.3759, 0.0001, "apMag at ref radius : %f", growth->apRef);
        ok_float_tol(growth->fitMag, -10.3759, 0.0001, "fitMag : %f", growth->fitMag);
        ok_float(growth->apLoss, 0.0, "apLoss : %f", growth->apLoss);

        // create template model and measure apMag at fractional offsets
        // XXX note model is at 0.5,0.5 subpix center
        pmModel *modelRef = pmModelAlloc(psf->type);
        modelRef->params->data.F32[PM_PAR_SKY] = 0;
        modelRef->params->data.F32[PM_PAR_I0] = 1000;
        modelRef->params->data.F32[PM_PAR_XPOS] = 32.5;
        modelRef->params->data.F32[PM_PAR_YPOS] = 32.5;

        // measure growth-corrected photometry:
        pmSource *source = pmSourceAlloc ();
        source->modelPSF = pmModelFromPSF (modelRef, psf);
        source->type = PM_SOURCE_TYPE_STAR;
        source->pixels = psMemIncrRefCounter (readout->image);
        source->maskObj = psMemIncrRefCounter (readout->mask);

        source->modelPSF->dparams->data.F32[PM_PAR_I0] = 1;
        source->mode = PM_SOURCE_MODE_PSFSTAR;

        source->apRadius = 15.0;

        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        double refMag = source->apMag;

        source->apRadius = 10.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, +0.0000, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 8.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, +0.0000, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 6.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, +0.0003, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 4.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, +0.0020, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 3.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, -0.0001, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 2.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, -0.0075, 0.0001, "growth offset is is %f", refMag - source->apMag);

        // XXX include some apertures outside of growth correction range

        psFree(modelRef);
        psFree(source);
        psFree(readout);
        psFree(psf);

        skip_end();

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // EM test 02: test allocation
    // offset of 0.2,0.2 wrt growth ref source
    {
        psMemId id = psMemGetId();

        pmGrowthCurve *growth = pmGrowthCurveAlloc (2.0, 100.0, 15.0);

        ok(growth != NULL, "growth curve allocated");
        skip_start(growth == NULL, 0, "Skipping tests because pmShutterCorrParsAlloc() failed");

        ok(growth->radius->n == (100.0 - 2.0), "correct number of growth radii");
        ok(growth->apMag->n == (100.0 - 2.0), "correct number of growth apMags");

        ok_float(growth->refRadius, 15.0, "correct refRadius");
        ok_float(growth->maxRadius, 100.0, "correct maxRadius");

        // does the growth curve correctly fix aperture mags?

        // generate a simple readout
        pmReadout *readout = pmReadoutAlloc (NULL);
        readout->image = psImageAlloc (64, 64, PS_TYPE_F32);
        readout->mask  = psImageAlloc (64, 64, PS_TYPE_U8);

        // create an empty reference image
        psImageInit (readout->image, 0.0);
        psImageInit (readout->mask, 0);

        // generate a simple psf
        pmPSF *psf = pmPSFBuildSimple ("PS_MODEL_GAUSS", 1.5, 1.5, 0.0);
        psf->growth = growth;

        pmGrowthCurveGenerate (readout, psf, false, 0, 0);

        // check ap mags for a few radii set by hand
        ok_float_tol(growth->apMag->data.F32[0],   -9.7805, 0.0001, "apMag at radius 0: %f", growth->apMag->data.F32[0]);
        ok_float_tol(growth->apMag->data.F32[3],  -10.3722, 0.0001, "apMag at radius 3: %f", growth->apMag->data.F32[3]);
        ok_float_tol(growth->apMag->data.F32[10], -10.3759, 0.0001, "apMag at radius 10: %f", growth->apMag->data.F32[10]);
        ok_float_tol(growth->apMag->data.F32[30], -10.3759, 0.0001, "apMag at radius 30: %f", growth->apMag->data.F32[30]);

        ok_float_tol(growth->apRef,  -10.3759, 0.0001, "apMag at ref radius : %f", growth->apRef);
        ok_float_tol(growth->fitMag, -10.3759, 0.0001, "fitMag : %f", growth->fitMag);
        ok_float(growth->apLoss, 0.0, "apLoss : %f", growth->apLoss);

        // create template model and measure apMag at fractional offsets
        // XXX note model is at 0.5,0.5 subpix center
        pmModel *modelRef = pmModelAlloc(psf->type);
        modelRef->params->data.F32[PM_PAR_SKY] = 0;
        modelRef->params->data.F32[PM_PAR_I0] = 1000;
        modelRef->params->data.F32[PM_PAR_XPOS] = 32.3;
        modelRef->params->data.F32[PM_PAR_YPOS] = 32.3;

        // measure growth-corrected photometry:
        pmSource *source = pmSourceAlloc ();
        source->modelPSF = pmModelFromPSF (modelRef, psf);
        source->type = PM_SOURCE_TYPE_STAR;
        source->pixels = psMemIncrRefCounter (readout->image);
        source->maskObj = psMemIncrRefCounter (readout->mask);

        source->modelPSF->dparams->data.F32[PM_PAR_I0] = 1;
        source->mode = PM_SOURCE_MODE_PSFSTAR;

        source->apRadius = 15.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        double refMag = source->apMag;

        source->apRadius = 10.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, +0.0000, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 8.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, +0.0000, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 6.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, +0.0004, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 4.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, +0.0026, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 3.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, -0.0001, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 2.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, -0.0103, 0.0001, "growth offset is is %f", refMag - source->apMag);

        // XXX include some apertures outside of growth correction range

        psFree(modelRef);
        psFree(source);
        psFree(readout);
        psFree(psf);

        skip_end();

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // EM test 03: test allocation
    // offset of 0.4,0.4 wrt growth ref source
    {
        psMemId id = psMemGetId();

        pmGrowthCurve *growth = pmGrowthCurveAlloc (2.0, 100.0, 15.0);

        ok(growth != NULL, "growth curve allocated");
        skip_start(growth == NULL, 0, "Skipping tests because pmShutterCorrParsAlloc() failed");

        ok(growth->radius->n == (100.0 - 2.0), "correct number of growth radii");
        ok(growth->apMag->n == (100.0 - 2.0), "correct number of growth apMags");

        ok_float(growth->refRadius, 15.0, "correct refRadius");
        ok_float(growth->maxRadius, 100.0, "correct maxRadius");

        // does the growth curve correctly fix aperture mags?

        // generate a simple readout
        pmReadout *readout = pmReadoutAlloc (NULL);
        readout->image = psImageAlloc (64, 64, PS_TYPE_F32);
        readout->mask  = psImageAlloc (64, 64, PS_TYPE_U8);

        // create an empty reference image
        psImageInit (readout->image, 0.0);
        psImageInit (readout->mask, 0);

        // generate a simple psf
        pmPSF *psf = pmPSFBuildSimple ("PS_MODEL_GAUSS", 1.5, 1.5, 0.0);
        psf->growth = growth;

        pmGrowthCurveGenerate(readout, psf, false, 0, 0);

        // check ap mags for a few radii set by hand
        ok_float_tol(growth->apMag->data.F32[0],   -9.7805, 0.0001, "apMag at radius 0: %f", growth->apMag->data.F32[0]);
        ok_float_tol(growth->apMag->data.F32[3],  -10.3722, 0.0001, "apMag at radius 3: %f", growth->apMag->data.F32[3]);
        ok_float_tol(growth->apMag->data.F32[10], -10.3759, 0.0001, "apMag at radius 10: %f", growth->apMag->data.F32[10]);
        ok_float_tol(growth->apMag->data.F32[30], -10.3759, 0.0001, "apMag at radius 30: %f", growth->apMag->data.F32[30]);

        ok_float_tol(growth->apRef,  -10.3759, 0.0001, "apMag at ref radius : %f", growth->apRef);
        ok_float_tol(growth->fitMag, -10.3759, 0.0001, "fitMag : %f", growth->fitMag);
        ok_float(growth->apLoss, 0.0, "apLoss : %f", growth->apLoss);

        // create template model and measure apMag at fractional offsets
        // XXX note model is at 0.5,0.5 subpix center
        pmModel *modelRef = pmModelAlloc(psf->type);
        modelRef->params->data.F32[PM_PAR_SKY] = 0;
        modelRef->params->data.F32[PM_PAR_I0] = 1000;
        modelRef->params->data.F32[PM_PAR_XPOS] = 32.1;
        modelRef->params->data.F32[PM_PAR_YPOS] = 32.1;

        // measure growth-corrected photometry:
        pmSource *source = pmSourceAlloc ();
        source->modelPSF = pmModelFromPSF (modelRef, psf);
        source->type = PM_SOURCE_TYPE_STAR;
        source->pixels = psMemIncrRefCounter (readout->image);
        source->maskObj = psMemIncrRefCounter (readout->mask);

        source->modelPSF->dparams->data.F32[PM_PAR_I0] = 1;
        source->mode = PM_SOURCE_MODE_PSFSTAR;

        source->apRadius = 15.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        double refMag = source->apMag;

        source->apRadius = 10.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, +0.0000, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 8.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, +0.0000, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 6.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, +0.0006, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 4.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, +0.0038, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 3.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, +0.0000, 0.0001, "growth offset is is %f", refMag - source->apMag);

        source->apRadius = 2.0;
        pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP, 0);
        ok_float_tol(refMag - source->apMag, -0.0164, 0.0001, "growth offset is is %f", refMag - source->apMag);

        // XXX include some apertures outside of growth correction range

        psFree(modelRef);
        psFree(source);
        psFree(readout);
        psFree(psf);

        skip_end();

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

