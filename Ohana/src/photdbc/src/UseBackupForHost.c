# include "dvodist.h"

# define DEBUG 1
# define DELETE_ORIGINAL 0
// make this a user option

// set DATA_USE_BCK for all tables with DATA_ON_BCK and host matches srcHost
int UseBackupForHost (char *catdir, SkyList *skylist, HostTable *table) {
  OHANA_UNUSED_PARAM(catdir);

  int i;
  int srcID, srcIndex;

  // get the IDs for the src and dst machines
  srcID = srcIndex = 0;
  for (i = 0; i < table->Nhosts; i++) {
    if (!strcmp (table->hosts[i].hostname, srcHostname)) { 
      srcID = table->hosts[i].hostID; 
      srcIndex = i; 
    }
  }
  if (!srcID) {
    fprintf (stderr, "failed to find source host %s\n", srcHostname);
    exit (1);
  }

  for (i = 0; i < skylist->Nregions; i++) {
    // skip unassigned tables
    if (skylist->regions[i]->hostID != srcID) continue;

    // skip catalogs which have not been copied
    if (!(skylist->regions[i]->hostFlags & DATA_ON_BCK)) continue;

    // skip catalogs which have an uncleared error
    if (skylist->regions[i]->hostFlags & DATA_COPY_FAILURE) continue;

    // set the USE_BCK flag
    skylist->regions[i]->hostFlags |= DATA_USE_BCK;
  }
  
  return (TRUE);
}

/*** 

     hostID, backupID, and hostFlags:

     hostID gives the primary location of the data
     backupID gives the second location of the data
     
     DATA_ON_TGT  : 0x01 : file exists on hostID
     DATA_ON_BCK  : 0x02 : file exists on backupID
     DATA_USE_BCK : 0x04 : read from backupID, not hostID

     DATA_COPY_FAILURE : error copying data to TGT (set for retry)


 ***/
