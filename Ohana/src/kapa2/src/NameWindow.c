# include "Ximage.h"

/************** NameWindow *************/
void NameWindow (Graphic *graphic, char *Name) {

  char       *name;
  char       *class_name;
  char       *class_type;
  XClassHint *classhints;

  name = strrchr (Name, '/');
  if (name != NULL) 
    name ++;
  else 
    name = Name;

  class_type = class_name = name;
  classhints = XAllocClassHint ();

  if (classhints != (XClassHint *) NULL)  {
    classhints[0].res_name = class_name;
    classhints[0].res_class = class_type;
    XSetClassHint (graphic->display, graphic->window, classhints);
    XFree (classhints);
  }
  
  XStoreName (graphic->display, graphic->window, name);
  XSetIconName (graphic->display, graphic->window, name);
}
