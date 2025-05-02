# include "fakeastro.h"

int fakeastro_galaxy () {

  int n, i;

  SkyTable *sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  SkyList *skylistInput = SkyListByPatch (sky, -1, &UserPatch);

  // load the list of hosts
  HostTable *hosts = NULL;
  if (PARALLEL) {
    hosts = HostTableLoad (CATDIR, sky->hosts);
    if (!hosts) {
      fprintf (stderr, "ERROR: failure reading Host Table %s for database %s\n", sky->hosts, CATDIR);
      exit (1);
    }    

    // ensure that the paths are absolute path names
    for (i = 0; i < hosts->Nhosts; i++) {
      char *tmppath = abspath (hosts->hosts[i].pathname, DVO_MAX_PATH);
      free (hosts->hosts[i].pathname);
      hosts->hosts[i].pathname = tmppath;
    }

    // set up the array of active hosts
    init_remote_hosts ();
  }

  for (n = 0; n < FAKEASTRO_NLOOP; n++) {

    INITTIME;

    int Nstars = FAKEASTRO_NSTARS;
    FakeAstro_Stars *stars = make_fakestars (Nstars);

    // only generate the QSOs 1x:
    if (n == 0) {
      FakeAstro_Stars *qso_icrf = make_fakeqsos (FAKEASTRO_NQSO_ICRF, TRUE); // quasars with known positions
      REALLOCATE (stars, FakeAstro_Stars, Nstars + FAKEASTRO_NQSO_ICRF);
      for (i = 0; i < FAKEASTRO_NQSO_ICRF; i++) {
	stars[Nstars+i] = qso_icrf[i];
      }
      Nstars += FAKEASTRO_NQSO_ICRF;
      FakeAstro_Stars *qso_zero = make_fakeqsos (FAKEASTRO_NQSO_ZERO, FALSE); // quasars with unknown positions, but zero velocity
      REALLOCATE (stars, FakeAstro_Stars, Nstars + FAKEASTRO_NQSO_ZERO);
      for (i = 0; i < FAKEASTRO_NQSO_ZERO; i++) {
	stars[Nstars+i] = qso_zero[i];
      }
      Nstars += FAKEASTRO_NQSO_ZERO;
    }

    MARKTIME ("generate %d fake stars in %f sec\n", Nstars, dtime);

    // sort the stars by RA
    sortStars (stars, Nstars);

    MARKTIME ("sorted %d fake stars: %f sec\n", Nstars, dtime);

    // scan through the stars, loading the containing catalogs
    // skip through table for unsaved stars
    for (i = 0; i < Nstars; i++) {
      if (stars[i].flag) continue;

      // identify the relevant catalog
      SkyList *skylist = SkyRegionByPoint_List (skylistInput, -1, stars[i].R, stars[i].D);
      if (skylist[0].Nregions == 0) {
	SkyListFree (skylist);
	continue;
      }
      SkyRegion *region = skylist[0].regions[0];

      // select stars matching this region
      int Nsubset;
      FakeAstro_Stars *subset = make_subset (stars, Nstars, i, region, &Nsubset);
      // MARKTIME ("subset %d fake stars: %f sec\n", Nsubset, dtime);

      // In parallel mode, write out the subset to a disk file.  Block until a remote host
      // is available.  In serial mode, just match against the appropriate region and save
      save_fakestars (subset, Nsubset, hosts, region, skylist[0].filename[0]);
      // MARKTIME ("saved %d fake stars: %f sec\n", Nsubset, dtime);

      free (subset);
      SkyListFree (skylist);
    }
    free (stars);

    MARKTIME ("saved %d fake stars: %f sec\n", Nstars, dtime);

    // wait for last remote clients to finish
    harvest_all ();
  }

  return TRUE;
}

  /* outline

   * generate N stars (random draw from galaxy mode)
   * assign them to catalogs and distribute

   */

