# include "dvo.h"
# include "get_graphdata.h"

// check for 'where' or 'matched to'; return first field after the word, and the value
int dbCmdlineConditions (int argc, char **argv, int first, int *nextField) {

  int isWhere, isMatch;

  *nextField = argc;

  // require 'where' or 'matched to' before boolean math
  if (first == argc) return DVO_DB_CMDLINE_IS_END;

  isWhere = !strcasecmp(argv[first], "where");
  isMatch = !strcasecmp(argv[first], "match");
    
  if (!isMatch && !isWhere) {
    gprint (GP_ERR, "WHERE or MATCH TO\n");
    return DVO_DB_CMDLINE_ERROR;
  }
  if (isMatch && ((first + 1 >= argc - 1) || strcasecmp(argv[first+1], "to"))) {
    gprint (GP_ERR, "missing boolean expression\n");
    return DVO_DB_CMDLINE_ERROR;
  }
  if (isWhere && (first >= argc - 1)) {
    gprint (GP_ERR, "missing boolean expression\n");
    return DVO_DB_CMDLINE_ERROR;
  }

  if (isWhere) {
    *nextField = first + 1;
    return DVO_DB_CMDLINE_IS_WHERE;
  }

  if (isMatch) {
    *nextField = first + 2;
    return DVO_DB_CMDLINE_IS_MATCH;
  }

  gprint (GP_ERR, "programming error?\n");
  return DVO_DB_CMDLINE_ERROR;
}

// identify the fields to be extracted (check syntax)
// the 'last' pointer ends on the first word after the fields (where, match or EOL) 
dbField *dbCmdlineFields (int argc, char **argv, int table, int *last, int *nfields) {

  int i, status, Nfields, NFIELDS;
  char *p, *q, *field;
  dbField *fields;

  *nfields = 0;
  Nfields = 0;
  NFIELDS = 10;
  ALLOCATE (fields, dbField, NFIELDS);
  dbInitField (&fields[0]);

  status = FALSE;

  // examine each argv[i] entry until we reach a 'where', a 'matched', or the end of the line
  for (i = 1; (i < argc) && strcasecmp (argv[i], "where") && strcasecmp (argv[i], "match"); i++) {
    // split the word by ","
    p = argv[i];
    while (*p) {
      q = strchr (p, ',');
      if (q == NULL) {
	field = strcreate (p);
	p = p + strlen(p);
      } else {
	field = strncreate (p, q-p);
	p = q + 1;
      }
      // identify field for word
      // need to know which type of fields to look for...
      // xxx extend this more generally later
      if (table == DVO_TABLE_MEASURE) {
	status = ParseMeasureField (&fields[Nfields], field);
      } 
      if (table == DVO_TABLE_AVERAGE) {
	status = ParseAverageField (&fields[Nfields], field);
      } 
      if (table == DVO_TABLE_IMAGE) {
	status = ParseImageField (&fields[Nfields], field);
      } 
      if (!status) {
	free (field);
	dbFreeFields (fields, Nfields);
	return (NULL);
      }
      free (field);

      Nfields ++;
      CHECK_REALLOCATE (fields, dbField, NFIELDS, Nfields, 10);
      dbInitField (&fields[Nfields]);
    }
  }

  *last = i;
  *nfields = Nfields;

  return (fields);
}

char *strfloat (float value) {

  int Nbyte;
  char *output;
  char tmp;

  Nbyte = snprintf (&tmp, 0, "%f", value);
  ALLOCATE (output, char, Nbyte + 1);
  snprintf (output, Nbyte + 1, "%f", value);
  return output;
}

