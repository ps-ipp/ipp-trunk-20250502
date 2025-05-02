# include <kapa_internal.h>

int KiiCursorOn (int fd) {

  KiiSendCommand (fd, 4, "CURS");
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KiiCursorOff (int fd) {

  KiiSendCommand (fd, 4, "NCUR");
  KiiWaitAnswer (fd, "DONE");
  return (TRUE);
}

int KiiCursorRead (int fd, double *x, double *y, double *z, double *r, double *d, char *key) {

  KiiScanMessage (fd, "%s %lf %lf %lf %lf %lf", key, x, y, z, r, d);
  if (ispunct(key[0])) strcpy (key, "_");
  return (TRUE);
}
