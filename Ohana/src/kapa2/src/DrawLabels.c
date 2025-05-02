# include "Ximage.h"
  
void EraseLabels (KapaGraphWidget *graph) {
  
  Graphic *graphic;
  graphic = GetGraphic();

  DrawLabelsRaw (graphic, graph, graphic->back);
}

void DrawLabels (KapaGraphWidget *graph) {
  
  Graphic *graphic;
  graphic = GetGraphic();

  DrawLabelsRaw (graphic, graph, graphic->fore);
}

void DrawLabelsRaw (Graphic *graphic, KapaGraphWidget *graph, int color) {

  int i, pos, x, y, size;
  double angle;
  char *fontname, defaultname[64];

  // save the current default font and size
  pos = 0;
  fontname = GetRotFont (&size);
  strcpy (defaultname, fontname);

  XSetForeground (graphic->display, graphic->gc, color);
  DrawRotTextInit (graphic->display, graphic->window, graphic->gc, color, graphic->back);

  /* each label is drawn independently */
  for (i = 0; i < 8; i++) {
    if (strcmp (graph[0].label[i].text, "")) {
      angle = 0;
      switch (i) {
      case 0: pos = 7; break;
      case 2: pos = 1; break;
      case 1: pos = 1; angle = -90; break;
      case 3: pos = 1; angle =  90; break;
      case 4: pos = 2; break;
      case 5: pos = 0; break;
      case 6: pos = 8; break;
      case 7: pos = 6; break;
      }	
      x = graph[0].label[i].x;
      y = graph[0].label[i].y;
      SetRotFont (graph[0].label[i].font, graph[0].label[i].size);
      DrawRotText (x, y, graph[0].label[i].text, pos, angle);
    }
  }
  SetRotFont (defaultname, size);
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
          
 6____7___8 
  |       | 
  |       | 
 3|   4   |5
  |       |
  |       |
  ---------
  0   1   2

  */
