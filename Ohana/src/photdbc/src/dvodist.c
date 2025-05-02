# include "dvodist.h"

// dvodist (-out | -in) (catdir)
// dvodist -out : copy catalog tables to distributed locations in host table (from catdir)
// dvodist -in  : copy catalog tables to catdir (from distributed locations in host table)

int main (int argc, char **argv) {

  SkyTable *sky;
  SkyList *skylist;

  /* get configuration info, args, lockfile */
  SetSignals ();
  initialize (argc, argv);
  char *catdir = strcreate (argv[1]);

  // LockDatabase (catdir);
  
  // load the current SkyTable. If SkyTable does not exist, we must fail
  sky = SkyTableLoadOptimal (catdir, NULL, NULL, TRUE, SKY_DEPTH_HST, VERBOSE);
  SkyTableSetFilenames (sky, catdir, "cpt");
  skylist = SkyListByPatch (sky, -1, &UserPatch);

  HostTable *hosts = HostTableLoad (catdir, sky->hosts);
  if (!hosts) {
    Shutdown ("failed to load Host Table %s for %s\n", catdir, sky->hosts);
  }

  // XXX I would like to do something so a big dvodist operation is easy to recover
  // XXX BUT, I cannot save the table here without some caution (byteswap to and fro)

  switch (MODE) {
    case MODE_OUT:
      CheckHostsAndPaths(hosts);
      AssignSkyToHost (skylist, hosts);
      CopyToHostLocation (catdir, skylist, hosts);
      break;
    case MODE_IN:
      CheckHostsAndPaths(hosts);
      CopyFromHostLocation (catdir, skylist, hosts);
      break;
    case MODE_FIX:
      FixSkyForHost (catdir, skylist, hosts);
      break;
    case MODE_OUT_BACKUP:
      CopyBackupToHost (catdir, skylist, hosts);
      break;
    case MODE_USE_BACKUP:
      UseBackupForHost (catdir, skylist, hosts);
      break;
    default:
      fprintf (stderr, "impossible!");
      abort();
  }

  char *skyfile = SkyTableFilename (catdir);
  SkyTableSave (sky, skyfile);

  exit (0);
}
