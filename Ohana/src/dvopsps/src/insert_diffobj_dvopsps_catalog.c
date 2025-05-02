# include "dvopsps.h"
# define DEBUG 0

int insert_diffobj_dvopsps_catalog (Catalog *catalog, char *basename, MYSQL *mysql) {

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
  insert_diffobj_mysql_create_tables (cleanname, mysql);

  INITTIME;
  insert_diffobj_mysql_init (&ave_buffer, &sec_buffer, cleanname);
  int Ninsert = 0;

  // NOTE for testing, just do a few objects
  if (!mysql) {
    catalog[0].Naverage = 5;
  }
  for (i = 0; i < catalog[0].Naverage; i++) {

    if (average[i].extID == 0) {
      missingID ++;
    }

    // XXX check return status
    insert_diffobj_mysql_value (&ave_buffer, &sec_buffer, &average[i], &secfilt[i*Nsecfilt], Nsecfilt);

    // fprintf (stderr, "%f : %d %d %d\n", average[i].ChiSqPM, isinf(average[i].ChiSqPM), isnan(average[i].ChiSqPM), isfinite(average[i].ChiSqPM));

    // Average is a bigger table, but Nsecfilt*secfilt might be bigger, so check both
    if ((ave_buffer.Nbuffer > MAX_BUFFER) || (sec_buffer.Nbuffer > MAX_BUFFER)) {
      insert_diffobj_mysql_commit (&ave_buffer, &sec_buffer, mysql);
      if (DEBUG) fprintf (stderr, "inserted %d rows\n", Ninsert);
      Ninsert = 0;
      if (0) {
	FlushIOBuffer (&ave_buffer);
	FlushIOBuffer (&sec_buffer);
      } else {
	ave_buffer.Nbuffer = 0;
	bzero (ave_buffer.buffer, ave_buffer.Nalloc);
	sec_buffer.Nbuffer = 0;
	bzero (sec_buffer.buffer, sec_buffer.Nalloc);
      }
      insert_diffobj_mysql_init (&ave_buffer, &sec_buffer, cleanname);
    }
    Ninsert ++;
    found ++;
  }
  insert_diffobj_mysql_commit (&ave_buffer, &sec_buffer, mysql);
  if (VERBOSE) fprintf (stderr, "inserted "OFF_T_FMT" average, "OFF_T_FMT" secfilt\n", catalog[0].Naverage, Nsecfilt*catalog[0].Naverage);
  FreeIOBuffer (&ave_buffer);
  FreeIOBuffer (&sec_buffer);
  free (cleanname);

  MARKTIME("-- inserted "OFF_T_FMT" diffobj in %f sec, skipped %d without IDs\n", found, dtime, missingID);
  return (TRUE);
}

