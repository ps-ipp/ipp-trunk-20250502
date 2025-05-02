/** Diagnostic plots for relastro
 * @author Chris Beaumont, IfA
 */

#include "relastro.h"

#define TESTING

#define KAPAX 700
#define KAPAY 700

static int kapa1 = -1;
static int kapa2 = -1;
static int kapa3 = -1;
static int kapa4 = -1;

static int isVisual = FALSE;
static int plotRawRef = TRUE;
// static int plotScatter = TRUE;
// static int plotResid = TRUE;
// static int plotVector = TRUE;
static int plotOutliers = TRUE;

/** Spawn a kapa window */
static int initWindow(int *kapid) {
  if (NTHREADS) {
    fprintf (stderr, "-visual and -threads are incompatible, quitting\n");
    exit (2);
  }
  if (*kapid == -1) {
    *kapid = KapaOpenNamedSocket("kapa", "relastro_plots");
    if (*kapid == -1) {
      fprintf(stderr, "Failure to open kapa.\n");
      isVisual = 0;
      return FALSE;
    }
  }
  return TRUE;
}

/** Ask the user how to proceed */
static int askUser(int *flag) {
  char key[10];
  fprintf (stdout, "[c]ontinue? [s]kip the rest of these plots? [a]bort all visual plots?");
  if (!fgets(key, 8, stdin)) {
    fprintf(stderr, "warning: Unable to read option");
  }
  if (key[0] == 's') {
    *flag = 0;
  }
  if (key[0] == 'a') {
    isVisual = 0;
  }
  return TRUE;
}

void relastroSetVisual(int state) {
  isVisual = state;
}

int relastroGetVisual(void) {
  return isVisual;
}

/** Size graphdata to encompass all points */
static int scaleGraphdata(float x[], float y[], Graphdata *graphdata, int n) {
    float xlo = FLT_MAX, xhi = -FLT_MAX, ylo = FLT_MAX, yhi = -FLT_MAX;
    int i;
    int goodData = 0;
    for(i = 0; i < n; i++) {
        goodData++;
        if(!finite(x[i]) || !finite(y[i])) continue;
        if(x[i] < xlo) xlo = x[i];
        if(x[i] > xhi) xhi = x[i];
        if(y[i] < ylo) ylo = y[i];
        if(y[i] > yhi) yhi = y[i];
    }
    if (goodData < 2) return 0;
    graphdata->xmin = xlo;
    graphdata->ymin = ylo;
    graphdata->xmax = xhi;
    graphdata->ymax = yhi;
    return TRUE;
}

