# include "dvopsps.h"
# define DEBUG 0

int Format_PSX_Name (char *buffer, int Nbuffer, double tmp_ra, double tmp_dec);

int insert_objects_dvopsps_catalog (Catalog *catalog, char *basename, MYSQL *mysql) {

  off_t i;
  int missingID = 0;

  IOBuffer ave_buffer, sec_buffer;
  ave_buffer.Nalloc = 0;
  sec_buffer.Nalloc = 0;

  Average *average = catalog->average;
  SecFilt *secfilt = catalog->secfilt;
  int Nsecfilt = catalog->Nsecfilt;

  off_t found = 0;

  char *cleanname = strcreate (basename);
  for (i = 0; i < strlen(cleanname); i++) {
    if (cleanname[i] == '.') cleanname[i] = '_';
    if (cleanname[i] == '/') cleanname[i] = '_';
  }
  insert_objects_mysql_create_tables (cleanname, mysql);

  INITTIME;
  insert_objects_mysql_init (&ave_buffer, &sec_buffer, cleanname);
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

    // XXX check return status
    insert_objects_mysql_value (&ave_buffer, &sec_buffer, &average[i], &secfilt[i*Nsecfilt], Nsecfilt);

    // fprintf (stderr, "%f : %d %d %d\n", average[i].ChiSqPM, isinf(average[i].ChiSqPM), isnan(average[i].ChiSqPM), isfinite(average[i].ChiSqPM));

    // Average is a big table, but Nsecfilt*secfilt might be bigger, so check both
    if ((ave_buffer.Nbuffer > MAX_BUFFER) || (sec_buffer.Nbuffer > MAX_BUFFER)) {
      insert_objects_mysql_commit (&ave_buffer, &sec_buffer, mysql);
      if (DEBUG) fprintf (stderr, "inserted %d rows\n", Ninsert);
      Ninsert = 0;
      ave_buffer.Nbuffer = 0;
      bzero (ave_buffer.buffer, ave_buffer.Nalloc);
      sec_buffer.Nbuffer = 0;
      bzero (sec_buffer.buffer, sec_buffer.Nalloc);
      insert_objects_mysql_init (&ave_buffer, &sec_buffer, cleanname);
    }
    Ninsert ++;
    found ++;
  }
  insert_objects_mysql_commit (&ave_buffer, &sec_buffer, mysql);
  if (VERBOSE) fprintf (stderr, "inserted "OFF_T_FMT" average, "OFF_T_FMT" secfilt\n", catalog[0].Naverage, Nsecfilt*catalog[0].Naverage);
  FreeIOBuffer (&ave_buffer);
  FreeIOBuffer (&sec_buffer);
  free (cleanname);

  insert_manifest_mysql ("objects", mysql, catalog->catID, basename);

  MARKTIME("-- inserted "OFF_T_FMT" objects in %f sec, skipped %d without IDs\n", found, dtime, missingID);
  return (TRUE);
}

int insert_objects_mysql_create_tables (char *basename, MYSQL *mysql) {

  int status;
  IOBuffer buffer;
  InitIOBuffer (&buffer, 1024);
  // MYSQL_RES *result;

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
		 "CAT_ID        INT,      " 
		 "TESS_ID       TINYINT,  " 
		 "PROJECTION_ID SMALLINT, " 
		 "SKYCELL_ID    TINYINT,  " 
		 "RA_STK        DOUBLE,   "          
		 "DEC_STK       DOUBLE,   "         
		 "RA_STK_ERR    FLOAT,    "      
		 "DEC_STK_ERR   FLOAT,    "     
		 "RA_MEAN       DOUBLE,   "          
		 "DEC_MEAN      DOUBLE,   "         
		 "EPOCH_MEAN    DOUBLE,   "         
		 "RA_ERR        FLOAT,    "      
		 "DEC_ERR       FLOAT,    "
		 "PSO_NAME      VARCHAR(32), "
		 "PSX_NAME      VARCHAR(32), "
		 "CHISQ_POS     FLOAT,    "   
		 "CHISQ_PM      FLOAT,    "    
		 "CHISQ_PAR     FLOAT,    "   
		 "FLAGS         INT       "       
		 ")\n", basename);

