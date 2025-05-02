# include "Ximage.h"
# define FILTER_MODS 1

int InterpretKeys (Graphic *graphic, XKeyEvent *event) {

  float           *imdata;
  double 	   X, Y, Z, R, D, offset;
  int    	   sock, DX, DY, modstate;
  char   	  *name, string[16];
  KeySym           keysym;
  XComposeStatus   composestatus;
  Section         *section;
  KapaImageWidget *image;
  KapaGraphWidget *graph;

  // XXX select the window element which contains the event
  section = GetActiveSection();
  image   = section->image;
  graph   = section->graph;

  XLookupString (event, string, 9, &keysym, &composestatus);
  modstate = event[0].state;

  // return graph coords by default, image if graph does not exist
  // XXX allow user to choose graph or image coords
  if (ACTIVE_CURSOR) {

    sock = GetActiveSocket ();
    if (sock == -1) goto skip_cursor;

    name = XKeysymToString (keysym);

    // skip the following keys: 
    if (name == NULL) goto skip_cursor;
    if (FILTER_MODS) {
      if (!strcmp (name, "Shift_L")) goto skip_cursor;
      if (!strcmp (name, "Shift_R")) goto skip_cursor;
      if (!strcmp (name, "Control_L")) goto skip_cursor;
      if (!strcmp (name, "Control_R")) goto skip_cursor;
      if (!strcmp (name, "Alt_L")) goto skip_cursor;
      if (!strcmp (name, "Alt_R")) goto skip_cursor;
      if (!strcmp (name, "Super_L")) goto skip_cursor;
      if (!strcmp (name, "Super_R")) goto skip_cursor;
      if (!strcmp (name, "Caps_Lock")) goto skip_cursor;
      if (!strcmp (name, "Pause")) goto skip_cursor;
      if (!strcmp (name, "Continue")) goto skip_cursor;
      if (!strcmp (name, "Num_Lock")) goto skip_cursor;
      if (!strcmp (name, "Scroll_Lock")) goto skip_cursor;
      if (!strcmp (name, "Print")) goto skip_cursor;
      if (!strcmp (name, "(null)")) goto skip_cursor;
    }

    Z = -1;

    int haveGraph = graph && graph->haveGraph;
    if (haveGraph) {
      X = (event[0].x - graph[0].axis[0].fx)*(graph[0].axis[0].max - graph[0].axis[0].min)/graph[0].axis[0].dfx + graph[0].axis[0].min;
      Y = (event[0].y - graph[0].axis[1].fy)*(graph[0].axis[1].max - graph[0].axis[1].min)/graph[0].axis[1].dfy + graph[0].axis[1].min;
      XY_to_RD (&R, &D, X, Y, &graph[0].data.coords);
    } 
    if (image && !haveGraph) {
      if (event[0].x < image[0].picture.x) goto skip_cursor;
      if (event[0].y < image[0].picture.y) goto skip_cursor;
      if (event[0].x > image[0].picture.x + image[0].picture.dx) goto skip_cursor;
      if (event[0].y > image[0].picture.y + image[0].picture.dy) goto skip_cursor;
      Screen_to_Image (&X, &Y, (double)event[0].x, (double)event[0].y, &image[0].picture);
      XY_to_RD (&R, &D, X, Y, &image[0].image[0].coords);

      DX = image[0].image[0].matrix.Naxis[0];
      DY = image[0].image[0].matrix.Naxis[1];

      if (X < 0) goto off_image;
      if (Y < 0) goto off_image;
      if (X >= DX) goto off_image;
      if (Y >= DY) goto off_image;
      imdata = (float *) image[0].image[0].matrix.buffer;
      Z      = imdata[DX*(int)(Y) + (int)(X)];
    }
  off_image:
    KiiSendMessage (sock, "%12s %12.6f %12.6f %12.6f %12.6f %12.6f", name, X, Y, Z, R, D);
  }

skip_cursor:
  if (image == NULL) return (TRUE);

  // offset is in image pixels: 
  // 0.5 image pixels is 1 screen pixel for expand == +2
  // 2.0 image pixels is 1 screen pixel for expand == -2
  if (image[0].picture.expand == 0) image[0].picture.expand = 1;
  offset = (image[0].picture.expand > 0) ? 1.0 / image[0].picture.expand : -image[0].picture.expand;
  if (modstate & ControlMask) offset *= 100;

  switch (keysym) {

# define SET_CHANNEL_CASE(NCHAN) \
    case XK_F##NCHAN: \
      image[0].currentChannel = NCHAN-1; \
      image[0].image = &image[0].channel[NCHAN-1]; \
      SetColorScale (graphic, image); \
      Reorient (graphic, image, image[0].picture.Xc, image[0].picture.Yc, 0); \
      Screen_to_Image (&X, &Y, (double)(event[0].x + 0.5), (double)(event[0].y + 0.5), &image[0].picture); \
      UpdateStatusBox (graphic, image, X, Y, 0.0, 1); \
      Remap (graphic, image); \
      Refresh (); \
      break;

    // the number of entries here must match the value of NCHANNELS in contants.h
    SET_CHANNEL_CASE(1);
    SET_CHANNEL_CASE(2);
    SET_CHANNEL_CASE(3);
    SET_CHANNEL_CASE(4);
    SET_CHANNEL_CASE(5);
    SET_CHANNEL_CASE(6);
    SET_CHANNEL_CASE(7);
    SET_CHANNEL_CASE(8);
    SET_CHANNEL_CASE(9);
    SET_CHANNEL_CASE(10);

    case XK_KP_Home:
    case XK_Home:
      image[0].picture.expand = 1;
      image[0].zoom.expand = 5;
      Reorient (graphic, image, image[0].picture.Xc, image[0].picture.Yc, 0);
      break;
    case XK_KP_End:
    case XK_End:
      image[0].picture.expand = 1;
      image[0].zoom.expand = 5;
      Reorient (graphic, image, 0.5*image[0].image[0].matrix.Naxis[0], 0.5*image[0].image[0].matrix.Naxis[1], 0);
      break;
    case XK_KP_Enter:
    case XK_KP_Begin:
    case XK_Return:
      Screen_to_Image (&X, &Y, event[0].x + 0.5, event[0].y + 0.5, &image[0].picture);
      Reorient (graphic, image, X, Y, 0);
      break;
    case XK_Prior:
    case XK_KP_Prior:
      Reorient (graphic, image, image[0].picture.Xc, image[0].picture.Yc, +1);
      break;
    case XK_Next:
    case XK_KP_Next:
      Reorient (graphic, image, image[0].picture.Xc, image[0].picture.Yc, -1);
      break;
    case XK_Up:
    case XK_KP_Up:
      Reorient (graphic, image, image[0].picture.Xc, image[0].picture.Yc + offset, 0);
      break;
    case XK_Down:
    case XK_KP_Down:
      Reorient (graphic, image, image[0].picture.Xc, image[0].picture.Yc - offset, 0);
      break;
    case XK_Left:
    case XK_KP_Left:
      Reorient (graphic, image, image[0].picture.Xc + offset, image[0].picture.Yc, 0);
      break;
    case XK_Right:
    case XK_KP_Right:
      Reorient (graphic, image, image[0].picture.Xc - offset, image[0].picture.Yc, 0);
      break;

    case XK_plus:
    case XK_equal:
    case XK_KP_Add:
      if (modstate & ControlMask) {
	  image[0].image[0].zero -= 0.05*image[0].image[0].range;
	  image[0].image[0].range *= 1.1;
      } else {
	  image[0].image[0].zero += 0.1*image[0].image[0].range;
      }
      SetColorScale (graphic, image);
      Remap (graphic, image);
      Reorient (graphic, image, image[0].picture.Xc, image[0].picture.Yc, 0);
      break;

    case XK_minus:
    case XK_underscore:
    case XK_KP_Subtract:
      if (modstate & ControlMask) {
	  image[0].image[0].zero += 0.05*image[0].image[0].range;
	  image[0].image[0].range *= 0.90;
      } else {
	  image[0].image[0].zero -= 0.1*image[0].image[0].range;
      }
      SetColorScale (graphic, image);
      Remap (graphic, image);
      Reorient (graphic, image, image[0].picture.Xc, image[0].picture.Yc, 0);
      break;

    case XK_Tab:
      image[0].MovePointer = image[0].MovePointer ^ TRUE;
      if (image[0].MovePointer) UpdatePointer (graphic, (XMotionEvent *)event);
      break;
  }

  return (TRUE);
}
