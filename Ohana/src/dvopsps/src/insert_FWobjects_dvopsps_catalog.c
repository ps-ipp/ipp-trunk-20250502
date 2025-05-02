# include "dvopsps.h"
# define DEBUG 0

int insert_FWobjects_dvopsps_catalog (Catalog *catalog, char *basename, MYSQL *mysql) {

  off_t i;
  int missingID = 0;
  int noLensobj = 0;

  IOBuffer ave_buffer, sec_buffer, cpy_buffer;
  ave_buffer.Nalloc = 0;
  sec_buffer.Nalloc = 0;
  cpy_buffer.Nalloc = 0;

  Average *average = catalog->average;
  SecFilt *secfilt = catalog->secfilt;
  Lensobj *lensobj = catalog->lensobj;
  int Nsecfilt = catalog->Nsecfilt;

  off_t found = 0;

  char *cleanname = strcreate (basename);
  for (i = 0; i < strlen(cleanname); i++) {
    if (cleanname[i] == '.') cleanname[i] = '_';
    if (cleanname[i] == '/') cleanname[i] = '_';
  }
  insert_FWobjects_mysql_create_tables (cleanname, mysql);

  INITTIME;
  insert_FWobjects_mysql_init (&ave_buffer, &sec_buffer, &cpy_buffer, cleanname);
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

    if (!TEST_MODE) {
      int hasPS1 = FALSE;
      for (int j = 0; !hasPS1 && (j < Nsecfilt); j++) {
        if (secfilt[i*Nsecfilt + j].flags & hasPS1_flag) hasPS1 = TRUE;
      }
      if (!hasPS1) continue; // skip non-PS1 data
    }

    off_t m = average[i].lensobjOffset;
    off_t Nlensobj = average[i].Nlensobj;

    // skip objects with no lensing object
    if (!Nlensobj) {
      noLensobj ++;
      continue;
    }

    // XXX check return status
    insert_FWobjects_mysql_value (&ave_buffer, &sec_buffer, &cpy_buffer, 
				  &average[i], &secfilt[i*Nsecfilt], Nsecfilt, &lensobj[m], Nlensobj);

    // fprintf (stderr, "%f : %d %d %d\n", average[i].ChiSqPM, isinf(average[i].ChiSqPM), isnan(average[i].ChiSqPM), isfinite(average[i].ChiSqPM));

    // Average is a bigger table, but Nsecfilt*secfilt might be bigger, so check both
    int fullBuffer = FALSE;
    fullBuffer = fullBuffer || (ave_buffer.Nbuffer > MAX_BUFFER);
    fullBuffer = fullBuffer || (sec_buffer.Nbuffer > MAX_BUFFER);
    fullBuffer = fullBuffer || (cpy_buffer.Nbuffer > MAX_BUFFER);

    if (fullBuffer) {
      insert_FWobjects_mysql_commit (&ave_buffer, &sec_buffer, &cpy_buffer, mysql);
      if (DEBUG) fprintf (stderr, "inserted %d rows\n", Ninsert);
      Ninsert = 0;
      if (0) {
	FlushIOBuffer (&ave_buffer);
	FlushIOBuffer (&sec_buffer);
	FlushIOBuffer (&cpy_buffer);
      } else {
	ave_buffer.Nbuffer = 0;
	bzero (ave_buffer.buffer, ave_buffer.Nalloc);
	sec_buffer.Nbuffer = 0;
	bzero (sec_buffer.buffer, sec_buffer.Nalloc);
	cpy_buffer.Nbuffer = 0;
	bzero (cpy_buffer.buffer, cpy_buffer.Nalloc);
      }
      insert_FWobjects_mysql_init (&ave_buffer, &sec_buffer, &cpy_buffer, cleanname);
    }
    Ninsert ++;
    found ++;
  }
  if (Ninsert) {
    insert_FWobjects_mysql_commit (&ave_buffer, &sec_buffer, &cpy_buffer, mysql);
  }
  if (VERBOSE) fprintf (stderr, "inserted "OFF_T_FMT" average, "OFF_T_FMT" secfilt, "OFF_T_FMT" lensobj\n", 
			catalog[0].Naverage, Nsecfilt*catalog[0].Naverage, catalog[0].Nlensobj);
  FreeIOBuffer (&ave_buffer);
  FreeIOBuffer (&sec_buffer);
  FreeIOBuffer (&cpy_buffer);
  free (cleanname);

  MARKTIME("-- inserted "OFF_T_FMT" objects in %f sec, skipped %d without IDs, %d with no lensobj\n", found, dtime, missingID, noLensobj);
  return (TRUE);
}

