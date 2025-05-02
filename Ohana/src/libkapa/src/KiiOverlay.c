# include <kapa_internal.h>

# define NOVERLAY_TYPE 5
static char KiiOverlayTypeName[NOVERLAY_TYPE][16] = {
  "NONE",
  "TEXT", 
  "BOX", 
  "LINE",
  "CIRCLE", 
};

int KiiOverlayTypeByName (char *overname) {

  int i;

  for (i = 1; i < NOVERLAY_TYPE; i++) {
    if (!strcasecmp (overname, KiiOverlayTypeName[i])) return (i);
  }
  return (0);
}

char *KiiOverlayTypeByNumber (int n) {

  if ((n < 0) || (n >= NOVERLAY_TYPE)) return NULL;
  return (KiiOverlayTypeName[n]);
}

int KiiSelectOverlay (char *overname, int *number) {

  *number = -1;
  if (!strcmp (overname, "red") || !strcmp (overname, "0")) {
    *number = 0;
    return (TRUE);
  }
  if (!strcmp (overname, "green") || !strcmp (overname, "1")) {
    *number = 1;
    return (TRUE);
  }
  if (!strcmp (overname, "blue") || !strcmp (overname, "2")) {
    *number = 2;
    return (TRUE);
  }
  if (!strcmp (overname, "yellow") || !strcmp (overname, "3")) {
    *number = 3;
    return (TRUE);
  }

  fprintf (stderr, "valid overlays may be: red (0), green (1), blue (2), yellow (3)\n");
  return (FALSE);
}

int KiiLoadOverlay (int fd, KiiOverlay *overlay, int Noverlay, char *overname) {

  int i, overnum, Ntextdata, NTEXTDATA, Ntext, Nchar, nchar;
  char *textdata;
  KiiOverlayBase *buffer;

  Ntext = 0;
  KiiSelectOverlay (overname, &overnum);

  Ntextdata = 0;
  NTEXTDATA = 1024;
  ALLOCATE (textdata, char, 1024);

  // we send the position information as a binary block
  ALLOCATE (buffer, KiiOverlayBase, Noverlay);
  for (i = 0; i < Noverlay; i++) {
    buffer[i].x     = overlay[i].x;
    buffer[i].y     = overlay[i].y;
    buffer[i].dx    = overlay[i].dx;
    buffer[i].dy    = overlay[i].dy;
    buffer[i].angle = overlay[i].angle;
    buffer[i].type  = overlay[i].type;
    if (buffer[i].type == KII_OVERLAY_TEXT) {
      Ntext ++;
      Nchar = strlen(overlay[i].text) + 1;
      if (Ntextdata + Nchar >= NTEXTDATA) {
	NTEXTDATA += 1024;
	REALLOCATE (textdata, char, NTEXTDATA);
      }
      sprintf (&textdata[Ntextdata], "%s\n", overlay[i].text);
      Ntextdata += Nchar;
    }
  }

  KiiSendCommand (fd,  4, "LOAD");
  KiiSendMessage (fd, "%d %d %d %d", overnum, Noverlay, Ntext, Ntextdata);

  // we could break this into segments if we want to trap an interrupt, but why bother?
  nchar = Noverlay*sizeof(KiiOverlayBase);
  Nchar = write (fd, buffer, nchar);
  if (Nchar != nchar) {
    fprintf (stderr, "comm error\n");
    return FALSE;
  }
  KiiWaitAnswer (fd, "DONE");

  Nchar = write (fd, textdata, Ntextdata);
  if (Nchar != Ntextdata) {
    fprintf (stderr, "comm error\n");
    return FALSE;
  }
  KiiWaitAnswer (fd, "DONE"); // this 'DONE' notes the end of the textdata buffer

  free (buffer);
  free (textdata);

  KiiWaitAnswer (fd, "DONE"); // this 'DONE' notes the end of the command
  return (TRUE);
}

int KiiEraseOverlay (int fd, char *overname) {

  int n;

  KiiSelectOverlay (overname, &n);
    
  KiiSendCommand (fd, 4, "ERSO");
  KiiSendCommand (fd, 16, "OVERLAY %7d ", n);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KiiSaveOverlay (int fd, int celestial, char *overname, char *file) {

  int n;

  KiiSelectOverlay (overname, &n);
    
  if (celestial) {
    KiiSendCommand (fd, 4, "CSVE");
  } else {
    KiiSendCommand (fd, 4, "SAVE");
  }

  KiiSendMessage (fd, "FILE: %d %s", n, file);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}