/** 4-panel plot of x vs dx, y vs dx, x vs dy, y vs dy **/
int relastroVisualRawRef(int *kapaID, float *x, float *y, float *dx, float *dy, float *dPos, int npts) {

    Graphdata graphdata;
    KapaSection section;

    if (!initWindow(kapaID)) return FALSE;

    KapaInitGraph(&graphdata);
    KapaClearSections(*kapaID);
    KapaSetFont(*kapaID, "helvetica", 14);

    section.name = "0";
    section.x = 0.0; section.y = 0.0;
    section.dx = .45, section.dy = .45;
    section.bg = KapaColorByName("white");
    graphdata.ptype = 7;
    graphdata.style = 2;
    graphdata.etype |= 0x01;

    KapaSetSection(*kapaID, &section);
    if(!scaleGraphdata(x, dx, &graphdata, npts)) return 0;
    KapaSetLimits(*kapaID, &graphdata);
    KapaBox(*kapaID, &graphdata);
    KapaSendLabel(*kapaID, "x", KAPA_LABEL_XM);
    KapaSendLabel(*kapaID, "dx", KAPA_LABEL_YM);
    KapaPrepPlot(*kapaID, npts, &graphdata);
    KapaPlotVector(*kapaID, npts, x, "x");
    KapaPlotVector(*kapaID, npts, dx, "y");
    KapaPlotVector(*kapaID, npts, dPos, "dym");
    KapaPlotVector(*kapaID, npts, dPos, "dyp");

    section.x = .5; section.y = 0; section.name="1";
    KapaSetSection(*kapaID, &section);
    if(!scaleGraphdata(x, dy, &graphdata, npts)) return 0;
    KapaSetLimits(*kapaID, &graphdata);
    KapaBox(*kapaID, &graphdata);
    KapaSendLabel(*kapaID, "x", KAPA_LABEL_XM);
    KapaSendLabel(*kapaID, "dy", KAPA_LABEL_YP);
    KapaPrepPlot(*kapaID, npts, &graphdata);
    KapaPlotVector(*kapaID, npts, x, "x");
    KapaPlotVector(*kapaID, npts, dy, "y");
    KapaPlotVector(*kapaID, npts, dPos, "dym");
    KapaPlotVector(*kapaID, npts, dPos, "dyp");

    section.x = .0; section.y = .5; section.name="2";
    KapaSetSection(*kapaID, &section);
    if(!scaleGraphdata(y, dx, &graphdata, npts)) return 0;;
    KapaSetLimits(*kapaID, &graphdata);
    KapaBox(*kapaID, &graphdata);
    KapaSendLabel(*kapaID, "y", KAPA_LABEL_XM);
    KapaSendLabel(*kapaID, "dx", KAPA_LABEL_YM);
    KapaPrepPlot(*kapaID, npts, &graphdata);
    KapaPlotVector(*kapaID, npts, y, "x");
    KapaPlotVector(*kapaID, npts, dx, "y");
    KapaPlotVector(*kapaID, npts, dPos, "dym");
    KapaPlotVector(*kapaID, npts, dPos, "dyp");

    section.x = .5; section.y = .5; section.name="3";
    KapaSetSection(*kapaID, &section);
    if(!scaleGraphdata(y, dy, &graphdata, npts)) return 0;
    KapaSetLimits(*kapaID, &graphdata);
    KapaBox(*kapaID, &graphdata);
    KapaSendLabel(*kapaID, "y", KAPA_LABEL_XM);
    KapaSendLabel(*kapaID, "dy", KAPA_LABEL_YP);
    KapaPrepPlot(*kapaID, npts, &graphdata);
    KapaPlotVector(*kapaID, npts, y, "x");
    KapaPlotVector(*kapaID, npts, dy, "y");
    KapaPlotVector(*kapaID, npts, dPos, "dym");
    KapaPlotVector(*kapaID, npts, dPos, "dyp");

    return TRUE;
}


/** Plot a vector field (circles at vector origin, scaled lines giving vector directions */
int relastroVisualVectorField(int *kapaID, float *x, float *y, float *dx, float *dy, int npts, double maxVecLength) {

    Graphdata graphdata;
    float *xVec, *yVec;
    float vecScaleFactor;
    float graphSize;
    int i;
    char plotTitle[50];

    if (!npts) return FALSE;
    if (!initWindow(kapaID)) return 0;

    snprintf(plotTitle, 50, "Maximum Vector Size = %5.1e", maxVecLength);

    KapaInitGraph(&graphdata);
    KapaClearSections(*kapaID);
    KapaSetFont(*kapaID, "helvetica", 14);

    if (!scaleGraphdata(x, y, &graphdata, npts)) return 0;

    graphSize = graphdata.xmax - graphdata.xmin;
    if ((graphdata.ymax - graphdata.ymin) > graphSize) {
        graphSize = graphdata.ymax - graphdata.ymin;
    }
    vecScaleFactor = graphSize * 0.02 / (float)maxVecLength;

    // fprintf(stderr, "GraphSize: %e\n maxVecLength: %e\n vecScaleFactor: %e\n", graphSize, maxVecLength, vecScaleFactor);

    KapaSetFont (*kapaID, "helvetica", 14);
    KapaSetLimits(*kapaID, &graphdata);
    KapaBox(*kapaID, &graphdata);
    KapaSendLabel(*kapaID, plotTitle, KAPA_LABEL_XP);

    graphdata.ptype = 7;
    graphdata.style = 2;
    graphdata.color = KapaColorByName("black");
    KapaPrepPlot(*kapaID, npts, &graphdata);
    KapaPlotVector(*kapaID, npts, x, "x");
    KapaPlotVector(*kapaID, npts, y, "y");

    ALLOCATE (xVec, float, 2*npts);
    ALLOCATE (yVec, float, 2*npts);
    for (i = 0; i < npts; i++) {
      xVec[2*i + 0] = x[i];
      yVec[2*i + 0] = y[i];
      xVec[2*i + 1] = x[i] + dx[i] * vecScaleFactor;
      yVec[2*i + 1] = y[i] + dy[i] * vecScaleFactor;
    }

    graphdata.ptype = 100; // line segements by point pair
    graphdata.style = 2;
    graphdata.color = KapaColorByName("blue");
    KapaPrepPlot  (*kapaID, 2*npts, &graphdata);
    KapaPlotVector(*kapaID, 2*npts, xVec, "x");
    KapaPlotVector(*kapaID, 2*npts, yVec, "y");

    free (xVec);
    free (yVec);
    return TRUE;
}

