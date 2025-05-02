# include "dvodist.h"

# define DEBUG 1
# define DELETE_ORIGINAL 0
// make this a user option

// XXX collapse this and CopyToHostLocation (only difference is name of src/tgt and bits set on success)
int CopyFromHostLocation (char *catdir, SkyList *skylist, HostTable *table) {

  int i, j, k;
  struct stat filestat;
  char srcname[1024];
  char tgtname[1024];
  char outdir[1024];
    
  md5_byte_t srcDigest[NDIGEST];
  md5_byte_t tgtDigest[NDIGEST];
    
  uid_t uid = getuid();
  gid_t gid = getgid();

  int Nfailure = 0;
  for (i = 0; i < skylist->Nregions; i++) {
    if (VERBOSE) fprintf (stderr, "%s\n", skylist[0].regions[i][0].name);
    
    // move each table from table->hosts[].pathname/name to CATDIR/name

    // skip unassigned tables
    if (!skylist->regions[i]->hostID) continue;

    // skip catalogs which are not on remote location
    if (!(skylist->regions[i]->hostFlags & DATA_ON_TGT)) continue;

    // skip catalogs which have an uncleared error
    if (skylist->regions[i]->hostFlags & DATA_COPY_FAILURE) continue;

    // find the host for the table
    int realID = skylist->regions[i]->hostID;
    short index = table->index[realID];
    HostInfo *host = &table->hosts[index];

    // have to succeed on all 4 tables; if we fail on any one, 
    int success = TRUE;

    char *subdir = pathname (skylist->regions[i]->name);
    sprintf (outdir, "%s/%s", host->pathname, subdir);
    free (subdir);

    int status = check_dir_access (outdir, DEBUG);
    if (!status) {
      fprintf (stderr, "failed to find / create target path\n");
      exit (2);
    }

    char extname[4][16] = {"cpm", "cpt", "cps", "cpn"};
    for (j = 0; success && (j < 4); j++) {

      // set the in and out table names
      sprintf (srcname, "%s/%s.%s", host->pathname, skylist->regions[i]->name, extname[j]);
      sprintf (tgtname, "%s/%s.%s", catdir, skylist->regions[i]->name, extname[j]);

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

      // the output (tgtname) could exist and be a link to the same file; delete it if it exists
      status = stat (tgtname, &filestat);
      if (!status) {
	// a tgtfile already exists; get rid of it
	if (unlink (tgtname)) {
	  fprintf (stderr, "failed to remove CATDIR file %s\n", tgtname);
	  success = FALSE;
	  continue;
	}
      }

      // read srcname, write to tgtname, get MD5 sum for srcname as we go:
      if (!get_md5_with_copy (srcname, tgtname, srcDigest)) {
	fprintf (stderr, "error reading %s, getting md5, or writing %s\n", srcname, tgtname);
	success = FALSE;
	continue;
      }
	
      // need to re-open and re-read tgtname to check md5sum
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
      // let's use the upper 2 bits of the hostID for informational purposes. this
      // leaves us with up to 16383 hosts.

      // data on src  = (hostID & 0x8000) >> 15
      // copy failed  = (hostID & 0x4000) >> 14
      // if we failed to make a copy, 
	  
      skylist->regions[i]->hostFlags |= DATA_COPY_FAILURE;
      for (j = 0; success && (j < 4); j++) {
	sprintf (tgtname, "%s/%s.%s", catdir, skylist->regions[i]->name, extname[j]);
	if (!DEBUG) unlink (tgtname);
      }
      Nfailure ++;
    } else {
      skylist->regions[i]->hostFlags &= ~DATA_ON_TGT;
      for (j = 0; success && (j < 4); j++) {
	sprintf (srcname, "%s/%s.%s", host->pathname, skylist->regions[i]->name, extname[j]);
	if (!DEBUG && DELETE_ORIGINAL) unlink (srcname);
      }
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

/* need to think a bit about behavior in the context of SPLIT vs MEF:

 * SPLIT : all 4 tables either exist or none do
 * MEF only cpt table exists

 * read the SPLIT headers to see if MEASURE, MISSING, SECFILT fields exist?
 */
