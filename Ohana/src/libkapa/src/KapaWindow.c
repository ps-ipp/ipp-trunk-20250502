# include <kapa_internal.h>

int KiiCenter (int fd, double x, double y, int zoom) {

  KiiSendCommand (fd, 4, "CENT");
  KiiSendMessage (fd, "%8.3f %8.3f %8d ", x, y, zoom);
  KiiWaitAnswer (fd, "DONE");
  
  return (TRUE);
}

int KiiParity (int fd, int xflip, int yflip) {

  KiiSendCommand (fd, 4, "PARI");
  KiiSendMessage (fd, "%2d %2d ", xflip, yflip);
  KiiWaitAnswer (fd, "DONE");
  
  return (TRUE);
}

int KiiResize (int fd, int Nx, int Ny) {

  KiiSendCommand (fd, 4, "RSIZ");
  KiiSendMessage (fd, "%d %d ", Nx, Ny); 
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KiiResizeByImage (int fd) {

  KiiSendCommand (fd, 4, "ISIZ");
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KiiRelocate (int fd, int x, int y) {

  KiiSendCommand (fd, 4, "MOVE");
  KiiSendMessage (fd, "%d %d ", x, y); 
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaBox (int fd, Graphdata *graphdata) {

  KiiSendCommand (fd, 4, "DBOX");

  KapaSendGraphData (fd, graphdata);

  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaClearCurrentPlot (int fd) {
  
  KiiSendCommand (fd, 4, "ERSC");
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaClearPlots (int fd) {
  
  KiiSendCommand (fd, 4, "ERSP");
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaClearSections (int fd) {
  
  KiiSendCommand (fd, 4, "ERSS");
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaClearImage (int fd) {
  
  KiiSendCommand (fd, 4, "ERSI");
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaSetToolbox (int fd, int location) {
  
  KiiSendCommand (fd, 4, "TOOL");
  KiiSendMessage (fd, "%d ", location); 
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

// called in kapa2/src/Graphs.c to init the graph
int KapaInitGraph (Graphdata *graphdata) {

  graphdata[0].xmin = graphdata[0].ymin = 0.0;
  graphdata[0].xmax = graphdata[0].ymax = 1.0;

  graphdata[0].style = KAPA_PLOT_POINTS;
  graphdata[0].ptype = KAPA_POINT_BOX_SOLID;
  graphdata[0].ltype = graphdata[0].color = 0;
  graphdata[0].etype = graphdata[0].ebar = 0;
  graphdata[0].lweight = graphdata[0].size = 1.0;
  graphdata[0].alpha = 1.0;
    
  InitCoords (&graphdata[0].coords, "DEC--LIN");
  graphdata[0].flipeast = TRUE; // default coords has EW flipped relative to sky
  graphdata[0].flipnorth = FALSE;

  strcpy (graphdata[0].axis, "2222");
  strcpy (graphdata[0].ticks, "2222");
  strcpy (graphdata[0].labels, "2222");

  strcpy (graphdata[0].formatXm, "auto"); 
  strcpy (graphdata[0].formatXp, "auto"); 
  strcpy (graphdata[0].formatYm, "auto"); 
  strcpy (graphdata[0].formatYp, "auto"); 

  graphdata[0].ticktextPad = NAN;

  graphdata[0].labelPadXm = NAN;
  graphdata[0].labelPadXp = NAN;
  graphdata[0].labelPadYm = NAN;
  graphdata[0].labelPadYp = NAN;

  graphdata[0].padXm = NAN;
  graphdata[0].padXp = NAN;
  graphdata[0].padYm = NAN;
  graphdata[0].padYp = NAN;

  graphdata[0].fLabelRangeXm = 1.0;
  graphdata[0].fLabelRangeXp = 1.0;
  graphdata[0].fLabelRangeYm = 1.0;
  graphdata[0].fLabelRangeYp = 1.0;

  graphdata[0].fMinorXm = 5.0;
  graphdata[0].fMinorXp = 5.0;
  graphdata[0].fMinorYm = 5.0;
  graphdata[0].fMinorYp = 5.0;

  graphdata[0].dMajorXm = NAN;
  graphdata[0].dMajorXp = NAN;
  graphdata[0].dMajorYm = NAN;
  graphdata[0].dMajorYp = NAN;

  return (TRUE);
}

int KapaPrepPlot (int fd, int Npts, Graphdata *data) {

  /* tell kapa to look for the incoming image */
  KiiSendCommand (fd, 4, "PLOT"); 
  
  /* send kapa the plot details */
  KiiSendMessage (fd, "%8d %8d %d %d %d %d %d %f %f %f ", 
		  Npts, data[0].style, 
		  data[0].ptype, data[0].ltype, 
		  data[0].etype, data[0].ebar, data[0].color, 
		  data[0].alpha, data[0].lweight, data[0].size);
  KiiSendMessage (fd, "%g %g %g %g ", 
		  data[0].xmin, data[0].xmax, 
		  data[0].ymin, data[0].ymax);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaSendGraphData (int fd, Graphdata *data) {

  /* send kapa the plot details */
  KiiSendMessage (fd, "%g %g %g %g ", 
		  data[0].xmin, data[0].xmax, 
		  data[0].ymin, data[0].ymax);

  KiiSendMessage (fd, "%8d %d %d %d %d %d %f %f %f ", 
		  data[0].style, 
		  data[0].ptype, data[0].ltype, 
		  data[0].etype, data[0].ebar, data[0].color, 
		  data[0].alpha, data[0].lweight, data[0].size);

  KiiSendMessage (fd, "%g %g %g %g %g ", 
		  data[0].ticktextPad,
		  data[0].padXm, data[0].padXp, 
		  data[0].padYm, data[0].padYp);

  KiiSendMessage (fd, "%g %g %g %g ", 
		  data[0].labelPadXm, data[0].labelPadXp, 
		  data[0].labelPadYm, data[0].labelPadYp);

  KiiSendMessage (fd, "%lf %lf %lf %lf", 
		  data[0].fLabelRangeXm, data[0].fLabelRangeXp, 
		  data[0].fLabelRangeYm, data[0].fLabelRangeYp);

  KiiSendMessage (fd, "%lf %lf %lf %lf", 
		  data[0].fMinorXm, data[0].fMinorXp, 
		  data[0].fMinorYm, data[0].fMinorYp);

  KiiSendMessage (fd, "%lf %lf %lf %lf", 
		  data[0].dMajorXm, data[0].dMajorXp, 
		  data[0].dMajorYm, data[0].dMajorYp);

  KiiSendMessage (fd, "%s %s %s %s", 
		  data[0].formatXm, data[0].formatXp, 
		  data[0].formatYm, data[0].formatYp);

  KiiSendMessage (fd, "%g %g %g %g ", 
		  data[0].coords.pc1_1, data[0].coords.pc2_2,
		  data[0].coords.pc1_2, data[0].coords.pc2_1);

  KiiSendMessage (fd, "%d %d %s ", 
		  data[0].flipeast, data[0].flipnorth,
		  data[0].coords.ctype);

  KiiSendMessage (fd, "%g %g %g %g %g %g ", 
		  data[0].coords.crval1,
		  data[0].coords.crval2,
		  data[0].coords.crpix1,
		  data[0].coords.crpix2,
		  data[0].coords.cdelt1,
		  data[0].coords.cdelt2);

  KiiSendMessage (fd, "%s %s %s ", data[0].axis, data[0].ticks, data[0].labels);

  return (TRUE);
}

int KapaScanGraphData (int fd, Graphdata *data) {

  /* send kapa the plot details */
  KiiScanMessage (fd, "%lf %lf %lf %lf", 
		  &data[0].xmin, &data[0].xmax, 
		  &data[0].ymin, &data[0].ymax);

  KiiScanMessage (fd, "%d %d %d %d %d %d %lf %lf %lf", 
		  &data[0].style, 
		  &data[0].ptype, &data[0].ltype, 
		  &data[0].etype, &data[0].ebar, &data[0].color, 
		  &data[0].alpha, &data[0].lweight, &data[0].size);

  KiiScanMessage (fd, "%lf %lf %lf %lf %lf", 
		  &data[0].ticktextPad, 
		  &data[0].padXm, &data[0].padXp, 
		  &data[0].padYm, &data[0].padYp);

  KiiScanMessage (fd, "%lf %lf %lf %lf", 
		  &data[0].labelPadXm, &data[0].labelPadXp, 
		  &data[0].labelPadYm, &data[0].labelPadYp);

  KiiScanMessage (fd, "%lf %lf %lf %lf", 
		  &data[0].fLabelRangeXm, &data[0].fLabelRangeXp, 
		  &data[0].fLabelRangeYm, &data[0].fLabelRangeYp);

  KiiScanMessage (fd, "%lf %lf %lf %lf", 
		  &data[0].fMinorXm, &data[0].fMinorXp, 
		  &data[0].fMinorYm, &data[0].fMinorYp);

  KiiScanMessage (fd, "%lf %lf %lf %lf", 
		  &data[0].dMajorXm, &data[0].dMajorXp, 
		  &data[0].dMajorYm, &data[0].dMajorYp);

  KiiScanMessage (fd, "%s %s %s %s", 
		  data[0].formatXm, data[0].formatXp, 
		  data[0].formatYm, data[0].formatYp);

  KiiScanMessage (fd, "%f %f %f %f", 
		  &data[0].coords.pc1_1, &data[0].coords.pc2_2,
		  &data[0].coords.pc1_2, &data[0].coords.pc2_1);

  KiiScanMessage (fd, "%d %d %s", 
		  &data[0].flipeast, &data[0].flipnorth,
		  data[0].coords.ctype);

  KiiScanMessage (fd, "%lf %lf %f %f %f %f", 
		  &data[0].coords.crval1,
		  &data[0].coords.crval2,
		  &data[0].coords.crpix1,
		  &data[0].coords.crpix2,
		  &data[0].coords.cdelt1,
		  &data[0].coords.cdelt2);

  KiiScanMessage (fd, "%s %s %s", data[0].axis, data[0].ticks, data[0].labels);

  // XXX at some point, we need to add polynomials and 2-level mosaic
  // astrometry here.

  data[0].coords.Npolyterms = 0;

  return (TRUE);
}

int KapaSetGraphData (int fd, Graphdata *data) {

  /* tell kapa to look for the incoming image */
  KiiSendCommand (fd, 4, "SSTY"); 
  
  KapaSendGraphData (fd, data);

  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaGetGraphData (int fd, Graphdata *data) {

  /* tell kapa to look for the incoming image */
  KiiSendCommand (fd, 4, "GSTY"); 
  
  KapaScanGraphData (fd, data);

  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaSetImageData (int fd, KapaImageData *data) {

  /* tell kapa to look for the incoming image */
  KiiSendCommand (fd, 4, "SIMD"); 
  
  /* send kapa the plot details */
  KiiSendMessage (fd, "%g %g %s %s ", 
		  data[0].zero, data[0].range, data[0].name, data[0].file);

  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaGetImageData (int fd, KapaImageData *data) {

  /* tell kapa to look for the incoming image */
  KiiSendCommand (fd, 4, "GIMD"); 
  
  KiiScanMessage (fd, "%lf %lf %s %s", 
		  &data[0].zero, &data[0].range, data[0].name, data[0].file);

  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaPlotVector (int fd, int Npts, float *values, char *type) {

  int Nbytes, Nwrite;
  int swap;

# ifdef BYTE_SWAP
  swap = 1;
# else
  swap = 0;
# endif

  Nbytes = Npts * sizeof (float);

  if (!strcmp(type, "x")) goto valid;
  if (!strcmp(type, "y")) goto valid;
  if (!strcmp(type, "z")) goto valid;
  if (!strcmp(type, "dym")) goto valid;
  if (!strcmp(type, "dyp")) goto valid;
  if (!strcmp(type, "dxm")) goto valid;
  if (!strcmp(type, "dxp")) goto valid;
  return (FALSE);

valid:
  KiiSendCommand (fd, 4, "PLOB"); 
  KiiSendMessage (fd, "%s %d %d %d ", type, Npts, Nbytes, swap); 

  Nwrite = write (fd, values, Nbytes);
  if (Nwrite != Nbytes) {
    fprintf (stderr, "error sending data\n");
    return FALSE;
  }
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaSetFont (int fd, char *name, int size) {

  KiiSendCommand (fd, 4, "FONT");
  KiiSendCommand (fd, 16, "%s ", name);
  KiiSendCommand (fd, 16, "%d ", size);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaSendLabel (int fd, char *string, int mode) {

  KiiSendCommand (fd, 4, "LABL");
  KiiSendMessage (fd, "%6d ", mode);
  KiiSendData (fd, string, strlen(string));
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaSendTextline (int fd, char *string, float x, float y, float angle, int justify, int color) {
  
  // must be in range 0 - 8
  justify = MIN(MAX(justify, 0), 8);

  KiiSendCommand (fd, 4, "PTXT");
  KiiSendMessage (fd, "%f %f %f %d %d", x, y, angle, justify, color);
  KiiSendData (fd, string, strlen(string));
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaSetLimits (int fd, Graphdata *data) {

  KiiSendCommand (fd, 4, "SLIM");
  KiiSendMessage (fd, "%g %g %g %g ", data[0].xmin, data[0].xmax, data[0].ymin, data[0].ymax);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

// for now, this just gets the dimensions
int KapaGetLimits (int fd, float *dx, float *dy) {

  KiiSendCommand (fd, 4, "GLIM"); 
  KiiScanMessage (fd, "%f %f", dx, dy); 
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaSetSection (int fd, KapaSection *section) {

  KiiSendCommand (fd, 4, "DSEC");
  KiiSendMessage (fd, "%s %6.3f %6.3f %6.3f %6.3f %3d ", 
		  section[0].name, 
		  section[0].x,
		  section[0].y,
		  section[0].dx,
		  section[0].dy,
		  section[0].bg);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaSetSectionByImage (int fd, KapaSection *section) {

  KiiSendCommand (fd, 4, "ISEC");
  KiiSendMessage (fd, "%s %6.3f %6.3f %3d ", 
		  section[0].name, 
		  section[0].x,
		  section[0].y,
		  section[0].bg);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaSectionBG (int fd, char *name, int bg) {

  KiiSendCommand (fd, 4, "BSEC");
  KiiSendMessage (fd, "%s %3d ", name, bg);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaMoveSection (int fd, char *name, char *direction) {

  if (!strcasecmp(direction, "up")) goto valid;
  if (!strcasecmp(direction, "down")) goto valid;
  if (!strcasecmp(direction, "top")) goto valid;
  if (!strcasecmp(direction, "bottom")) goto valid;
  
  fprintf (stderr, "unexpected direction %s\n", direction); 
  return (FALSE);

valid:
  KiiSendCommand (fd, 4, "MSEC");
  KiiSendMessage (fd, "%s %s ", name, direction);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaSelectSection (int fd, char *name) {

  KiiSendCommand (fd, 4, "SSEC");
  KiiSendMessage (fd, "%s ", name);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaGetSection (int fd, char *name) {

  KiiSendCommand (fd, 4, "LSEC");
  KiiSendMessage (fd, "%s ", name);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaSetSmoothSigma (int fd, float sigma) {

  KiiSendCommand (fd, 4, "SIGM");
  KiiSendMessage (fd, "%4.1f ", sigma);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaMemoryDump (int fd) {

  KiiSendCommand (fd, 4, "MEMD");
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaMemoryDumpLines (int fd, int Nlines) {

  KiiSendCommand (fd, 4, "MEML");
  KiiSendMessage (fd, "%d ", Nlines);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaMemoryDumpOnExit (int fd, int state) {

  KiiSendCommand (fd, 4, "MEMX");
  KiiSendMessage (fd, "%d ", state);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

