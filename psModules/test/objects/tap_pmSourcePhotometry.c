#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

#include "tap.h"
#include "pstap.h"

bool pmSourcePhotometry_TestOffsets (double radius, double sigma, double fitMag, double apMag, double err1, double err2);

int main (void)
{

    pmModelGroupInit ();

    plan_tests(240);

    diag("pmSourcePhotometry tests");

    // test consistency of interpolated photometry for a range of apertures (for a fixed PSF sigma)
    pmSourcePhotometry_TestOffsets (15.0, 2.0, -10.3759, -10.3759, +0.0000, +0.0000);
    pmSourcePhotometry_TestOffsets (10.0, 2.0, -10.3759, -10.3759, +0.0000, +0.0000);
    pmSourcePhotometry_TestOffsets ( 8.0, 2.0, -10.3759, -10.3759, +0.0000, +0.0000);
    pmSourcePhotometry_TestOffsets ( 7.0, 2.0, -10.3759, -10.3759, +0.0000, +0.0001);
    pmSourcePhotometry_TestOffsets ( 6.0, 2.0, -10.3759, -10.3758, +0.0001, +0.0004);
    pmSourcePhotometry_TestOffsets ( 5.0, 2.0, -10.3759, -10.3733, +0.0003, +0.0011);
    pmSourcePhotometry_TestOffsets ( 4.0, 2.0, -10.3759, -10.3520, +0.0006, +0.0018);
    pmSourcePhotometry_TestOffsets ( 3.0, 2.0, -10.3759, -10.2626, +0.0001, +0.0002);
    pmSourcePhotometry_TestOffsets ( 2.0, 2.0, -10.3759,  -9.7729, -0.0027, -0.0089);
    pmSourcePhotometry_TestOffsets ( 1.0, 2.0, -10.3759,  -8.8689, -0.0051, -0.0161);

    // test consistency of interpolated photometry for a range of apertures (for a fixed PSF sigma)
    pmSourcePhotometry_TestOffsets (15.0, 1.5, -10.3759, -10.3759, +0.0000, +0.0000);
    pmSourcePhotometry_TestOffsets (10.0, 1.5, -10.3759, -10.3759, +0.0000, +0.0000);
    pmSourcePhotometry_TestOffsets ( 8.0, 1.5, -10.3759, -10.3759, +0.0000, +0.0000);
    pmSourcePhotometry_TestOffsets ( 7.0, 1.5, -10.3759, -10.3759, +0.0000, +0.0001);
    pmSourcePhotometry_TestOffsets ( 6.0, 1.5, -10.3759, -10.3758, +0.0001, +0.0004);
    pmSourcePhotometry_TestOffsets ( 5.0, 1.5, -10.3759, -10.3733, +0.0003, +0.0011);
    pmSourcePhotometry_TestOffsets ( 4.0, 1.5, -10.3759, -10.3520, +0.0006, +0.0018);
    pmSourcePhotometry_TestOffsets ( 3.0, 1.5, -10.3759, -10.2626, +0.0001, +0.0002);
    pmSourcePhotometry_TestOffsets ( 2.0, 1.5, -10.3759,  -9.7729, -0.0027, -0.0089);
    pmSourcePhotometry_TestOffsets ( 1.0, 1.5, -10.3759,  -8.8689, -0.0051, -0.0161);

    // test consistency of interpolated photometry for a range of apertures (for a fixed PSF sigma)
    pmSourcePhotometry_TestOffsets (15.0, 1.0,  -9.4955,  -9.4955, +0.0000, +0.0000);
    pmSourcePhotometry_TestOffsets (10.0, 1.0,  -9.4955,  -9.4955, +0.0000, +0.0000);
    pmSourcePhotometry_TestOffsets ( 8.0, 1.0,  -9.4955,  -9.4955, +0.0000, +0.0000);
    pmSourcePhotometry_TestOffsets ( 7.0, 1.0,  -9.4955,  -9.4955, +0.0000, +0.0000);
    pmSourcePhotometry_TestOffsets ( 6.0, 1.0,  -9.4955,  -9.4955, +0.0000, +0.0001);
    pmSourcePhotometry_TestOffsets ( 5.0, 1.0,  -9.4955,  -9.4955, +0.0001, +0.0002);
    pmSourcePhotometry_TestOffsets ( 4.0, 1.0,  -9.4955,  -9.4968, +0.0006, +0.0022);
    pmSourcePhotometry_TestOffsets ( 3.0, 1.0,  -9.4955,  -9.4945, +0.0021, +0.0068);
    pmSourcePhotometry_TestOffsets ( 2.0, 1.0,  -9.4955,  -9.3323, -0.0034, -0.0118);
    pmSourcePhotometry_TestOffsets ( 1.0, 1.0,  -9.4955,  -8.6844, -0.0141, -0.0440);

    return exit_status();
}

