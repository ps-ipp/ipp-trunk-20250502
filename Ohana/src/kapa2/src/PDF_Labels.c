# include "Ximage.h"
  
void PDF_Labels (KapaGraphWidget *graph, IOBuffer *buffer) {
  
  int i, pos, x, y, size;
  double angle;
  char *fontname;
  Graphic *graphic;

  graphic = GetGraphic();

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
      y = graphic->dy - graph[0].label[i].y;
      SetRotFont (graph[0].label[i].font, graph[0].label[i].size); 
      PDFRotText (buffer, x, y, graph[0].label[i].text, pos, angle);
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


