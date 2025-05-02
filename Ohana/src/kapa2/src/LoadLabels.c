# include "Ximage.h"

int LoadLabels (int sock) {
  
  char *c, *label;
  int mode, size, Nbytes;
  Section *section;
  KapaGraphWidget *graph;

  section = GetActiveSection();
  if (section->graph == NULL) {
    section->graph = InitGraph ();
    SetSectionSizes (section);
  }
  graph = section->graph;
  graph[0].haveGraph = TRUE;

  KiiScanMessage (sock, "%d", &mode);
  label = KiiRecvData (sock);

  if (USE_XWINDOW) EraseLabels (graph);
  bzero (graph[0].label[mode].text, LABEL_MAXLEN);

  Nbytes = MIN (strlen(label), LABEL_MAXLEN - 1);
  strncpy_nowarn (graph[0].label[mode].text, label, Nbytes);
  
  FREE (label);

  c = GetRotFont (&size);

  graph[0].label[mode].size = size;
  strcpy (graph[0].label[mode].font, c);
  if (USE_XWINDOW) DrawLabels (graph);
  
  FlushDisplay ();
  
  return (TRUE);
  
}