bool pmSourcePhotometry_TestOffsets (double radius, double sigma, double fitMag, double apMag, double err1, double err2)
{
    psMemId id = psMemGetId();

    diag("pmSourcePhotometry test offsets for radius %f", radius);

    // generate a simple readout
    pmReadout *readout = pmReadoutAlloc (NULL);
    skip_start(readout == NULL, 0, "Skipping tests because pmReadoutAlloc failed");

    readout->image = psImageAlloc (64, 64, PS_TYPE_F32);
    readout->mask  = psImageAlloc (64, 64, PS_TYPE_U8);

    // create an empty reference image
    psImageInit (readout->image, 0.0);
    psImageInit (readout->mask, 0);

    // generate a simple psf
    pmPSF *psf = pmPSFBuildSimple ("PS_MODEL_GAUSS", sigma, sigma, 0.0);
    // psf->growth = pmGrowthCurveAlloc (2.0, 100.0, 15.0);
    // pmGrowthCurveGenerate (readout, psf, false);

    // create a source
    pmSource *source = pmSourceAlloc ();
    source->pixels = psMemIncrRefCounter (readout->image);
    source->mask   = psMemIncrRefCounter (readout->mask);
    source->type   = PM_SOURCE_TYPE_STAR;
    source->mode   = PM_SOURCE_MODE_SUBTRACTED;

    // create template model and measure apMag at fractional offsets
    pmModel *modelRef = pmModelAlloc(psf->type);
    modelRef->params->data.F32[PM_PAR_SKY] = 0;
    modelRef->params->data.F32[PM_PAR_I0] = 1000;
    modelRef->params->data.F32[PM_PAR_XPOS] = 32.5;
    modelRef->params->data.F32[PM_PAR_YPOS] = 32.5;

    // create modelPSF from this model
    source->modelPSF = pmModelFromPSF (modelRef, psf);
    source->modelPSF->dparams->data.F32[PM_PAR_I0] = 1;
    source->apRadius = radius;

    // measure photometry for centered source (fractional pix : 0.5,0.5)
    // pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_GROWTH | PM_SOURCE_PHOT_INTERP);
    pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_INTERP);
    ok_float_tol(source->psfMag, fitMag, 0.0002, "source fitMag is %f", source->psfMag);
    ok_float_tol(source->apMag,  apMag, 0.0002, "source apMag is %f", source->apMag);
    ok_float(source->psfMagErr,   0.001, "source psfMagErr is %f", source->psfMagErr);
    float refMag = source->apMag;

    // these use an offset of 0.2,0.2
    // measure photometry for a sub-pixel offset position
    source->modelPSF->params->data.F32[PM_PAR_XPOS] = 32.3;
    source->modelPSF->params->data.F32[PM_PAR_YPOS] = 32.3;
    pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_INTERP);
    ok_float_tol(refMag - source->apMag,  err1, 0.0002, "offset error is %f", refMag - source->apMag);

    // measure photometry for a sub-pixel offset position
    source->modelPSF->params->data.F32[PM_PAR_XPOS] = 32.7;
    source->modelPSF->params->data.F32[PM_PAR_YPOS] = 32.3;
    pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_INTERP);
    ok_float_tol(refMag - source->apMag,  err1, 0.0002, "offset error is %f", refMag - source->apMag);

    // measure photometry for a sub-pixel offset position
    source->modelPSF->params->data.F32[PM_PAR_XPOS] = 32.3;
    source->modelPSF->params->data.F32[PM_PAR_YPOS] = 32.7;
    pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_INTERP);
    ok_float_tol(refMag - source->apMag,  err1, 0.0002, "offset error is %f", refMag - source->apMag);

    // measure photometry for a sub-pixel offset position
    source->modelPSF->params->data.F32[PM_PAR_XPOS] = 32.7;
    source->modelPSF->params->data.F32[PM_PAR_YPOS] = 32.7;
    pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_INTERP);
    ok_float_tol(refMag - source->apMag,  err1, 0.0002, "offset error is %f", refMag - source->apMag);

    // these use an offset of 0.4,0.4
    // measure photometry for a sub-pixel offset position
    source->modelPSF->params->data.F32[PM_PAR_XPOS] = 32.1;
    source->modelPSF->params->data.F32[PM_PAR_YPOS] = 32.1;
    pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_INTERP);
    ok_float_tol(refMag - source->apMag,  err2, 0.0002, "offset error is %f", refMag - source->apMag);

    // measure photometry for a sub-pixel offset position
    source->modelPSF->params->data.F32[PM_PAR_XPOS] = 32.9;
    source->modelPSF->params->data.F32[PM_PAR_YPOS] = 32.1;
    pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_INTERP);
    ok_float_tol(refMag - source->apMag,  err2, 0.0002, "offset error is %f", refMag - source->apMag);

    // measure photometry for a sub-pixel offset position
    source->modelPSF->params->data.F32[PM_PAR_XPOS] = 32.1;
    source->modelPSF->params->data.F32[PM_PAR_YPOS] = 32.9;
    pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_INTERP);
    ok_float_tol(refMag - source->apMag,  err2, 0.0002, "offset error is %f", refMag - source->apMag);

    // measure photometry for a sub-pixel offset position
    source->modelPSF->params->data.F32[PM_PAR_XPOS] = 32.9;
    source->modelPSF->params->data.F32[PM_PAR_YPOS] = 32.9;
    pmSourceMagnitudes (source, psf, PM_SOURCE_PHOT_INTERP);
    ok_float_tol(refMag - source->apMag,  err2, 0.0002, "offset error is %f", refMag - source->apMag);

    psFree (source);
    psFree (modelRef);
    psFree (psf);
    psFree (readout);

    skip_end();

    ok(!psMemCheckLeaks (id, NULL, stdout, false), "no memory leaks");
    return true;
}
