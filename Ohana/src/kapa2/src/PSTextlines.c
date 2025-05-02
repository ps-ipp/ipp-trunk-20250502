# include "Ximage.h"
  
void PSTextlines (KapaGraphWidget *graph, FILE *f) {

  int i, x, y, size;
  double angle;
  char *fontname;
  Graphic *graphic;

  graphic = GetGraphic();

  fontname = GetRotFont (&size);
  for (i = 0; i < graph[0].Ntextline; i++) {
    if (strcmp (graph[0].textline[i].text, "")) {
      angle = graph[0].textline[i].angle;
      x = graph[0].textline[i].x;
      y = graphic->dy - graph[0].textline[i].y;
      if (graph[0].textline[i].color >= 0) {
	fprintf (f, "%s setrgbcolor\n", KapaColorRGBString(graph[0].textline[i].color));
      }
      SetRotFont (graph[0].textline[i].font, graph[0].textline[i].size);
      PSRotText (f, x, y, graph[0].textline[i].text, graph[0].textline[i].justify, angle);
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
