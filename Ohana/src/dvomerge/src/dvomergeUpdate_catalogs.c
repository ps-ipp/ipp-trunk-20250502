# include "dvomerge.h"
# define DEBUG 1
# define MACHINE_GROUPS 1

// in parallel mode, the target database may be distributed, while the input database is
// only on a single machine

// this function only merges tables which are at equal or shallower depth compared to the
// output tables

int dvomergeUpdate_parallel (char *input, char *output, SkyTable *outsky, IDmapType *IDmap);
int dvomergeUpdate_parallel_group (HostTableGroup *group, char *absinput, char *absoutput);

int dvomergeUpdate_catalogs (char *input, char *output, SkyList *inlist, SkyTable *outsky, int NsecfiltInput, int NsecfiltOutput, IDmapType *IDmap, int *secfiltMap) {

  off_t i, j;
  SkyList *outlist;
  Catalog incatalog, outcatalog;

  if (PARALLEL && !HOST_ID) {
    int status = dvomergeUpdate_parallel (input, output, outsky, IDmap);
    return status;
  }

  // load the list of hosts
  HostTable *table_input = NULL;
  if (PARALLEL_INPUT) {
    table_input = HostTableLoad (input, inlist->hosts);
    if (!table_input) {
      fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", inlist->hosts, input);
      exit (1);
    }    

    // ensure we have absolute paths for hostdirs
    for (i = 0; i < table_input->Nhosts; i++) {
      char *tmppath = abspath (table_input->hosts[i].pathname, DVO_MAX_PATH);
      free (table_input->hosts[i].pathname);
      table_input->hosts[i].pathname = tmppath;
    }
  }

  // loop over the populated input regions
  for (i = 0; i < inlist[0].Nregions; i++) {
    if (!inlist[0].regions[i][0].table) continue;
    if (VERBOSE) fprintf (stderr, "input: %s\n", inlist[0].regions[i][0].name);

    // in a parallel context, we need to re-map the input filename
    char *filename_input = strcreate (inlist[0].filename[i]);
    if (PARALLEL_INPUT) {

      int hostID_input = inlist[0].regions[i]->hostID;
      int table_index = table_input->index[hostID_input];
      char *hostdir_input = table_input->hosts[table_index].pathname;

      // set the parameters which guide catalog open/load/create
      char hostfile_input[DVO_MAX_PATH];
      snprintf (hostfile_input, DVO_MAX_PATH, "%s/%s.cpt", hostdir_input, inlist[0].regions[i]->name);
      free (filename_input);
      filename_input = strcreate (hostfile_input);
    }

# ifdef OLD_CODE
    // NOTE: i was having trouble dropping objects on edges with the option below.  Since an object can move outside the catalog grab the neighbors.
    // SkyListByBounds will return neighbor catalogs if the boundaries exactly match (due to rounding).  Since the regions are not infinitely small, 
    // compare to a slightly reduced footprint
    float dPos = 2.0/3600.0;
    outlist = SkyListByBounds (outlist, -1, inlist[0].regions[i][0].Rmin + dPos, inlist[0].regions[i][0].Rmax - dPos, inlist[0].regions[i][0].Dmin + dPos, inlist[0].regions[i][0].Dmax - dPos);
# endif

    if (MATCHED_TABLES) {
      // if we know the output will have the same depth / layout as the input, just choose the matched table
      outlist = SkyRegionByIndex (outsky, inlist[0].regions[i][0].index);
    } else {
      // Since an object can move outside the catalog grab the neighbors.
      float dPos = 2.0/3600.0;
      outlist = SkyListByBounds (outsky, -1, inlist[0].regions[i][0].Rmin - dPos, inlist[0].regions[i][0].Rmax + dPos, inlist[0].regions[i][0].Dmin - dPos, inlist[0].regions[i][0].Dmax + dPos);
    }

    if (CPTLIST) {
      outlist = SkyListMatchList (outlist, CPTLIST, NCPTLIST);
    }
    
    OutputStatus *outstat = OutputStatusInit (outlist->Nregions);

    // there may be more than one output region for a given input region.  
    // are any of these output regions relevant to this machine?
    int found = FALSE;
    for (j = 0; j < outlist[0].Nregions; j++) {
      outstat[j].valid = HostTableTestHost(outlist[0].regions[j], HOST_ID);
      found = (found || outstat[j].valid);
    }

    // skip this input table for if no output files are on this machine
    if (!found) {
      OutputStatusFree (outstat, outlist->Nregions);
      SkyListFree (outlist); 
      free (filename_input);
      continue; 
    }

    // get stats for history check, skip input catalog if file not found (NULL inStats)
    dmhObjectStats *inStats = dmhObjectStatsRead (filename_input);
    if (!inStats) {
      if (VERBOSE) fprintf (stderr, "skipping %s, empty \n", filename_input);
      OutputStatusFree (outstat, outlist->Nregions);
      SkyListFree (outlist); 
      free (filename_input);
      continue;
    }

    // Check if any of the output files have NOT yet received data from this input file
    // If none have been missed, we can skip the input file completely
    int missed = FALSE;
    for (j = 0; j < outlist[0].Nregions; j++) {
      if (!outstat[j].valid) continue;

      // set the parameters which guide catalog open/load/create
      char hostfile[DVO_MAX_PATH];
      snprintf (hostfile, DVO_MAX_PATH, "%s/%s.cpt", HOSTDIR, outlist[0].regions[j]->name);
      char *filename = HOST_ID ? hostfile : outlist[0].filename[j];
      outstat[j].filename = strcreate (filename);

      outstat[j].history = dmhObjectRead (outstat[j].filename);

      // have we already merged this database?
      outstat[j].missed = !dmhObjectCheck (outstat[j].history, inStats);
      missed = (missed || outstat[j].missed);
    }
    if (!FORCE_MERGE && !missed) {
      if (VERBOSE || VERIFY) fprintf (stderr, "skipping %s, already merged\n", filename_input);
      OutputStatusFree (outstat, outlist->Nregions);
      dmhObjectStatsFree (inStats);
      SkyListFree (outlist); 
      free (filename_input);
      continue;
    }
    if (VERIFY) { 
      fprintf (stderr, "%s NOT merged\n", filename_input);
      OutputStatusFree (outstat, outlist->Nregions);
      dmhObjectStatsFree (inStats);
      SkyListFree (outlist); 
      free (filename_input);
      continue;
    }

    // read the input catalog
    LoadCatalog (&incatalog, &inlist[0].regions[i][0], filename_input, "r", NsecfiltInput);

    // skip empty input catalogs
    if (!incatalog.Naverage_disk) {
	dvo_catalog_unlock (&incatalog);
	dvo_catalog_free (&incatalog);
	OutputStatusFree (outstat, outlist->Nregions);
	dmhObjectStatsFree (inStats);
	SkyListFree (outlist); 
	free (filename_input);
	continue;
    }

    if (!incatalog.sorted) {
      fprintf (stderr, "ERROR: input catalog %s is not sorted (and must be for dvomerge)\n", filename_input);
      exit (1);
    }

    // for SKIP_IMAGES, this is a NO-OP
    dvo_update_image_IDs (IDmap, &incatalog);

    // merge input into the appropriate output tables
    for (j = 0; j < outlist[0].Nregions; j++) {
      // skip tables for which the output files are not on this machine
      if (!outstat[j].valid) continue; 

      // skip if we have already done the merge
      if (!FORCE_MERGE && !outstat[j].missed) continue; 

      if (VERBOSE) fprintf (stderr, "output : %s\n", outlist[0].regions[j][0].name);

      // the real filename
      outcatalog.filename = outstat[j].filename; 

      // load input catalog
      LoadCatalog (&outcatalog, outlist[0].regions[j], outcatalog.filename, "w", NsecfiltOutput);

      if (RESET_STARPAR) {
	myAssert (MATCHED_TABLES, "must use -matched-tables to reset starpar");
	ResetStarPar (&outcatalog);
      }
      if (RESET_LENSING) {
	myAssert (MATCHED_TABLES, "must use -matched-tables to reset lensing");
	ResetLensing (&outcatalog);
      }

      if (UPDATE_CATFORMAT) {
	outcatalog.catformat = dvo_catalog_catformat (UPDATE_CATFORMAT);
      } else {
	// IF no catalog already exists, use the input catalog to define the format
	if (outcatalog.Naverage_disk == 0) {
	  outcatalog.catformat = incatalog.catformat;
	}
      }

      if (UPDATE_CATCOMPRESS) {
	outcatalog.catcompress = dvo_catalog_catcompress (UPDATE_CATCOMPRESS);
      } else {
	// IF no catalog already exists, use the input catalog to define the compression
	if (outcatalog.Naverage_disk == 0) {
	  outcatalog.catcompress = incatalog.catcompress;
	}
      }

      if (REPAIR_BY_OBJID) {
	// For gpc1 / PV3, I broke some catalogs with dvomerge -replace: the last object can
	// contaminate the first set of new measurements
	repair_catalog_by_objID (&outcatalog);
      }

      merge_catalogs_old (outlist[0].regions[j], &outcatalog, &incatalog, RADIUS, secfiltMap);

      if (outstat[j].missed) {
	dmhObjectAdd (outstat[j].history, &outcatalog.header, inStats);
      }

      if (!dvo_catalog_backup (&outcatalog, "~", TRUE)) {
	fprintf (stderr, "ERROR: failed to make backup for catalog %s\n", outlist[0].filename[j]);
	exit (1);
      }

      // if we receive a signal which would cause us to exit, wait until the full catalog is written
      SetProtect (TRUE);
      if (!dvo_catalog_save (&outcatalog, VERBOSE)) { fprintf (stderr, "ERROR: failed to save catalog %s\n", outcatalog.filename); exit (1); }
      if (!dvo_catalog_unlock (&outcatalog)) { fprintf (stderr, "ERROR: failed to unlock catalog %s\n", outcatalog.filename); exit (1); }
      SetProtect (FALSE);

      if (!dvo_catalog_unlink_backup (&outcatalog, "~", TRUE)) {
	fprintf (stderr, "WARNING: failed to remove backup for catalog %s\n", outlist[0].filename[j]);
      }

      dvo_catalog_free (&outcatalog);

      fprintf (stderr, "merged %s into %s\n", inlist[0].regions[i][0].name, outlist[0].regions[j][0].name);
    }

    OutputStatusFree (outstat, outlist->Nregions);
    SkyListFree (outlist);
    dmhObjectStatsFree (inStats);

    free (filename_input);
    dvo_catalog_unlock (&incatalog);
    dvo_catalog_free (&incatalog);
  }

  dvo_report_image_IDs (IDmap);

  FreeHostTable(table_input);

  return TRUE;
}

