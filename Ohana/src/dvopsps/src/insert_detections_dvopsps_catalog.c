# include "dvopsps.h"
# define DEBUG 0

int Ndetections = 0;
int NDETECTIONS = 0;
Detections *detections = NULL;

static float ZeroPoint = 25.0;

int init_detections () {
  NDETECTIONS = 1000;
  ALLOCATE (detections, Detections, NDETECTIONS);
  return TRUE;
}

int append_detections_dvopsps_catalog (Catalog *catalog) {

  off_t i, j;

  ZeroPoint = GetZeroPoint();
  int Nsecfilt = GetPhotcodeNsecfilt ();

  int missingID = 0;

  if (Ndetections + catalog[0].Nmeasure >= NDETECTIONS) {
    NDETECTIONS = Ndetections + catalog[0].Nmeasure + 1000;
    REALLOCATE (detections, Detections, NDETECTIONS);
  }

  time_t TIME_START_SEC = 0;
  time_t TIME_END_SEC   = 0;
  if (TIME_START) ohana_str_to_time (TIME_START, &TIME_START_SEC); // we validate this in the args.c
  if (TIME_END)   ohana_str_to_time (TIME_END,   &TIME_END_SEC);   // we validate this in the args.c

  for (i = 0; i < catalog[0].Naverage; i++) {

    if (catalog[0].average[i].extID == 0) {
      missingID ++;
    }

    off_t m = catalog[0].average[i].measureOffset;
    for (j = 0; j < catalog[0].average[i].Nmeasure; j++) {

      Average *average = &catalog->average[i];
      Measure *measure = &catalog->measure[m + j];

      // some filters -- these are the detections we skip
      if (TIME_START && (measure->t < TIME_START_SEC)) continue;
      if (TIME_END   && (measure->t >=  TIME_END_SEC)) continue;

      if (measure->photcode < PHOTCODE_START) continue;
      if (measure->photcode >=  PHOTCODE_END) continue;

      int equivCode = GetPhotcodeEquivCodebyCode (measure->photcode);
      int Nsec = GetPhotcodeNsec (equivCode);

      SecFilt *secfilt = (Nsec >= 0) ? &catalog->secfilt[Nsecfilt*i + Nsec] : NULL;

      assign_detection_values (&detections[Ndetections], measure, average, secfilt);
      Ndetections ++;

      myAssert (Ndetections <= NDETECTIONS, "programming error");
    }
  }

  fprintf (stderr, "write %d detections, %d missing extID\n", Ndetections, missingID);
  return TRUE;
}

// XXX write a single table for all detections or a series of tables?
// if I write a single table, then I have to store all data in memory
// if I write multiple tables, then I need to (a) iterate over the tables and (b) 
// incur seek hits on the file
int save_detections_dvopsps () {

  int status = TRUE;

  if (!DetectionsSave (RESULT_FILE, detections, Ndetections)) {
    fprintf (stderr, "failed to save detection file %s\n", RESULT_FILE);
    status = FALSE;
  }

  free (detections);
  Ndetections = 0;
  return (status);
}

time_t TIME_START_SEC = 0;
time_t TIME_END_SEC   = INT_MAX;