// identify the fields to be extracted (test for where, check syntax)
int dbAstroRegionLimits (dbStack **stack, int *nstack, SkyRegionSelection *selection, int table) {
  
  int N;
  double Rmin, Rmax, Dmin, Dmax;
  char *Rname, *Dname;

  if (!selection->useDisplay && !selection->useSkyregion) return (TRUE);

  // get the ra,dec limits...
  if (selection->useDisplay) {
    // return (TRUE);
    // XXX fix this: be more careful with the projection & limits

    Graphdata graphsky;
    if (!GetGraphdata (&graphsky, NULL, NULL)) {
      gprint (GP_ERR, "region display not available\n");
      return (FALSE);
    }
    
    double Radius = MAX (fabs(graphsky.xmax), fabs(graphsky.ymax));
    Dmin = graphsky.coords.crval2 - Radius;
    Dmax = graphsky.coords.crval2 + Radius;
    
    if ((Dmin <= -89) || (Dmax >= 89)) {
      Rmin = 0;
      Rmax = 360;
    } else {
      double Rmod = MAX (Radius / (cos(Dmin*RAD_DEG)), Radius / (cos(Dmax*RAD_DEG)));
      Rmin = graphsky.coords.crval1 - Rmod;
      Rmax = graphsky.coords.crval1 + Rmod;
    }
    // XXX the ra and dec range depend on the projection. 
    // XXX this is wrong...
    // int status;
    // XY_to_RD (&Rmin, &Dmin, graphsky.xmin, graphsky.ymin, &graphsky.coords);
    // XY_to_RD (&Rmax, &Dmax, graphsky.xmax, graphsky.ymax, &graphsky.coords);
  }

  if (selection->useSkyregion) {
    get_skyregion (&Rmin, &Rmax, &Dmin, &Dmax);
  }    

  N = *nstack;
  REALLOCATE (*stack, dbStack, N + 20);

  Rname = Dname = NULL;
  if (table == DVO_TABLE_MEASURE) {
    Rname = strcreate ("RA:AVE");
    Dname = strcreate ("DEC:AVE");
  }
  if (table == DVO_TABLE_AVERAGE) {
    Rname = strcreate ("RA");
    Dname = strcreate ("DEC");
  }
  if (table == DVO_TABLE_IMAGE) {
    Rname = strcreate ("RA");
    Dname = strcreate ("DEC");
  } 
  if (Rname == NULL) return (FALSE);

  // add: ((ra > rmin) && (ra < rmax) && (dec > dmin) && (dec < dmax))
  // prepend with && if *nstack > 0

  stack[0][N +  0].name = strcreate (Rname);
  stack[0][N +  0].type = DB_STACK_VALUE;
  stack[0][N +  1].name = strfloat (Rmin);
  stack[0][N +  1].type = DB_STACK_VALUE;
  stack[0][N +  2].name = strcreate (">");
  stack[0][N +  2].type = DB_STACK_COMPARE;

  stack[0][N +  3].name = strcreate (Rname);
  stack[0][N +  3].type = DB_STACK_VALUE;
  stack[0][N +  4].name = strfloat (Rmax);
  stack[0][N +  4].type = DB_STACK_VALUE;
  stack[0][N +  5].name = strcreate ("<");
  stack[0][N +  5].type = DB_STACK_COMPARE;
  stack[0][N +  6].name = strcreate ("A");
  stack[0][N +  6].type = DB_STACK_LOGIC;

  stack[0][N +  7].name = strcreate (Dname);
  stack[0][N +  7].type = DB_STACK_VALUE;
  stack[0][N +  8].name = strfloat (Dmin);
  stack[0][N +  8].type = DB_STACK_VALUE;
  stack[0][N +  9].name = strcreate (">");
  stack[0][N +  9].type = DB_STACK_COMPARE;
  stack[0][N + 10].name = strcreate ("A");
  stack[0][N + 10].type = DB_STACK_LOGIC;

  stack[0][N + 11].name = strcreate (Dname);
  stack[0][N + 11].type = DB_STACK_VALUE;
  stack[0][N + 12].name = strfloat (Dmax);
  stack[0][N + 12].type = DB_STACK_VALUE;
  stack[0][N + 13].name = strcreate ("<");
  stack[0][N + 13].type = DB_STACK_COMPARE;
  stack[0][N + 14].name = strcreate ("A");
  stack[0][N + 14].type = DB_STACK_LOGIC;

  if (N == 0) {
    N += 15;
  } else {
    stack[0][N + 15].name = strcreate ("A");
    stack[0][N + 15].type = DB_STACK_LOGIC;
    N += 16;
  }    

  free (Rname);
  free (Dname);

  *nstack = N;
  return (TRUE);
}

