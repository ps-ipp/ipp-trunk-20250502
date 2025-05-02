# include "Ximage.h"

int SetGraphStyle (int sock) {
  
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
  
  // get graph style from client 
  KiiScanMessage (sock, "%d %d %d %d %d %d %lf %lf", 
		  &graph[0].style.style, 
		  &graph[0].style.ptype, 
		  &graph[0].style.ltype, 
		  &graph[0].style.etype, 
		  &graph[0].style.ebar, 
		  &graph[0].style.color, 
		  &graph[0].style.lweight, 
		  &graph[0].style.size);

  KiiScanMessage (sock, "%lf %lf %lf %lf", 
		  &graph[0].style.xmin, 
		  &graph[0].style.xmax, 
		  &graph[0].style.ymin, 
		  &graph[0].style.ymax);

  xmin = graph[0].style.xmin;
  xmax = graph[0].style.xmax;
  ymin = graph[0].style.ymin;
  ymax = graph[0].style.ymax;

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

int GetGraphStyle (int sock) {
  
  Section *section;
  KapaGraphWidget *graph;

  section = GetActiveSection();
  if (section->graph == NULL) {
    section->graph = InitGraph ();
    SetSectionSizes (section);
  }
  graph = section->graph;

  KiiSendMessage (sock, "%8d %d %d %d %d %d %f %f", 
		  graph[0].style.style, 
		  graph[0].style.ptype, graph[0].style.ltype, 
		  graph[0].style.etype, graph[0].style.ebar, graph[0].style.color, 
		  graph[0].style.lweight, graph[0].style.size);
  KiiSendMessage (sock, "%g %g %g %g", 
		  graph[0].style.xmin, graph[0].style.xmax, 
		  graph[0].style.ymin, graph[0].style.ymax);

  return (TRUE);
}