int insert_detections_dvopsps_catalog (Catalog *catalog, MYSQL *mysql) {

  off_t i, j;
  int missingID = 0;

  IOBuffer buffer;
  buffer.Nalloc = 0;

  Average *average = catalog->average;
  Measure *measure = catalog->measure;

  off_t found = 0;

  ZeroPoint = GetZeroPoint();
  int Nsecfilt = GetPhotcodeNsecfilt ();

  INITTIME;
  insert_detections_mysql_init (&buffer);
  int Ninsert = 0;
  int Ntotal = 0;

  int status = TRUE;

  if (TIME_START) ohana_str_to_time (TIME_START, &TIME_START_SEC); // we validate this in the args.c
  if (TIME_END)   ohana_str_to_time (TIME_END,   &TIME_END_SEC);   // we validate this in the args.c

  // NOTE for testing, just do a few objects
  for (i = 0; i < catalog[0].Naverage; i++) {

    if (average[i].extID == 0) {
      missingID ++;
    }

    off_t m = average[i].measureOffset;
    for (j = 0; j < average[i].Nmeasure; j++) {

      off_t Nmeas = m + j;

      // some filters -- these are the detections we skip (not an error, just skipped)
      if (TIME_START && (measure[Nmeas].t < TIME_START_SEC)) continue;
      if (TIME_END   && (measure[Nmeas].t >=  TIME_END_SEC)) continue;
  
      if (measure[Nmeas].photcode < PHOTCODE_START) continue;
      if (measure[Nmeas].photcode >=  PHOTCODE_END) continue;

      int equivCode = GetPhotcodeEquivCodebyCode (measure[Nmeas].photcode);
      int Nsec = GetPhotcodeNsec (equivCode);

      SecFilt *secfilt = (Nsec > -1) ? &catalog->secfilt[Nsecfilt*i + Nsec] : NULL;

      // XXX check return status
      Detections detection;
      assign_detection_values (&detection, &measure[Nmeas], &average[i], secfilt);

      if (!insert_detections_mysql_detvalue (&buffer, &detection)) {
	fprintf (stderr, "failure to insert detections in mysql\n");
	status = FALSE;
      }
      Ninsert ++;
      Ntotal ++;

      if (buffer.Nbuffer > MAX_BUFFER) {
	if (!insert_detections_mysql_commit (&buffer, mysql)) {
	  fprintf (stderr, "failure to insert detections in mysql (commit)\n");
	  status = FALSE;
	}
	if (DEBUG) fprintf (stderr, "inserted %d rows\n", Ninsert);
	Ninsert = 0;
	if (0) {
	  FlushIOBuffer (&buffer);
	} else {
	  buffer.Nbuffer = 0;
	  bzero (buffer.buffer, buffer.Nalloc);
	}
	insert_detections_mysql_init (&buffer);
      }
      found ++;
    }
  }
  if (Ninsert > 0) {
    // insert the remaining detections loaded in the buffer
    if (!insert_detections_mysql_commit (&buffer, mysql)) {
      fprintf (stderr, "failure to insert detections in mysql (commit)\n");
      status = FALSE;
    }
  }
  
  if (VERBOSE) fprintf (stderr, "inserted %d rows\n", Ntotal);
  FreeIOBuffer (&buffer);

  if (!status) {
    MARKTIME("-- failed to insert "OFF_T_FMT" rows in %f sec\n", found, dtime);
    return (FALSE);
  }

  MARKTIME("-- inserted "OFF_T_FMT" rows in %f sec, skipped %d without IDs\n", found, dtime, missingID);
  return (TRUE);
}

int insert_detections_mysql_init (IOBuffer *buffer) {

  if (buffer->Nalloc == 0) {
    InitIOBuffer (buffer, 1024);
  }

  PrintIOBuffer (buffer, "INSERT INTO dvoDetectionFull (objID, detectID, ippObjID, ippDetectID, imageID, catID, ");
  PrintIOBuffer (buffer, "ra, dec_, raErr, decErr, zpPSF, zpFactorPSF, zpAPER, zpFactorAPER, telluricExt, airmass, expTime, ");
  PrintIOBuffer (buffer, "Mpsf, dMpsf, Mkron, dMkron, Map, dMap, flags, objflags, filtflags) VALUES \n");

  return TRUE;
}

