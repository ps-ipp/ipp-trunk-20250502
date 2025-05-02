 # include "Ximage.h"

/* Set the dimensions of the specific image based on the current window size.  The image
   is placed within the window at the fractional position defined by the section */

// XXX currently only set to fill the window; does not adjust for section range
void SetImageSize (Section *section) {

  int Xs, Ys, dX, dY;
  int textpad, textdY, WdY; 
  int haveGraph;
  KapaImageWidget *image;
  KapaGraphWidget *graph;
  Graphic *graphic;

  if (section == NULL) return;
  image = section->image;
  if (image == NULL) return;
  graph = section->graph;
  haveGraph = graph && graph->haveGraph;

  graphic = GetGraphic ();

  /* the image is placed within the graphic window in region specified by section */
  Xs = graphic[0].dx * section[0].x;
  Ys = graphic[0].dy * (1 - section[0].y - section[0].dy);
  dX = graphic[0].dx * section[0].dx;
  dY = graphic[0].dy * section[0].dy;

  textpad = USE_XWINDOW ? graphic[0].font[0].ascent : 10;
  textdY = 6*textpad + 7*PAD1;
  WdY = MAX (ZOOM_Y, textdY + 2*BUTTON_HEIGHT + PAD1);

  switch (image[0].location) {

    case 0: // no zoom / status / wide
      if (haveGraph) {
	  image[0].picture.x  = graph[0].axis[0].fx;
	  image[0].picture.y  = graph[0].axis[1].fy + graph[0].axis[1].dfy;
	  image[0].picture.dx = MAX(fabs(graph[0].axis[0].dfx) - 1, 1);
	  image[0].picture.dy = MAX(fabs(graph[0].axis[1].dfy) - 1, 1);
      } else {
	  image[0].picture.x  = Xs + PAD1;
	  image[0].picture.y  = Ys + PAD1;
	  image[0].picture.dx = dX - 2*PAD1 - 1; 
	  image[0].picture.dy = dY - 2*PAD1 - 1;
      }
      if (USE_XWINDOW) CreatePicture (image, graphic);
      Remap (graphic, image);
      return;

    case 1: // zoom / status / wide on bottom (-x)

      if (haveGraph) {
	image[0].picture.x = graph[0].axis[0].fx;
	image[0].picture.y = graph[0].axis[1].fy + graph[0].axis[1].dfy;
	image[0].picture.dx = MAX(fabs(graph[0].axis[0].dfx) - 1, 1);
	image[0].picture.dy = MAX(fabs(graph[0].axis[1].dfy) - 1, 1);
      } else {
	image[0].picture.x  = Xs + PAD1;
	image[0].picture.y  = Ys + 2*PAD1 + COLORPAD;
	image[0].picture.dx = dX - 2*PAD1 - 1; 
	image[0].picture.dy = dY - 4*PAD1 - 1 - WdY - COLORPAD;
      }

      image[0].cmapbar.dx = dX - 2*PAD1; 
      image[0].cmapbar.dy = COLORPAD;
      image[0].cmapbar.x = Xs + PAD1;
      image[0].cmapbar.y = Ys + PAD1;

      // XXX zoom should scale somewhat with the image? (with a min and a max)
      // XXX actually, it is limited by the buttons and status region
      image[0].zoom.dx = ZOOM_X; 
      image[0].zoom.dy = ZOOM_Y;
      image[0].zoom.x = Xs + PAD1;
      image[0].zoom.y = Ys + dY - PAD1 - WdY;

      /** everything below is tied in x-dir to the zoom box **/
      image[0].text_x = image[0].zoom.x + image[0].zoom.dx + PAD1;
      image[0].text_y = image[0].zoom.y;
      image[0].text_dx = ZOOM_X;
      image[0].text_dy = 6*textpad + 7*PAD1;
      image[0].text_dyo = 3*textpad + 4*PAD1;

      image[0].overlay_button[0].x = image[0].text_x;
      image[0].overlay_button[0].y = image[0].text_y + image[0].text_dy + PAD1;
   
      image[0].overlay_button[1].x = image[0].overlay_button[0].x + image[0].overlay_button[0].dx + PAD1;
      image[0].overlay_button[1].y = image[0].overlay_button[0].y;

      image[0].overlay_button[2].x = image[0].overlay_button[1].x + image[0].overlay_button[1].dx + PAD1;
      image[0].overlay_button[2].y = image[0].overlay_button[0].y;

      image[0].overlay_button[3].x = image[0].overlay_button[2].x + image[0].overlay_button[2].dx + PAD1;
      image[0].overlay_button[3].y = image[0].overlay_button[0].y;

      image[0].hms_button.x = image[0].overlay_button[3].x + image[0].overlay_button[3].dx + PAD1;
      image[0].hms_button.y = image[0].overlay_button[0].y;

      image[0].hex_button.x = image[0].overlay_button[3].x + image[0].overlay_button[3].dx + PAD1;
      image[0].hex_button.y = image[0].overlay_button[0].y + image[0].hms_button.dy + 1;

      image[0].PS_button.x = image[0].text_x;
      image[0].PS_button.y = image[0].overlay_button[0].y + BUTTON_HEIGHT + PAD1;

      /** everything below is tied to the PS_button in y-dir + the neighbor in x-dir **/
      image[0].grey_button.x = image[0].PS_button.x + image[0].PS_button.dx + PAD1;
      image[0].grey_button.y = image[0].PS_button.y;

      image[0].rainbow_button.x = image[0].grey_button.x + image[0].grey_button.dx + PAD1;
      image[0].rainbow_button.y = image[0].PS_button.y;

      image[0].heat_button.x = image[0].rainbow_button.x + image[0].rainbow_button.dx + PAD1;
      image[0].heat_button.y = image[0].PS_button.y;

      image[0].recenter_button.x = image[0].heat_button.x + image[0].heat_button.dx + PAD1;
      image[0].recenter_button.y = image[0].PS_button.y;

      // add just below
      image[0].flipx_button.x = image[0].recenter_button.x + image[0].recenter_button.dx + PAD1;
      image[0].flipx_button.y = image[0].recenter_button.y;

      image[0].flipy_button.x = image[0].hms_button.x + image[0].hms_button.dx + PAD1;
      image[0].flipy_button.y = image[0].hms_button.y;

      // XXX zoom should scale somewhat with the image? (with a min and a max)
      // XXX actually, it is limited by the buttons and status region
      image[0].wide.dx = ZOOM_X; 
      image[0].wide.dy = ZOOM_Y;
      image[0].wide.x = image[0].flipx_button.x + image[0].flipx_button.dx + PAD1;
      image[0].wide.y = image[0].zoom.y;
      break;

    case 3: // zoom / status / wide on top (+x)

      if (haveGraph) {
	image[0].picture.x = graph[0].axis[0].fx;
	image[0].picture.y = graph[0].axis[1].fy + graph[0].axis[1].dfy;
	image[0].picture.dx = MAX(fabs(graph[0].axis[0].dfx) - 1, 1);
	image[0].picture.dy = MAX(fabs(graph[0].axis[1].dfy) - 1, 1);
      } else {
	image[0].picture.x = Xs + PAD1;
	image[0].picture.y = Ys + 3*PAD1 + COLORPAD + WdY;
	image[0].picture.dx = dX - 2*PAD1 - 1; 
	image[0].picture.dy = dY - 4*PAD1 - 1 - WdY - COLORPAD;
      }

      image[0].cmapbar.dx = dX - 2*PAD1; 
      image[0].cmapbar.dy = COLORPAD;
      image[0].cmapbar.x = Xs + PAD1;
      image[0].cmapbar.y = Ys + PAD1;

      // XXX zoom should scale somewhat with the image? (with a min and a max)
      // XXX actually, it is limited by the buttons and status region
      image[0].zoom.dx = ZOOM_X; 
      image[0].zoom.dy = ZOOM_Y;
      image[0].zoom.x = Xs + PAD1;
      image[0].zoom.y = Ys + 2*PAD1 + COLORPAD;

      /** everything below is tied in x-dir to the zoom box **/
      image[0].text_x = image[0].zoom.x + image[0].zoom.dx + PAD1;
      image[0].text_y = image[0].zoom.y;
      image[0].text_dx = ZOOM_X;
      image[0].text_dy = 6*textpad + 7*PAD1;
      image[0].text_dyo = 3*textpad + 4*PAD1;

      image[0].overlay_button[0].x = image[0].text_x;
      image[0].overlay_button[0].y = image[0].text_y + image[0].text_dy + PAD1;
   
      image[0].overlay_button[1].x = image[0].overlay_button[0].x + image[0].overlay_button[0].dx + PAD1;
      image[0].overlay_button[1].y = image[0].overlay_button[0].y;

      image[0].overlay_button[2].x = image[0].overlay_button[1].x + image[0].overlay_button[1].dx + PAD1;
      image[0].overlay_button[2].y = image[0].overlay_button[0].y;

      image[0].overlay_button[3].x = image[0].overlay_button[2].x + image[0].overlay_button[2].dx + PAD1;
      image[0].overlay_button[3].y = image[0].overlay_button[0].y;

      image[0].hms_button.x = image[0].overlay_button[3].x + image[0].overlay_button[3].dx + PAD1;
      image[0].hms_button.y = image[0].overlay_button[0].y;

      image[0].hex_button.x = image[0].overlay_button[3].x + image[0].overlay_button[3].dx + PAD1;
      image[0].hex_button.y = image[0].overlay_button[0].y + image[0].hms_button.dy + 1;

      image[0].PS_button.x = image[0].text_x;
      image[0].PS_button.y = image[0].overlay_button[0].y + BUTTON_HEIGHT + PAD1;

      /** everything below is tied to the PS_button in y-dir + the neighbor in x-dir **/
      image[0].grey_button.x = image[0].PS_button.x + image[0].PS_button.dx + PAD1;
      image[0].grey_button.y = image[0].PS_button.y;

      image[0].rainbow_button.x = image[0].grey_button.x + image[0].grey_button.dx + PAD1;
      image[0].rainbow_button.y = image[0].PS_button.y;

      image[0].heat_button.x = image[0].rainbow_button.x + image[0].rainbow_button.dx + PAD1;
      image[0].heat_button.y = image[0].PS_button.y;

      image[0].recenter_button.x = image[0].heat_button.x + image[0].heat_button.dx + PAD1;
      image[0].recenter_button.y = image[0].PS_button.y;

      // add just below
      image[0].flipx_button.x = image[0].recenter_button.x + image[0].recenter_button.dx + PAD1;
      image[0].flipx_button.y = image[0].recenter_button.y;

      image[0].flipy_button.x = image[0].hms_button.x + image[0].hms_button.dx + PAD1;
      image[0].flipy_button.y = image[0].hms_button.y;

      // XXX zoom should scale somewhat with the image? (with a min and a max)
      // XXX actually, it is limited by the buttons and status region
      image[0].wide.dx = ZOOM_X; 
      image[0].wide.dy = ZOOM_Y;
      image[0].wide.x = image[0].flipx_button.x + image[0].flipx_button.dx + PAD1;
      image[0].wide.y = image[0].zoom.y;
      break;

    case 2: // zoom / status / wide on left (-y)

      if (haveGraph) {
	image[0].picture.x = graph[0].axis[0].fx;
	image[0].picture.y = graph[0].axis[1].fy + graph[0].axis[1].dfy;
	image[0].picture.dx = MAX(fabs(graph[0].axis[0].dfx) - 1, 1);
	image[0].picture.dy = MAX(fabs(graph[0].axis[1].dfy) - 1, 1);
      } else {
	image[0].picture.x = Xs + 2*PAD1 + ZOOM_X;
	image[0].picture.y = Ys + 2*PAD1 + COLORPAD;
	image[0].picture.dx = dX - 3*PAD1 - 1 - ZOOM_X; 
	image[0].picture.dy = dY - 3*PAD1 - 1 - COLORPAD;
      }

      image[0].cmapbar.dx = dX - 2*PAD1; 
      image[0].cmapbar.dy = COLORPAD;
      image[0].cmapbar.x = Xs + PAD1;
      image[0].cmapbar.y = Ys + PAD1;

      // XXX zoom should scale somewhat with the image? (with a min and a max)
      // XXX actually, it is limited by the buttons and status region
      image[0].zoom.dx = ZOOM_X; 
      image[0].zoom.dy = ZOOM_Y;
      image[0].zoom.x = Xs + PAD1;
      image[0].zoom.y = Ys + 2*PAD1 + COLORPAD;

      /** everything below is tied in x-dir to the zoom box **/
      image[0].text_x = image[0].zoom.x;
      image[0].text_y = image[0].zoom.y + image[0].zoom.dy + PAD1;
      image[0].text_dx = ZOOM_X;
      image[0].text_dy = 6*textpad + 7*PAD1;
      image[0].text_dyo = 3*textpad + 4*PAD1;

      image[0].overlay_button[0].x = image[0].text_x;
      image[0].overlay_button[0].y = image[0].text_y + image[0].text_dy + PAD1;
   
      image[0].overlay_button[1].x = image[0].overlay_button[0].x + image[0].overlay_button[0].dx + PAD1;
      image[0].overlay_button[1].y = image[0].overlay_button[0].y;

      image[0].overlay_button[2].x = image[0].overlay_button[1].x + image[0].overlay_button[1].dx + PAD1;
      image[0].overlay_button[2].y = image[0].overlay_button[0].y;

      image[0].overlay_button[3].x = image[0].overlay_button[2].x + image[0].overlay_button[2].dx + PAD1;
      image[0].overlay_button[3].y = image[0].overlay_button[0].y;

      image[0].hms_button.x = image[0].overlay_button[3].x + image[0].overlay_button[3].dx + PAD1;
      image[0].hms_button.y = image[0].overlay_button[0].y;

      image[0].hex_button.x = image[0].overlay_button[3].x + image[0].overlay_button[3].dx + PAD1;
      image[0].hex_button.y = image[0].overlay_button[0].y + image[0].hms_button.dy + 1;

      image[0].PS_button.x = image[0].zoom.x;
      image[0].PS_button.y = image[0].overlay_button[0].y + BUTTON_HEIGHT + PAD1;

      /** everything below is tied to the PS_button in y-dir + the neighbor in x-dir **/
      image[0].grey_button.x = image[0].PS_button.x + image[0].PS_button.dx + PAD1;
      image[0].grey_button.y = image[0].PS_button.y;

      image[0].rainbow_button.x = image[0].grey_button.x + image[0].grey_button.dx + PAD1;
      image[0].rainbow_button.y = image[0].PS_button.y;

      image[0].heat_button.x = image[0].rainbow_button.x + image[0].rainbow_button.dx + PAD1;
      image[0].heat_button.y = image[0].PS_button.y;

      image[0].recenter_button.x = image[0].heat_button.x + image[0].heat_button.dx + PAD1;
      image[0].recenter_button.y = image[0].PS_button.y;

      // add just below
      image[0].flipx_button.x = image[0].PS_button.x;
      image[0].flipx_button.y = image[0].PS_button.y + BUTTON_HEIGHT + PAD1;

      image[0].flipy_button.x = image[0].flipx_button.x + image[0].flipx_button.dx + PAD1;
      image[0].flipy_button.y = image[0].flipx_button.y;

      // XXX zoom should scale somewhat with the image? (with a min and a max)
      // XXX actually, it is limited by the buttons and status region
      image[0].wide.dx = ZOOM_X; 
      image[0].wide.dy = ZOOM_Y;
      image[0].wide.x = image[0].flipx_button.x;
      image[0].wide.y = image[0].flipx_button.y + BUTTON_HEIGHT + PAD1;
      break;

    case 4:  // zoom / status / wide on right (+y)

      if (haveGraph) {
	image[0].picture.x = graph[0].axis[0].fx;
	image[0].picture.y = graph[0].axis[1].fy + graph[0].axis[1].dfy;
	image[0].picture.dx = MAX(fabs(graph[0].axis[0].dfx) - 1, 1);
	image[0].picture.dy = MAX(fabs(graph[0].axis[1].dfy) - 1, 1);
      } else {
	image[0].picture.x = Xs + PAD1;
	image[0].picture.y = Ys + 2*PAD1 + COLORPAD;
	image[0].picture.dx = MAX(dX - 3*PAD1 - 1 - ZOOM_X, 1); 
	image[0].picture.dy = MAX(dY - 3*PAD1 - 1 - COLORPAD, 1);
      }

      image[0].cmapbar.dx = dX - 2*PAD1; 
      image[0].cmapbar.dy = COLORPAD;
      image[0].cmapbar.x = Xs + PAD1;
      image[0].cmapbar.y = Ys + PAD1;

      // XXX zoom should scale somewhat with the image? (with a min and a max)
      // XXX actually, it is limited by the buttons and status region
      image[0].zoom.dx = ZOOM_X; 
      image[0].zoom.dy = ZOOM_Y;
      image[0].zoom.x = Xs + dX - ZOOM_X - PAD1;
      image[0].zoom.y = Ys + 2*PAD1 + COLORPAD;

      /** everything below is tied in x-dir to the zoom box **/
      image[0].text_x = image[0].zoom.x;
      image[0].text_y = image[0].zoom.y + image[0].zoom.dy + PAD1;
      image[0].text_dx = ZOOM_X;
      image[0].text_dy = 6*textpad + 7*PAD1;
      image[0].text_dyo = 3*textpad + 4*PAD1;

      image[0].overlay_button[0].x = image[0].text_x;
      image[0].overlay_button[0].y = image[0].text_y + image[0].text_dy + PAD1;
   
      image[0].overlay_button[1].x = image[0].overlay_button[0].x + image[0].overlay_button[0].dx + PAD1;
      image[0].overlay_button[1].y = image[0].overlay_button[0].y;

      image[0].overlay_button[2].x = image[0].overlay_button[1].x + image[0].overlay_button[1].dx + PAD1;
      image[0].overlay_button[2].y = image[0].overlay_button[0].y;

      image[0].overlay_button[3].x = image[0].overlay_button[2].x + image[0].overlay_button[2].dx + PAD1;
      image[0].overlay_button[3].y = image[0].overlay_button[0].y;

      image[0].hms_button.x = image[0].overlay_button[3].x + image[0].overlay_button[3].dx + PAD1;
      image[0].hms_button.y = image[0].overlay_button[0].y;

      image[0].hex_button.x = image[0].overlay_button[3].x + image[0].overlay_button[3].dx + PAD1;
      image[0].hex_button.y = image[0].overlay_button[0].y + image[0].hms_button.dy + 1;

      image[0].PS_button.x = image[0].zoom.x;
      image[0].PS_button.y = image[0].overlay_button[0].y + BUTTON_HEIGHT + PAD1;

      /** everything below is tied to the PS_button in y-dir + the neighbor in x-dir **/
      image[0].grey_button.x = image[0].PS_button.x + image[0].PS_button.dx + PAD1;
      image[0].grey_button.y = image[0].PS_button.y;

      image[0].rainbow_button.x = image[0].grey_button.x + image[0].grey_button.dx + PAD1;
      image[0].rainbow_button.y = image[0].PS_button.y;

      image[0].heat_button.x = image[0].rainbow_button.x + image[0].rainbow_button.dx + PAD1;
      image[0].heat_button.y = image[0].PS_button.y;

      image[0].recenter_button.x = image[0].heat_button.x + image[0].heat_button.dx + PAD1;
      image[0].recenter_button.y = image[0].PS_button.y;

      // add just below
      image[0].flipx_button.x = image[0].zoom.x;
      image[0].flipx_button.y = image[0].PS_button.y + BUTTON_HEIGHT + PAD1;

      image[0].flipy_button.x = image[0].flipx_button.x + image[0].flipx_button.dx + PAD1;
      image[0].flipy_button.y = image[0].flipx_button.y;

      // XXX zoom should scale somewhat with the image? (with a min and a max)
      // XXX actually, it is limited by the buttons and status region
      image[0].wide.dx = ZOOM_X; 
      image[0].wide.dy = ZOOM_Y;
      image[0].wide.x = image[0].flipx_button.x;
      image[0].wide.y = image[0].flipx_button.y + BUTTON_HEIGHT + PAD1;
      break;

    default:
      abort ();
      break;
  }

  if (USE_XWINDOW) {
    CreatePicture (image, graphic);
    CreateColorbar (image, graphic);
    CreateZoom (graphic, image); 
    CreateWide (graphic, image); 
  }
  Remap (graphic, image);

  return;
}
