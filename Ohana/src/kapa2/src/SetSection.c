# include "Ximage.h"

// set active section
int SetSection (int sock) {
  
  int N;
  char name[128];

  KiiScanMessage (sock, "%s", name);
  
  N = GetSectionByName (name);
  if (N < 0) {
    fprintf (stderr, "section %s not found\n", name);
    return (TRUE);
  }

  SetActiveSectionByNumber (N);
  return (TRUE);
}
