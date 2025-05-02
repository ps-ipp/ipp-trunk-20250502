#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"

int main(int argc, char **argv) {

  psLogSetFormat("HLNM");
  psLogSetLevel(PS_LOG_INFO);
  psTraceSetLevel("err", 5);
  plan_tests(53);

  // call pmModelAlloc() with acceptable input params
  {
    psMemId id = psMemGetId();
    pmModelClassInit();
    
    pmModelType type = pmModelClassGetType ("PS_MODEL_PS1_V1");
    ok(type == 3, "pmModelClassGetType returned the right value");
    
    pmModel *model = pmModelAlloc(type);
    ok(model != NULL && psMemCheckModel(model), "pmModelAlloc() returned a non-NULL pmModel");

    int nParams = pmModelClassParameterCount(type);
    ok(model->params != NULL && model->params->n == nParams, "pmModelAlloc() set the pmModel->params psVector correctly");
    ok(model->dparams != NULL && model->dparams->n == nParams, "pmModelAlloc() set the pmModel->dparams psVector correctly");

    float core;
    for (core = -1.33; core < 15.0; core += 0.33) {
      model->params->data.F32[PM_PAR_7] = core;
      float fwhm = model->class->modelSetFWHM(model->params, 1.0);
      fprintf (stderr, "%f : %f\n", core, fwhm);
    }
    psFree(model);
    pmModelClassCleanup();
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
  }
}


