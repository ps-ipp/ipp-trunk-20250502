# include "data.h"
# if (HAVE_MYSQL_H) 
# include "mysql.h"

int dbadmin (int argc, char **argv) {
  
  int i, Nbytes;
  char *query;

  MYSQL *connection = NULL;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: dbadmin [command]\n");
    gprint (GP_ERR, "  currently allowed commands: CREATE, DROP, USE\n");
    return FALSE;
  }

  connection = db_getConnection ();
  if (connection == NULL) {
    gprint (GP_ERR, "database not defined; use dbconnect\n");
    return (FALSE);
  }

  // check for allowed sql commands
  if (!strcasecmp(argv[1], "USE")) goto valid;
  if (!strcasecmp(argv[1], "DROP")) goto valid;
  if (!strcasecmp(argv[1], "CREATE")) goto valid;
  gprint (GP_ERR, "currently allowed dbadmin commands are CREATE, DROP\n");
  return (FALSE);

valid:

  // generate the query line (concat the argv[i] entries)
  Nbytes = 0;
  for (i = 1; i < argc; i++) {
    Nbytes += strlen(argv[i]) + 1;
  }
  Nbytes += 10;

  ALLOCATE (query, char, Nbytes);
  bzero (query, Nbytes);
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

int dbadmin (int argc, char **argv) {

  gprint (GP_ERR, "mysql library is not available\n");
  return FALSE;
}

# endif
