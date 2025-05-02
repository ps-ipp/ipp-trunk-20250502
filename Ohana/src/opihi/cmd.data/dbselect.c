# include "data.h"
# if (HAVE_MYSQL_H) 
# include "mysql.h"

int dbselect (int argc, char **argv) {
  
  time_t seconds;
  int i, j, Nbytes, Ncols, Nrows;
  char *query;
  Vector **vec;

  MYSQL_RES *result;
  MYSQL_ROW row;
  MYSQL_FIELD *fields;
  MYSQL *connection = NULL;

  if (argc < 4) {
    gprint (GP_ERR, "USAGE: dbselect (fields) from (table) [where]\n");
    return FALSE;
  }

  connection = db_getConnection ();
  if (connection == NULL) {
    gprint (GP_ERR, "database not defined; use dbconnect\n");
    return (FALSE);
  }

  // generate the query line (concat the argv[i] entries)
  Nbytes = 0;
  for (i = 1; i < argc; i++) {
    Nbytes += strlen(argv[i]) + 1;
  }
  Nbytes += 10;

  ALLOCATE (query, char, Nbytes);
  bzero (query, Nbytes);
  strcat (query, "select ");
  for (i = 1; i < argc; i++) {
    strcat (query, argv[i]);
    strcat (query, " ");
  }
  // strcat (query, ";");
  // fprintf (stderr, "query: %s\n", query);

  if (mysql_query (connection, query)) {
    gprint (GP_ERR, "problem with query\n");
    gprint (GP_ERR, "%s\n", mysql_error (connection));
    free (query);
    return (FALSE);
  }
    
  result = mysql_store_result (connection);

  Nrows = mysql_num_rows(result);
  Ncols = mysql_num_fields(result);
  fields = mysql_fetch_fields (result);

  // fprintf (stderr, "Nrows: %d\n", Nrows);
  // fprintf (stderr, "Ncols: %d\n", Ncols);

  ALLOCATE (vec, Vector *, Ncols);
  for (i = 0; i < Ncols; i++) {
    char *name = fields[i].name ? fields[i].name : fields[i].org_name;
    if ((vec[i] = SelectVector (name, ANYVECTOR, TRUE)) == NULL) {
      gprint (GP_ERR, "trouble creating vector named %s\n", name);
      free (query);
      free (vec);
      return (FALSE);
    }
    switch (fields[i].type) {
      case FIELD_TYPE_TINY:
      case FIELD_TYPE_SHORT:
      case FIELD_TYPE_LONG:
      case FIELD_TYPE_INT24:
      case FIELD_TYPE_LONGLONG:
	ResetVector (vec[i], OPIHI_INT, Nrows);
	break;
      case FIELD_TYPE_DECIMAL:
      case FIELD_TYPE_FLOAT:
      case FIELD_TYPE_DOUBLE:
      case FIELD_TYPE_TIME:
      case FIELD_TYPE_TIMESTAMP:
      case FIELD_TYPE_DATE:
      case FIELD_TYPE_DATETIME:
	ResetVector (vec[i], OPIHI_FLT, Nrows);
	break;
      default:
	ResetVector (vec[i], OPIHI_STR, Nrows);
	break;
    }
  }

  for (j = 0; j < Nrows; j++) {
    row = mysql_fetch_row(result);
    if (row == NULL) {
      gprint (GP_ERR, "inconsistent row count: expected %d, got %d\n", Nrows, j);
      free (query);
      free (vec);
      mysql_free_result (result);
      return (FALSE);
    }
    for (i = 0; i < Ncols; i++) {
      if (row[i]) {
	switch (fields[i].type) {
	  case FIELD_TYPE_TINY:
	  case FIELD_TYPE_SHORT:
	  case FIELD_TYPE_LONG:
	  case FIELD_TYPE_INT24:
	  case FIELD_TYPE_LONGLONG:
	    vec[i][0].elements.Int[j] = atol (row[i]);
	    break;
	  case FIELD_TYPE_DECIMAL:
	  case FIELD_TYPE_FLOAT:
	  case FIELD_TYPE_DOUBLE:
	    vec[i][0].elements.Flt[j] = atof (row[i]);
	    break;
	  case FIELD_TYPE_TIME:
	  case FIELD_TYPE_TIMESTAMP:
	  case FIELD_TYPE_DATE:
	  case FIELD_TYPE_DATETIME:
	    seconds = ohana_date_to_sec (row[i]);
	    vec[i][0].elements.Flt[j] = ohana_sec_to_mjd (seconds);
	    break;
	  default:
	    vec[i][0].elements.Str[j] = strcreate(row[i]);
	}
      } else {
	vec[i][0].elements.Flt[j] = NAN;
      }
    }
  }
  free (query);
  free (vec);
  mysql_free_result (result);
  return (TRUE);
}
# else 

int dbselect (int argc, char **argv) {

  gprint (GP_ERR, "mysql library is not available\n");
  return FALSE;
}

# endif