// skipping these cpt fields:
// 		 "U_RA        FLOAT,  "        
// 		 "U_DEC       FLOAT,  "       
// 		 "V_RA_ERR    FLOAT,  "    
// 		 "V_DEC_ERR   FLOAT,  "   
// 		 "PAR         FLOAT,  "         
// 		 "PAR_ERR     FLOAT,  "     
//		 "OFF_MEASURE INT,  " 
//		 "OFF_MISSING INT,  " 
//		 "OFF_EXTEND  INT,  "  
//		 "PSF_QF      FLOAT, "      
//		 "PSF_QF_PERF FLOAT,  " 
//		 "STARGAL_SEP FLOAT,  " 
//		 "NUMBER_POS  SMALLINT,  "  
//		 "NMEASURE    SMALLINT,  "    
//		 "NMISSING    SMALLINT,  "    
//		 "NEXTEND     SMALLINT,  "     
//		 "PHOTFLAGS_U INT,  " 
//		 "PHOTFLAGS_L INT,  " 



  if (DEBUG) fprintf (stderr, "%s\n", buffer.buffer);
  status = mysql_query(mysql, buffer.buffer); 
  if (status) {
    fprintf (stderr, "failed to create table:\n");
    fprintf (stderr, "%s\n", mysql_error(mysql));
    fprintf (stderr, "Nbuffer: %d\n", buffer.Nbuffer);
  }
  // result = mysql_store_result (mysql);
  // mysql_free_result (result);

  buffer.Nbuffer = 0;
  bzero (buffer.buffer, buffer.Nalloc);

  PrintIOBuffer (&buffer, "DROP TABLE IF EXISTS %s_cps\n", basename);
  status = mysql_query(mysql, buffer.buffer); 
  if (status) {
    fprintf (stderr, "failed to drop table:\n");
    fprintf (stderr, "%s\n", mysql_error(mysql));
  }
  buffer.Nbuffer = 0;
  bzero (buffer.buffer, buffer.Nalloc);

  PrintIOBuffer (&buffer, "CREATE TABLE %s_cps ("
		 "MAG            FLOAT,  "             
		 "MAG_ERR        FLOAT,  "         
		 "MAG_STDEV      FLOAT,  "         
		 "MAG_MIN        FLOAT,  "         
		 "MAG_MAX        FLOAT,  "         
		 "NUSED          SMALLINT,  "           
		 "MAG_KRON       FLOAT,  "        
		 "MAG_KRON_ERR   FLOAT,  "    
		 "MAG_KRON_STDEV FLOAT,  "    
		 "NUSED_KRON     SMALLINT,  "    
		 "MAG_AP         FLOAT,  "             
		 "MAG_AP_ERR     FLOAT,  "             
		 "MAG_AP_STDEV   FLOAT,  "             
		 "NUSED_AP       SMALLINT,  "    
		 "NCODE          SMALLINT,  "           
		 "NSTACK_DET     SMALLINT,  "           
		 "PSF_QF_PERF_MAX FLOAT, "           
		 "FLAGS          INT "           
		 ")\n", basename);

  if (DEBUG) fprintf (stderr, "%s\n", buffer.buffer);
  status = mysql_query(mysql, buffer.buffer); 
  if (status) {
    fprintf (stderr, "failed to create table:\n");
    fprintf (stderr, "%s\n", mysql_error(mysql));
    fprintf (stderr, "Nbuffer: %d\n", buffer.Nbuffer);
  }
  // result = mysql_store_result (mysql);
  // mysql_free_result (result);

  FreeIOBuffer (&buffer);
  return TRUE;
}

