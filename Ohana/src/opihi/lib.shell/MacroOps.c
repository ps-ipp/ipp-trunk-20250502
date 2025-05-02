# include "opihi.h"

// the macro table is only modified by macro_create: if thread protection is needed, it
// should be added there.  it may not be needed if programs only call macro_create in a
// single thread.  pantasks_server only calls macro_create in the 'input' thread, so it is
// safe.

static char dot[] = ".";

static Macro *macros;
static int   Nmacros;
static int   NMACROS;

static char *MacroName = dot;
static int   MacroDepth = 0;

void InitMacros () {
  NMACROS = 20;
  Nmacros = 0;
  ALLOCATE (macros, Macro, NMACROS);
  MacroName = dot;
  MacroDepth = 0;
}

void FreeMacros () {

  int i;

  for (i = 0; i < Nmacros; i++) {
    FreeMacro (&macros[i]);
  }
  free (macros);
}

void SetCurrentMacroData (char *name, int depth) {
  MacroName = name;
  MacroDepth = depth;
}

char *GetMacroName () {
  return (MacroName);
}

int GetMacroDepth () {
  return (MacroDepth);
}

Macro *NewMacro (char *name) {
  
  macros[Nmacros].name = strcreate (name);;
  macros[Nmacros].Nlines = 0;
  ALLOCATE (macros[Nmacros].line, char *, 1);
  Nmacros ++;
  if (Nmacros == NMACROS) {
    NMACROS += 20;
    REALLOCATE (macros, Macro, NMACROS);
  }
  return (&macros[Nmacros-1]);
}

void ListMacro (Macro *macro) {

  int i;

  if ((macro == NULL) || (macro[0].Nlines == 0)) {
    gprint (GP_ERR, "  macro not defined\n");
    return;
  }
  for (i = 0; i < macro[0].Nlines; i++) {
    gprint (GP_ERR, "  %s\n", macro[0].line[i]);
  }
  return;
}

void ListMacros () {
  int i;
  for (i = 0; i < Nmacros; i++) {
    gprint (GP_ERR, "%s\n", macros[i].name);
  }
}

void FreeMacro (Macro *macro) {
  
  int i;

  if (macro == NULL) return;

  for (i = 0; i < macro[0].Nlines; i++) {
    free (macro[0].line[i]);
  }
  free (macro[0].line);
  free (macro[0].name);
  return;
}

int DeleteMacro (Macro *macro) {

  int i, Nm;

  Nm = -1;
  for (i = 0; i < Nmacros; i++) {
    if (macro == &macros[i]) {
      Nm = i;
      break;
    }
  }
  if (Nm == -1) {
    gprint (GP_ERR, "programming error: macro not found\n");
    return (FALSE);
  }

  FreeMacro (&macros[Nm]);
  for (i = Nm + 1; i < Nmacros; i++)
    macros[i - 1] = macros[i];
  Nmacros --;
  return (TRUE);
}

/* return macro which unambiguously matches name */
Macro *MatchMacro (char *name, int VERBOSE, int EXACT) {

  int i, match[10], Nmatch;

  /* try for an exact match first */
  for (i = 0; i < Nmacros; i++) {
    if (!strcmp (macros[i].name, name)) {
      return (&macros[i]);
    }
  }
  if (EXACT) {
    if (VERBOSE) gprint (GP_ERR, "no exact match to %s\n", name);
    return (NULL);
  }
      
  /* not found as complete macro, try partial */
  Nmatch = 0;
  for (i = 0; (Nmatch < 10) && (i < Nmacros); i++) {
    if (!strncmp (macros[i].name, name, strlen(name))) {  /* found a macro */
      match[Nmatch] = i;
      Nmatch ++;
    }
  }
  if (Nmatch == 1) return (&macros[match[0]]);

  if (Nmatch > 1) {
    if (VERBOSE) {
      gprint (GP_ERR, "ambiguous macro: %s ( ", name);
      for (i = 0; i < Nmatch; i++) {
	gprint (GP_ERR, "%s ", macros[match[i]].name);
      }
      gprint (GP_ERR, ")\n");
    }
    return (NULL);
  }
  if (VERBOSE) gprint (GP_ERR, "%s: Macro not found.\n", name);
  return (NULL);
}  

