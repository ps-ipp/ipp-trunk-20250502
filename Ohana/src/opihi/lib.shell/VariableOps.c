# include "opihi.h"
# define USE_DEPTH 1

Variable *variables;   /* variable to store the list of all variables */
int      Nvariables;   /* number of currently available variables */

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// this function is NOT thread protected : it is only used in startup and/or shutdown
void InitVariables () {
  Nvariables = 0;
  ALLOCATE (variables, Variable, 1); 
}

// this function is NOT thread protected : it is only used in startup and/or shutdown
void FreeVariables () {

  int i; 

  for (i = 0; i < Nvariables; i++) {
    free (variables[i].name);
    free (variables[i].value);
  }
  free (variables);
}

int set_local_variable (char *name, char *value) {

  int i;
  char *local;

  /* a local variable has the form (macroname).(depth).(varname) */
  char *MacroName = GetMacroName ();
  int   MacroDepth = GetMacroDepth ();

  /* need to use a mutex to prevent two threads from changing variable list simultaneously */
  pthread_mutex_lock (&mutex);

  /* look for existing local variable */
  int maxLen = strlen(name) + strlen(MacroName) + 16;
  ALLOCATE (local, char, maxLen);
  if (USE_DEPTH) {
    snprintf (local, maxLen, "%s.%d.%s", MacroName, MacroDepth, name);
  } else {
    snprintf (local, maxLen, "%s.%s", MacroName, name);
  }
  for (i = 0; i < Nvariables; i++) {
    if (!strcmp (local, variables[i].name)) {
      free (variables[i].value);
      free (local);
      variables[i].value = strcreate (value);
      pthread_mutex_unlock (&mutex);
      return (TRUE);
    }
  }
  /* NEW variable */
  Nvariables ++;
  REALLOCATE (variables, Variable, Nvariables);
  variables[Nvariables - 1].name = local;
  variables[Nvariables - 1].value = strcreate (value);
  pthread_mutex_unlock (&mutex);
  return (TRUE);
}

int set_str_variable (char *name, char *value) {

  int i;
  char *local;

  char *MacroName = GetMacroName ();
  int   MacroDepth = GetMacroDepth ();

  /* need to use a mutex to prevent two threads from changing variable list simultaneously */
  pthread_mutex_lock (&mutex);

  /* look for local variable first */
  int maxLen = strlen(name) + strlen(MacroName) + 16;
  ALLOCATE (local, char, maxLen);
  if (USE_DEPTH) {
    snprintf (local, maxLen, "%s.%d.%s", MacroName, MacroDepth, name);
  } else {
    snprintf (local, maxLen, "%s.%s", MacroName, name);
  }
  for (i = 0; i < Nvariables; i++) {
    if (!strcmp (local, variables[i].name)) {
      free (variables[i].value);
      free (local);
      variables[i].value = strcreate (value);
      pthread_mutex_unlock (&mutex);
      return (TRUE);
    }
  }
  free (local);

  /* look for global variable */
  for (i = 0; i < Nvariables; i++) {
    if (!strcmp (name, variables[i].name)) {
      free (variables[i].value);
      variables[i].value = strcreate (value);
      pthread_mutex_unlock (&mutex);
      return (TRUE);
    }
  }
  /* NEW variable */
  Nvariables ++;
  REALLOCATE (variables, Variable, Nvariables);
  variables[Nvariables - 1].name = strcreate (name);
  variables[Nvariables - 1].value = strcreate (value);
  pthread_mutex_unlock (&mutex);
  return (TRUE);
}

int set_variable (char *name, double dvalue) {

  char value[1024];

  sprintf (value, "%.12g", dvalue);
  set_str_variable (name, value);
  return (TRUE);
}

int set_int_variable (char *name, int ivalue) {

  char value[1024];

  sprintf (value, "%d", ivalue);
  set_str_variable (name, value);
  return (TRUE);
}

static char variable_true[] = "1";
static char variable_false[] = "0";

char *get_local_variable_ptr (char *name) {
  
  int i;
  char *local;

  char *MacroName = GetMacroName ();
  int   MacroDepth = GetMacroDepth ();

  /* look for local variable first */
  int maxLen = strlen(name) + strlen(MacroName) + 16;
  ALLOCATE (local, char, maxLen);
  if (USE_DEPTH) {
    snprintf (local, maxLen, "%s.%d.%s", MacroName, MacroDepth, name);
  } else {
    snprintf (local, maxLen, "%s.%s", MacroName, name);
  }

  /* need to use a mutex to prevent another thread from misleading on size of Nvariables */
  pthread_mutex_lock (&mutex);

  for (i = 0; i < Nvariables; i++) { /* find the variable mentioned */
    if (!strcmp(local, variables[i].name)) {
      pthread_mutex_unlock (&mutex);
      free (local);
      return (variables[i].value);
    }
  }
  pthread_mutex_unlock (&mutex);
  free (local);
  return (NULL);
}

