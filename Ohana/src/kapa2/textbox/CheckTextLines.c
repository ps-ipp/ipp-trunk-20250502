# include "Ximage.h"

/******** Here we test the TextLines specific to this program  ****
TextLine *
CheckTextLines (graphic, event, layout)
Graphic         graphic[];
XEvent          event[];
Layout          layout[];
{

  TextLine *textline;
  textline = (TextLine *) NULL;

  if (InTextLine (graphic, event, &layout[0].zero))
    textline = &layout[0].zero;
  if (InTextLine (graphic, event, &layout[0].range))
    textline = &layout[0].range;
  if (InTextLine (graphic, event, &layout[0].effects))
    textline = &layout[0].effects;
  if (InTextLine (graphic, event, &layout[0].command))
    textline = &layout[0].command;

  return (textline);

}


***/
