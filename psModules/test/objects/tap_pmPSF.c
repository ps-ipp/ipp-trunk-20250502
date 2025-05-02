#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/*
Tested:
    pmPSFOptions *pmPSFOptionsAlloc()
    pmPSF *pmPSFAlloc (pmPSFOptions *options)
    bool psMemCheckPSF(psPtr ptr)
Must test:
    double pmPSF_SXYfromModel (psF32 *modelPar)
    double pmPSF_SXYtoModel (psF32 *fittedPar)
    bool pmGrowthCurveGenerate (pmReadout *readout, pmPSF *psf, bool ignore, psMas!
    pmPSF *pmPSFBuildSimple (char *typeName, float sxx, float syy, float sxy, ...)
    bool pmPSF_AxesToModel (psF32 *modelPar, psEllipseAxes axes)
    bool pmPSF_FitToModel (psF32 *fittedPar, float minMinorAxis)
    psEllipsePol pmPSF_ModelToFit (psF32 *modelPar)
    psEllipseAxes pmPSF_ModelToAxes (psF32 *modelPar, double maxAR)
*/


#define VERBOSE                 0
#define ERR_TRACE_LEVEL         10
#define TEST_FLOATS_EQUAL(X, Y) (abs((X) - (Y)) < 0.0001)

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(83);

    // ----------------------------------------------------------------------
    // pmPSFOptionsAlloc() tests
    // pmPSFOptions *pmPSFOptionsAlloc()
    {
        psMemId id = psMemGetId();
        pmPSFOptions *psfOptions = pmPSFOptionsAlloc();
        ok(psfOptions != NULL && psMemCheckPSFOptions(psfOptions), "pmPSFOptionsAlloc() returned non-NULL");
        ok(psfOptions->type == 0, "pmPSFOptionsAlloc set pmPSFOptions->type ");
        ok(psfOptions->stats == NULL, "pmPSFOptionsAlloc set pmPSFOptions->stats ");
        ok(psfOptions->psfTrendMode == PM_TREND_NONE, "pmPSFOptionsAlloc set pmPSFOptions->psfTrendMode ");
        ok(psfOptions->psfTrendNx == 0, "pmPSFOptionsAlloc set pmPSFOptions->psfTrendNx ");
        ok(psfOptions->psfTrendNy == 0, "pmPSFOptionsAlloc set pmPSFOptions->psfTrendNy ");
        ok(psfOptions->psfFieldNx == 0, "pmPSFOptionsAlloc set pmPSFOptions->psfFieldNx ");
        ok(psfOptions->psfFieldNy == 0, "pmPSFOptionsAlloc set pmPSFOptions->psfFieldNy ");
        ok(psfOptions->psfFieldXo == 0, "pmPSFOptionsAlloc set pmPSFOptions->psfFieldXo ");
        ok(psfOptions->psfFieldYo == 0, "pmPSFOptionsAlloc set pmPSFOptions->psfFieldYo ");
        ok(psfOptions->poissonErrorsPhotLMM == true, "pmPSFOptionsAlloc set pmPSFOptions->poissonErrorsPhotLMM ");
        ok(psfOptions->poissonErrorsPhotLin == false, "pmPSFOptionsAlloc set pmPSFOptions->poissonErrorsPhotLin ");
        ok(psfOptions->poissonErrorsParams  == true, "pmPSFOptionsAlloc set pmPSFOptions->poissonErrorsParams ");
        psFree(psfOptions);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmPSFAlloc() tests
    // call pmPSFAlloc() with NULL input parameters
    #define TEST_POISSON_ERRORS true
    {
        psMemId id = psMemGetId();
        pmPSF *psf = pmPSFAlloc(NULL);
        ok(psf == NULL, "pmPSFAlloc() returned NULL with NULL input parameters");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // call pmPSFAlloc() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmPSFOptions *psfOptions = pmPSFOptionsAlloc();
        psfOptions->psfTrendNx = 1;
        psfOptions->psfTrendNy = 2;
        psfOptions->psfFieldNx = 3;
        psfOptions->psfFieldNy = 4;
        psfOptions->psfFieldXo = 5;
        psfOptions->psfFieldYo = 6;
        pmModelClassInit();
        psfOptions->type = pmModelClassGetType("PS_MODEL_GAUSS");
        pmPSF *psf = pmPSFAlloc(psfOptions);
        ok(psf != NULL && psMemCheckPSF(psf), "pmPSFAlloc() returned non-NULL");
        ok(psf->type == psfOptions->type, "pmPSFAlloc() set the pmPSF->type correctly");
        ok(psf->chisq == 0.0, "pmPSFAlloc() set the pmPSF->chisq correctly");
        ok(psf->ApResid == 0.0, "pmPSFAlloc() set the pmPSF->ApResid correctly");
        ok(psf->dApResid == 0.0, "pmPSFAlloc() set the pmPSF->dApResid correctly");
        ok(psf->skyBias == 0.0, "pmPSFAlloc() set the pmPSF->skyBias correctly");
        ok(psf->skySat == 0.0, "pmPSFAlloc() set the pmPSF->skySat correctly");
        ok(psf->nPSFstars == 0, "pmPSFAlloc() set the pmPSF->nPSFstars correctly");
        ok(psf->nApResid == 0, "pmPSFAlloc() set the pmPSF->nApResid correctly");
        int Nparams = pmModelClassParameterCount(psfOptions->type);
        ok(psf->params != NULL &&
           psMemCheckArray(psf->params) &&
           psf->params->n == Nparams, "pmPSFAlloc() set the pmPSF->params correctly");
        ok(psf->poissonErrorsPhotLMM == psfOptions->poissonErrorsPhotLMM, "pmPSFAlloc() set the pmPSF->poissonErrorsPhotLMM");
        ok(psf->poissonErrorsPhotLin == psfOptions->poissonErrorsPhotLin, "pmPSFAlloc() set the pmPSF->poissonErrorsPhotLin");
        ok(psf->poissonErrorsParams == psfOptions->poissonErrorsParams, "pmPSFAlloc() set the pmPSF->poissonErrorsParams");
        ok(psf->ApTrend == NULL, "pmPSFAlloc() set the pmPSF->ApTrend");
        ok(psf->FluxScale == NULL, "pmPSFAlloc() set the pmPSF->FluxScale");
        ok(psf->growth == NULL, "pmPSFAlloc() set the pmPSF->growth");
        ok(psf->residuals == NULL, "pmPSFAlloc() set the pmPSF->residuals");
        ok(psf->psfTrendMode == psfOptions->psfTrendMode, "pmPSFAlloc() set the pmPSF->psfTrendMode");
        ok(psf->trendNx == psfOptions->psfTrendNx, "pmPSFAlloc() set the pmPSF->trendNx (%d %d)", psf->trendNx, psfOptions->psfTrendNx);
        ok(psf->trendNy == psfOptions->psfTrendNy, "pmPSFAlloc() set the pmPSF->trendNy (%d %d)", psf->trendNy, psfOptions->psfTrendNy);
        ok(psf->fieldNx == psfOptions->psfFieldNx, "pmPSFAlloc() set the pmPSF->fieldNx");
        ok(psf->fieldNy == psfOptions->psfFieldNy, "pmPSFAlloc() set the pmPSF->fieldNy");
        ok(psf->fieldXo == psfOptions->psfFieldXo, "pmPSFAlloc() set the pmPSF->fieldXo");
        ok(psf->fieldYo == psfOptions->psfFieldYo, "pmPSFAlloc() set the pmPSF->fieldYo");
        ok(psf->ApTrend == NULL, "pmPSFAlloc() set the pmPSF->ApTrend correctly");
        ok(psf->FluxScale == NULL, "pmPSFAlloc() set the pmPSF->FluxScale correctly");

        if (psf->poissonErrorsPhotLMM) {
            ok(psf->ChiTrend->nX == 1, "pmPSFAlloc() set the pmPSF->ChiTrend correctly");
	} else {
            ok(psf->ChiTrend->nX == 2, "pmPSFAlloc() set the pmPSF->ChiTrend correctly");
	}
        ok(psf->growth == NULL, "pmPSFAlloc() set the pmPSF->growth correctly");
        ok(psf->residuals == NULL, "pmPSFAlloc() set the pmPSF->residuals correctly");
        ok(psf->params != NULL && psMemCheckArray(psf->params) && psf->params->n == Nparams, 
           "pmPSFAlloc() set the pmPSF->params psVector correctly");

        pmModelClassCleanup();
        psFree(psf);
        psFree(psfOptions);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmPSF_SXYfromModel() tests
    // Call pmPSF_SXYfromModel() with NULL input parameters
    {
        psMemId id = psMemGetId();
        double tmpD = pmPSF_SXYfromModel(NULL);
        ok(isnan(tmpD), "pmPSF_SXYfromModel() returned NULL with NULL input parameters");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSF_SXYfromModel() with NULL input parameters
    {
        psMemId id = psMemGetId();
        psF32 modelPar[20];
        modelPar[PM_PAR_SXX] = 2.0;
        modelPar[PM_PAR_SYY] = 3.0;
        modelPar[PM_PAR_SXY] = 5.0;
        double SXX = modelPar[PM_PAR_SXX];
        double SYY = modelPar[PM_PAR_SYY];
        double SXY = modelPar[PM_PAR_SXY];
        psF32 verF = SXY / PS_SQR(1.0 / PS_SQR(SXX) + 1.0 / PS_SQR(SYY));
        psF32 testF = pmPSF_SXYfromModel(modelPar);
        ok(verF == testF, "pmPSF_SXYfromModel() calculated correctly");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmPSF_SXYtoModel() tests
    // Call pmPSF_SXYtoModel() with NULL input parameters
    {
        psMemId id = psMemGetId();
        double tmpD = pmPSF_SXYtoModel(NULL);
        ok(isnan(tmpD), "pmPSF_SXYtoModel() returned NULL with NULL input parameters");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSF_SXYtoModel() with NULL input parameters
    {
        psMemId id = psMemGetId();
        psF32 fittedPar[20];
        fittedPar[PM_PAR_SXX] = 2.0;
        fittedPar[PM_PAR_SYY] = 3.0;
        fittedPar[PM_PAR_SXY] = 5.0;
        double SXX = fittedPar[PM_PAR_SXX];
        double SYY = fittedPar[PM_PAR_SYY];
        double fit = fittedPar[PM_PAR_SXY];
        double verF = fit * PS_SQR(1.0 / PS_SQR(SXX) + 1.0 / PS_SQR(SYY));
        psF32 testF = pmPSF_SXYtoModel(fittedPar);
        ok(TEST_FLOATS_EQUAL(verF, testF), "pmPSF_SXYtoModel() calculated correctly (%f %f)", verF, testF);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmPSF_FitToModel() tests
    // Call pmPSF_FitToModel() with NULL input parameters
    {
        psMemId id = psMemGetId();
        bool rc = pmPSF_FitToModel(NULL, 0.0);
        ok(rc == false, "pmPSF_FitToModel() returned NULL with NULL input parameters");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSF_FitToModel() with NULL input parameters
    {
        #define MIN_MINOR_AXIS 1.0
        psMemId id = psMemGetId();
        psF32 origFittedPar[3], testFittedPar[3];
        psEllipsePol pol;
        pol.e0 = origFittedPar[PM_PAR_E0] = testFittedPar[PM_PAR_E0] = 2.0;
        pol.e1 = origFittedPar[PM_PAR_E1] = testFittedPar[PM_PAR_E1] = 3.0;
        pol.e2 = origFittedPar[PM_PAR_E2] = testFittedPar[PM_PAR_E2] = 5.0;
        ok(pmPSF_FitToModel(testFittedPar, MIN_MINOR_AXIS), "pmPSF_FitToModel() returned TRUE with acceptable input parameters");

        psEllipseAxes axes;
        psEllipsePolToAxes(pol, MIN_MINOR_AXIS);
        psEllipseShape shape = psEllipseAxesToShape(axes);

        ok(TEST_FLOATS_EQUAL(testFittedPar[PM_PAR_SXX], shape.sx * M_SQRT2),
          "pmPSF_FitToModel() set fittedPar[PM_PAR_SXX] correctly");
        ok(TEST_FLOATS_EQUAL(testFittedPar[PM_PAR_SYY], shape.sy * M_SQRT2),
          "pmPSF_FitToModel() set fittedPar[PM_PAR_SYY] correctly");
        ok(TEST_FLOATS_EQUAL(testFittedPar[PM_PAR_SXY], shape.sxy),
          "pmPSF_FitToModel() set fittedPar[PM_PAR_SXY] correctly");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmPSF_ModelToFit() tests
    // psEllipsePol pmPSF_ModelToFit (psF32 *modelPar)
    // Call pmPSF_ModelToFit() with NULL input parameters
    {
        psMemId id = psMemGetId();
        psEllipsePol pol = pmPSF_ModelToFit(NULL);
        ok(isnan(pol.e0), "pmPSF_ModelToFit() returned NULL (psEllipsePol) with NULL input parameters");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSF_ModelToFit() with NULL input parameters
    {
        #define MIN_MINOR_AXIS 1.0
        psMemId id = psMemGetId();
        psF32 modelPar[3];
        modelPar[PM_PAR_SXX] = 2.0;
        modelPar[PM_PAR_SYY] = 3.0;
        modelPar[PM_PAR_SXY] = 5.0;

        psEllipsePol pol = pmPSF_ModelToFit(modelPar);
        ok(!isnan(pol.e0), "pmPSF_ModelToFit() returned TRUE with acceptable input parameters");

        psEllipseShape shape;
        shape.sx  = modelPar[PM_PAR_SXX] / M_SQRT2;
        shape.sy  = modelPar[PM_PAR_SYY] / M_SQRT2;
        shape.sxy = modelPar[PM_PAR_SXY];
        psEllipsePol actPol = psEllipseShapeToPol(shape);
        ok(TEST_FLOATS_EQUAL(pol.e0, actPol.e0), "pmPSF_ModelToFit() set psEllipsePol.e0 correctly");
        ok(TEST_FLOATS_EQUAL(pol.e1, actPol.e1), "pmPSF_ModelToFit() set psEllipsePol.e1 correctly");
        ok(TEST_FLOATS_EQUAL(pol.e2, actPol.e2), "pmPSF_ModelToFit() set psEllipsePol.e2 correctly");

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmPSF_ModelToAxes() tests
    // psEllipseAxes pmPSF_ModelToAxes (psF32 *modelPar, double maxAR)
    // Call pmPSF_ModelToAxes() with NULL input parameters
    {
        psMemId id = psMemGetId();
        psEllipseAxes axes = pmPSF_ModelToAxes(NULL, 1.0);
        ok(isnan(axes.major), "pmPSF_ModelToAxes() returned NULL (psEllipseAxes) with NULL input parameters");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSF_ModelToAxes() with NULL input parameters
    {
        #define MAX_AX 1.0
        psMemId id = psMemGetId();
        psF32 modelPar[3];
        modelPar[PM_PAR_SXX] = 2.0;
        modelPar[PM_PAR_SYY] = 3.0;
        modelPar[PM_PAR_SXY] = 5.0;

        psEllipseShape shape;
        shape.sx  = modelPar[PM_PAR_SXX] / M_SQRT2;
        shape.sy  = modelPar[PM_PAR_SYY] / M_SQRT2;
        shape.sxy = modelPar[PM_PAR_SXY];
        psEllipseAxes axes = psEllipseShapeToAxes (shape, MAX_AX);

        psEllipseAxes actAxes = pmPSF_ModelToAxes(modelPar, MAX_AX);
        ok(!isnan(actAxes.major), "pmPSF_ModelToAxes() returned TRUE with acceptable input parameters");
        ok(TEST_FLOATS_EQUAL(actAxes.major, axes.major), "pmPSF_ModelToAxes() set psEllipseAxes.major correctly");
        ok(TEST_FLOATS_EQUAL(actAxes.minor, axes.minor), "pmPSF_ModelToAxes() set psEllipseAxes.minor correctly");
        ok(TEST_FLOATS_EQUAL(actAxes.theta, axes.theta), "pmPSF_ModelToAxes() set psEllipseAxes.theta correctly");

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmPSF_AxesToModel() tests
    // bool pmPSF_AxesToModel (psF32 *modelPar, psEllipseAxes axes)
    // Call pmPSF_AxesToModel() with NULL input parameters
    {
        psMemId id = psMemGetId();
        psEllipseAxes axes;
        bool rc = pmPSF_AxesToModel(NULL, axes);
        ok(rc == false, "pmPSF_AxesToModel() returned NULL with NULL input parameters");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmPSF_AxesToModel() with NULL input parameters
    {
        #define MIN_MINOR_AXIS 1.0
        psMemId id = psMemGetId();
        psF32 modelPar[3];
        psEllipseAxes axes;
        axes.major = 2.0;
        axes.minor = 3.0;
        axes.theta = 5.0;
        ok(pmPSF_AxesToModel(modelPar, axes), "pmPSF_AxesToModel() returned TRUE with acceptable input parameters");
        psEllipseShape shape = psEllipseAxesToShape(axes);
        ok(TEST_FLOATS_EQUAL(modelPar[PM_PAR_SXX], shape.sx * M_SQRT2), "pmPSF_AxesToModel() set modelPar[PM_PAR_SXX] correctly");
        ok(TEST_FLOATS_EQUAL(modelPar[PM_PAR_SYY], shape.sy * M_SQRT2), "pmPSF_AxesToModel() set modelPar[PM_PAR_SYY] correctly");
        ok(TEST_FLOATS_EQUAL(modelPar[PM_PAR_SXY], shape.sxy), "pmPSF_AxesToModel() set modelPar[PM_PAR_SXY] correctly");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
