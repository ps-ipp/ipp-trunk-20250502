# include "dvodist.h"

# define DEBUG 1
# define DELETE_ORIGINAL 0
// make this a user option

// copy all *.bck currently on the srcHost to dstHost
int CopyBackupToHost (char *catdir, SkyList *skylist, HostTable *table) {

  int i, j, k;
  struct stat filestat;
  char srcname[1024];
  char tgtname[1024];
  char outdir[1024];

  md5_byte_t srcDigest[NDIGEST];
  md5_byte_t tgtDigest[NDIGEST];
    
  uid_t uid = getuid();
  gid_t gid = getgid();

  // get the IDs for the src and dst machines
  int srcID = 0;
  int dstID = 0;
  int dstIndex = 0;
  for (i = 0; i < table->Nhosts; i++) {
    if (!strcmp (table->hosts[i].hostname, srcHostname)) { srcID = table->hosts[i].hostID; }
    if (!strcmp (table->hosts[i].hostname, dstHostname)) { dstID = table->hosts[i].hostID; dstIndex = i; }
  }
  if (!srcID) {
    fprintf (stderr, "failed to find source host %s\n", srcHostname);
    exit (1);
  }
  if (!dstID) {
    fprintf (stderr, "failed to find destination host %s\n", dstHostname);
    exit (1);
  }
  if (srcID == dstID) {
    fprintf (stderr, "invalid destination host %s : matches source host %s\n", dstHostname, srcHostname);
    exit (1);
  }

  HostInfo *dstHost = &table->hosts[dstIndex];

  int Nfailure = 0;
  for (i = 0; i < skylist->Nregions; i++) {
    // skip unassigned tables
    if (skylist->regions[i]->hostID != srcID) continue;

    // skip catalogs which have already been copied
    if (skylist->regions[i]->hostFlags & DATA_ON_BCK) continue;

    // skip catalogs which have an uncleared error
    if (skylist->regions[i]->hostFlags & DATA_COPY_FAILURE) continue;

    // assign the dstHost to the backup ID
    skylist->regions[i]->backupID = dstID;

    // we are copying CATDIR/*.bck to dstHost/pathname/*

    // have to succeed on all 4 tables; if we fail on any one, mark it as failed
    int success = TRUE;

    char *subdir = pathname (skylist->regions[i]->name);
    sprintf (outdir, "%s/%s", dstHost->pathname, subdir);
    free (subdir);

    int status = check_dir_access (outdir, DEBUG);
    if (!status) {
      fprintf (stderr, "failed to find / create target path\n");
      exit (2);
    }

    char extname[4][16] = {"cpm", "cpt", "cps", "cpn"};
    for (j = 0; success && (j < 4); j++) {

      // set the in and out table names
      sprintf (srcname, "%s/%s.%s.bck", catdir, skylist->regions[i]->name, extname[j]);
      sprintf (tgtname, "%s/%s.%s", dstHost->pathname, skylist->regions[i]->name, extname[j]);

      // does srcname exist & can it be read?

      // check permission to read file 
      status = stat (srcname, &filestat);
      if (status) {
	if (errno == ENOENT) continue;  // file does not exist (handle broken table with cpt + cpm but no cpn,cps?)
	perror ("stat:");
	fprintf (stderr, "failure to access file %s\n", srcname);
	success = FALSE;
	continue;
      }
    
      // can we read the file?
      if (uid == filestat.st_uid) {
	if (filestat.st_mode & S_IRUSR) goto valid;
      }
      if (gid == filestat.st_gid) {
	if (filestat.st_mode & S_IRGRP) goto valid;
      }
      if (filestat.st_mode & S_IROTH) goto valid;
    
      fprintf (stderr, "cannot read file %s, skipping\n", srcname);
      success = FALSE;
      continue;

    valid:
      if (j == 0) {
	fprintf (stderr, "copying %s to %s\n", skylist->regions[i]->name, dstHost->pathname);
      }

      // read srcname, write to tgtname, get MD5 sum for srcname as we go:
      // XXX files that fail here should be skipped (but don't give up)
      if (!get_md5_with_copy (srcname, tgtname, srcDigest)) {
	fprintf (stderr, "error reading %s, getting md5, or writing %s\n", srcname, tgtname);
	success = FALSE;
	continue;
      }
	
      // need to re-open and re-read tgtname to check md5sum
      // XXX files that fail here need to be checked
      // char *absname = abspath (tgtname, 1024);
      // if (!get_md5_from_pclient (host, absname, tgtDigest)) {
      if (!get_md5_with_copy (tgtname, NULL, tgtDigest)) {
	fprintf (stderr, "error reading %s or getting md5\n", tgtname);
	success = FALSE;
	continue;
      }

      // compare the two digest values
      int match = TRUE;
      for (k = 0; match && (k < NDIGEST); k++) {
	match &= (tgtDigest[k] == srcDigest[k]);
      }

      if (!match) {
	if (DEBUG) {
	  fprintf (stderr, "failed to copy %s to %s\n", srcname, tgtname);
	}
	success = FALSE;
      }
    }	 

    if (!success) {
      // if we failed to make a copy, remove the failure, mark the failure so we can retry
      skylist->regions[i]->hostFlags |= DATA_COPY_FAILURE;
      for (j = 0; success && (j < 4); j++) {
	sprintf (tgtname, "%s/%s.%s", dstHost->pathname, skylist->regions[i]->name, extname[j]);
	if (!DEBUG) unlink (tgtname);
      }
      Nfailure ++;
    } else {
      skylist->regions[i]->hostFlags |= DATA_ON_BCK;
    }
  }
  
  if (!Nfailure) {
    fprintf (stderr, "all tables successfully moved\n");
  } else {
    fprintf (stderr, "%d tables were not moved\n", Nfailure);
  }

  return (TRUE);
}

/* state table for copy to/from location 

   data on src (local catdir)  : 00.hostID
   data on tgt (remote catdir) : 10.hostID
   failure to copy to tgt      : 01.hostID
   failure to copy to src      : 11.hostID
*/

/*** 

     hostID, backupID, and hostFlags:

     hostID gives the primary location of the data
     backupID gives the second location of the data
     
     DATA_ON_TGT  : 0x01 : file exists on hostID
     DATA_ON_BCK  : 0x02 : file exists on backupID
     DATA_USE_BCK : 0x04 : read from backupID, not hostID

     DATA_COPY_FAILURE : error copying data to TGT (set for retry)


 ***/