int relastroVisualPlotScatter(int *kapaID, float *dXfit, float *dYfit, float *mag, int npts) {

    Graphdata graphdata;
    KapaSection section;

    if (!initWindow(kapaID)) return 0;

    KapaInitGraph(&graphdata);
    KapaClearSections(*kapaID);
    KapaSetFont(*kapaID, "helvetica", 14);

    section.name = "a";
    section.x = 0.0; section.y = 0.0; section.dx = 1.0; section.dy = 0.5;
    section.bg = KapaColorByName("white");
    KapaSetSection(*kapaID, &section);

    graphdata.ptype = 2;
    graphdata.style = 2;

    if(!scaleGraphdata(mag, dXfit, &graphdata, npts)) return 0;
    KapaSetLimits(*kapaID, &graphdata);
    KapaBox(*kapaID, &graphdata);
    KapaSendLabel(*kapaID, "mag", KAPA_LABEL_XM);
    KapaSendLabel(*kapaID, "dXfit", KAPA_LABEL_YM);
    KapaPrepPlot(*kapaID, npts, &graphdata);
    KapaPlotVector(*kapaID, npts, mag, "x");
    KapaPlotVector(*kapaID, npts, dXfit, "y");

    section.name = "b";
    section.x = 0.0; section.y = 0.5; section.dx = 1.0; section.dy = 0.5;
    section.bg = KapaColorByName("white");
    KapaSetSection(*kapaID, &section);

    graphdata.ptype = 2;
    graphdata.style = 2;

    if(!scaleGraphdata(mag, dYfit, &graphdata, npts)) return 0;
    KapaSetLimits(*kapaID, &graphdata);
    KapaBox(*kapaID, &graphdata);
    KapaSendLabel(*kapaID, "mag", KAPA_LABEL_XM);
    KapaSendLabel(*kapaID, "dYfit", KAPA_LABEL_YM);
    KapaPrepPlot(*kapaID, npts, &graphdata);
    KapaPlotVector(*kapaID, npts, mag, "x");
    KapaPlotVector(*kapaID, npts, dYfit, "y");

    return TRUE;
}

/** plot raw vs ref (L, M). Only those whose distance is < drMax are used in fit*/
int relastroVisualPlotFittedStars(int *kapaID, float *rawX, float *rawY, float *refX, float *refY, int numNoFit, float *rawXfit, float *rawYfit, float *refXfit, float *refYfit, int numFit) {

  Graphdata graphdata;

  if (!initWindow(kapaID)) return 0;

  KapaInitGraph(&graphdata);
  KapaClearPlots(*kapaID);

  graphdata.ptype = 7;
  graphdata.style = 2;

  if(!scaleGraphdata(rawXfit, rawYfit, &graphdata, numFit)) {
      fprintf(stderr, "Not enough finite points for plotting");
      return 0;
  }

  KapaSetFont(*kapaID, "helvetica", 14);
  KapaSetLimits(*kapaID, &graphdata);
  KapaBox(*kapaID, &graphdata);
  KapaSendLabel( *kapaID, "X", KAPA_LABEL_XM);
  KapaSendLabel( *kapaID, "Y", KAPA_LABEL_YM);
  KapaSendLabel( *kapaID, "orange, red, green, blue: (raw, ref), (nofit, fit)",
                 KAPA_LABEL_XP);

  graphdata.color = KapaColorByName("orange");
  graphdata.size = 1;
  KapaPrepPlot(*kapaID, numNoFit, &graphdata);
  KapaPlotVector(*kapaID, numNoFit, rawX, "x");
  KapaPlotVector(*kapaID, numNoFit, rawY, "y");

  graphdata.color = KapaColorByName("red");
  graphdata.size = 2;
  KapaPrepPlot(*kapaID, numNoFit, &graphdata);
  KapaPlotVector(*kapaID, numNoFit, refX, "x");
  KapaPlotVector(*kapaID, numNoFit, refY, "y");

  graphdata.color = KapaColorByName("green");
  graphdata.size = 1;
  KapaPrepPlot(*kapaID, numFit,  &graphdata);
  KapaPlotVector(*kapaID, numFit, rawXfit, "x");
  KapaPlotVector(*kapaID, numFit, rawYfit, "y");

  graphdata.color = KapaColorByName("blue");
  graphdata.size = 2;
  KapaPrepPlot(*kapaID, numFit, &graphdata);
  KapaPlotVector(*kapaID, numFit, refXfit, "x");
  KapaPlotVector(*kapaID, numFit, refYfit, "y");

  return TRUE;
}

