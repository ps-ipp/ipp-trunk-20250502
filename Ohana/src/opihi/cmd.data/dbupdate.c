# include "data.h"
# if (HAVE_MYSQL_H) 
# include "mysql.h"

int dbupdate (int argc, char **argv) {
  
  int i, Nbytes;
  char *query;

  MYSQL *connection = NULL;

  if (argc < 4) {
    gprint (GP_ERR, "USAGE: dbupdate (tablerefs) set field = value,... [where]");
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
  strcat (query, "update ");
  for (i = 1; i < argc; i++) {
    strcat (query, argv[i]);
    strcat (query, " ");
  }
  
  fprintf (stderr, "query: %s\n", query);

  if (mysql_query (connection, query)) {
    gprint (GP_ERR, "problem with query\n");
    gprint (GP_ERR, "%s\n", mysql_error (connection));
    free (query);
    return (FALSE);
  }
  free (query);
  return (TRUE);
}
# else 

int dbupdate (int argc, char **argv) {

  gprint (GP_ERR, "mysql library is not available\n");
  return FALSE;
}

# endif
