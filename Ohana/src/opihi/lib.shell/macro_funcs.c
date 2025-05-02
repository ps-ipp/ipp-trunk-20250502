# include "basic.h"

static Command macro_command[] = {
  {1, "create", macro_create, "create a macro *"},
  {1, "delete", macro_delete, "delete a macro *"},
  {1, "list",   macro_list_f, "list a macro *"},
  {1, "edit",   macro_edit,   "edit a macro <not working yet!> *"},
  {1, "read",   macro_read,   "read a macro from a file <not working yet!> *"},
  {1, "write",  macro_write,  "write a macro to a file <not working yet!> *"}
};

CommandF *find_macro_command (char *name) {

  int i, N;

  N = sizeof (macro_command) / sizeof (Command);

  /* find the macro sub-command which matches from the list. */
  for (i = 0; i < N; i++) {
    if (!strcmp (macro_command[i].name, name)) {
      return (macro_command[i].func);
    }
  }

  return NULL;
}