// launch the dvomergeUpdate_client jobs to the parallel hosts
int dvomergeUpdate_parallel (char *input, char *output, SkyTable *outsky, IDmapType *IDmap) {

  // ensure that the paths are absolute path names
  char *absinput  = abspath (input,  DVO_MAX_PATH);
  char *absoutput = abspath (output, DVO_MAX_PATH);

  // name of image subset file:
  char IDmapFilename[DVO_MAX_PATH];
  snprintf (IDmapFilename, DVO_MAX_PATH, "%s/IDmap.fits", absoutput);

  if (!VERIFY_CATALOG_ONLY) {
    // save IDmap information
    if (!IDmapSave (IDmapFilename, IDmap)) {
      fprintf (stderr, "ERROR: failure to save the image ID map\n");
      exit (1);
    }
  }

  // load the list of hosts
  HostTable *table = HostTableLoad (output, outsky->hosts);
  if (!table) {
    fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", outsky->hosts, output);
    exit (1);
  }    

  int Ngroups;
  HostTableGroup *groups = HostTableGroupsMaxNumber (table, &Ngroups, MAX_CLIENTS);
  // split the table into Ngroups, each with a unique set of machines (this avoids overloading the remote host machines)

  int i;
  for (i = 0; i < Ngroups; i++) {
    // update only a max of MaxClient machines at a time
    dvomergeUpdate_parallel_group (&groups[i], absinput, absoutput);
  }

  for (i = 0; i < Ngroups; i++) {
    free (groups[i].hosts);
  }
  free (groups);

  FreeHostTable(table);

  free (absinput);
  free (absoutput);
  return TRUE;
}