int insert_objects_mysql_init (IOBuffer *ave_buffer, IOBuffer *sec_buffer, char *basename) {

  if (ave_buffer->Nalloc == 0) {
    InitIOBuffer (ave_buffer, 1024);
  }

  PrintIOBuffer (ave_buffer, "INSERT INTO %s_cpt ("
		 "EXT_ID,        "
		 "OBJ_ID,        "      
		 "CAT_ID,        "      
		 "TESS_ID,       "      
		 "PROJECTION_ID, "      
		 "SKYCELL_ID,    "      
		 "RA_STK,        "          
		 "DEC_STK,       "         
		 "RA_STK_ERR,    "      
		 "DEC_STK_ERR,   "     
		 "RA_MEAN,       "          
		 "DEC_MEAN,      "         
		 "EPOCH_MEAN,    "         
		 "RA_ERR,        "      
		 "DEC_ERR,       "
		 "PSO_NAME,      "
		 "PSX_NAME,      "
		 "CHISQ_POS,     "   
		 "CHISQ_PM,      "    
		 "CHISQ_PAR,     "   
		 "FLAGS          "       
		 ") VALUES \n", basename);

  if (sec_buffer->Nalloc == 0) {
    InitIOBuffer (sec_buffer, 1024);
  }

  PrintIOBuffer (sec_buffer, "INSERT INTO %s_cps ("
		 "MAG, "             
		 "MAG_ERR, "         
		 "MAG_STDEV, "       
		 "MAG_MIN, "          
		 "MAG_MAX, "          
		 "NUSED, "           

		 "MAG_KRON, "        
		 "MAG_KRON_ERR, "    
		 "MAG_KRON_STDEV, "    
		 "NUSED_KRON, "           

		 "MAG_AP, "             
		 "MAG_AP_ERR, "             
		 "MAG_AP_STDEV, "             
		 "NUSED_AP, "           

		 "NCODE, "           
		 "NSTACK_DET,  "           
		 "PSF_QF_PERF_MAX, "           
		 "FLAGS "           
		 ") VALUES \n", basename);
  return TRUE;
}

# define PRINT_FLOAT(BUFFER,FIELD,FORMAT)		  \
  if (isinf(FIELD) || isnan(FIELD)) PrintIOBuffer (BUFFER, "NULL, ");	\
  else PrintIOBuffer (BUFFER, FORMAT, FIELD); 

