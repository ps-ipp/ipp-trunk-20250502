# include "elixir.h"

static struct timeval then = {0.0, 0.0};

/* LoadPending returns the number of objects loaded from inlist
   
   state can be: 
   0 - success
   1 - file locked
   2 - file has EOF flag
   3 - error in input 
*/

int LoadPending (Process *global, char *filename, int *state, int *dynamic) {

  int i, status, depend, Nobjects;
  int lockstate, mode;
  FILE *f;
  char name[7][256], line[1024];
  struct timeval now;
  Object *object;

  *state = 0;
  Nobjects = 0;

  /* we should only do this check every 1sec or so */
  gettimeofday (&now, (void *) NULL);
  if (DTIME (now, then) < 1.0) return (Nobjects);
  then = now;

  /* if dynamic, we must lock the file first, but not if not dynamic */
  if (*dynamic) {
    
    /* check lockfile - don't remove list if locked */
    f = fsetlockfile (filename, 1.0, LCK_XCLD, &lockstate);
    if (f == NULL) { 
      *state = 1;
      return (Nobjects);
    }
  } else {
    f = fopen (filename, "r");
    if (f == (FILE *) NULL) {
      *state = 1;
      return (Nobjects);
    }
  }

  /* read lines, parse each line */
  while (1) {
    if (scan_line (f, line) == EOF) goto escape;

    switch (global[0].argc) {
    case 1:
      status = sscanf (line, "%s", name[0]);
      break;
    case 2:
      status = sscanf (line, "%s%s", name[0], name[1]);
      break;
    case 3:
      status = sscanf (line, "%s%s%s", name[0], name[1], name[2]);
      break;
    case 4:
      status = sscanf (line, "%s%s%s%s", name[0], name[1], name[2], name[3]);
      break;
    case 5:
      status = sscanf (line, "%s%s%s%s%s", name[0], name[1], name[2], name[3], name[4]);
      break;
    case 6:
      status = sscanf (line, "%s%s%s%s%s%s", name[0], name[1], name[2], name[3], name[4], name[5]);
      break;
    case 7:
      status = sscanf (line, "%s%s%s%s%s%s%s", name[0], name[1], name[2], name[3], name[4], name[5], name[6]);
      break;
    default:
      fprintf (stderr, "ERROR: unexpected number of entries per line in input file\n");
      *state = 3;
      goto escape;
    }

    if (status == 0) continue;  /* an empty or blank line */

    if (!strcasecmp (name[0], "EOF")) {
      /* dynamic -> static */
      *state = 2;
      goto escape;
    }

    /* silently ignore lines with the wrong number of args */
    if (status != global[0].argc) continue;

    ALLOCATE (object, Object, 1);
    object[0].argc = status;
    ALLOCATE (object[0].argv, char *, status);
    for (i = 0; i < status; i++) {
      object[0].argv[i] = strcreate (name[i]);
    }
    
    /* convert the global.logfile description to a specific logfile for this object */
    ParseLine (global[0].argv[3], object[0].argc, object[0].argv, &depend, &object[0].logfile);
    object[0].status = 0;
    object[0].timer.tv_sec = 0;
    object[0].lastproc = (char *) NULL;
    PutObject (global[0].pending, object);
    Nobjects ++;
  }

  escape:
  if (*dynamic) {
    if (truncate (filename, 0)) {
      fprintf (stderr, "failed to clear source %s (errno %d)\n", filename, errno);
      Shutdown (1);
    }

    /* clean up fifo - set mode to 666, unlock */
    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    chmod (filename, mode);
    fclearlockfile (filename, f, LCK_XCLD, &lockstate);
  } else {
    fclose (f);
  }

  if ((*state == 2) && *dynamic) {
    *dynamic = FALSE;
  }
  return (Nobjects);

}
