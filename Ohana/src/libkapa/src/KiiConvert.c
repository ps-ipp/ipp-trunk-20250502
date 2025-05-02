# include <kapa_internal.h>

int KiiJPEG (int fd, const char *filename) {

  KiiSendCommand (fd, 4, "JPEG");
  KiiSendMessage (fd, "%s", filename);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaPNG (int fd, const char *filename) {

  KiiSendCommand (fd, 4, "PNGF");
  KiiSendMessage (fd, "%s", filename);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaPPM (int fd, const char *filename) {

  KiiSendCommand (fd, 4, "PPMF");
  KiiSendMessage (fd, "%s", filename);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KiiPS (int fd, const char *filename, int scaleMode, int pageMode, char *pagename) {

  KiiSendCommand (fd, 4, "PSIT");
  KiiSendMessage (fd, "%s %s %d %d", filename, pagename, scaleMode, pageMode);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KapaPDF (int fd, const char *filename, int scaleMode, int pageMode, char *pagename) {

  KiiSendCommand (fd, 4, "PDFT");
  KiiSendMessage (fd, "%s %s %d %d", filename, pagename, scaleMode, pageMode);
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}