int insert_detections_mysql_commit (IOBuffer *buffer, MYSQL *mysql) {

  MYSQL_RES *result;

  int status = TRUE;

  // check that the last two chars are ,\n and replace with ;\n
  if (!strcmp(&buffer->buffer[buffer->Nbuffer-2], ",\n")) {
    buffer->buffer[buffer->Nbuffer-2] = ';';
  } else {
    fprintf (stderr, "invalid sql?\n");
    int Nstart = MAX(0, buffer->Nbuffer - 3000);
    fprintf (stderr, "buffer: ...%s...\n", &buffer->buffer[Nstart]);
    return FALSE;
  }

  // XXX check return status
  if (mysql) {
    int mysqlStatus = mysql_query(mysql, buffer->buffer); 
    if (mysqlStatus) {
      fprintf (stderr, "error with insert:\n");
      fprintf (stderr, "%s\n", mysql_error(mysql));
      fprintf (stderr, "Nbuffer: %d\n", buffer->Nbuffer);
      if (DEBUG) {
	FILE *f = fopen ("dvopsps.dump.sql", "w");
	fprintf (f, "%s\n", buffer->buffer);
	fclose (f);
      }
      status = FALSE;
    }
    result = mysql_store_result (mysql);
    mysql_free_result (result);
  } else {
    fprintf (stderr, ".");
  }

  return status;
}

# define PRINT_FLOAT(BUFFER,FIELD,FORMAT)		  \
  if (!isfinite(FIELD)) PrintIOBuffer (BUFFER, "NULL, "); \
  else PrintIOBuffer (BUFFER, FORMAT, FIELD); 

int insert_detections_mysql_detvalue (IOBuffer *buffer, Detections *detection) {

# if (0) 
  if (detection->detectID == 164458937060000101) {
    fprintf (stderr, "isfinite: %d\n", isfinite(detection->dMpsf));
    fprintf (stderr, "isnan: %d\n", isnan(detection->dMpsf));
    fprintf (stderr, "isinf: %d\n", isinf(detection->dMpsf));
  }
# endif

  // XXX I needed OFF_T_FMT on my 32bit ubuntu laptop; ok on 64bit?
  PrintIOBuffer (buffer, "("OFF_T_FMT", ", detection->objID);	    // objID
  PrintIOBuffer (buffer,  OFF_T_FMT", ", detection->detectID);    // detectID
  PrintIOBuffer (buffer,  OFF_T_FMT", ", detection->ippObjID);    // ippObjID
  PrintIOBuffer (buffer,  "%u,  ", detection->ippDetectID); // ippDetectID
  PrintIOBuffer (buffer,  "%d,  ", detection->imageID);	    // imageID
  PrintIOBuffer (buffer,  "%d,  ", detection->catID);	    // DVO Catalog ID

  PRINT_FLOAT(buffer, detection->ra,          "%.8f, ");
  PRINT_FLOAT(buffer, detection->dec,         "%.8f, ");
  PRINT_FLOAT(buffer, detection->raErr,       "%.6f, ");
  PRINT_FLOAT(buffer, detection->decErr,      "%.6f, ");
  PRINT_FLOAT(buffer, detection->zpPSF,       "%.6f, ");
  PRINT_FLOAT(buffer, detection->zpFactorPSF, "%.6e, ");
  PRINT_FLOAT(buffer, detection->zpAPER,      "%.6f, ");
  PRINT_FLOAT(buffer, detection->zpFactorAPER,"%.6e, ");
  PRINT_FLOAT(buffer, detection->telluricExt, "%.6f, ");
  PRINT_FLOAT(buffer, detection->airmass,     "%.6f, ");
  PRINT_FLOAT(buffer, detection->expTime,     "%.6f, ");

  PRINT_FLOAT(buffer, detection->Mpsf,        "%.6f, ");
  PRINT_FLOAT(buffer, detection->dMpsf,       "%.6f, ");
  PRINT_FLOAT(buffer, detection->Mkron,       "%.6f, ");
  PRINT_FLOAT(buffer, detection->dMkron,      "%.6f, ");
  PRINT_FLOAT(buffer, detection->Map,         "%.6f, ");
  PRINT_FLOAT(buffer, detection->dMap,        "%.6f, ");

  PrintIOBuffer (buffer, "%u, ", detection->flags);	 // measure.flags
  PrintIOBuffer (buffer, "%u, ", detection->objflags); // average.flags
  PrintIOBuffer (buffer, "%u),\n", detection->filtflags); // secfilt.flags
  return TRUE;
}