int dvomergeUpdate_parallel_group (HostTableGroup *group, char *absinput, char *absoutput) {

  int i;
  for (i = 0; i < group->Nhosts; i++) {

    // ensure that the paths are absolute path names
    char *tmppath = abspath (group->hosts[i][0].pathname, DVO_MAX_PATH);
    free (group->hosts[i][0].pathname);
    group->hosts[i][0].pathname = tmppath;

    // options / arguments that can affect relastro_client -update-objects:
    char *command = NULL;
    strextend (&command, "dvomerge_client %s into %s -hostID %d -hostdir %s -region %f %f %f %f -D ADDSTAR_RADIUS %f", 
	       absinput, absoutput, group->hosts[i][0].hostID, group->hosts[i][0].pathname, 
	       UserPatch.Rmin, UserPatch.Rmax, UserPatch.Dmin, UserPatch.Dmax, RADIUS);

    if (VERBOSE)               { strextend (&command, "-v"); }
    if (VERIFY)                { strextend (&command, "-verify"); }
    if (VERIFY_CATALOG_ONLY)   { strextend (&command, "-verify-catalogs"); }
    if (REPLACE_BY_PHOTCODE)   { strextend (&command, "-replace"); }
    if (REPLACE_TYCHO)         { strextend (&command, "-replace-tycho"); }
    if (PARALLEL_INPUT)        { strextend (&command, "-parallel-input"); }
    if (FORCE_MERGE)           { strextend (&command, "-force-merge"); }
    if (ACCEPT_MOTION)         { strextend (&command, "-accept-motion"); }
    if (ACCEPT_ASTROM)         { strextend (&command, "-accept-astrom"); }
    if (RETAIN_AVE_PHOTOMETRY) { strextend (&command, "-retain-ave-photometry"); }
    if (MATCHED_TABLES)        { strextend (&command, "-matched-tables"); }
    if (MATCH_BY_EXTERN_ID)    { strextend (&command, "-match-by-extern-id"); }
    if (ONLY_MATCHES)          { strextend (&command, "-only-matches"); }
    if (UPDATE_CATFORMAT)      { strextend (&command, "-update-catformat %s", UPDATE_CATFORMAT); }
    if (UPDATE_CATCOMPRESS)    { strextend (&command, "-update-catcompress %s", UPDATE_CATCOMPRESS); }
    if (REPAIR_BY_OBJID)       { strextend (&command, "-repair-by-objid"); }

    if (SKIP_MEASURE)               { strextend (&command, "-skip-measure"); }
    if (SKIP_MISSING)               { strextend (&command, "-skip-missing"); }
    if (SKIP_LENSING)               { strextend (&command, "-skip-lensing"); }
    if (SKIP_LENSOBJ)               { strextend (&command, "-skip-lensobj"); }
    if (SKIP_GALPHOT)               { strextend (&command, "-skip-galphot"); }
    if (SKIP_STARPAR)               { strextend (&command, "-skip-starpar"); }

    if (RESET_STARPAR)              { strextend (&command, "-reset-starpar"); }
    if (RESET_LENSING)              { strextend (&command, "-reset-lensing"); }
    if (ALLOW_MISSING_INPUT_IMAGES) { strextend (&command, "-allow-missing-input-images"); }
    if (CPTLIST_FILENAME)           { strextend (&command, "-restrict-cpt %s", CPTLIST_FILENAME); }
    if (REPAIR_BY_OBJID)            { strextend (&command, "-repair-by-objid"); }

    // add some config variables:
    strextend (&command, "-D CATMODE %s", CATMODE);
    strextend (&command, "-D CATFORMAT %s", CATFORMAT); 
    strextend (&command, "-D SKY_DEPTH %d", SKY_DEPTH);

    fprintf (stderr, "command: %s\n", command);

    if (PARALLEL_MANUAL) continue;

    if (PARALLEL_SERIAL) {
      int status = system (command);
      if (status) {
	fprintf (stderr, "ERROR running dvomerge_client\n");
	exit (2);
      }
    } else {
      // launch the job on the remote machine (no handshake)
      int errorInfo = 0;
      int pid = rconnect ("ssh", group->hosts[i][0].hostname, command, group->hosts[i][0].stdio, &errorInfo, FALSE);
      if (!pid) {
	if (DEBUG) fprintf (stderr, "failure to start %s (error %d)\n", group->hosts[i][0].hostname, errorInfo);
	exit (1);
      }
      group->hosts[i][0].pid = pid; // save for future reference
    }
    free (command);
  }

  if (PARALLEL_MANUAL) {
    fprintf (stderr, "run the dvomerge_client commands above.  when these are done, hit return\n");
    getchar();
  }
  if (!PARALLEL_MANUAL && !PARALLEL_SERIAL) {
    int status = HostTableGroupWaitJobsGetIO (group, __FILE__, __LINE__, VERBOSE);
    if (!status) {
      fprintf (stderr, "error running one of the remote clients\n");
      return status;
    }
  }

  return TRUE;
}      
