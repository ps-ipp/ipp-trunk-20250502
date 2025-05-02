# include "dvoshell.h"

/* list or return all photcodes equivalent to the given filter */
int photcodes (int argc, char **argv) {
  
  int i, Np;
  int *list, Nlist;
  char name[64];
  PhotCode *code;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: photcodes (photcode)\n");
    return (FALSE);
  }

  /* load photcodes, convert name to code */
  if (!InitPhotcodes ()) return (FALSE);

  if (!(Np = GetPhotcodeCodebyName (argv[1]))) {
    gprint (GP_ERR, "ERROR: photcode not found in photcode table\n");
    return (FALSE);
  }

  list = GetPhotcodeEquivList (Np, &Nlist);
  
  for (i = 0; i < Nlist; i++) {
    code = GetPhotcodebyCode (list[i]);

    sprintf (name, "photcode:name:%d", i);
    set_str_variable (name, code[0].name);

    sprintf (name, "photcode:C:%d", i);
    set_variable (name, 0.001*code[0].C);

    sprintf (name, "photcode:K:%d", i);
    set_variable (name, code[0].K);

    sprintf (name, "photcode:X:%d", i);
    set_variable (name, code[0].X[0]);

    sprintf (name, "photcode:dX:%d", i);
    set_variable (name, 0.001*code[0].dX);

    sprintf (name, "photcode:code:%d", i);
    set_int_variable (name, code[0].code);

    sprintf (name, "photcode:filter:%d", i);
    set_str_variable (name, GetPhotcodeNamebyCode (code[0].equiv));

    sprintf (name, "photcode:c1:%d", i);
    set_str_variable (name, GetPhotcodeNamebyCode (code[0].c1));

    sprintf (name, "photcode:c2:%d", i);
    set_str_variable (name, GetPhotcodeNamebyCode (code[0].c2));

    gprint (GP_ERR, "%5d %s %7.4f %7.4f %7.4f\n", 
	     code[0].code, code[0].name, 0.001*code[0].C, code[0].K, code[0].X[0]);
  }
  set_int_variable ("photcode:n", Nlist);
  free (list);
  return (TRUE);
}

