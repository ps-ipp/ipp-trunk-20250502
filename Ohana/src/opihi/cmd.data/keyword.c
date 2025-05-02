# include "data.h"

enum {NONE, STRING, FLOAT, INT, BOOLEAN, KEYCOMMENT, COMMENT};

/** WARNING: no error checking on variable validity **/
int keyword (int argc, char **argv) {

  int ivalue, status, N, ascomment, asfloat, delete, soft, Wmode;
  char line[80];
  double value;
  Buffer *buf;

  asfloat = FALSE;
  if ((N = get_argument (argc, argv, "-f"))) {
    remove_argument (N, &argc, argv);
    asfloat = TRUE;
  }
  
  ascomment = FALSE;
  if ((N = get_argument (argc, argv, "-c"))) {
    remove_argument (N, &argc, argv);
    ascomment = TRUE;
  }
  
  delete = FALSE;
  if ((N = get_argument (argc, argv, "-d"))) {
    remove_argument (N, &argc, argv);
    delete = TRUE;
  }
  
  /* return TRUE for missing keyword if soft */
  soft = FALSE;
  if ((N = get_argument (argc, argv, "-soft"))) {
    remove_argument (N, &argc, argv);
    soft = TRUE;
  }
  
  /* identify write modes */
  Wmode = NONE;
  if ((N = get_argument (argc, argv, "-w"))) {
    remove_argument (N, &argc, argv);
    strcpy (line, argv[N]);
    remove_argument (N, &argc, argv);
    Wmode = STRING;
  }
  if ((N = get_argument (argc, argv, "-wf"))) {
    remove_argument (N, &argc, argv);
    strcpy (line, argv[N]);
    remove_argument (N, &argc, argv);
    Wmode = FLOAT;
  }
  if ((N = get_argument (argc, argv, "-wd"))) {
    remove_argument (N, &argc, argv);
    strcpy (line, argv[N]);
    remove_argument (N, &argc, argv);
    Wmode = INT;
  }
  if ((N = get_argument (argc, argv, "-wc"))) {
    remove_argument (N, &argc, argv);
    strcpy (line, argv[N]);
    remove_argument (N, &argc, argv);
    Wmode = KEYCOMMENT;
  }
  if ((N = get_argument (argc, argv, "-ws"))) {
    remove_argument (N, &argc, argv);
    strcpy (line, argv[N]);
    remove_argument (N, &argc, argv);
    Wmode = COMMENT;
  }
  if ((N = get_argument (argc, argv, "-wb"))) {
    remove_argument (N, &argc, argv);
    strcpy (line, argv[N]);
    remove_argument (N, &argc, argv);
    Wmode = BOOLEAN;
  }

  if (!((argc == 3) || (!N && argc == 4))) {
    gprint (GP_ERR, "USAGE: keyword <buffer> (KEYWORD) [variable] [-d] [-w(mode) value]\n");
    gprint (GP_ERR, " -w modes: \n");
    gprint (GP_ERR, "  -w  - string\n");
    gprint (GP_ERR, "  -wf - float\n");
    gprint (GP_ERR, "  -wd - int\n");
    gprint (GP_ERR, "  -wb - boolean\n");
    gprint (GP_ERR, "  -wc - comment\n");
    gprint (GP_ERR, "  -ws - full string comment\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  if (Wmode != NONE) {
    switch (Wmode) {
    case STRING:
      gfits_modify (&buf[0].header, argv[2], "%s", 1, line);
      return (TRUE);
    case FLOAT:
      value = atof(line);
      gfits_modify (&buf[0].header, argv[2], "%lf", 1, value);
      return (TRUE);
    case INT:
      gfits_modify (&buf[0].header, argv[2], "%d", 1, atoi(line));
      return (TRUE);
    case BOOLEAN:
      if (strcasecmp (line, "T") && strcasecmp (line, "TRUE") && strcasecmp (line, "F") && strcasecmp (line, "FALSE")) {
	gprint (GP_ERR, "syntax error in boolean value\n");
	return (FALSE);
      }
      ivalue = !strcasecmp (line, "T");
      gfits_modify_alt (&buf[0].header, argv[2], "%t", 1, ivalue);
      return (TRUE);
    case KEYCOMMENT:
      gfits_modify_alt (&buf[0].header, argv[2], "%C", 1, line);
      return (TRUE);
    case COMMENT:
      gfits_modify_alt (&buf[0].header, argv[2], "%S", 0, line);
      return (TRUE);
    }
  }
  
  if (delete) {
    gfits_delete (&buf[0].header, argv[2], -1);
    return (TRUE);
  }
  
  /* grab the value in the given format, either a string or a digit */
  if (asfloat) {
    status = gfits_scan (&buf[0].header, argv[2], "%lf", 1, &value);
    if (!status) goto failure;
    if (argc == 4) 
      set_variable (argv[3], value);
    else 
      gprint (GP_LOG, "%s: %f\n", argv[2], value);
    return (TRUE);
  } 

  if (ascomment) {
    status = gfits_scan_alt (&buf[0].header, argv[2], "%C", 1, line);
    if (!status) goto failure;
    if (argc == 4) 
      set_str_variable (argv[3], line);
    else 
      gprint (GP_LOG, "%s: %s\n", argv[2], line);
    return (TRUE);
  }    

  /* not-specified */
  status = gfits_scan (&buf[0].header, argv[2], "%s", 1, line);
  if (!status) goto failure;
  if (argc == 4) 
    set_str_variable (argv[3], line);
  else 
    gprint (GP_LOG, "%s: %s\n", argv[2], line);
  return (TRUE);

 failure: 
  if (!soft) gprint (GP_ERR, "keyword %s not found\n", argv[2]);
  return (soft);
}
