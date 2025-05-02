# include "dvomerge.h"

// We track the dvomerge history (dmh) in the headers of the Image table (Image.dat) and
// each of the Object tables (CPT files).  In the Image table, we track the IDs of the dvo
// databases which have been merged.  In the object tables, we track the date and size of
// the CPT files already merged.  When we merge a new dvodb, we block against re-merging
// any Object table if the current one matches one of the already ingested tables.

// XXX should I require all IDs to be in sequence?  The sequence has no meaning except to
// ensure the keyword numbering.  If we require the sequence, then it simplifies the code
// somewhat

// we have a sequence of dvomerge events.  
// in the image table header, we want to track the IDs of the databases 
// which have been merged

// some (possible?) functions needed to track the history

// user must pass in a char * of length DVO_DBID_LEN
char *dmhImageReadID (FITS_DB *db) {

  char dbID[33];
  int status = gfits_scan (&db->header, "DVO_DBID", "%s", 1, dbID);      
  if (!status) {
    return NULL;
  }

  char *ID = strcreate (dbID);
  return ID;
}

void dmhImageFree(dmhImage *history) {
  if (!history) return;
  
  int i;
  for (i = 0; i < history->Nmerge; i++) {
    FREE (history->IDs[i]);
  }
  FREE (history->IDs);
  FREE (history);
}

dmhImage *dmhImageRead (FITS_DB *db) {

  if (!db) return FALSE;
  if (!db->header.buffer) return FALSE;

  dmhImage *history = NULL;
  ALLOCATE (history, dmhImage, 1);

  // should track the number in the header (would overwrite)
  int status = gfits_scan (&db->header, "NMERGE", "%d", 1, &history->Nmerge); 
  if (!status) {
    history->Nmerge = 0;
  }

  // ALLOCATE full list
  ALLOCATE (history->IDs, char *, history->Nmerge);

  int i;
  char name[16], ID[80];
  for (i = 0; i < history->Nmerge; i++) {
    snprintf (name, 16, "DM_%05d", i);
    status = gfits_scan (&db->header, name, "%s", 1, ID);
    if (!status) {
      fprintf (stderr, "failed to read %s\n", name);
      exit (6);
    }
    history->IDs[i] = strcreate (ID);
  }
  return history;
}

// compare stats for the input to the full history for the output
// have we merged this file?
int dmhImageCheck (dmhImage *history, char *dbID) {

  // if the input catalog does not exist, we did not miss it
  // if the output catalog does not exist, we must have missed it

  if (!dbID) return FALSE; // no ID to merge, probably an error to get here
  if (!history) return FALSE; // we must not have merged since it does not exist

  int i;
  for (i = 0; i < history->Nmerge; i++) {
    // do we match this size?
    if (!strcmp(history->IDs[i], dbID)) return TRUE; // we match (already merged)
  }
  return FALSE; // no match, not yet merged
}

int dmhImageAdd (FITS_DB *db, dmhImage *history, char *dbID) {

  // update the arrays
  int last = history->Nmerge;

  history->Nmerge ++;
  REALLOCATE (history->IDs, char *, history->Nmerge);

  history->IDs[last] = strcreate (dbID);

  char name[16];
  snprintf (name, 16, "DM_%05d", last);
  gfits_modify (&db->header, name, "%s", 1, dbID);

  gfits_modify (&db->header, "NMERGE", "%d", 1, history->Nmerge);
  return TRUE;
}

dmhObjectStats *dmhObjectStatsRead (char *filename) {

  // instats.st_size & instats.st_mtime

  // get the stats on this input file (for comparison with the output headers)
  struct stat instats;
  int stat_result = stat (filename, &instats);
  if (stat_result) {
    if (errno == ENOENT) return NULL;
    fprintf (stderr, "cannot read stats on input file %s\n", filename);
    perror ("stats error message:");
    exit (2);
  }
  
  dmhObjectStats *stats = NULL;  
  ALLOCATE (stats, dmhObjectStats, 1);
  stats->size = instats.st_size;
  stats->time = instats.st_mtime;
  stats->date = ohana_sec_to_date (instats.st_mtime);
  return stats;
}

void dmhObjectStatsFree (dmhObjectStats *stats) {
  if (!stats) return;
  if (stats->date) free (stats->date);
  free (stats);
}

dmhObject *dmhObjectAlloc (void) {

  dmhObject *history = NULL;
  ALLOCATE (history, dmhObject, 1);
  history->Nmerge = 0;
  
  // ALLOCATE full list
  ALLOCATE (history->size, off_t,  history->Nmerge);
  ALLOCATE (history->time, time_t, history->Nmerge);
  ALLOCATE (history->date, char *, history->Nmerge);
  return history;
}

void dmhObjectFree (dmhObject *history) {
  if (!history) return;

  FREE (history->size);
  FREE (history->time);

  if (history->date) {
    int i;
    for (i = 0; i < history->Nmerge; i++) {
      FREE (history->date[i]);
    }
  }
  FREE (history->date);
  FREE (history);
  return;
}

