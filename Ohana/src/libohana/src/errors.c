# include "ohana.h"

int Nline = 0;
int NLINE = 0;
char **errorlines = NULL;

int init_error () {

  if (!errorlines) {
    NLINE = 10;
    ALLOCATE (errorlines, char *, NLINE);
    for (int i = 0; i < NLINE; i++) {
      errorlines[i] = NULL;
    }
  }

  for (int i = 0; i < NLINE; i++) {
    FREE (errorlines[i]);
    errorlines[i] = NULL;
  }

  Nline = 0;

  return (TRUE);
}

int push_error (char *line) {

  if (!errorlines) {
    NLINE = 10;
    ALLOCATE (errorlines, char *, NLINE);
    for (int i = 0; i < NLINE; i++) {
      errorlines[i] = NULL;
    }
  }

  errorlines[Nline] = strcreate (line);
  Nline ++;

  if (Nline >= NLINE) {
    NLINE += 10;
    REALLOCATE (errorlines, char *, NLINE);
    for (int i = Nline; i < NLINE; i++) {
      errorlines[i] = NULL;
    }
  }

  return (TRUE);
}

int print_error () {

  for (int i = 0; i < Nline; i++) {
    gprint (GP_ERR, "%s\n", errorlines[i]);
  }
  
  init_error();

  return (TRUE);
}

void free_error() {
  for (int i = 0; i < Nline; i++) {
    FREE (errorlines[i]);
  }
  FREE (errorlines);
}
