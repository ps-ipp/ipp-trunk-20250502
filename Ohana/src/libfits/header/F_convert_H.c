# include <ohana.h>
# include <gfitsio.h>

int gfits_primary_to_extended (Header *header, char *exttype, char *comment) {

  int Ns, No;
  char line[81];

  // XXX check for valid exttype, comment, output string
  snprintf (line, 81, "%-8s= '%-18s' / %-s ", "XTENSION", exttype, comment);
  Ns = strlen (line);
  No = 80 - Ns;
  strncpy_nowarn (header->buffer, line, Ns);
  memset (&header->buffer[Ns], ' ', No);

  return (TRUE);
}

// don't require the current to have SIMPLE
int gfits_modify_extended (Header *header, char *exttype, char *comment) {

  int Ns, No;
  char line[81];

  // XXX check for valid exttype, comment, output string
  snprintf (line, 81, "%-8s= '%-18s' / %-s ", "XTENSION", exttype, comment);
  Ns = strlen (line);
  No = 80 - Ns;
  strncpy_nowarn (header->buffer, line, Ns);
  memset (&header->buffer[Ns], ' ', No);

  return (TRUE);
}

int gfits_extended_to_primary (Header *header, int simple, char *comment) {

  int Ns, No;
  char line[81];

  // XXX check for valid exttype, comment, output string
  if (simple) {
    snprintf (line, 81, "%-8s= %-18s T / %-s ", "SIMPLE", " ", comment);
  } else {
    snprintf (line, 81, "%-8s= %-18s F / %-s ", "SIMPLE", " ", comment);
  }
  Ns = strlen (line);
  No = 80 - Ns;
  strncpy_nowarn (header->buffer, line, Ns);
  memset (&header->buffer[Ns], ' ', No);

  return (TRUE);
}
