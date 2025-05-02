# include "data.h"
# if (HAVE_MYSQL_H)
# include "mysql.h"

MYSQL mysql;
MYSQL *connection = NULL;

int dbconnect (int argc, char **argv) {
  
  int N;
  char query[256];
  MYSQL_RES *result;

  char *password = NULL;
  if ((N = get_argument (argc, argv, "-p"))) {
    remove_argument (N, &argc, argv);
    password = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: dbconnect (hostname) (username) (database)\n");
    return FALSE;
  }

  if (!password) {
      ALLOCATE (password, char, 1024);
      fprintf (stdout, "Enter password: ");
      scan_line (stdin, password);

# if (0)
      int i;
      char c;

      initscr();
      noecho();
      i = 0;
      while (((c = getch()) != EOF) && (c != '\n')) {
	  password[i] = c;
	  i++;
      }
      password[i] = 0;
# endif
  }

  // XXX do I need to call mysql_library_init()?

  mysql_init (&mysql);
  connection = mysql_real_connect (&mysql, argv[1], argv[2], password, argv[3], 0, 0, 0);

  if (connection == NULL) {
    gprint (GP_ERR, "failed to connect to database\n");
    gprint (GP_ERR, "%s\n", mysql_error (&mysql));
    return (FALSE);
  }
    
  sprintf (query, "set @@interactive_timeout = 30000");
  if (mysql_query (connection, query)) {
    gprint (GP_ERR, "failed to set interactive timout\n");
    gprint (GP_ERR, "%s\n", mysql_error (connection));
    return (FALSE);
  }
  result = mysql_store_result (connection);
  mysql_free_result (result);
    
  sprintf (query, "set @@wait_timeout = 30000");
  if (mysql_query (connection, query)) {
    gprint (GP_ERR, "failed to set wait timout\n");
    gprint (GP_ERR, "%s\n", mysql_error (connection));
    return (FALSE);
  }
  result = mysql_store_result (connection);
  mysql_free_result (result);
    
# if (0)
  int Nrows;
  MYSQL_ROW row;

  sprintf (query, "select @@interactive_timeout");
  if (mysql_query (connection, query)) {
    gprint (GP_ERR, "failed to get timout\n");
    gprint (GP_ERR, "%s\n", mysql_error (connection));
    return (FALSE);
  }
  result = mysql_store_result (connection);
  Nrows = mysql_num_rows(result);
  row = mysql_fetch_row(result);
  fprintf (stderr, "interactive timeout: %s\n", row[0]);
  mysql_free_result (result);

  sprintf (query, "select @@wait_timeout");
  if (mysql_query (connection, query)) {
    gprint (GP_ERR, "failed to get timout\n");
    gprint (GP_ERR, "%s\n", mysql_error (connection));
    return (FALSE);
  }
  result = mysql_store_result (connection);
  Nrows = mysql_num_rows(result);
  row = mysql_fetch_row(result);
  fprintf (stderr, "wait timeout: %s\n", row[0]);
  mysql_free_result (result);
# endif
    
  return (TRUE);
}

// XXX do I need to close the connection before opening a new one? 

void *db_getConnection () {
  return connection;
}

# else 

int dbconnect (int argc, char **argv) {

  gprint (GP_ERR, "mysql library is not available\n");
  return FALSE;
}

void *db_getConnection () {
  return NULL;
}

# endif
