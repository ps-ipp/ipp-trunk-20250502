# include "Ximage.h"
  
void DrawTextlines (KapaGraphWidget *graph) {
  
  int i, x, y, size;
  double angle;
  char *fontname;
  Graphic *graphic;

  graphic = GetGraphic();

  fontname = GetRotFont (&size);
  XSetForeground (graphic->display, graphic->gc, graphic->fore);

  for (i = 0; i < graph[0].Ntextline; i++) {
    if (strcmp (graph[0].textline[i].text, "")) {
      DrawRotTextInit (graphic->display, graphic->window, graphic->gc, graphic->color[graph[0].textline[i].color], graphic->back);
      angle = graph[0].textline[i].angle;
      x = graph[0].textline[i].x;
      y = graph[0].textline[i].y;
      SetRotFont (graph[0].textline[i].font, graph[0].textline[i].size);
      DrawRotText (x, y, graph[0].textline[i].text, graph[0].textline[i].justify, angle);
    }
  }
  SetRotFont (fontname, size);
}

  /* pos values
            
 4____2___5 
  |       | 
  |       | 
 1|   8   |3
  |       |
  |       |
  ---------
  6   0   7

  */
