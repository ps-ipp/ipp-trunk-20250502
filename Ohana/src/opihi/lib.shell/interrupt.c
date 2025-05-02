# include "opihi.h"

static int Nint = 0;

/* return FALSE if we are called many times in a row 
   without a valid answer or any answer */

void handle_interrupt (int input) {
  OHANA_UNUSED_PARAM(input);
  
  char string[64];
  int Nask;

  // signal (SIGINT, SIG_IGN);

  Nask = 0;
  Nint ++;

  // 3 ctrl-c in a row will interrupt regardless
  if (Nint > 3) { 
    interrupt = TRUE;
    return;
  }
  if (Nint > 100) { 
    exit (3);
  }
  
  while (1) {
    gprint (GP_ERR, "operation halted, continue? (y/n) ");
    if (fscanf (stdin, "%s", string) != 1) fprintf (stderr, "what?\n");
    
    if ((string[0] == 'y') || (string[0] == 'Y')) {
      interrupt = FALSE;
      // signal (SIGINT, handle_interrupt);
      Nint = 0;
      return;
    }

    if ((string[0] == 'n') || (string[0] == 'N')) {
      interrupt = TRUE;
      // signal (SIGINT, handle_interrupt);
      Nint = 0;
      return;
    }
    Nask ++;
    if (Nask > 3) {
      interrupt = TRUE;
      // signal (SIGINT, handle_interrupt);
      Nint = 0;
      return;
    }
  }  

}

struct sigaction *SetInterrupt () {

  struct sigaction  new_sigaction;
  struct sigaction *old_sigaction;

  ALLOCATE (old_sigaction, struct sigaction, 1);

  new_sigaction.sa_handler = handle_interrupt;
  new_sigaction.sa_flags = 0;

  int sigstat = sigaction (SIGINT, &new_sigaction, old_sigaction);
  if (sigstat) {
    perror ("failed to set signal handler: ");
    free (old_sigaction);
    return NULL;
  }

  interrupt = FALSE;
  return old_sigaction;
}

int ClearInterrupt (struct sigaction *old_sigaction) {

  // interrupt = FALSE;

  if (!old_sigaction) return TRUE;

  struct sigaction new_sigaction;

  if (sigaction (SIGINT, old_sigaction, &new_sigaction)) {
    perror ("failed to reset signal handler: ");
    FREE (old_sigaction);
    return FALSE;
  }
  FREE (old_sigaction);

  return TRUE;
}
