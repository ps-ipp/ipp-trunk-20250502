# include "Ximage.h"
  
void bDrawLabels (bDrawBuffer *buffer, KapaGraphWidget *graph) {
  
  int i, pos, x, y, size;
  double angle;
  char *fontname;

  pos = 0;
  fontname = GetRotFont (&size);
  for (i = 0; i < 8; i++) {
    if (strcmp (graph[0].label[i].text, "")) {
      angle = 0;
      switch (i) {
      case 0: pos = 7; break;
      case 1: pos = 1; angle = -90; break;
      case 2: pos = 1; break;
      case 3: pos = 1; angle =  90; break;
      case 4: pos = 2; break;
      case 5: pos = 0; break;
      case 6: pos = 8; break;
      case 7: pos = 6; break;
      }	
      x = graph[0].label[i].x;
      y = graph[0].label[i].y;
      SetRotFont (graph[0].label[i].font, graph[0].label[i].size); 
      bDrawRotText (buffer, x, y, graph[0].label[i].text, pos, angle);
    }
  }
  SetRotFont (fontname, size);
}

void bDrawTextlines (bDrawBuffer *buffer, KapaGraphWidget *graph) {

  int i, x, y, size;
  double angle;
  char *fontname;

  fontname = GetRotFont (&size);
  for (i = 0; i < graph[0].Ntextline; i++) {
    if (strcmp (graph[0].textline[i].text, "")) {
      bDrawSetColor (buffer, graph[0].textline[i].color);
      angle = graph[0].textline[i].angle;
      x = graph[0].textline[i].x;
      y = graph[0].textline[i].y;
      SetRotFont (graph[0].textline[i].font, graph[0].textline[i].size);
      bDrawRotText (buffer, x, y, graph[0].textline[i].text, graph[0].textline[i].justify, angle);
    }
  }
  SetRotFont (fontname, size);
}

/*
            
 4____2___5 
  |       | 
  |       | 
 1|       |3
  |       |
  |       |
  ---------
  6   0   7

*/