char *get_variable_ptr (char *name) {
  
  int i;
  char *local, *value;

  if (name == NULL) return (NULL);
  if (*name == 0) return (NULL);

  /* check for the existence of the given name (after ?) */
  /* return a string which should not be freed */
  if (*name == '?') {
    value = get_variable_ptr (&name[1]);
    if (value == NULL) {
      return variable_false;
    }
    return variable_true;
  }

  // check first for a local variable
  local = get_local_variable_ptr (name);
  if (local != NULL) return (local);

  /* need to use a mutex to prevent another thread from misleading on size of Nvariables */
  pthread_mutex_lock (&mutex);

  /* look for global variable */
  for (i = 0; i < Nvariables; i++) { /* find the variable mentioned */
    if (!strcmp(name, variables[i].name)) {
      pthread_mutex_unlock (&mutex);
      return (variables[i].value);
    }
  }
  pthread_mutex_unlock (&mutex);
  return (NULL);
}

char *get_variable (char *name) {
  
  char *ptr, *value;

  if (name == NULL) return (NULL);
  if (*name == 0) return (NULL);

  ptr = get_variable_ptr (name);
  if (ptr) {
    value = strcreate (ptr);
    return value;
  }
  return (NULL);
}

/* return TRUE / FALSE if name is an existant variable */
// XXX is this function better than IsScalar?
# if (0) 
int get_variable_exists (char *name) {
  
  char *ptr;

  ptr = get_variable_ptr (name);
  if (ptr) return (TRUE);
  return (FALSE);
}
# endif

float get_variable_default (char *name, float dvalue) {

  char *value;
  float fvalue;

  value = get_variable (name);
  if (value == NULL) {
    return (dvalue);
  }
  fvalue = atof (value);
  return (fvalue);
}

double get_double_variable (char *name, int *found) {
  
  char *ptr;

  ptr = get_variable_ptr (name);
  if (ptr) {
    *found = TRUE;
    return (atof (ptr));
  }

  *found = FALSE;
  return (0.0);
}

int get_int_variable (char *name, int *found) {
  
  char *ptr;

  ptr = get_variable_ptr (name);
  if (ptr) {
    *found = TRUE;
    return (atof (ptr));
  }
  *found = FALSE;
  return (0.0);
}

int DeleteNamedScalar (char *name) {

  int i, j;

  /* need to use a mutex to prevent two threads from simultaneously modifying Nvariables */
  pthread_mutex_lock (&mutex);

  for (i = 0; i < Nvariables; i++) {
    if (!strcmp (name, variables[i].name)) {
      free (variables[i].name);
      free (variables[i].value);
      for (j = i; j < Nvariables - 1; j++) {
	variables[j] = variables[j + 1];
      }
      Nvariables --;
      REALLOCATE (variables, Variable, MAX (Nvariables, 1));
      pthread_mutex_unlock (&mutex);
      return (TRUE);
    }
  }
  pthread_mutex_unlock (&mutex);
  return (FALSE);
}

int IsScalar (char *name) { 

  int i;

  /* need to use a mutex to prevent another thread from misleading on size of Nvariables */
  pthread_mutex_lock (&mutex);

  for (i = 0; i < Nvariables; i++) {
    if (!strcmp (name, variables[i].name)) {
      pthread_mutex_unlock (&mutex);
      return (TRUE);
    }
  }
  pthread_mutex_unlock (&mutex);
  return (FALSE);
}

void ListVariables () {

  int i;

  if (Nvariables == 0) {
    gprint (GP_ERR, "No defined variables\n");
    return;
  }

  /* need to use a mutex to prevent another thread from misleading on size of Nvariables */
  pthread_mutex_lock (&mutex);

  for (i = 0; i < Nvariables; i++) {
    gprint (GP_ERR, "%s = %s\n", variables[i].name, variables[i].value);
  }

  pthread_mutex_unlock (&mutex);
  return;
}

int SelectScalar (char *string, double *value) {

  char *end;

  /* if string is a number, return TRUE */
  *value = strtod (string, &end);
  if (end == string + strlen(string)) return (TRUE);

  return (FALSE);
}