// read the array of merged history information from this file's header
dmhObject *dmhObjectRead (char *filename) {

  // if the file does not exist, return NULL (means 'missed')
  struct stat outstats;
  int stat_result = stat (filename, &outstats);
  if (stat_result) {
    if (errno == ENOENT) {
      dmhObject *history = dmhObjectAlloc();
      return history;
    }
    fprintf (stderr, "cannot read stats on output file %s\n", filename);
    perror ("stats error message:");
    exit (2);
  }

  FILE *fout = fopen (filename, "r");
  if (!fout) {
    fprintf (stderr, "problem opening output file to read header %s\n", filename);
    perror ("stats error message:");
    exit (2);
  }

  Header outheader;
  if (!gfits_fread_header (fout, &outheader)) {
    fprintf (stderr, "problem reading header for output file %s\n", filename);
    exit (2);
  }
  fclose (fout);

  dmhObject *history = dmhObjectAlloc();

  int status = gfits_scan (&outheader, "NMERGE", "%d", 1, &history->Nmerge);      
  if (!status) {
    history->Nmerge = 0;
  }

  // ALLOCATE full list
  REALLOCATE (history->size, off_t,  history->Nmerge);
  REALLOCATE (history->time, time_t, history->Nmerge);
  REALLOCATE (history->date, char *, history->Nmerge);

  int i;
  for (i = 0; i < history->Nmerge; i++) {
    off_t  size;
    char   date[80];

    char name[16];
    
    snprintf (name, 16, "MS_%05d", i);
    status = gfits_scan (&outheader, name, OFF_T_FMT, 1, &size);
    if (!status) {
      fprintf (stderr, "failed to read %s\n", name);
      exit (6);
    }
    history->size[i] = size;

    snprintf (name, 16, "MT_%05d", i);
    status = gfits_scan (&outheader, name, "%s", 1, date);
    if (!status) {
      fprintf (stderr, "failed to read %s\n", name);
      exit (7);
    }
    history->date[i] = strcreate (date);
    history->time[i] = ohana_date_to_sec (date);
  }
  gfits_free_header (&outheader);
  return history;
}
  
// compare stats for the input to the full history for the output
// have we merged this file?
int dmhObjectCheck (dmhObject *history, dmhObjectStats *inStats) {

  // if the input catalog does not exist, we did not miss it
  // if the output catalog does not exist, we must have missed it

  if (!inStats) return TRUE; // we do not need to merge something that does not exist...
  if (!history) return FALSE; // we must not have merged since it does not exist

  int i;
  for (i = 0; i < history->Nmerge; i++) {
    
    // do we match this size?
    if (history->size[i] != inStats->size) continue;

    // do we match this date?
    if (history->time[i] != inStats->time) continue;

    return TRUE;  // we match (already merged)
  }
  return FALSE; // no match, not yet merged
}

// add the stats for the given input file to the output structure and header
int dmhObjectAdd (dmhObject *history, Header *header, dmhObjectStats *inStats) {

  // update the arrays
  int last = history->Nmerge;

  history->Nmerge ++;
  REALLOCATE (history->size, off_t,  history->Nmerge);
  REALLOCATE (history->time, time_t, history->Nmerge);
  REALLOCATE (history->date, char *, history->Nmerge);

  history->size[last] = inStats->size;
  history->time[last] = inStats->time;
  history->date[last] = strcreate (inStats->date);

  char name[16];
  snprintf (name, 16, "MS_%05d", last);

  int status = gfits_modify (header, name, OFF_T_FMT, 1, inStats->size); 
  if (!status) { 
    fprintf (stderr, "error: failed to add size to header (%d)\n", (int) inStats->size);
    exit (1);
  }

  snprintf (name, 16, "MT_%05d", last);
  status = gfits_modify (header, name, "%s", 1, inStats->date);
  if (!status) {     
    fprintf (stderr, "error: failed to add date to header (%s)\n", inStats->date);
    exit (1);
  }

  gfits_modify (header, "NMERGE", "%d", 1, history->Nmerge);
  return TRUE;
}

OutputStatus *OutputStatusInit (int N) {

  int i;

  OutputStatus *outstat = NULL;
  ALLOCATE (outstat, OutputStatus, N);
  for (i = 0; i < N; i++) {
    outstat[i].valid = FALSE;
    outstat[i].missed = FALSE;
    outstat[i].history = NULL;
    outstat[i].filename = NULL;
  }
  return outstat;
}

int OutputStatusFree (OutputStatus *outstat, int N) {

  int i;
  for (i = 0; i < N; i++) {
    if (outstat[i].history)  { dmhObjectFree (outstat[i].history); }
    if (outstat[i].filename) { free (outstat[i].filename); }
  }
  free (outstat);
  return TRUE;
}