int insert_objects_mysql_value (IOBuffer *ave_buffer, IOBuffer *sec_buffer, Average *average, SecFilt *secfilt, int Nsecfilt) {

  int i;

  double Tmean = average->Tmean == 0 ? NAN : ohana_sec_to_mjd (average->Tmean);

  char coord_buffer[128];

  // XXX this bit could/should be autocoded...
  PrintIOBuffer (ave_buffer, " (");
  PrintIOBuffer (ave_buffer, OFF_T_FMT", ", average->extID);          
  PrintIOBuffer (ave_buffer, "%u,  ", average->objID);          
  PrintIOBuffer (ave_buffer, "%u,  ", average->catID);          

  PrintIOBuffer (ave_buffer, "%hhd, ", average->tessID);          
  PrintIOBuffer (ave_buffer, "%hd,  ", average->projectionID);          
  PrintIOBuffer (ave_buffer, "%hhd, ", average->skycellID);          

  PRINT_FLOAT(ave_buffer, average->Rstk,     "%.8f, ");  // 0.036 mas precision 
  PRINT_FLOAT(ave_buffer, average->Dstk,     "%.8f, ");  // 0.036 mas precision 
  PRINT_FLOAT(ave_buffer, average->dRstk,    "%.5f, ");  // 0.010 mas precision
  PRINT_FLOAT(ave_buffer, average->dDstk,    "%.5f, ");  // 0.010 mas precision

  PRINT_FLOAT(ave_buffer, average->R,  	     "%.8f, ");  // 0.036 mas precision 
  PRINT_FLOAT(ave_buffer, average->D,  	     "%.8f, ");  // 0.036 mas precision 
  PRINT_FLOAT(ave_buffer, Tmean,             "%.8f, ");  // 0.864 msec precision MJD
  PRINT_FLOAT(ave_buffer, average->dR, 	     "%.5f, ");  // 0.010 mas precision
  PRINT_FLOAT(ave_buffer, average->dD, 	     "%.5f, ");  // 0.010 mas precision

  int useStack = (!isfinite(average->R) || !isfinite(average->D)) && (secfilt->NstackDet != 0) && isfinite(average->Rstk) && isfinite(average->Dstk);

  double tmp_ra  = useStack ? average->Rstk : average->R;
  double tmp_dec = useStack ? average->Dstk : average->D;

  // PSO NAME (IAU approved)
  PrintIOBuffer(ave_buffer, "'PSO J%08.4f%+08.4f',  ", tmp_ra, tmp_dec);

  // PSX NAME
  Format_PSX_Name (coord_buffer, 128, tmp_ra, tmp_dec);
  PrintIOBuffer(ave_buffer, "'%s',  ", coord_buffer);

  // End names
  
  PRINT_FLOAT(ave_buffer, average->ChiSqAve, "%.4f, ");
  PRINT_FLOAT(ave_buffer, average->ChiSqPM,  "%.4f, ");
  PRINT_FLOAT(ave_buffer, average->ChiSqPar, "%.4f, ");

  PrintIOBuffer (ave_buffer, "%u ",  average->flags);          

  PrintIOBuffer (ave_buffer, "),\n");

  // XXX what rules for keeping or NAN-ing various mags?
  for (i = 0; i < Nsecfilt; i++) {
    float meanPSFMag    = NAN;
    float meanPSFMagErr = NAN;
    float meanPSFMagStd = NAN;
    float meanPSFMagMin = NAN;
    float meanPSFMagMax = NAN;
    if (isfinite(secfilt->dMpsfChp) && isfinite(secfilt->MpsfChp)) {
      meanPSFMag    = secfilt->MpsfChp;
      meanPSFMagErr = secfilt->dMpsfChp;
      meanPSFMagStd = secfilt->sMpsfChp;
      meanPSFMagMin = secfilt->Mmin;
      meanPSFMagMax = secfilt->Mmax;
    }

    float meanKronMag    = NAN;
    float meanKronMagErr = NAN;
    float meanKronMagStd = NAN;
    if (isfinite(secfilt->dMkronChp) && isfinite(secfilt->MkronChp) && (secfilt->dMkronChp < 0.333)) {
      meanKronMag    = secfilt->MkronChp;
      meanKronMagErr = secfilt->dMkronChp;
      meanKronMagStd = secfilt->sMkronChp;
    }

    float meanApMag    = NAN;
    float meanApMagErr = NAN;
    float meanApMagStd = NAN;
    if (isfinite(secfilt->dMapChp) && isfinite(secfilt->MapChp)) {
      meanApMag    = secfilt->MapChp;
      meanApMagErr = secfilt->dMapChp;
      meanApMagStd = secfilt->sMapChp;
    }

    PrintIOBuffer (sec_buffer, " (");
    PRINT_FLOAT(sec_buffer, meanPSFMag,      "%.6f, "); // umag precision
    PRINT_FLOAT(sec_buffer, meanPSFMagErr,   "%.6f, "); // umag precision
    PRINT_FLOAT(sec_buffer, meanPSFMagStd,   "%.6f, "); // umag precision
    PRINT_FLOAT(sec_buffer, meanPSFMagMin,   "%.6f, "); // umag precision
    PRINT_FLOAT(sec_buffer, meanPSFMagMax,   "%.6f, "); // umag precision
    PrintIOBuffer (sec_buffer, "%hd,  ", secfilt->Nused);        

    PRINT_FLOAT(sec_buffer, meanKronMag,     "%.6f, "); // umag precision
    PRINT_FLOAT(sec_buffer, meanKronMagErr,  "%.6f, "); // umag precision
    PRINT_FLOAT(sec_buffer, meanKronMagStd,  "%.6f, "); // umag precision
    PrintIOBuffer (sec_buffer, "%hd,  ", secfilt->NusedKron);        

    PRINT_FLOAT(sec_buffer, meanApMag,     "%.6f, "); // umag precision
    PRINT_FLOAT(sec_buffer, meanApMagErr,  "%.6f, "); // umag precision
    PRINT_FLOAT(sec_buffer, meanApMagStd,  "%.6f, "); // umag precision
    PrintIOBuffer (sec_buffer, "%hd,  ", secfilt->NusedAp);        
    PrintIOBuffer (sec_buffer, "%hd,  ", secfilt->Ncode);        
    PrintIOBuffer (sec_buffer, "%hd,  ", secfilt->NstackDet);        

    PRINT_FLOAT(sec_buffer, secfilt->psfQfPerfMax, "%.6f,");       
    PrintIOBuffer (sec_buffer, "%u ", secfilt->flags);       
    PrintIOBuffer (sec_buffer, "),\n");
    secfilt ++;
  }
  return TRUE;
}
    
