# include "dvopsps.h"
# define DEBUG 0

int insert_FGshape_dvopsps_catalog (Catalog *catalog, char *basename, MYSQL *mysql) {

  off_t i;
  int missingID = 0;
  int noGalphot = 0;

  IOBuffer ave_buffer, gal_buffer;
  ave_buffer.Nalloc = 0;
  gal_buffer.Nalloc = 0;

  Average *average = catalog->average;
  SecFilt *secfilt = catalog->secfilt;
  GalPhot *galphot = catalog->galphot;
  int Nsecfilt = catalog->Nsecfilt;

  off_t found = 0;

  char *cleanname = strcreate (basename);
  for (i = 0; i < strlen(cleanname); i++) {
    if (cleanname[i] == '.') cleanname[i] = '_';
    if (cleanname[i] == '/') cleanname[i] = '_';
  }
  insert_FGshape_mysql_create_tables (cleanname, mysql);

  INITTIME;
  insert_FGshape_mysql_init (&ave_buffer, &gal_buffer, cleanname);
  int Ninsert = 0;

  int hasPS1_flag = ID_SECF_HAS_PS1 | ID_SECF_HAS_PS1_STACK;

  // NOTE for testing, just do a few objects
  if (!mysql) {
    catalog[0].Naverage = 5;
  }
  for (i = 0; i < catalog[0].Naverage; i++) {

    if (average[i].extID == 0) {
      missingID ++;
    }

    int hasPS1 = FALSE;
    for (int j = 0; !hasPS1 && (j < Nsecfilt); j++) {
      if (secfilt[i*Nsecfilt + j].flags & hasPS1_flag) hasPS1 = TRUE;
    }
    if (!hasPS1) continue; // skip non-PS1 data

    off_t m = average[i].galphotOffset;
    off_t Ngalphot = average[i].Ngalphot;

    // skip objects with no lensing object
    if (!Ngalphot) {
      noGalphot ++;
      continue;
    }

    // XXX check return status
    insert_FGshape_mysql_value (&ave_buffer, &gal_buffer, &average[i], &galphot[m], Ngalphot);

    // Average is a big table, but Ngalphot*galphot might be bigger, so check both
    int fullBuffer = FALSE;
    fullBuffer = fullBuffer || (ave_buffer.Nbuffer > MAX_BUFFER);
    fullBuffer = fullBuffer || (gal_buffer.Nbuffer > MAX_BUFFER);

    if (fullBuffer) {
      insert_FGshape_mysql_commit (&ave_buffer, &gal_buffer, mysql);
      if (DEBUG) fprintf (stderr, "inserted %d rows\n", Ninsert);
      Ninsert = 0;
      // empty buffers but do not use FlushBuffer as that resets the allocation
      ave_buffer.Nbuffer = 0;
      bzero (ave_buffer.buffer, ave_buffer.Nalloc);
      gal_buffer.Nbuffer = 0;
      bzero (gal_buffer.buffer, gal_buffer.Nalloc);
      insert_FGshape_mysql_init (&ave_buffer, &gal_buffer, cleanname);
    }
    Ninsert ++;
    found ++;
  }
  if (Ninsert) {
    insert_FGshape_mysql_commit (&ave_buffer, &gal_buffer, mysql);
  }
  if (VERBOSE) fprintf (stderr, "inserted "OFF_T_FMT" average, "OFF_T_FMT" galphot\n", 
			catalog[0].Naverage, catalog[0].Ngalphot);
  FreeIOBuffer (&ave_buffer);
  FreeIOBuffer (&gal_buffer);
  free (cleanname);

  MARKTIME("-- inserted "OFF_T_FMT" objects in %f sec, skipped %d without IDs, %d with no galphot\n", found, dtime, missingID, noGalphot);
  return (TRUE);
}

