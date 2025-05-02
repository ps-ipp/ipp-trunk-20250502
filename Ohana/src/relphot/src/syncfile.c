# include "relphot.h"
# define MSG_LENGTH 12

char *make_filename (char *dirname, char *hostname, int hostID, char *tailname) {

  char tmp[10], *line; 
  int Nchar = snprintf (tmp, 0, "%s/%s.%03d.%s", dirname, hostname, hostID, tailname);
  
  ALLOCATE (line, char, Nchar + 1);
  snprintf (line, Nchar + 1, "%s/%s.%03d.%s", dirname, hostname, hostID, tailname);

  return line;
}

int check_sync_file (char *filename, int nloop) {

  char message[MSG_LENGTH];

  FILE *f = NULL; 

  while (TRUE) {

    f = fopen (filename, "r");
    if (!f) {
      usleep (2000000);
      continue;
    }

    // XXX MSG_LENGTH : 0 EOL byte?
    int Nread = fread (message, 1, MSG_LENGTH, f);
    if (Nread < MSG_LENGTH) {
      fclose (f);
      usleep (2000000);
      continue;
    }
    fclose (f);

    // message is of the form: NLOOP: %03d
    int loop;
    sscanf (message, "%*s %d", &loop);
    if (loop != nloop) {
      usleep (2000000);
      continue;
    }
    return TRUE;
  }
  return FALSE;
}

int clear_sync_file (char *filename) {
  // delete file contents
  if (truncate (filename, 0)) fprintf (stderr, "trouble clearing file %s\n", filename);

  return TRUE;
}

int update_sync_file (char *filename, int nloop) {

  char message[MSG_LENGTH];

  FILE *f = fopen (filename, "w");
  if (!f) { 
    fprintf (stderr, "failure to open sync file for write\n");
    exit (4);
  }

  snprintf (message, MSG_LENGTH, "NLOOP: %03d\n", nloop);
  
  int Nwrite = fwrite (message, 1, MSG_LENGTH, f);
  if (Nwrite != MSG_LENGTH) {
    fprintf (stderr, "failure to write sync message\n");
    exit (3);
  }

  fclose (f);
  return TRUE;
}
