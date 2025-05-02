# include "Ximage.h"

int SetLimits (int sock) {
  
  int i;
  double xmin, xmax, ymin, ymax;
  Graphic *graphic;
  Section *section;
  KapaGraphWidget *graph;

  KiiScanMessage (sock, "%lf %lf %lf %lf", &xmin, &xmax, &ymin, &ymax);

  graphic = GetGraphic();

  section = GetActiveSection();
  if (section->graph == NULL) {
    section->graph = InitGraph ();
    SetSectionSizes (section);
  }
  graph = section->graph;
  
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

int GetLimits (int sock) {
  
  double dX, dY;
  Section *section;
  KapaGraphWidget *graph;

  section = GetActiveSection();
  graph = section->graph;

  if (graph == NULL) {
    dX = 0.0;
    dY = 0.0;
  } else {
    dX = graph[0].axis[0].dfx;
    dY = graph[0].axis[1].dfy;
  }

  KiiSendMessage (sock, "%8.1f %8.1f ", dX, dY);
  
  return (TRUE);
}
