# include "Ximage.h"

/* initialize graph data */
KapaGraphWidget *InitGraph () {

  int i;
  KapaGraphWidget *graph;

  ALLOCATE (graph, KapaGraphWidget, 1);

  graph[0].haveGraph = FALSE;

  /* set up axis positions */
  for (i = 0; i < 4; i++) {
    graph[0].axis[i].min = 0.0;
    graph[0].axis[i].max = 1.0;
    graph[0].axis[i].isaxis = FALSE;
    graph[0].axis[i].areticks = FALSE;
    graph[0].axis[i].islabel = FALSE;
    graph[0].axis[i].lweight = 0.0;
    graph[0].axis[i].color = 0;
    graph[0].axis[i].ticktextPad = NAN;
    graph[0].axis[i].labelPad = NAN;
    graph[0].axis[i].pad = NAN;
    graph[0].axis[i].fLabelRange = 1.0;
    graph[0].axis[i].fMinor = 5.0;
    graph[0].axis[i].dMajor = NAN;
    memset (graph[0].axis[i].format, 0, 16);
  }    

  KapaInitGraph (&graph[0].data);

  for (i = 0; i < 8; i++) {
    strcpy (graph[0].label[i].text, "");
  }

  graph[0].Nobjects = 0;
  graph[0].Ntextline = 0;

  ALLOCATE (graph[0].objects, Gobjects, 1);  /* allocate so later free will not crash! */
  ALLOCATE (graph[0].textline, Label, 1);    /* allocate so later free will not crash! */

  graph[0].objects[0].x   = graph[0].objects[0].y   = graph[0].objects[0].z = NULL;
  graph[0].objects[0].dxm = graph[0].objects[0].dxp = NULL;
  graph[0].objects[0].dym = graph[0].objects[0].dyp = NULL;

  return (graph);
}

void DrawGraph (KapaGraphWidget *graph) {
  if (graph == NULL) return;
  if (!graph[0].haveGraph) return;

  DrawFrame    (graph);
  DrawObjects  (graph);
  DrawLabels   (graph);
  DrawTextlines(graph);
}

/* remove objects */
void EraseGraph (KapaGraphWidget *graph) {

  int i;

  if (graph == NULL) return;
  graph[0].haveGraph = FALSE;

  /* free data objects, then re-alloc those needed */
  for (i = 0; i < graph[0].Nobjects; i++) {
    FREE (graph[0].objects[i].x);
    FREE (graph[0].objects[i].y);
    FREE (graph[0].objects[i].z);
    FREE (graph[0].objects[i].dxm);
    FREE (graph[0].objects[i].dxp);
    FREE (graph[0].objects[i].dym);
    FREE (graph[0].objects[i].dyp);
  }
    
  /* reset axes and labels */
  for (i = 0; i < 4; i++) {
    graph[0].axis[i].isaxis = FALSE;
    graph[0].axis[i].islabel = FALSE;
    graph[0].axis[i].areticks = FALSE;
  }
  for (i = 0; i < 8; i++) {
    strcpy (graph[0].label[i].text, "");
  }
    
  graph[0].Nobjects = 0;
  graph[0].Ntextline = 0;
  REALLOCATE (graph[0].objects, Gobjects, 1);
  REALLOCATE (graph[0].textline, Label, 1);

  graph[0].objects[0].x   = graph[0].objects[0].y   = graph[0].objects[0].z = NULL;
  graph[0].objects[0].dxm = graph[0].objects[0].dxp = NULL;
  graph[0].objects[0].dym = graph[0].objects[0].dyp = NULL;
}

/* remove objects */
void FreeGraph (KapaGraphWidget *graph) {

  int i;

  if (graph == NULL) return;

  /* free data objects, then re-alloc those needed */
  for (i = 0; i < graph[0].Nobjects; i++) {
    FREE (graph[0].objects[i].x);
    FREE (graph[0].objects[i].y);
    FREE (graph[0].objects[i].z);
    FREE (graph[0].objects[i].dxm);
    FREE (graph[0].objects[i].dxp);
    FREE (graph[0].objects[i].dym);
    FREE (graph[0].objects[i].dyp);
  }
    
  FREE (graph[0].objects);
  FREE (graph[0].textline);
  free (graph);
  return;
}