int insert_FGshape_mysql_create_tables (char *basename, MYSQL *mysql) {

  int status;
  IOBuffer buffer;
  InitIOBuffer (&buffer, 1024);
  // MYSQL_RES *result;

  // *** Average / cpt *** 
  PrintIOBuffer (&buffer, "DROP TABLE IF EXISTS %s_cpt\n", basename);
  status = mysql_query(mysql, buffer.buffer); 
  if (status) {
    fprintf (stderr, "failed to drop table:\n");
    fprintf (stderr, "%s\n", mysql_error(mysql));
  }
  buffer.Nbuffer = 0;
  bzero (buffer.buffer, buffer.Nalloc);

  // Only send the necessary fields (eg, do not sent parallax and pm)
  PrintIOBuffer (&buffer, "CREATE TABLE %s_cpt ("
		 "EXT_ID        BIGINT,   "
		 "OBJ_ID        INT,      "      
		 "CAT_ID        INT       " 
		 ")\n", basename);

  if (DEBUG) fprintf (stderr, "%s\n", buffer.buffer);
  status = mysql_query(mysql, buffer.buffer); 
  if (status) {
    fprintf (stderr, "failed to create table:\n");
    fprintf (stderr, "%s\n", mysql_error(mysql));
    fprintf (stderr, "Nbuffer: %d\n", buffer.Nbuffer);
  }
  buffer.Nbuffer = 0;
  bzero (buffer.buffer, buffer.Nalloc);

  // *** Galphot / cpq ***
  PrintIOBuffer (&buffer, "DROP TABLE IF EXISTS %s_cpq\n", basename);
  status = mysql_query(mysql, buffer.buffer); 
  if (status) {
    fprintf (stderr, "failed to drop table:\n");
    fprintf (stderr, "%s\n", mysql_error(mysql));
  }
  buffer.Nbuffer = 0;
  bzero (buffer.buffer, buffer.Nalloc);

  PrintIOBuffer (&buffer, "CREATE TABLE %s_cpq ("
		 "OBJ_ID             INT, "      
		 "CAT_ID             INT, " 
		 "DET_ID             INT, "
		 "IMAGE_ID           INT, "
		 "PHOTCODE           SMALLINT, "
		 "MODEL_TYPE         SMALLINT, "

		 "MAG                FLOAT, "
		 "MAG_ERR            FLOAT, "
		 "MAJOR_AXIS         FLOAT, "
		 "MINOR_AXIS         FLOAT, "
		 "MAJOR_AXIS_ERR     FLOAT, "
		 "MINOR_AXIS_ERR     FLOAT, "
		 "THETA              FLOAT, "
		 "THETA_ERR          FLOAT, "
		 "SERSIC_INDEX       FLOAT, "
		 "CHISQ              FLOAT,  "

		 "FLAGS              INT, "
		 "NPIX               INT "
		 ")\n", basename);

  if (DEBUG) fprintf (stderr, "%s\n", buffer.buffer);
  status = mysql_query(mysql, buffer.buffer); 
  if (status) {
    fprintf (stderr, "failed to create table:\n");
    fprintf (stderr, "%s\n", mysql_error(mysql));
    fprintf (stderr, "Nbuffer: %d\n", buffer.Nbuffer);
  }

  FreeIOBuffer (&buffer);
  return TRUE;
}

int insert_FGshape_mysql_init (IOBuffer *ave_buffer, IOBuffer *gal_buffer, char *basename) {

  // *** Average / cpt *** 
  if (ave_buffer->Nalloc == 0) {
    InitIOBuffer (ave_buffer, 1024);
  }
  PrintIOBuffer (ave_buffer, "INSERT INTO %s_cpt ("
		 "EXT_ID,        "
		 "OBJ_ID,        "      
		 "CAT_ID         "      
		 ") VALUES \n", basename);

  // *** Galphot / cpq ***
  if (gal_buffer->Nalloc == 0) {
    InitIOBuffer (gal_buffer, 1024);
  }
  PrintIOBuffer (gal_buffer, "INSERT INTO %s_cpq ("
		 "OBJ_ID, "      
		 "CAT_ID, " 
		 "DET_ID, "
		 "IMAGE_ID, "

		 "PHOTCODE, "
		 "MODEL_TYPE, "
		 "MAG, "
		 "MAG_ERR, "

		 "MAJOR_AXIS, "
		 "MINOR_AXIS, "
		 "MAJOR_AXIS_ERR, "
		 "MINOR_AXIS_ERR, "

		 "THETA, "
		 "THETA_ERR, "
		 "SERSIC_INDEX, "
		 "CHISQ, "
		 "FLAGS, "
		 "NPIX"
		 ") VALUES \n", basename);

  return TRUE;
}

# define PRINT_FLOAT(BUFFER,FIELD,FORMAT)		  \
  if (isinf(FIELD) || isnan(FIELD)) PrintIOBuffer (BUFFER, "NULL, ");	\
  else PrintIOBuffer (BUFFER, FORMAT, FIELD); 

