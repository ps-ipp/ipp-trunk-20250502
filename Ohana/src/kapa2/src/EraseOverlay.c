# include "Ximage.h"

int EraseOverlay (int sock) {

  int i, N;
  Section *section;
  KapaImageWidget *image;

  // We have to accept the incoming message even if we cannot perform the action
  KiiScanCommand (sock, 16, "%*s %d", &N);

  section = GetActiveSection();
  image = section->image;
  if (image == NULL) return (TRUE);

  if (N > NOVERLAYS) {
    REALLOCATE (image[0].tickmarks.objects, KiiOverlay, 1);
    image[0].tickmarks.Nobjects = 0;
  } else {
    for (i = 0; i < image[0].overlay[N].Nobjects; i++) {
      if (image[0].overlay[N].objects[i].type == KII_OVERLAY_TEXT) {
	free (image[0].overlay[N].objects[i].text);
      }
    }
    REALLOCATE (image[0].overlay[N].objects, KiiOverlay, 1);
    image[0].overlay[N].Nobjects = 0;
    image[0].overlay[N].active = FALSE;
  }

  if (USE_XWINDOW) Refresh ();
  return (TRUE);
}
