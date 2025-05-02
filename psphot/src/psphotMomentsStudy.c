# include "psphotInternal.h"

int SetUpKapa (Graphdata *graphdata);
int PlotVectors (int kapa, Graphdata *graphdata, psVector *inVector, psVector *outVector);
int PlotModelSet (int kapa, Graphdata *graphdata, pmReadout *readout, char *name, float par7);

FILE *output = NULL;

int main (int argc, char **argv) {

  Graphdata graphdata;
  int kapa;

  pmErrorRegister();                  // register psModule's error codes/messages
  pmModelClassInit();

  // loop over sigma_window
  // loop over sigma_input 
  // generate a fake source using the desired model
  // add noise?
  // measure the moments for the object
  // accumulate Mxx, Myy

  // plot sigma_input vs sigma_output == sqrt(Mxx)

  // pmSourceMoments requires a source with a peak and pixels

  if (argc != 2) {
    fprintf (stderr, "USAGE: psphotMomentsStudy (output)\n");
    exit (2);
  }

  output = fopen (argv[1], "w");
  if (!output) exit (1);
  fprintf (output, "# %4s %5s  %6s %6s %6s\n", "type", "par7", "sigWin", "sigIn", "sigOut");

  kapa = SetUpKapa(&graphdata);

  // create a containing image & associated readout
  pmReadout *readout = pmReadoutAlloc(NULL);
  readout->image = psImageAlloc(128, 128, PS_TYPE_F32);

  // create a dummy variance, but don't populate -- it is not used by pmSourceMoments if the sigma parameters is set to 0.0
  readout->variance = psImageAlloc(128, 128, PS_TYPE_F32);

  PlotModelSet (kapa, &graphdata, readout, "PS_MODEL_GAUSS",  0.0);
  PlotModelSet (kapa, &graphdata, readout, "PS_MODEL_PGAUSS", 0.0);

  PlotModelSet (kapa, &graphdata, readout, "PS_MODEL_PS1_V1", 0.3);
  PlotModelSet (kapa, &graphdata, readout, "PS_MODEL_PS1_V1", 1.0);
  PlotModelSet (kapa, &graphdata, readout, "PS_MODEL_PS1_V1", 3.0);

  PlotModelSet (kapa, &graphdata, readout, "PS_MODEL_SERSIC", 0.125);
  PlotModelSet (kapa, &graphdata, readout, "PS_MODEL_SERSIC", 0.250);
  PlotModelSet (kapa, &graphdata, readout, "PS_MODEL_SERSIC", 0.500);

  // pause and wait for user input:
  // continue, save (provide name), ??
  char key[10];
  fprintf (stdout, "[c]ontinue? ");
  if (!fgets(key, 8, stdin)) {
    psWarning("Unable to read option");
  }
  fclose (output);

  // psphotSaveImage(NULL, readout->image, "test.fits");
  exit (0);
}

int SetUpKapa (Graphdata *graphdata) {

  int kapa = KapaOpenNamedSocket ("kapa", "psphot");

  KapaClearPlots (kapa);
  KapaInitGraph (graphdata);
  KapaSetFont (kapa, "courier", 14);

  graphdata->color = KapaColorByName ("black");
  graphdata->xmin = -0.1;
  graphdata->xmax =  1.6;
  graphdata->ymin = -0.1;
  graphdata->ymax =  1.6;
  // graphdata->xmax = 10.1;
  // graphdata->ymax = 10.1;
  KapaSetLimits (kapa, graphdata);

  KapaBox (kapa, graphdata);
  // KapaSendLabel (kapa, "&ss&h_in| (pixels)", KAPA_LABEL_XM);
  // KapaSendLabel (kapa, "&ss&h_out| (pixels)", KAPA_LABEL_YM);
  KapaSendLabel (kapa, "&ss&h_out| / &ss&h_window|", KAPA_LABEL_XM);
  KapaSendLabel (kapa, "&ss&h_out| / &ss&h_in|", KAPA_LABEL_YM);

  return (kapa);
}

int PlotVectors (int kapa, Graphdata *graphdata, psVector *inVector, psVector *outVector) {

  graphdata->color = KapaColorByName ("black");
  graphdata->ptype = 2;
  graphdata->size = 1.0;
  graphdata->style = 2;
  KapaPrepPlot (kapa, inVector->n, graphdata);
  KapaPlotVector (kapa, inVector->n, inVector->data.F32, "x");
  KapaPlotVector (kapa, inVector->n, outVector->data.F32, "y");

  return 0;
}


int PlotModelSet (int kapa, Graphdata *graphdata, pmReadout *readout, char *name, float par7) {

  // create a model & associated source
  // pmModelType type = pmModelClassGetType("PS_MODEL_GAUSS");

  pmModelType type = pmModelClassGetType(name);
  pmModel *model = pmModelAlloc(type);

  // set the model parameters
  model->params->data.F32[PM_PAR_SKY]  = 0.0;
  model->params->data.F32[PM_PAR_I0]   = 100.0;
  model->params->data.F32[PM_PAR_XPOS] = 64.0;
  model->params->data.F32[PM_PAR_YPOS] = 64.0;

  model->params->data.F32[PM_PAR_SXX]  = 1.0 * M_SQRT2;
  model->params->data.F32[PM_PAR_SYY]  = 1.0 * M_SQRT2;
  model->params->data.F32[PM_PAR_SXY]  = 0.0;

  if (model->params->n > 7) {
    model->params->data.F32[PM_PAR_7]  = par7;
  }

  pmSource *source = pmSourceFromModel(model, readout, 32.0, PM_SOURCE_TYPE_STAR);

  psVector *inVector  = psVectorAllocEmpty(100, PS_TYPE_F32);
  psVector *outVector = psVectorAllocEmpty(100, PS_TYPE_F32);

  for (float sigWindow = 1.0; sigWindow < 10.0; sigWindow += 1.0) {

    for (float sigIn = 0.5; sigIn < 10.0; sigIn += 0.1) {

      model->params->data.F32[PM_PAR_SXX]  = sigIn * M_SQRT2;
      model->params->data.F32[PM_PAR_SYY]  = sigIn * M_SQRT2;

      // generate the modelFlux 
      pmSourceCacheModel(source, 0);

      // instantiate the source
      pmSourceAdd(source, PM_MODEL_OP_FUNC, 0); 

      pmSourceMoments (source, 32.0, sigWindow, 0.0, 0xffff);
      // fprintf (stderr, "sigOut : %f\n", sqrt(source->moments->Mxx));

      psVectorAppend(inVector,  sqrt(source->moments->Mxx) / sigWindow);
      psVectorAppend(outVector, sqrt(source->moments->Mxx) / sigIn);

      psImageInit (readout->image, 0.0);

      fprintf (output, "  %4d %5.2f  %6.3f %6.3f %6.3f\n", type, par7, sigWindow, sigIn, sqrt(source->moments->Mxx));
    }    
  }

  PlotVectors (kapa, graphdata, inVector, outVector);
  inVector->n = 0;
  outVector->n = 0;

  psFree (source);
  psFree (inVector);
  psFree (outVector);

  return 1;
}