int insert_FWobjects_mysql_create_tables (char *basename, MYSQL *mysql) {

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

		 "FLUX_PSF_WRP        FLOAT,  "             
		 "FLUX_PSF_WRP_ERR    FLOAT,  "         
		 "FLUX_PSF_WRP_STDEV  FLOAT,  "         
		 "MAG_PSF_WRP         FLOAT,  "
		 "MAG_PSF_WRP_ERR     FLOAT,  "
		 
		 "FLUX_KRON_WRP       FLOAT,  "        
		 "FLUX_KRON_WRP_ERR   FLOAT,  "    
		 "FLUX_KRON_WRP_STDEV FLOAT,  "    
		 "MAG_KRON_WRP        FLOAT,  "
		 "MAG_KRON_WRP_ERR    FLOAT,  "
		 
		 "FLUX_AP_WRP         FLOAT,  "             
		 "FLUX_AP_WRP_ERR     FLOAT,  "             
		 "FLUX_AP_WRP_STDEV   FLOAT,  "
		 "MAG_AP_WRP          FLOAT,  "
		 "MAG_AP_WRP_ERR      FLOAT,  "

		 "FLAGS               INT "      
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

  PrintIOBuffer (&buffer, "DROP TABLE IF EXISTS %s_cpy\n", basename);
  status = mysql_query(mysql, buffer.buffer); 
  if (status) {
    fprintf (stderr, "failed to drop table:\n");
    fprintf (stderr, "%s\n", mysql_error(mysql));
  }
  buffer.Nbuffer = 0;
  bzero (buffer.buffer, buffer.Nalloc);

  PrintIOBuffer (&buffer, "CREATE TABLE %s_cpy ("
		 "OBJ_ID             INT, "      
		 "CAT_ID             INT, " 
		 "PHOTCODE           SMALLINT, "

		 "FLUX_AP_R5         FLOAT, "
		 "FLUX_ERR_AP_R5     FLOAT, "
		 "FLUX_STD_AP_R5     FLOAT, "
		 "FLUX_FIL_AP_R5     FLOAT, "
		 "MAG_AP_R5          FLOAT, "
		 "MAG_ERR_AP_R5      FLOAT, "

		 "FLUX_AP_R6         FLOAT, "
		 "FLUX_ERR_AP_R6     FLOAT, "
		 "FLUX_STD_AP_R6     FLOAT, "
		 "FLUX_FIL_AP_R6     FLOAT, "
		 "MAG_AP_R6          FLOAT, "
		 "MAG_ERR_AP_R6      FLOAT, "

		 "FLUX_AP_R7         FLOAT, "
		 "FLUX_ERR_AP_R7     FLOAT, "
		 "FLUX_STD_AP_R7     FLOAT, "
		 "FLUX_FIL_AP_R7     FLOAT, "
		 "MAG_AP_R7          FLOAT, "
		 "MAG_ERR_AP_R7      FLOAT, "

		 "X11_SM_OBJ         FLOAT, "
		 "X12_SM_OBJ         FLOAT, "
		 "X22_SM_OBJ         FLOAT, "
		 "E1_SM_OBJ          FLOAT, "
		 "E2_SM_OBJ          FLOAT, "
		 "X11_SH_OBJ         FLOAT, "
		 "X12_SH_OBJ         FLOAT, "
		 "X22_SH_OBJ         FLOAT, "
		 "E1_SH_OBJ          FLOAT, "
		 "E2_SH_OBJ          FLOAT, "

		 "X11_SM_PSF         FLOAT, "
		 "X12_SM_PSF         FLOAT, "
		 "X22_SM_PSF         FLOAT, "
		 "E1_SM_PSF          FLOAT, "
		 "E2_SM_PSF          FLOAT, "
		 "X11_SH_PSF         FLOAT, "
		 "X12_SH_PSF         FLOAT, "
		 "X22_SH_PSF         FLOAT, "
		 "E1_SH_PSF          FLOAT, "
		 "E2_SH_PSF          FLOAT, "

		 "GAMMA              FLOAT, "
		 "E1                 FLOAT, "
		 "E2                 FLOAT, "

		 "NMEAS              INT "
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

int insert_FWobjects_mysql_init (IOBuffer *ave_buffer, IOBuffer *sec_buffer, IOBuffer *cpy_buffer, char *basename) {

  if (ave_buffer->Nalloc == 0) {
    InitIOBuffer (ave_buffer, 1024);
  }
  PrintIOBuffer (ave_buffer, "INSERT INTO %s_cpt ("
		 "EXT_ID,        "
		 "OBJ_ID,        "      
		 "CAT_ID         "      
		 ") VALUES \n", basename);

  if (sec_buffer->Nalloc == 0) {
    InitIOBuffer (sec_buffer, 1024);
  }
  PrintIOBuffer (sec_buffer, "INSERT INTO %s_cps ("
		 "NWARP, "           
		 "NUSED_WRP, "    
		 "NUSED_KRON_WRP, "    
		 "NUSED_AP_WRP, "    

		 "FLUX_PSF_WRP, "             
		 "FLUX_PSF_WRP_ERR, "         
		 "FLUX_PSF_WRP_STDEV, "
		 "MAG_PSF_WRP,  "
		 "MAG_PSF_WRP_ERR, "

		 "FLUX_KRON_WRP, "        
		 "FLUX_KRON_WRP_ERR, "    
		 "FLUX_KRON_WRP_STDEV, "
		 "MAG_KRON_WRP, "
		 "MAG_KRON_WRP_ERR, "

		 "FLUX_AP_WRP, "             
		 "FLUX_AP_WRP_ERR, "             
		 "FLUX_AP_WRP_STDEV, "
		 "MAG_AP_WRP, "
		 "MAG_AP_WRP_ERR, "

		 "FLAGS"      
		 ") VALUES \n", basename);

  if (cpy_buffer->Nalloc == 0) {
    InitIOBuffer (cpy_buffer, 1024);
  }
  PrintIOBuffer (cpy_buffer, "INSERT INTO %s_cpy ("
		 "OBJ_ID, "      
		 "CAT_ID, " 
		 "PHOTCODE, "

		 "FLUX_AP_R5, "
		 "FLUX_ERR_AP_R5, "
		 "FLUX_STD_AP_R5, "
		 "FLUX_FIL_AP_R5, "
		 "MAG_AP_R5, "
		 "MAG_ERR_AP_R5, "

		 "FLUX_AP_R6, "
		 "FLUX_ERR_AP_R6, "
		 "FLUX_STD_AP_R6, "
		 "FLUX_FIL_AP_R6, "
		 "MAG_AP_R6, "
		 "MAG_ERR_AP_R6, "

		 "FLUX_AP_R7, "
		 "FLUX_ERR_AP_R7, "
		 "FLUX_STD_AP_R7, "
		 "FLUX_FIL_AP_R7, "
		 "MAG_AP_R7, "
		 "MAG_ERR_AP_R7, "

		 "X11_SM_OBJ, "
		 "X12_SM_OBJ, "
		 "X22_SM_OBJ, "
		 "E1_SM_OBJ, "
		 "E2_SM_OBJ, "
		 "X11_SH_OBJ, "
		 "X12_SH_OBJ, "
		 "X22_SH_OBJ, "
		 "E1_SH_OBJ, "
		 "E2_SH_OBJ, "

		 "X11_SM_PSF, "
		 "X12_SM_PSF, "
		 "X22_SM_PSF, "
		 "E1_SM_PSF, "
		 "E2_SM_PSF, "
		 "X11_SH_PSF, "
		 "X12_SH_PSF, "
		 "X22_SH_PSF, "
		 "E1_SH_PSF, "
		 "E2_SH_PSF, "

		 "GAMMA, "
		 "E1, "
		 "E2, "
		 "NMEAS"
		 ") VALUES \n", basename);

  return TRUE;
}

# define PRINT_FLOAT(BUFFER,FIELD,FORMAT)		  \
  if (isinf(FIELD) || isnan(FIELD)) PrintIOBuffer (BUFFER, "NULL, ");	\
  else PrintIOBuffer (BUFFER, FORMAT, FIELD); 

int insert_FWobjects_mysql_value (IOBuffer *ave_buffer, IOBuffer *sec_buffer, IOBuffer *cpy_buffer, Average *average, SecFilt *secfilt, int Nsecfilt, Lensobj *lensobj, int Nlensobj) {

  int i;

  PrintIOBuffer (ave_buffer, " (");
  PrintIOBuffer (ave_buffer, OFF_T_FMT", ", average->extID);          
  PrintIOBuffer (ave_buffer, "%u,  ", average->objID);          
  PrintIOBuffer (ave_buffer, "%u   ", average->catID);          
  PrintIOBuffer (ave_buffer, "),\n");

  // XXX what rules for keeping or NAN-ing various mags?
  for (i = 0; i < Nsecfilt; i++) {
    float meanPSFFlux    = NAN;
    float meanPSFFluxErr = NAN;
    float meanPSFFluxStd = NAN;
    float meanPSFMag     = NAN;
    float meanPSFMagErr  = NAN;
    if (isfinite(secfilt->dFpsfWrp) && isfinite(secfilt->FpsfWrp)) {
      meanPSFFlux    = secfilt-> FpsfWrp;
      meanPSFFluxErr = secfilt->dFpsfWrp;
      meanPSFFluxStd = secfilt->sFpsfWrp;
      if (meanPSFFlux > 0.0) {
	meanPSFMag    = -2.5 * log10(meanPSFFlux / 3631.0);
	meanPSFMagErr = (2.5 * meanPSFFluxErr) / (meanPSFFlux * log(10));
      }
    }

    float meanKronFlux    = NAN;
    float meanKronFluxErr = NAN;
    float meanKronFluxStd = NAN;
    float meanKronMag     = NAN;
    float meanKronMagErr  = NAN;
    if (isfinite(secfilt->dFkronWrp) && isfinite(secfilt->FkronWrp)) {
      meanKronFlux    = secfilt-> FkronWrp;
      meanKronFluxErr = secfilt->dFkronWrp;
      meanKronFluxStd = secfilt->sFkronWrp;
      if (meanKronFlux > 0.0) {
	meanKronMag    = -2.5 * log10(meanKronFlux / 3631.0);
	meanKronMagErr = (2.5 * meanKronFluxErr) / (meanKronFlux * log(10));
      }      
    }

    float meanApFlux    = NAN;
    float meanApFluxErr = NAN;
    float meanApFluxStd = NAN;
    float meanApMag     = NAN;
    float meanApMagErr  = NAN;
    if (isfinite(secfilt->dFapWrp) && isfinite(secfilt->FapWrp)) {
      meanApFlux    = secfilt-> FapWrp;
      meanApFluxErr = secfilt->dFapWrp;
      meanApFluxStd = secfilt->sFapWrp;
      if (meanApFlux > 0.0) {
	meanApMag    = -2.5 * log10(meanApFlux / 3631.0);
	meanApMagErr = (2.5 * meanApFluxErr) / (meanApFlux * log(10));
      }
    }

    PrintIOBuffer (sec_buffer, " (");
    PrintIOBuffer (sec_buffer, "%hd,  ", secfilt->Nwarp);        
    PrintIOBuffer (sec_buffer, "%hd,  ", secfilt->NusedWrp);        
    PrintIOBuffer (sec_buffer, "%hd,  ", secfilt->NusedKronWrp);        
    PrintIOBuffer (sec_buffer, "%hd,  ", secfilt->NusedApWrp);        

    // use %e?
    PRINT_FLOAT(sec_buffer, meanPSFFlux,      "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanPSFFluxErr,   "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanPSFFluxStd,   "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanPSFMag,       "%.6f, ");
    PRINT_FLOAT(sec_buffer, meanPSFMagErr,    "%.6f, ");    
    
    PRINT_FLOAT(sec_buffer, meanKronFlux,     "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanKronFluxErr,  "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanKronFluxStd,  "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanKronMag,      "%.6f, ");
    PRINT_FLOAT(sec_buffer, meanKronMagErr,   "%.6f, ");

    PRINT_FLOAT(sec_buffer, meanApFlux,     "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanApFluxErr,  "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanApFluxStd,  "%.6e, "); // uflux precision
    PRINT_FLOAT(sec_buffer, meanApMag,      "%.6f, ");
    PRINT_FLOAT(sec_buffer, meanApMagErr,   "%.6f, ");

    PrintIOBuffer (sec_buffer, "%u ", secfilt->flags);       
    PrintIOBuffer (sec_buffer, "),\n");
    secfilt ++;
  }

  for (i = 0; i < Nlensobj; i++) {
    PrintIOBuffer (cpy_buffer, " (");
    PrintIOBuffer (cpy_buffer, "%u,  ", lensobj->objID);          
    PrintIOBuffer (cpy_buffer, "%u,  ", lensobj->catID);          
    PrintIOBuffer (cpy_buffer, "%hd, ", lensobj->photcode);          

    float magApR5    = NAN;
    float magErrApR5 = NAN;
    float magApR6    = NAN;
    float magErrApR6 = NAN;
    float magApR7    = NAN;
    float magErrApR7 = NAN;

    if (lensobj->F_ApR5 > 0.0) {
      magApR5    = -2.5 * log10(lensobj->F_ApR5 / 3631.0);
      magErrApR5 = (2.5 * lensobj->dF_ApR5) / (lensobj->F_ApR5 * log(10));
    }
    if (lensobj->F_ApR6 > 0.0) {
      magApR6    = -2.5 * log10(lensobj->F_ApR6 / 3631.0);
      magErrApR6 = (2.5 * lensobj->dF_ApR6) / (lensobj->F_ApR6 * log(10));
    }
    if (lensobj->F_ApR7 > 0.0) {
      magApR7    = -2.5 * log10(lensobj->F_ApR7 / 3631.0);
      magErrApR7 = (2.5 * lensobj->dF_ApR7) / (lensobj->F_ApR7 * log(10));
    }
    
    PRINT_FLOAT(cpy_buffer, lensobj-> F_ApR5,     "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->dF_ApR5,     "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->sF_ApR5,     "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->fF_ApR5,     "%.6e, ");
    PRINT_FLOAT(cpy_buffer, magApR5,              "%.6f, ");
    PRINT_FLOAT(cpy_buffer, magErrApR5,           "%.6f, ");
    PRINT_FLOAT(cpy_buffer, lensobj-> F_ApR6,     "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->dF_ApR6,     "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->sF_ApR6,     "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->fF_ApR6,     "%.6e, ");
    PRINT_FLOAT(cpy_buffer, magApR6,              "%.6f, ");
    PRINT_FLOAT(cpy_buffer, magErrApR6,           "%.6f, ");
    PRINT_FLOAT(cpy_buffer, lensobj-> F_ApR7,     "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->dF_ApR7,     "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->sF_ApR7,     "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->fF_ApR7,     "%.6e, ");
    PRINT_FLOAT(cpy_buffer, magApR7,              "%.6f, ");
    PRINT_FLOAT(cpy_buffer, magErrApR7,           "%.6f, ");
    PRINT_FLOAT(cpy_buffer, lensobj->X11_sm_obj,  "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->X12_sm_obj,  "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->X22_sm_obj,  "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->E1_sm_obj,   "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->E2_sm_obj,   "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->X11_sh_obj,  "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->X12_sh_obj,  "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->X22_sh_obj,  "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->E1_sh_obj,   "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->E2_sh_obj,   "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->X11_sm_psf,  "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->X12_sm_psf,  "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->X22_sm_psf,  "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->E1_sm_psf,   "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->E2_sm_psf,   "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->X11_sh_psf,  "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->X12_sh_psf,  "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->X22_sh_psf,  "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->E1_sh_psf,   "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->E2_sh_psf,   "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->gamma,       "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->E1, 	  "%.6e, ");
    PRINT_FLOAT(cpy_buffer, lensobj->E2, 	  "%.6e, ");

    PrintIOBuffer (cpy_buffer, "%u ", lensobj->Nmeas); // NOTE: PRINT_FLOAT always adds a trailing comma -- need to use a different method on the last entry
    PrintIOBuffer (cpy_buffer, "),\n");
    lensobj ++;
  }

  return TRUE;
}
    
int insert_FWobjects_mysql_commit (IOBuffer *ave_buffer, IOBuffer *sec_buffer, IOBuffer *cpy_buffer, MYSQL *mysql) {

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

  // check that the last two chars are ,\n and replace with ;\n
  if (!strcmp(&cpy_buffer->buffer[cpy_buffer->Nbuffer-2], ",\n")) {
    cpy_buffer->buffer[cpy_buffer->Nbuffer-2] = ';';
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

    if (DEBUG) fprintf (stderr, "%s\n", cpy_buffer->buffer);
    status = mysql_query(mysql, cpy_buffer->buffer); 
    if (status) {
      fprintf (stderr, "error with insert:\n");
      fprintf (stderr, "%s\n", mysql_error(mysql));
      fprintf (stderr, "Nbuffer: %d\n", cpy_buffer->Nbuffer);
    }
    result = mysql_store_result (mysql);
    if (result) mysql_free_result (result);
  } else {
    fprintf (stderr, "%s\n", sec_buffer->buffer);
  }

  return TRUE;
}
