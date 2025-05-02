# include "dvopsps.h"
# define DEBUG 1
# define USE_MYSQL 1

// determine the relevant catalogs, launch parallel clients if desired
int insert_skytable () {

  off_t i;
  SkyTable *sky = NULL;

  IOBuffer buffer;
  buffer.Nalloc = 0;

  // load the current sky table (layout of all SkyRegions) 
  sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, FALSE, -1, VERBOSE);
  
# if (USE_MYSQL)
  // NOTE: mysql connection happens here since each dvopsps_client makes its own connection
  MYSQL  mysqlBase;
  MYSQL *mysqlReal = mysql_dvopsps_connect (&mysqlBase);
  if (!mysqlReal) {
    fprintf (stderr, "failed to connect to mysql\n");
    exit (1);
  }
# else
  MYSQL *mysqlReal = NULL;
# endif

  INITTIME;

  insert_skytable_mysql_init (&buffer);
  int Ninsert = 0;

  for (i = 0; i < sky->Nregions; i++) {
    insert_skytable_mysql_value (&buffer, &sky->regions[i]);

    // if (buffer.Nbuffer > 1024) {
    if (buffer.Nbuffer > MAX_BUFFER) {
	insert_skytable_mysql_commit (&buffer, mysqlReal);
	if (DEBUG) fprintf (stderr, "inserted %d rows\n", Ninsert);
	Ninsert = 0;
	if (0) {
	  FlushIOBuffer (&buffer);
	} else {
	  buffer.Nbuffer = 0;
	  bzero (buffer.buffer, buffer.Nalloc);
	}
	insert_skytable_mysql_init (&buffer);
      }
      Ninsert ++;
  }

  insert_skytable_mysql_commit (&buffer, mysqlReal);
  FreeIOBuffer (&buffer);

  MARKTIME("-- inserted %d rows in %f sec\n", Ninsert, dtime);
  return (TRUE);
}

int insert_skytable_mysql_init (IOBuffer *buffer) {

  if (buffer->Nalloc == 0) {
    InitIOBuffer (buffer, 1024);
  }

  PrintIOBuffer (buffer, "INSERT INTO dvoSkyTable (R_MIN, R_MAX, D_MIN, D_MAX, REGION_ID, NAME) VALUES \n");

  return TRUE;
}

int insert_skytable_mysql_value (IOBuffer *buffer, SkyRegion *region) {
  
  PrintIOBuffer (buffer, "    (%f, %f, %f, %f, %d, '%s'),\n",
		 region->Rmin,    // Rmin
		 region->Rmax,    // Rmax
		 region->Dmin,    // Dmin
		 region->Dmax,    // Dmax
		 region->index,   // index
		 region->name     // name
    ); 
  return TRUE;
}
    
int insert_skytable_mysql_commit (IOBuffer *buffer, MYSQL *mysql) {

  MYSQL_RES *result;

  // check that the last two chars are ,\n and replace with ;\n
  if (!strcmp(&buffer->buffer[buffer->Nbuffer-2], ",\n")) {
    buffer->buffer[buffer->Nbuffer-2] = ';';
  } else {
    fprintf (stderr, "invalid sql?\n");
    return FALSE;
  }

  // XXX check return status
  if (mysql) {
    // if (DEBUG) fprintf (stderr, "%s\n", buffer->buffer);
    int status = mysql_query(mysql, buffer->buffer); 
    if (status) {
      fprintf (stderr, "error with insert:\n");
      fprintf (stderr, "%s\n", mysql_error(mysql));
      fprintf (stderr, "Nbuffer: %d\n", buffer->Nbuffer);
    }
    result = mysql_store_result (mysql);
    mysql_free_result (result);
  } else {
    fprintf (stderr, "%s\n", buffer->buffer);
  }

  return TRUE;
}