int insert_FGshape_mysql_value (IOBuffer *ave_buffer, IOBuffer *gal_buffer, Average *average, GalPhot *galphot, int Ngalphot) {

  int i;

  PrintIOBuffer (ave_buffer, " (");
  PrintIOBuffer (ave_buffer, OFF_T_FMT", ", average->extID);          
  PrintIOBuffer (ave_buffer, "%u,  ", average->objID);          
  PrintIOBuffer (ave_buffer, "%u   ", average->catID);          
  PrintIOBuffer (ave_buffer, "),\n");

  for (i = 0; i < Ngalphot; i++) {
    PrintIOBuffer (gal_buffer, " (");
    PrintIOBuffer (gal_buffer, "%u,  ", galphot->objID);          
    PrintIOBuffer (gal_buffer, "%u,  ", galphot->catID);          
    PrintIOBuffer (gal_buffer, "%u,  ", galphot->detID);          
    PrintIOBuffer (gal_buffer, "%u,  ", galphot->imageID);          

    PrintIOBuffer (gal_buffer, "%hd, ", galphot->photcode);          
    PrintIOBuffer (gal_buffer, "%hd, ", galphot->modelType);          

    float sersic_index = galphot->index;
    if (galphot->modelType == 6) {
      sersic_index = 1.0;
    }
    else if (galphot->modelType == 7) {
      sersic_index = 4.0;
    }
    else {
      sersic_index = 1.0 / (2.0 * sersic_index);
    }

    PRINT_FLOAT(gal_buffer, galphot->mag,              "%.6e, ");
    PRINT_FLOAT(gal_buffer, galphot->magErr,           "%.6e, ");
    PRINT_FLOAT(gal_buffer, galphot->majorAxis,        "%.6e, ");
    PRINT_FLOAT(gal_buffer, galphot->minorAxis,        "%.6e, ");
    PRINT_FLOAT(gal_buffer, galphot->majorAxisErr,     "%.6e, ");
    PRINT_FLOAT(gal_buffer, galphot->minorAxisErr,     "%.6e, ");
    PRINT_FLOAT(gal_buffer, DEG_RAD*galphot->theta,    "%.6e, ");
    PRINT_FLOAT(gal_buffer, DEG_RAD*galphot->thetaErr, "%.6e, ");
    PRINT_FLOAT(gal_buffer, sersic_index,              "%.6e, ");
    PRINT_FLOAT(gal_buffer, galphot->chisq,            "%.6e, ");

    PrintIOBuffer (gal_buffer, "%d, ", galphot->flags);
    PrintIOBuffer (gal_buffer, "%d ", (int) galphot->Npix); // NOTE: PRINT_FLOAT always adds a trailing comma -- need to use a different method on the last entry          
    PrintIOBuffer (gal_buffer, "),\n");
    galphot ++;
  }

  return TRUE;
}
    
int insert_FGshape_mysql_commit (IOBuffer *ave_buffer, IOBuffer *gal_buffer, MYSQL *mysql) {

  MYSQL_RES *result;

  // check that the last two chars are ,\n and replace with ;\n
  if (!strcmp(&ave_buffer->buffer[ave_buffer->Nbuffer-2], ",\n")) {
    ave_buffer->buffer[ave_buffer->Nbuffer-2] = ';';
  } else {
    fprintf (stderr, "invalid sql?\n");
    return FALSE;
  }

  // check that the last two chars are ,\n and replace with ;\n
  if (!strcmp(&gal_buffer->buffer[gal_buffer->Nbuffer-2], ",\n")) {
    gal_buffer->buffer[gal_buffer->Nbuffer-2] = ';';
  } else {
    fprintf (stderr, "invalid sql?\n");
    return FALSE;
  }

  // XXX check return status
  if (mysql) {
    int status;
    if (DEBUG) fprintf (stderr, "%s\n", ave_buffer->buffer);
    status = mysql_query(mysql, ave_buffer->buffer); 
    if (status) {
      fprintf (stderr, "error with insert:\n");
      fprintf (stderr, "%s\n", mysql_error(mysql));
      fprintf (stderr, "Nbuffer: %d\n", ave_buffer->Nbuffer);
    }
    result = mysql_store_result (mysql);
    if (result) mysql_free_result (result);

    if (DEBUG) fprintf (stderr, "%s\n", gal_buffer->buffer);
    status = mysql_query(mysql, gal_buffer->buffer); 
    if (status) {
      fprintf (stderr, "error with insert:\n");
      fprintf (stderr, "%s\n", mysql_error(mysql));
      fprintf (stderr, "Nbuffer: %d\n", gal_buffer->Nbuffer);
    }
    result = mysql_store_result (mysql);
    if (result) mysql_free_result (result);
  }

  return TRUE;
}
