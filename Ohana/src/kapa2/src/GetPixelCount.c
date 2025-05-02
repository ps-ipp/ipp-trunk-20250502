# include "Ximage.h"

int GetPixelCount (int sock) {
  
  Graphic *graphic;

  graphic = GetGraphic();

  KiiSendMessage (sock, "NPIX: %8d", graphic->Npixels);
  
  return (TRUE);
}
