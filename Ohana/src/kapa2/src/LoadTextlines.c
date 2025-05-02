# include "Ximage.h"

int LoadTextlines (int sock) {
  
  char *string;
  int N, size, justify, color;
  double tX, tY, tT, L;
  Section *section;
  KapaGraphWidget *graph;

  section = GetActiveSection();
  graph = section->graph;
  if (section->graph == NULL) {
    section->graph = InitGraph ();
    SetSectionSizes (section);
    graph = section->graph;
  }
  graph[0].haveGraph = TRUE;

  graph[0].Ntextline = MAX (graph[0].Ntextline, 0);
  N = graph[0].Ntextline;
  graph[0].Ntextline++;
  REALLOCATE (graph[0].textline, Label, graph[0].Ntextline);

  KiiScanMessage (sock, "%lf %lf %lf %d %d", &tX, &tY, &tT, &justify, &color);

  L = graph[0].axis[0].dfx;
  graph[0].textline[N].x = L * (tX - graph[0].axis[0].min) / (graph[0].axis[0].max - graph[0].axis[0].min) + graph[0].axis[0].fx;

  L = graph[0].axis[1].dfy;
  graph[0].textline[N].y = L * (tY - graph[0].axis[1].min) / (graph[0].axis[1].max - graph[0].axis[1].min) + graph[0].axis[1].fy;

  graph[0].textline[N].angle = tT;

  bzero (graph[0].textline[N].text, LABEL_MAXLEN);

  string = KiiRecvData (sock);

# if (0)
  /** test **/
  free (string);
  ALLOCATE (string, char, 121);
  for (int i = 0; i < 120; i++) {
    string[i] = i + 160;
  }
# endif
  
  strcpy (graph[0].textline[N].text, string);
  free (string);
  
  string = GetRotFont (&size);
  graph[0].textline[N].size = size;
  strcpy (graph[0].textline[N].font, string);
  graph[0].textline[N].justify = justify;
  graph[0].textline[N].color = color;

  if (USE_XWINDOW) DrawTextlines (graph);
  
  FlushDisplay ();
  
  return (TRUE);
  
}