/** create various plots using the data in raw and ref **/
int relastroVisualPlotChipFit(StarData *raw, StarData *ref, double dRmax, int numObj) {

  float *rawX, *rawY,  *refX, *refY;
  float *rawXfit, *rawYfit, *refXfit, *refYfit;
  float *magRaw, *magRef, *magRawfit, *magReffit;
  float *dXfit, *dYfit, *dPos;
  int numFit = 0, numNoFit = 0;
  double dL, dM, dR;

  if (!isVisual) return TRUE;

  ALLOCATE(rawX,      float, numObj);
  ALLOCATE(rawY,      float, numObj);
  ALLOCATE(refX,      float, numObj);
  ALLOCATE(refY,      float, numObj);
  ALLOCATE(magRaw,    float, numObj);
  ALLOCATE(magRef,    float, numObj);

  ALLOCATE(rawXfit,   float, numObj);
  ALLOCATE(rawYfit,   float, numObj);
  ALLOCATE(refXfit,   float, numObj);
  ALLOCATE(refYfit,   float, numObj);
  ALLOCATE(magRawfit, float, numObj);
  ALLOCATE(magReffit, float, numObj);
  ALLOCATE(dXfit,     float, numObj);
  ALLOCATE(dYfit,     float, numObj);
  ALLOCATE(dPos,      float, numObj);

  int i;
  for (i = 0; i < numObj; i++) {
    if (raw[i].mask) continue; // XXX 

    dL = raw[i].L - ref[i].L;
    dM = raw[i].M - ref[i].M;
    dR = hypot (dL, dM);

    // XXX change the selection to a mask-based thing
    if (dR > dRmax) {
      // UNFITTED values
      rawX[numNoFit] = raw[i].X;
      rawY[numNoFit] = raw[i].Y;
      refX[numNoFit] = ref[i].X;
      refY[numNoFit] = ref[i].Y;
      magRaw[numNoFit] = raw[i].Mag;
      magRef[numNoFit] = ref[i].Mag;
      numNoFit++;
    } else {
      // FITTED values
      rawXfit[numFit] 	= raw[i].X;
      rawYfit[numFit] 	= raw[i].Y;
      refXfit[numFit] 	= ref[i].X;
      refYfit[numFit] 	= ref[i].Y;
      magRawfit[numFit] = raw[i].Mag;
      magReffit[numFit] = ref[i].Mag;
      dPos[numFit]    	= ref[i].dPos;
      dXfit[numFit]   	= raw[i].X - ref[i].X;
      dYfit[numFit]   	= raw[i].Y - ref[i].Y;
      numFit++;
    }
  }

  if (numFit == 0) return 0;

  // 4-panel plot of x vs dx, y vs dx, x vs dy, y vs dy 
  relastroVisualRawRef(&kapa1, rawXfit, rawYfit, dXfit, dYfit, dPos, numFit);

  // vector line plot for the fit
  relastroVisualVectorField(&kapa2, rawXfit, rawYfit, dXfit, dYfit, numFit, dRmax);

  // dXfit, dYfit vs mag
  relastroVisualPlotScatter(&kapa3, dXfit, dYfit, magRawfit, numFit);

  // plot the positions of the fitted stars on the chip
  relastroVisualPlotFittedStars(&kapa4, rawX, rawY, refX, refY, numNoFit, rawXfit, rawYfit, refXfit, refYfit, numFit);

  askUser(&plotRawRef);

  FREE(dXfit);
  FREE(dYfit);
  FREE(dPos);
  FREE(rawX);
  FREE(rawY);
  FREE(refX);
  FREE(refY);
  FREE(rawXfit);
  FREE(rawYfit);
  FREE(refXfit);
  FREE(refYfit);
  FREE(magRaw);
  FREE(magRef);
  FREE(magRawfit);
  FREE(magReffit);

  return TRUE;
}

