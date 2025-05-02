# include "Ximage.h"

static int DumpOnExit = FALSE;

int MemoryDump (int sock) {
  OHANA_UNUSED_PARAM(sock);

  ohana_memdump_file (stderr, TRUE);

  return TRUE;
}

int MemoryDumpLines (int sock) {

  int Nlines;
  KiiScanMessage (sock, "%d", &Nlines);

  ohana_memdump_set_maxlines (Nlines);

  return TRUE;
}

int MemoryDumpOnExit (int sock) {
  OHANA_UNUSED_PARAM(sock);

  int state;
  KiiScanMessage (sock, "%d", &state);

  MemoryDumpSetOnExit (state);
  return TRUE;
}

int MemoryDumpSetOnExit (int state) {
  DumpOnExit = state;
  return TRUE;
}

int MemoryDumpAndExit (void) {
  if (DumpOnExit) MemoryDump(0);
  exit (0);
}
