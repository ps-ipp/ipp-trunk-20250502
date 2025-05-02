# include "Ximage.h"

int InterpretPresses (Graphic *graphic, XButtonEvent *event) {

  int              sock, DX, DY, status, done, this_button;
  char             name[16];
  double           X, Y, Z, R, D;
  float           *imdata;
  Button          *button;
  Section         *section;
  KapaImageWidget *image;
  KapaGraphWidget *graph;

  // XXX select the window element which contains the event
  section = GetActiveSection();
  image   = section->image;
  graph   = section->graph;

  // return graph coords by default, image if graph does not exist
  // XXX allow user to choose graph or image coords
  if (ACTIVE_CURSOR) {
    sock = GetActiveSocket ();
    if (sock == -1) goto skip_cursor;

    sprintf (name, "Button%d", event[0].button);
    Z = 0;
    if (graph) {
      X = (event[0].x - graph[0].axis[0].fx)*(graph[0].axis[0].max - graph[0].axis[0].min)/graph[0].axis[0].dfx + graph[0].axis[0].min;
      Y = (event[0].y - graph[0].axis[1].fy)*(graph[0].axis[1].max - graph[0].axis[1].min)/graph[0].axis[1].dfy + graph[0].axis[1].min;
      XY_to_RD (&R, &D, X, Y, &graph[0].data.coords);
    } 
    if (image && !graph) {
      if (event[0].x < image[0].picture.x) goto skip_cursor;
      if (event[0].y < image[0].picture.y) goto skip_cursor;
      if (event[0].x > image[0].picture.x + image[0].picture.dx) goto skip_cursor;
      if (event[0].y > image[0].picture.y + image[0].picture.dy) goto skip_cursor;
      Screen_to_Image (&X, &Y, event[0].x + 0.5, event[0].y + 0.5, &image[0].picture);

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
  status = TRUE;
  this_button = event[0].button;

  // XXX add graph buttons here
  if (image == NULL) return (TRUE);
  
  if ((event[0].type == ButtonPress) && InPicture (event, &image[0].picture)) {
    ReorientOnButton (graphic, image, event);
  }

  if ((event[0].type == ButtonPress) && InPicture (event, &image[0].cmapbar)) {
    DragColorbar (graphic, image, event);
  }

  /* if on an exisiting button, Invert, wait for release, then go (or not) */
  if ((button = CheckButtons (event, image)) != (Button *) NULL) {
    InvertButton (graphic, button); 
    done = FALSE;
    while (!done) { /* wait for release of this button */
      XNextEvent (graphic[0].display, (XEvent *) event);
      if ((event[0].type == ButtonRelease) && (event[0].button == this_button)) {
	done = TRUE;
      }
    }
    DrawButton (graphic, button);
    if (InButton (event, button)) {
      switch (event[0].button) {
      case 1:
	status = button[0].function_1(graphic, image);
	break;
      case 2:
	status = button[0].function_2(graphic, image);
	break;
      case 3:
	status = button[0].function_3(graphic, image);
	break;
      }
    } else {
      return (status);
    }
  }
  return (status);
}

void ReorientOnButton (Graphic *graphic, KapaImageWidget *image, XButtonEvent *mouse_event) {

  double X, Y;

  Screen_to_Image (&X, &Y, mouse_event[0].x + 0.5, mouse_event[0].y + 0.5, &image[0].picture);

  switch (mouse_event[0].button) {
    case 1:
      Reorient (graphic, image, X, Y, 0);
      break;
    case 2:
      Reorient (graphic, image, X, Y, -1);
      break;
    case 3:
      Reorient (graphic, image, X, Y, +1);
      break;
    default:
      break;
  }
}