int insert_diffobj_mysql_create_tables (char *basename, MYSQL *mysql) {

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
		 "RA_MEAN       DOUBLE,   "          
		 "DEC_MEAN      DOUBLE,   "
		 "RA_ERR        FLOAT,    "      
		 "DEC_ERR       FLOAT,    "
		 "IAU_NAME      VARCHAR(32), "
		 "PSO_NAME      VARCHAR(32), "
		 "FLAGS         INT       "       
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

  PrintIOBuffer (&buffer, "DROP TABLE IF EXISTS %s_cps\n", basename);
  status = mysql_query(mysql, buffer.buffer); 
  if (status) {
    fprintf (stderr, "failed to drop table:\n");
    fprintf (stderr, "%s\n", mysql_error(mysql));
  }
  buffer.Nbuffer = 0;
  bzero (buffer.buffer, buffer.Nalloc);

  PrintIOBuffer (&buffer, "CREATE TABLE %s_cps ("
		 "NWARP               SMALLINT,  "           
		 "NUSED_WRP           SMALLINT,  "    
		 "NUSED_KRON_WRP      SMALLINT,  "    
		 "NUSED_AP_WRP        SMALLINT,  "    
		 "PSF_QF_PERF_MAX     FLOAT, "           

		 "FLUX_PSF_WRP        FLOAT,  "             
		 "FLUX_PSF_WRP_ERR    FLOAT,  "         
		 "FLUX_PSF_WRP_STDEV  FLOAT,  "         

		 "FLUX_KRON_WRP       FLOAT,  "        
		 "FLUX_KRON_WRP_ERR   FLOAT,  "    
		 "FLUX_KRON_WRP_STDEV FLOAT,  "    

		 "FLUX_AP_WRP         FLOAT,  "             
		 "FLUX_AP_WRP_ERR     FLOAT,  "             
		 "FLUX_AP_WRP_STDEV   FLOAT,  "             

		 "FLAGS               INT "      
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

int insert_diffobj_mysql_init (IOBuffer *ave_buffer, IOBuffer *sec_buffer, char *basename) {

  if (ave_buffer->Nalloc == 0) {
    InitIOBuffer (ave_buffer, 1024);
  }
  PrintIOBuffer (ave_buffer, "INSERT INTO %s_cpt ("
		 "EXT_ID,        "
		 "OBJ_ID,        "      
		 "CAT_ID,        "      
		 "RA_MEAN,       "          
		 "DEC_MEAN,      "         
		 "RA_ERR,        "      
		 "DEC_ERR,       "
		 "IAU_NAME,      "
		 "PSO_NAME,      "
		 "FLAGS          "       
		 ") VALUES \n", basename);

  if (sec_buffer->Nalloc == 0) {
    InitIOBuffer (sec_buffer, 1024);
  }
  PrintIOBuffer (sec_buffer, "INSERT INTO %s_cps ("
		 "NWARP, "           
		 "NUSED_WRP, "    
		 "NUSED_KRON_WRP, "    
		 "NUSED_AP_WRP, "    
		 "PSF_QF_PERF_MAX, "           

		 "FLUX_PSF_WRP, "             
		 "FLUX_PSF_WRP_ERR, "         
		 "FLUX_PSF_WRP_STDEV, "         

		 "FLUX_KRON_WRP, "        
		 "FLUX_KRON_WRP_ERR, "    
		 "FLUX_KRON_WRP_STDEV, "    

		 "FLUX_AP_WRP, "             
		 "FLUX_AP_WRP_ERR, "             
		 "FLUX_AP_WRP_STDEV, "             

		 "FLAGS"      
		 ") VALUES \n", basename);

  return TRUE;
}

# define PRINT_FLOAT(BUFFER,FIELD,FORMAT)		  \
  if (isinf(FIELD) || isnan(FIELD)) PrintIOBuffer (BUFFER, "NULL, ");	\
  else PrintIOBuffer (BUFFER, FORMAT, FIELD); 

int insert_diffobj_mysql_value (IOBuffer *ave_buffer, IOBuffer *sec_buffer, Average *average, SecFilt *secfilt, int Nsecfilt) {

  int i;

// XXX this bit could/should be autocoded...
  PrintIOBuffer (ave_buffer, " (");
  PrintIOBuffer (ave_buffer, OFF_T_FMT", ", average->extID);          
  PrintIOBuffer (ave_buffer, "%u,  ", average->objID);          
  PrintIOBuffer (ave_buffer, "%u,  ", average->catID);          

  PRINT_FLOAT(ave_buffer, average->R,  	     "%.8f, ");  // 0.036 mas precision 
  PRINT_FLOAT(ave_buffer, average->D,  	     "%.8f, ");  // 0.036 mas precision 
  PRINT_FLOAT(ave_buffer, average->dR, 	     "%.5f, ");  // 0.010 mas precision
  PRINT_FLOAT(ave_buffer, average->dD, 	     "%.5f, ");  // 0.010 mas precision

  // Add names.
  int ra_hr,ra_min,ra_sec,ra_fracsec;
  int dec_deg,dec_min,dec_sec,dec_fracsec;
  double tmp_ra = average->R;
  double tmp_dec = average->D;
  double dec_sign = tmp_dec / fabs(tmp_dec);
  tmp_dec = fabs(tmp_dec);

  tmp_ra /= 15.0;
  ra_hr   = (int) floor(tmp_ra);

  tmp_ra -= ra_hr;
  tmp_ra *= 60.0;
  ra_min  = (int) floor(tmp_ra);
  
  tmp_ra -= ra_min;
  tmp_ra *= 60.0;
  ra_sec  = (int) floor(tmp_ra);

  tmp_ra -= ra_sec;
  tmp_ra *= 1000.0;
  ra_fracsec = (int) floor(tmp_ra);

  dec_deg = (int) floor(tmp_dec);

  tmp_dec -= dec_deg;
  tmp_dec *= 60.0;
  dec_min = (int) floor(tmp_dec);

  tmp_dec -= dec_min;
  tmp_dec *= 60.0;
  dec_sec = (int) floor(tmp_dec);

  tmp_dec -= dec_sec;
  tmp_dec *= 1000.0;
  dec_fracsec = (int) floor(tmp_dec);

  dec_deg *= (int) dec_sign;

  // IAU NAME
  PrintIOBuffer(ave_buffer, "'PSO J%02d%02d%02d.%03d%+03d%02d%02d.%03d',  ",
		ra_hr,ra_min,ra_sec,ra_fracsec,
		dec_deg,dec_min,dec_sec,dec_fracsec);

  // PSO NAME
  tmp_ra = average->R;
  tmp_dec = average->D;
  PrintIOBuffer(ave_buffer, "'PSO J%.5f%+.5f',  ",
		tmp_ra,tmp_dec);

  // End names
  
  PrintIOBuffer (ave_buffer, "%u ",  average->flags);          
  PrintIOBuffer (ave_buffer, "),\n");

  // XXX what rules for keeping or NAN-ing various mags?
  for (i = 0; i < Nsecfilt; i++) {
    float meanPSFFlux    = NAN;
    float meanPSFFluxErr = NAN;
    float meanPSFFluxStd = NAN;
    if (isfinite(secfilt->dFpsfWrp) && isfinite(secfilt->FpsfWrp)) {
      meanPSFFlux    = secfilt-> FpsfWrp;
      meanPSFFluxErr = secfilt->dFpsfWrp;
      meanPSFFluxStd = secfilt->sFpsfWrp;
    }

    float meanKronFlux    = NAN;
    float meanKronFluxErr = NAN;
    float meanKronFluxStd = NAN;
    if (isfinite(secfilt->dFkronWrp) && isfinite(secfilt->FkronWrp)) {
      meanKronFlux    = secfilt-> FkronWrp;
      meanKronFluxErr = secfilt->dFkronWrp;
      meanKronFluxStd = secfilt->sFkronWrp;
    }

    float meanApFlux    = NAN;
    float meanApFluxErr = NAN;
    float meanApFluxStd = NAN;
    if (isfinite(secfilt->dFapWrp) && isfinite(secfilt->FapWrp)) {
      meanApFlux    = secfilt-> FapWrp;
      meanApFluxErr = secfilt->dFapWrp;
      meanApFluxStd = secfilt->sFapWrp;
    }

    PrintIOBuffer (sec_buffer, " (");
    PrintIOBuffer (sec_buffer, "%hd,  ", secfilt->Nwarp);        
    PrintIOBuffer (sec_buffer, "%hd,  ", secfilt->NusedWrp);        
    PrintIOBuffer (sec_buffer, "%hd,  ", secfilt->NusedKronWrp);        
    PrintIOBuffer (sec_buffer, "%hd,  ", secfilt->NusedApWrp);        
    PRINT_FLOAT(sec_buffer, secfilt->psfQfPerfMax, "%.6f,");       

    // use %e?
    PRINT_FLOAT(sec_buffer, meanPSFFlux,      "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanPSFFluxErr,   "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanPSFFluxStd,   "%.6e, "); // uflux precision

    PRINT_FLOAT(sec_buffer, meanKronFlux,     "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanKronFluxErr,  "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanKronFluxStd,  "%.6e, "); // uflux precision

    PRINT_FLOAT(sec_buffer, meanApFlux,     "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanApFluxErr,  "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanApFluxStd,  "%.6e, "); // uflux precision

    PrintIOBuffer (sec_buffer, "%u ", secfilt->flags);       
    PrintIOBuffer (sec_buffer, "),\n");
    secfilt ++;
  }

  return TRUE;
}
    
int insert_diffobj_mysql_commit (IOBuffer *ave_buffer, IOBuffer *sec_buffer, MYSQL *mysql) {

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
