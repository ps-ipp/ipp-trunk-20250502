# include "Ximage.h"

int SetGraphData (int sock) {
  
  int i;
  double xmin, xmax, ymin, ymax;
  Graphic *graphic;
  Section *section;
  KapaGraphWidget *graph;

  graphic = GetGraphic();

  section = GetActiveSection();
  if (section->graph == NULL) {
    section->graph = InitGraph ();
    SetSectionSizes (section);
  }
  graph = section->graph;
  
  KapaScanGraphData (sock, &graph[0].data);

  for (i = 0; i < 4; i++) {
    graph[0].axis[i].ticktextPad = graph[0].data.ticktextPad;
  }    
  graph[0].axis[0].labelPad = graph[0].data.labelPadXm;
  graph[0].axis[1].labelPad = graph[0].data.labelPadYm;
  graph[0].axis[2].labelPad = graph[0].data.labelPadXp;
  graph[0].axis[3].labelPad = graph[0].data.labelPadYp;

  graph[0].axis[0].fLabelRange = graph[0].data.fLabelRangeXm;
  graph[0].axis[1].fLabelRange = graph[0].data.fLabelRangeYm;
  graph[0].axis[2].fLabelRange = graph[0].data.fLabelRangeXp;
  graph[0].axis[3].fLabelRange = graph[0].data.fLabelRangeYp;

  graph[0].axis[0].fMinor = graph[0].data.fMinorXm;
  graph[0].axis[1].fMinor = graph[0].data.fMinorYm;
  graph[0].axis[2].fMinor = graph[0].data.fMinorXp;
  graph[0].axis[3].fMinor = graph[0].data.fMinorYp;

  graph[0].axis[0].dMajor = graph[0].data.dMajorXm;
  graph[0].axis[1].dMajor = graph[0].data.dMajorYm;
  graph[0].axis[2].dMajor = graph[0].data.dMajorXp;
  graph[0].axis[3].dMajor = graph[0].data.dMajorYp;

  strcpy (graph[0].axis[0].format, graph[0].data.formatXm);
  strcpy (graph[0].axis[1].format, graph[0].data.formatYm);
  strcpy (graph[0].axis[2].format, graph[0].data.formatXp);
  strcpy (graph[0].axis[3].format, graph[0].data.formatYp);

  graph[0].axis[0].pad = graph[0].data.padXm;
  graph[0].axis[1].pad = graph[0].data.padYm;
  graph[0].axis[2].pad = graph[0].data.padXp;
  graph[0].axis[3].pad = graph[0].data.padYp;

  xmin = graph[0].data.xmin;
  xmax = graph[0].data.xmax;
  ymin = graph[0].data.ymin;
  ymax = graph[0].data.ymax;

  // XXX there are now two things which track the graph limits
  // make sure these are kept in sync (or drop one!)
  graph[0].axis[2].min = graph[0].axis[0].min = xmin;
  graph[0].axis[2].max = graph[0].axis[0].max = xmax;
  graph[0].axis[3].min = graph[0].axis[1].min = ymin; 
  graph[0].axis[3].max = graph[0].axis[1].max = ymax;
  
  for (i = 0; i < graph[0].Nobjects; i++) {
    graph[0].objects[i].x0 = xmin;
    graph[0].objects[i].x1 = xmax;
    graph[0].objects[i].y0 = ymin;
    graph[0].objects[i].y1 = ymax;
  }

  if (USE_XWINDOW) XClearWindow (graphic->display, graphic->window);
  Refresh ();

  return (TRUE);  
}

int GetGraphData (int sock) {
  
  Section *section;
  KapaGraphWidget *graph;

  section = GetActiveSection();
  if (section->graph == NULL) {
    section->graph = InitGraph ();
    SetSectionSizes (section);
  }
  graph = section->graph;

  KapaSendGraphData (sock, &graph[0].data);

  return (TRUE);
}
