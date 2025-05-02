# include "Ximage.h"

int LoadOverlay (int sock) {
  
  int i, j, Ntotal, Nbytes, Nread, Nfound, Ntext, Nobjects, overnum;
  int Noverlay, Ntextdata;
  char *textdata, *buffer, *p, *q;
  Section *section;
  KapaImageWidget *image;
  Graphic *graphic;
  KiiOverlayBase *overlay;

  graphic = GetGraphic ();
  section = GetActiveSection();
  if (section->image == NULL) {
    section->image = InitImageWidget ();
    SetSectionSizes (section);
  }
  image = section->image;

  KiiScanMessage (sock, "%d %d %d %d", &overnum, &Noverlay, &Ntext, &Ntextdata);

  // XXX need to validate overnum 
  if ((overnum < 0) || (overnum >= NOVERLAYS)) overnum = 0;

  // read the overlay data as binary 
  Ntotal = 0;
  Nbytes = Noverlay*sizeof(KiiOverlayBase);
  fcntl (sock, F_SETFL, O_NONBLOCK);  
  ALLOCATE (overlay, KiiOverlayBase, Noverlay);
  buffer = (char *) overlay;
  while (Nbytes > 0) { 
    Nread = read (sock, &buffer[Ntotal], Nbytes);
    // fprintf (stderr, "read: %d of %d remaining, %d so far, %d expected\n", Nread, Nbytes, Ntotal, Noverlay*sizeof(KiiOverlayBase));
    if (Nread == 0) {  /* No more pipe */
      fprintf (stderr, "error: pipe closed\n");
      free (overlay);
      fcntl (sock, F_SETFL, !O_NONBLOCK);  
      return (FALSE);
    }
    if (Nread != -1) { /* pipe has data */
      Nbytes -= Nread;
      Ntotal += Nread;
    }
  }
  fcntl (sock, F_SETFL, !O_NONBLOCK);  
  KiiSendCommand (sock,  4, "DONE");

  // read the textdata as binary
  Ntotal = 0;
  Nbytes = Ntextdata;
  fcntl (sock, F_SETFL, O_NONBLOCK);  
  ALLOCATE (textdata, char, Ntextdata);
  while (Nbytes > 0) { 
    Nread = read (sock, &textdata[Ntotal], Nbytes);
    if (Nread == 0) {  /* No more pipe */
      fprintf (stderr, "error: pipe closed\n");
      free (textdata);
      free (overlay);
      fcntl (sock, F_SETFL, !O_NONBLOCK);  
      return (FALSE);
    }
    if (Nread != -1) { /* pipe has data */
      Nbytes -= Nread;
      Ntotal += Nread;
    }
  }
  fcntl (sock, F_SETFL, !O_NONBLOCK);  
  KiiSendCommand (sock,  4, "DONE");

  // add new overlay objects to existing data
  Nobjects = image[0].overlay[overnum].Nobjects + Noverlay;
  REALLOCATE (image[0].overlay[overnum].objects, KiiOverlay, Nobjects);

  j = image[0].overlay[overnum].Nobjects;
  for (i = 0; i < Noverlay; i++, j++) {
    image[0].overlay[overnum].objects[j].x     = overlay[i].x;
    image[0].overlay[overnum].objects[j].y     = overlay[i].y;
    image[0].overlay[overnum].objects[j].dx    = overlay[i].dx;
    image[0].overlay[overnum].objects[j].dy    = overlay[i].dy;
    image[0].overlay[overnum].objects[j].angle = overlay[i].angle;
    image[0].overlay[overnum].objects[j].type  = overlay[i].type;
    image[0].overlay[overnum].objects[j].text  = NULL;
  }

  // parse the text data : text lines are separated by '\n', one per text entry
  p = textdata;
  Nfound = 0;
  for (i = 0; i < Noverlay; i++) {
    if (overlay[i].type != KII_OVERLAY_TEXT) continue;
    if (Nfound >= Ntext) {
      fprintf (stderr, "inconsistent number of text lines\n");
      break;
    }
    if (! *p) {
      fprintf (stderr, "inconsistent number of text lines\n");
      break;
    }
    q = strchr (p, '\n');
    if (q == NULL) {
      fprintf (stderr, "inconsistent text line\n");
      break;
    }
    j = image[0].overlay[overnum].Nobjects + i;
    image[0].overlay[overnum].objects[j].text = strncreate (p, q-p);
    p = q + 1;
    Nfound ++;
  }
  if (Nfound != Ntext) {
    fprintf (stderr, "read %d text lines, expected %d\n", Nfound, Ntext);
  }

  free (textdata);
  free (overlay);

  image[0].overlay[overnum].Nobjects = Nobjects;
  image[0].overlay[overnum].active = TRUE;

  if (USE_XWINDOW) {
    for (i = 0; i < NOVERLAYS; i++) {
      if (image[0].overlay[i].active) {
	PaintOverlay (graphic, image, i);
      }
    }
    XFlush (graphic[0].display);
  }
  return (TRUE);
}
