# include "Ximage.h"

/************** MakeGC *************/
void MakeGC (Graphic *graphic) {

  XGCValues gcvalues;

  gcvalues.foreground = graphic->fore;
  gcvalues.background = graphic->back;
  gcvalues.arc_mode   = ArcPieSlice; // ArcChord;


  graphic->gc = XCreateGC (graphic->display, graphic->window, 
			     GCForeground | GCBackground | GCArcMode, &gcvalues);
  if (graphic->gc == 0)
    QuitX (graphic->display, "Error in creating a Graphics Context");
}
