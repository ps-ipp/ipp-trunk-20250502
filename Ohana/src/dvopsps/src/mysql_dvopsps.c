# include "dvopsps.h"

int dump_result (MYSQL *connection) {

  MYSQL_RES *result = mysql_store_result (connection);
  if (!result) return FALSE;
  int Nrows = mysql_num_rows(result);
  int Ncols = mysql_num_fields(result);

  int i, j;
  for (j = 0; j < Nrows; j++) {
    MYSQL_ROW row = mysql_fetch_row(result);
    for (i = 0; i < Ncols; i++) {
      fprintf (stderr, "%s ", row[i]);
    }
    fprintf (stderr, "\n");
  }
  mysql_free_result (result);
  return (TRUE);
}

// DATABASE_* are supplied on the command line and are global variables in dvopsps.h
MYSQL *mysql_dvopsps_connect (MYSQL *mysqlBase) {

  char query[256];

  mysql_init (mysqlBase);
  MYSQL *connection = mysql_real_connect (mysqlBase, DATABASE_HOST, DATABASE_USER, DATABASE_PASS, DATABASE_NAME, 0, 0, 0);

  if (connection == NULL) {
    fprintf (stderr, "failed to connect to database\n");
    fprintf (stderr, "%s\n", mysql_error (mysqlBase));
    return NULL;
  }
    
  sprintf (query, "set @@interactive_timeout = 30000;");
  if (mysql_query (connection, query)) {
    fprintf (stderr, "failed to set interactive timout\n");
    fprintf (stderr, "%s\n", mysql_error (connection));
    return NULL;
  }
  dump_result (connection);
    
  sprintf (query, "set @@wait_timeout = 30000;");
  if (mysql_query (connection, query)) {
    fprintf (stderr, "failed to set wait timout\n");
    fprintf (stderr, "%s\n", mysql_error (connection));
    return NULL;
  }
  dump_result (connection);
    
  // sprintf (query, "set max_allowed_packet = %d;", 2*MAX_BUFFER);
  // if (mysql_query (connection, query)) {
  //   fprintf (stderr, "failed to set max_allowed_packet\n");
  //   fprintf (stderr, "%s\n", mysql_error (connection));
  //   return NULL;
  // }
  // dump_result (connection);
    
  // sprintf (query, "show variables like 'max_allowed_packet';");
  // if (mysql_query (connection, query)) {
  //   fprintf (stderr, "failed to set max_allowed_packet\n");
  //   fprintf (stderr, "%s\n", mysql_error (connection));
  //   return NULL;
  // }
  // dump_result (connection);
    
  if (0) {
    sprintf (query, "set autocommit=0;");
    if (mysql_query (connection, query)) {
      fprintf (stderr, "failed to turn off autocommit\n");
      fprintf (stderr, "%s\n", mysql_error (connection));
      return NULL;
    }
    dump_result (connection);
  }
    
  return connection;
}

