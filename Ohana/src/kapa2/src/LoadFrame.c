# include "Ximage.h"

int LoadFrame (int sock) {
  
  int i;
  Section *section;
  KapaGraphWidget *graph;

  section = GetActiveSection();
  if (section->graph == NULL) {
    section->graph = InitGraph ();
    SetSectionSizes (section);
  }
  graph = section->graph;

  KapaScanGraphData (sock, &graph[0].data);

  // even if we have no actual elements, once we define a frame, we have a graph
  graph[0].haveGraph = TRUE;

  graph[0].axis[3].min = graph[0].axis[1].min;
  graph[0].axis[3].max = graph[0].axis[1].max;
  graph[0].axis[2].min = graph[0].axis[0].min;
  graph[0].axis[2].max = graph[0].axis[0].max;
  
  // XXX this is fragile
  graph[0].data.color = MAX (0, MIN (15, graph[0].data.color));

  graph[0].axis[0].pad = graph[0].data.padXm;
  graph[0].axis[1].pad = graph[0].data.padYm;
  graph[0].axis[2].pad = graph[0].data.padXp;
  graph[0].axis[3].pad = graph[0].data.padYp;

  graph[0].axis[0].labelPad = graph[0].data.labelPadXm;
  graph[0].axis[1].labelPad = graph[0].data.labelPadYm;
  graph[0].axis[2].labelPad = graph[0].data.labelPadXp;
  graph[0].axis[3].labelPad = graph[0].data.labelPadYp;

  graph[0].axis[0].fLabelRange = graph[0].data.fLabelRangeXm;
  graph[0].axis[1].fLabelRange = graph[0].data.fLabelRangeYm;
  graph[0].axis[2].fLabelRange = graph[0].data.fLabelRangeXp;
  graph[0].axis[3].fLabelRange = graph[0].data.fLabelRangeYp;

  // fMinor and dMajor should have defaults of NAN
  // with default behavior defined in TickMarks.c
  // for one axis (x or y), if both + and - are
  // NAN, keep the default behavior, but if one
  // is NOT NAN, use that for both (synchronization)

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

  for (i = 0; i < 4; i++) {
    graph[0].axis[i].lweight = graph[0].data.lweight;
    graph[0].axis[i].color = graph[0].data.color;

    graph[0].axis[i].ticktextPad = graph[0].data.ticktextPad;

    switch (graph[0].data.axis[i]) {
    case '0':
      graph[0].axis[i].isaxis = FALSE;
      break;
    case '1':
      graph[0].axis[i].isaxis = TRUE;
      break;
    case '2':
      graph[0].axis[i].isaxis = TRUE;
      break;
    }
    switch (graph[0].data.ticks[i]) {
    case '0':
      graph[0].axis[i].areticks = FALSE;
      break;
    case '1':
      graph[0].axis[i].areticks = TRUE;
      break;
    case '2':
      graph[0].axis[i].areticks = 2;
      break;
    case '3':
      graph[0].axis[i].areticks = 3;
      break;
    }
    switch (graph[0].data.labels[i]) {
    case '0':
      graph[0].axis[i].islabel = FALSE;
      break;
    case '1':
      graph[0].axis[i].islabel = TRUE;
      break;
    case '2':
      graph[0].axis[i].islabel = (i < 2);
      break;
    }
  }

  SetSectionSizes (section);
  Refresh();

  return (TRUE);
  
}
