# include "pantasks.h"
// we use fprintf for DEBUG statements to avoid deadlocking issues

static int Ninputs = 0;
static int NINPUTS = 0;
static char **inputs;

void InitInputs () {

  Ninputs = 0;
  NINPUTS = 10;
  ALLOCATE (inputs, char *, NINPUTS);
}

void FreeInputs () {
  int i;
  for (i = 0; i < Ninputs; i++) {
    free (inputs[i]);
  }
  free (inputs);
}

/* add this input to the inputs table */
void AddNewInput (char *input) {

  // XXX define the InputMutex
  SerialThreadLock ();

  if (DEBUG) fprintf (stderr, "adding a new input (%s)\n", input);
  inputs[Ninputs] = input;
  Ninputs ++;
  if (Ninputs >= NINPUTS - 1) {
    NINPUTS += 10;
    REALLOCATE (inputs, char *, NINPUTS);
  }
  if (DEBUG) fprintf (stderr, "done new input (%s)\n", input);
  SerialThreadUnlock ();
}

/* remove this input from the inputs table */
int DeleteInput (char *input) {

  int i, j;

  if (DEBUG) fprintf (stderr, "deleting an input (%s)\n", input);

  // XXX lock here
  for (i = 0; i < Ninputs; i++) {
    if (inputs[i] == input) {
      free (inputs[i]);
      for (j = i; j < Ninputs - 1; j++) {
	inputs[j] = inputs[j+1];
      }
      Ninputs --;
      if ((Ninputs > 10) && (Ninputs / 2 < NINPUTS)) {
	NINPUTS = Ninputs + 10;
	REALLOCATE (inputs, char *, NINPUTS);
      }
      // XXX unlock here
      if (DEBUG) fprintf (stderr, "deleted an input\n");
      return TRUE;
    }
  }
  // did not find the input
  // XXX unlock here
  return FALSE;
}

/* if any inputs are pending, run one */
void CheckInputs () {

  int Nbytes, status;
  char *input, *line, *outline, tmp;

  // XXX lock here
  if (Ninputs < 1) {
    // XXX unlock here
    return;
  }

  input = inputs[0];
  if (DEBUG) fprintf (stderr, "got an input (%s)\n", input);
  
  Nbytes = snprintf (&tmp, 0, "input %s", input);
  ALLOCATE (line, char, Nbytes + 1);
  snprintf (line, Nbytes + 1, "input %s", input);
  // XXX unlock here

  status = command (line, &outline, TRUE);
  free (outline);
  
  DeleteInput (input);
}