float getMagFromValueOrFlux (float flux, float mag, float zp) {

  // first, if mag is finite, use mag:
  if (isfinite(mag)) {
    return (zp + mag - ZeroPoint);
  }

  if (isfinite(flux) && (flux > 0.0)) {
    // fprintf (stderr, "funny flux: %f with mag %f\n", flux, mag);
    return (zp - 2.5*log10(flux));
  }

  if (isfinite(flux) && (flux <= 0.0)) {
    return (NAN);
  }
  return (NAN);
}

float getdMagFromValueOrFlux (float flux, float dflux, float dmag) {

  // first, if mag is finite, use mag:
  if (isfinite(dmag)) {
    return (dmag);
  }

  if (isfinite(flux) && isfinite(dflux)) {
    return (dflux / fabs(flux));
  }
  return (NAN);
}

int assign_detection_values (Detections *detection, Measure *measure, Average *average, SecFilt *secfilt) {

  // myAbort ("check on measure->McalPSF vs McalAPER");

  PhotCode *code = GetPhotcodebyCode(measure->photcode);

  uint64_t ippObjID = ((uint64_t)average->catID << 32) + (uint64_t)average->objID; // ippObjID

  float Mflat       = isfinite(measure->Mflat) ? measure->Mflat : 0.0;
  float nominalZP   = code->C * 0.001 + code->K * (measure->airmass - 1);
  float zpPSF       = nominalZP - measure->McalPSF - Mflat;
  float zpFactorPSF = pow(10.0, -0.4*zpPSF + 3.56);
  float zpAPER      = nominalZP - measure->McalAPER - Mflat;
  float zpFactorAPER= pow(10.0, -0.4*zpAPER + 3.56);
  float telluricExt = - measure->McalPSF;
  float expTime     = pow(10.0, 0.4 * measure->dt);
  float airmass     = measure->airmass;

  detection->objID	  = average->extID;   // objID
  detection->detectID     = measure->extID;   // detectID
  detection->ippObjID     = ippObjID;	      // ippObjID
  detection->ippDetectID  = measure->detID;   // ippDetectID
  detection->imageID      = measure->imageID; // imageID
  detection->catID        = measure->catID;   // catID

  detection->ra 	  = measure->R;    // ra
  detection->dec 	  = measure->D;    // dec
  detection->raErr 	  = measure->dXccd * 0.01 * fabs(measure->pltscale); // estimate of raErr
  detection->decErr	  = measure->dYccd * 0.01 * fabs(measure->pltscale); // estimate of decErr

  detection->zpPSF 	  = zpPSF;
  detection->zpFactorPSF  = zpFactorPSF;
  detection->zpAPER 	  = zpAPER;
  detection->zpFactorAPER = zpFactorAPER;
  detection->telluricExt  = telluricExt;
  detection->airmass      = airmass;
  detection->expTime      = expTime;

  // XXX clean this up with dvo_photcode_ops calls:
  // if (isfinite(measure->FluxPSF) && (measure->FluxPSF < 0.0)) 
  detection->Mpsf  = getMagFromValueOrFlux (measure->FluxPSF,  measure->M,     zpPSF);
  detection->Mkron = getMagFromValueOrFlux (measure->FluxKron, measure->Mkron, zpAPER);
  detection->Map   = getMagFromValueOrFlux (measure->FluxAp,   measure->Map,   zpAPER);

  detection->dMpsf  = getdMagFromValueOrFlux (measure->FluxPSF,  measure->dFluxPSF,  measure->dM);
  detection->dMkron = getdMagFromValueOrFlux (measure->FluxKron, measure->dFluxKron, measure->dMkron);
  detection->dMap   = getdMagFromValueOrFlux (measure->FluxAp,   measure->dFluxAp,   measure->dMap);

  detection->flags     = measure->dbFlags; // flags
  detection->objflags  = average->flags;   // flags
  detection->filtflags = secfilt ? secfilt->flags : 0; // flags

  return TRUE;
}

