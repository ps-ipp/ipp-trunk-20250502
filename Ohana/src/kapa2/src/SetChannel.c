# include "Ximage.h"

int SetChannel (int sock) {
  
  int Nchannel;
  Graphic *graphic;
  Section *section;
  KapaImageWidget *image;

  graphic = GetGraphic ();
  section = GetActiveSection();
  if (section->image == NULL) {
    section->image = InitImageWidget ();
    SetSectionSizes (section);
  }
  image = section->image;

  KiiScanMessage (sock, "%d", &Nchannel);

  if (Nchannel <  0) return (TRUE);
  if (Nchannel >= NCHANNELS) return (TRUE);
  
  image[0].currentChannel = Nchannel;
  image[0].image = &image[0].channel[image[0].currentChannel];
  SetColorScale (graphic, image);

  if (!USE_XWINDOW) return (TRUE);

  Remap (graphic, image);
  if (DEBUG) fprintf (stderr, "remapped image\n");
  Refresh ();
  if (DEBUG) fprintf (stderr, "refreshed\n");
  XFlush (graphic[0].display);

  return (TRUE);
}

int SetColormapFromPipe (int sock) {
  
  Graphic *graphic;
  Section *section;
  char colormap[256];
  int status;

  KiiScanMessage (sock, "%s", colormap);
  status = SetColormap (colormap);
  if (!status) { 
      fprintf (stderr, "unknown colormap %s\n", colormap);
      return (TRUE);
  }

  graphic = GetGraphic ();
  section = GetActiveSection();
  if (section->image) {
    // section->image = InitImageWidget ();
    // SetSectionSizes (section);
    // image = section->image;
    SetColorScale (graphic, section->image);
    if (!USE_XWINDOW) return TRUE;
    Remap (graphic, section->image);
  }

  Refresh ();
  if (USE_XWINDOW) XFlush (graphic[0].display);

  return (TRUE);
}

int SetNanColorFromPipe (int sock) {

  int i, red_value, green_value, blue_value;
  KiiScanMessage (sock, "%d %d %d", &red_value, &green_value, &blue_value);

  NAN_RED   = red_value;
  NAN_GREEN = green_value;
  NAN_BLUE  = blue_value;

  // use graphic->colormapName saved value
  SetColormap (NULL);

  if (!USE_XWINDOW) return (TRUE);
  
  Graphic *graphic = GetGraphic ();

  int NeedRefresh = FALSE;
  int Nsection = GetNumberOfSections ();
  for (i = 0; i < Nsection; i++) {
    Section *section = GetSectionByNumber (i);
    if (section->image) { 
      NeedRefresh = TRUE;
      SetColorScale (graphic, section->image);
      Remap (graphic, section->image);
    }
  }
  if (NeedRefresh) {
    Refresh ();
    XFlush (graphic[0].display);
  }

  return (TRUE);
}