int relastroVisualPlotOutliers(Catalog *catalog, int offset, int Nmeasure,
			       StatType statsR, StatType statsD, double thresh) {
    
  float *Din, *Rin, *Dout, *Rout;
  double xmin, xmax, ymin, ymax, range;
  float xCirc[100], yCirc[100];
  int m, i;
  int Nin, Nout;
  Measure meas;
  Graphdata graphdata;
  KapaSection section;
  
  if (!isVisual || !plotOutliers) return TRUE;
  if (!initWindow(&kapa1)) return 0;
  
  // populate vectors
  ALLOCATE(Din,  float, Nmeasure);
  ALLOCATE(Rin,  float, Nmeasure);
  ALLOCATE(Dout, float, Nmeasure);
  ALLOCATE(Rout, float, Nmeasure);
  
  //create the threshhold ellipse
  for(i = 0; i < 100; i++) {
    xCirc[i] = statsR.median + thresh * statsR.sigma * cos(2 * 3.14 / 99. * i);
    yCirc[i] = statsD.median + thresh * statsD.sigma * sin(2 * 3.14 / 99. * i);  
  }

  m = offset;
  Nin = Nout = 0;
  xmin = +FLT_MAX;
  xmax = -FLT_MAX;
  ymax = -FLT_MAX;
  ymin = +FLT_MAX;
  for(i = 0; i < Nmeasure; i++, m++) {
    meas = catalog[0].measure[m];
    if (!MeasFilterTest(&meas, FALSE)) continue;
    xmin = MIN(xmin, meas.R);
    xmax = MAX(xmax, meas.R);
    ymin = MIN(ymin, meas.D);
    ymax = MAX(ymax, meas.D);
    
    if (meas.dbFlags & ID_MEAS_POOR_ASTROM) {
      Rout[Nout] = (meas.R);
      Dout[Nout] = (meas.D);
      fprintf(stderr, "r: %f\td: %f\t outlier: 1\n", Rout[Nout], Dout[Nout]);
      Nout++;
    } else {
      Rin[Nin] = (meas.R);
      Din[Nin] = (meas.D);
      fprintf(stderr, "r: %f\td: %f\t outlier: 0\n", Rin[Nin], Din[Nin]);
      Nin++;
    }
  }
 
  range = (xmax - xmin);
  xmin -= .1 * range;
  xmax += .1 * range;
  range = (ymax - ymin);
  ymax += .1 * range;
  ymin -= .1 * range;

  //temporary fix
  xmin = -1; xmax = 1; ymin = -1; ymax = 1;


  //initialize graph info
  section.x = 0; section.y = 0; section.dx = 1; section.dy = 1;
  section.name = "junk";
  section.bg = KapaColorByName("white");
  
  KapaInitGraph(&graphdata);
  KapaClearPlots(kapa1);
  KapaSetFont(kapa1, "helvetica", 14);
 
  graphdata.ptype = 7;
  graphdata.style = 2;
  graphdata.size = 3;
  graphdata.xmin = xmin;
  graphdata.xmax = xmax;
  graphdata.ymin = ymin;
  graphdata.ymax = ymax;

  KapaSetSection(kapa1, &section);
  KapaSetLimits(kapa1, &graphdata);
  KapaBox(kapa1, &graphdata);

  KapaSendLabel( kapa1, "RA (arcsec)", KAPA_LABEL_XM);
  KapaSendLabel( kapa1, "Dec (arcsec)", KAPA_LABEL_YM);
  KapaSendLabel( kapa1, "Points flagged as outliers (red)",
		 KAPA_LABEL_XP);

  graphdata.color = KapaColorByName("green");
  KapaPrepPlot(kapa1, Nin, &graphdata);
  KapaPlotVector(kapa1, Nin, Rin, "x");
  KapaPlotVector(kapa1, Nin, Din, "y");

  graphdata.color = KapaColorByName("red");
  KapaPrepPlot(kapa1, Nout, &graphdata);
  KapaPlotVector(kapa1, Nout, Rout, "x");
  KapaPlotVector(kapa1, Nout, Dout, "y");

  graphdata.color = KapaColorByName("black");
  graphdata.ptype = 0;
  graphdata.style = 0;
  KapaPrepPlot(kapa1, 100, &graphdata);
  KapaPlotVector(kapa1, 100, xCirc, "x");
  KapaPlotVector(kapa1, 100, yCirc, "y");

  askUser(&plotOutliers);

  FREE(Rout);
  FREE(Dout);
  FREE(Rin);
  FREE(Din);

  return TRUE;
}
  
# if (0)
int relastroVisualSummaryChips() {

  // plot the dXsys, dYsys histograms

  // plot x vs dx, y vs dy, etc for all mosaics

  // plot a map of median star scatter

}
# endif

