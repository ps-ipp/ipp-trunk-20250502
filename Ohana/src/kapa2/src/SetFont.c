# include "Ximage.h"

int SetFont (int sock) {
  
  char name[64];
  int size;
  
  KiiScanCommand (sock, 16, "%s", name);
  KiiScanCommand (sock, 16, "%d", &size);

  SetRotFont (name, size);
  
  return (TRUE);
  
}