int insert_objects_mysql_commit (IOBuffer *ave_buffer, IOBuffer *sec_buffer, MYSQL *mysql) {

  MYSQL_RES *result;

  // check that the last two chars are ,\n and replace with ;\n
  if (!strcmp(&ave_buffer->buffer[ave_buffer->Nbuffer-2], ",\n")) {
    ave_buffer->buffer[ave_buffer->Nbuffer-2] = ';';
  } else {
    fprintf (stderr, "invalid sql?\n");
    return FALSE;
  }

  // check that the last two chars are ,\n and replace with ;\n
  if (!strcmp(&sec_buffer->buffer[sec_buffer->Nbuffer-2], ",\n")) {
    sec_buffer->buffer[sec_buffer->Nbuffer-2] = ';';
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

    if (DEBUG) fprintf (stderr, "%s\n", sec_buffer->buffer);
    status = mysql_query(mysql, sec_buffer->buffer); 
    if (status) {
      fprintf (stderr, "error with insert:\n");
      fprintf (stderr, "%s\n", mysql_error(mysql));
      fprintf (stderr, "Nbuffer: %d\n", sec_buffer->Nbuffer);
    }
    result = mysql_store_result (mysql);
    if (result) mysql_free_result (result);
  } else {
    fprintf (stderr, "%s\n", sec_buffer->buffer);
  }

  return TRUE;
}

int create_manifest_mysql (char *basename, MYSQL *mysql) {

  int status;
  IOBuffer buffer;
  InitIOBuffer (&buffer, 1024);

  // drop an old manifest table
  PrintIOBuffer (&buffer, "DROP TABLE IF EXISTS dvopsps_insert_%s\n", basename);
  status = mysql_query(mysql, buffer.buffer);
  if (status) {
    fprintf (stderr, "failed to drop dvopsps_insert_%s table:\n", basename);
    fprintf (stderr, "%s\n", mysql_error(mysql));
  }
  buffer.Nbuffer = 0;
  bzero (buffer.buffer, buffer.Nalloc);
  
  // create the manifest table
  PrintIOBuffer (&buffer, "CREATE TABLE dvopsps_insert_%s ( REGION_ID INT, REGION_NAME CHAR(18) )\n", basename);
  status = mysql_query(mysql, buffer.buffer);
  if (status) {
    fprintf (stderr, "failed to create dvopsps_insert_%s table:\n", basename);
    fprintf (stderr, "%s\n", mysql_error(mysql));
  }
  buffer.Nbuffer = 0;
  bzero (buffer.buffer, buffer.Nalloc);

  FreeIOBuffer (&buffer);
  return TRUE;
}

int insert_manifest_mysql (char *basename, MYSQL *mysql, int regionID, char *regionName) {

  int status;
  IOBuffer buffer;
  InitIOBuffer (&buffer, 1024);

  // drop an old manifest table
  PrintIOBuffer (&buffer, "INSERT INTO dvopsps_insert_%s ( REGION_ID, REGION_NAME ) VALUES (%d, '%s')\n", basename, regionID, regionName);
  status = mysql_query(mysql, buffer.buffer);
  if (status) {
    fprintf (stderr, "failed to insert into dvopsps_insert_%s table:\n", basename);
    fprintf (stderr, "%s\n", mysql_error(mysql));
  }
  buffer.Nbuffer = 0;
  bzero (buffer.buffer, buffer.Nalloc);
  
  FreeIOBuffer (&buffer);
  return TRUE;
}

// XXX if the coords have NAN values (not sure why that would happen), return FALSE?
int Format_PSX_Name (char *buffer, int Nbuffer, double tmp_ra, double tmp_dec) {

  // Add names.
  int ra_hr, ra_min, dec_deg, dec_min;
  float ra_sec, dec_sec;

  // convert to hours:
  tmp_ra /= 15.0;
  ra_hr   = (int) floor(tmp_ra);

  tmp_ra -= ra_hr;
  tmp_ra *= 60.0;
  ra_min  = (int) floor(tmp_ra);
  
  tmp_ra -= ra_min;
  tmp_ra *= 60.0;
  ra_sec = trunc(100.0*tmp_ra) / 100.0; // ensure truncation for XX.XX

  char dec_sign = (tmp_dec >= 0.0) ? '+' : '-';
  tmp_dec = fabs(tmp_dec);

  dec_deg = (int) floor(tmp_dec);

  tmp_dec -= dec_deg;
  tmp_dec *= 60.0;
  dec_min = (int) floor(tmp_dec);

  tmp_dec -= dec_min;
  tmp_dec *= 60.0;
  dec_sec = trunc(10.0*tmp_dec) / 10.0; // ensure truncation for XX.X

  // PSX NAME
  snprintf (buffer, Nbuffer, "PSX J%02d%02d%05.2f%c%02d%02d%04.1f", ra_hr, ra_min, ra_sec, dec_sign, dec_deg, dec_min, dec_sec);

  return TRUE;
}
