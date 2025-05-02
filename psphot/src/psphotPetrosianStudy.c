# include "psphotInternal.h"

# define DX 512
# define DY 512

// XXX add noise and seeing.
// XXX double check on sersic functional form
// XXX modify ratio if ratio > 1.0 (swap major and minor)

pmPeak *psphotLocalPeak(pmReadout *readout, int Xo, int Yo);

int main (int argc, char **argv) {

  pmErrorRegister();                  // register psModule's error codes/messages
  pmModelClassInit();

  int N;
  float Xo = 0.5*DX;
  float Yo = 0.5*DY;
  char *image = NULL;
  pmSource *source = NULL;

  float peak = 1000.0;
  float sigma = 2.0;	      // major axis size 
  float ARatio = 1.0;
  float angle = 0.0;
  float sersic = 0.5;
  float skynoise = 0.0;
  
  if ((N = psArgumentGet (argc, argv, "-peak"))) {
    psArgumentRemove (N, &argc, argv);
    peak = atof(argv[N]);
    psArgumentRemove (N, &argc, argv);
  }
  if ((N = psArgumentGet (argc, argv, "-sigma"))) {
    psArgumentRemove (N, &argc, argv);
    sigma = atof(argv[N]);
    psArgumentRemove (N, &argc, argv);
  }
  if ((N = psArgumentGet (argc, argv, "-aratio"))) {
    psArgumentRemove (N, &argc, argv);
    ARatio = atof(argv[N]);
    psArgumentRemove (N, &argc, argv);
  }
  if ((N = psArgumentGet (argc, argv, "-angle"))) {
    psArgumentRemove (N, &argc, argv);
    angle = PS_RAD_DEG*atof(argv[N]);
    psArgumentRemove (N, &argc, argv);
  }
  if ((N = psArgumentGet (argc, argv, "-sersic"))) {
    psArgumentRemove (N, &argc, argv);
    sersic = atof(argv[N]);
    psArgumentRemove (N, &argc, argv);
  }
  if ((N = psArgumentGet (argc, argv, "-skynoise"))) {
    psArgumentRemove (N, &argc, argv);
    skynoise = atof(argv[N]);
    psArgumentRemove (N, &argc, argv);
  }
  if ((N = psArgumentGet (argc, argv, "-visual"))) {
    psArgumentRemove (N, &argc, argv);
    pmVisualSetVisual(true);
  }
  if ((N = psArgumentGet (argc, argv, "-coords"))) {
    psArgumentRemove (N, &argc, argv);
    Xo = atof(argv[N]);
    psArgumentRemove (N, &argc, argv);
    Yo = atof(argv[N]);
    psArgumentRemove (N, &argc, argv);
  }
  if ((N = psArgumentGet (argc, argv, "-image"))) {
    psArgumentRemove (N, &argc, argv);
    image = psStringCopy (argv[N]);
    psArgumentRemove (N, &argc, argv);
  }

  if (argc != 1) {
    fprintf (stderr, "USAGE: psphotPetrosianStudy\n");
    exit (2);
  }

  // create a containing image & associated readout
  pmReadout *readout = pmReadoutAlloc(NULL);
  if (!image) {
      readout->image = psImageAlloc(DX, DY, PS_TYPE_F32);

      // create a dummy variance, but don't populate -- it is not used by pmSourceMoments if the sigma parameters is set to 0.0
      readout->variance = psImageAlloc(DX, DY, PS_TYPE_F32);

      // create a model & associated source
      pmModelType type = pmModelClassGetType("PS_MODEL_SERSIC");
      pmModel *model = pmModelAlloc(type);

      // set the model parameters
      model->params->data.F32[PM_PAR_SKY]  = 0.0;
      model->params->data.F32[PM_PAR_I0]   = peak;
      model->params->data.F32[PM_PAR_XPOS] = Xo;
      model->params->data.F32[PM_PAR_YPOS] = Yo;

      psEllipseAxes axes;
      axes.major = sigma;
      axes.minor = sigma*ARatio;
      axes.theta = angle;

      psEllipseShape shape = psEllipseAxesToShape (axes);

      // XXX set the sigma with user input
      model->params->data.F32[PM_PAR_SXX]  = shape.sx * M_SQRT2;
      model->params->data.F32[PM_PAR_SYY]  = shape.sy * M_SQRT2;
      model->params->data.F32[PM_PAR_SXY]  = shape.sxy;

      if (model->params->n > 7) {
	  model->params->data.F32[PM_PAR_7]  = sersic;
      }

      // generate source container & populate image
      source = pmSourceFromModel(model, readout, Xo, PM_SOURCE_TYPE_STAR);

      // generate the modelFlux 
      pmSourceCacheModel(source, 0);

      // instantiate the source
      pmSourceAdd(source, PM_MODEL_OP_FUNC, 0); 

      // XXX add noise here...
      psphotSaveImage(NULL, readout->image, "sersic.fits");

  } else {
      psRegion full = psRegionSet(0,0,0,0);
      psFits *fits = psFitsOpen(image, "r");
      readout->image = psFitsReadImage(fits, full, 0);

      source = pmSourceAlloc();
      source->peak = psphotLocalPeak(readout, Xo, Yo);
      pmSourceDefinePixels (source, readout, Xo, Yo, 128);
  }

  psphotPetrosianProfile (readout, source, skynoise);

  psFree (source);

  exit (0);
}

// Xo, Yo are in parent coords
pmPeak *psphotLocalPeak(pmReadout *readout, int Xo, int Yo) {

    int Xp = Xo;
    int Yp = Yo;
    float peakFlux = readout->image->data.F32[Yp][Xp];

    // find local peak within +/- 3 pix of the given coordinate
    for (int iy = Yo - 3; iy <= Yo + 3; iy++) {
	for (int ix = Xo - 3; ix <= Xo + 3; ix++) {
	    if (peakFlux < readout->image->data.F32[iy][ix]) {
		Xp = ix;
		Yp = iy;
		peakFlux = readout->image->data.F32[Yp][Xp];
	    }
	}
    }

    pmPeak *peak = pmPeakAlloc(Xp, Yp, peakFlux, PM_PEAK_LONE);

    // calculate fractional peak position relative to Xp,Yp
    psPolynomial2D *bicube = psImageBicubeFit (readout->image, Xp, Yp);
    psPlane min = psImageBicubeMin (bicube);
    psFree (bicube);

    // if min point is too deviant, use the peak value
    if ((fabs(min.x) < 1.5) && (fabs(min.y) < 1.5)) {
        peak->xf = min.x + Xp;
        peak->yf = min.y + Yp;

	// xf,yf must land on image with 0 pixel border
	peak->xf = PS_MAX (PS_MIN (peak->xf, readout->image->numCols - 1), readout->image->col0);
	peak->yf = PS_MAX (PS_MIN (peak->yf, readout->image->numRows - 1), readout->image->row0);
    } else {
        peak->xf = Xp;
        peak->yf = Yp;
    }

    return peak;
}
