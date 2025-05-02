# include "Ximage.h"

int main (int argc, char **argv) {
  
  args (&argc, argv);

  SetUpGraphic (&argc, argv);

  InitLayout (argc, argv);
  EventLoop ();

  CloseDisplay ();

  // free things
  FreeLayout();
  FreeGraphic();
  FREE (NAME_WINDOW);

  MemoryDumpAndExit ();
}
