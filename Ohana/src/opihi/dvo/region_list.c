# include "dvoshell.h"

/* XXX note : for RegionName or RegionList, we need to free the skylist
   elements, but not in the case of radius selection - this implies 
   information carried back up */

static SkyTable *sky = NULL;

int SetCATDIR (char *path, int verbose) {

  char *CATDIR  = NULL;
  char *newpath;
  char catdir_config[256];
  char gscfile[256];
  char skyfile[256];
  int  skydepth;

  /* find CATDIR in config system */
  if (path == NULL) {
    if (VarConfig ("CATDIR", "%s", catdir_config) == NULL) return (FALSE);
    newpath = catdir_config;
  } else {
    newpath = path;
  }

  CATDIR = newpath;
  // save the new value in libdvo
  dvo_set_catdir(CATDIR);

  if (VarConfig ("GSCFILE",  "%s", gscfile) == NULL) gscfile[0] = 0;
  if (VarConfig ("SKYFILE",  "%s", skyfile) == NULL) skyfile[0] = 0;
  if (VarConfig ("SKYDEPTH", "%d", &skydepth) == NULL) skydepth = 2;

  if (verbose) {
      gprint (GP_ERR, "CATDIR %s\n", CATDIR);
      gprint (GP_ERR, "GSCFILE %s\n", gscfile);
      gprint (GP_ERR, "SKYFILE %s\n", skyfile);
      gprint (GP_ERR, "SKYDEPTH %d\n", skydepth);
  }

  /* load the SkyTable at this point */
  /* set the image path as well */

  if (sky != NULL) SkyTableFree (sky);
  sky = SkyTableLoadOptimal (CATDIR, skyfile, gscfile, FALSE, skydepth, verbose);
  if (sky == NULL) return FALSE;

  SkyTableSetFilenames (sky, CATDIR, "cpt");

  return (TRUE);
}

char *GetCATDIR () {
  char *CATDIR = dvo_get_catdir();

  if (CATDIR == NULL) {
    if (SetCATDIR (NULL, FALSE)) {
        CATDIR = dvo_get_catdir();
    }
  }
  return (CATDIR);
}

SkyTable *GetSkyTable () {
  if (sky == NULL) {
    SetCATDIR (NULL, FALSE);
  }
  return (sky);
}

void FreeSkyRegionSelection (SkyRegionSelection *selection) {

  if (selection == NULL) return;
  if (selection[0].name != NULL) free (selection[0].name);
  if (selection[0].list != NULL) free (selection[0].list);
  free (selection);
}

SkyRegionSelection *SetRegionSelection (int *argc, char **argv) {
  
  int N;
  SkyRegionSelection *selection;

  ALLOCATE (selection, SkyRegionSelection, 1);
  selection[0].name = NULL;
  selection[0].list = NULL;
  selection[0].useDisplay = FALSE;
  selection[0].useSkyregion = FALSE;

  /* check for Region selection (named dvo catalog file) */
  if ((N = get_argument (*argc, argv, "-cpt"))) {
    remove_argument (N, argc, argv);
    selection[0].name = strcreate (argv[N]);
    remove_argument (N, argc, argv);
    return selection;
  }    

  /* check for Region list (file containing dvo catalog file list)*/
  if ((N = get_argument (*argc, argv, "-cptlist"))) {
    remove_argument (N, argc, argv);
    selection[0].list = strcreate (argv[N]);
    remove_argument (N, argc, argv);
    return selection;
  } 

  /* check for Region selection from display */
  if ((N = get_argument (*argc, argv, "-region"))) {
    remove_argument (N, argc, argv);
    selection[0].useDisplay = TRUE;
    return selection;
  }    

  /* check for Region selection from display */
  if ((N = get_argument (*argc, argv, "-skyregion"))) {
    if (N + 4 >= *argc) {
      gprint (GP_ERR, "USAGE: -skyregion (RA) (RA) (DEC) (DEC)\n");
      FreeSkyRegionSelection (selection);
      return NULL;
    }
    remove_argument (N, argc, argv);
    selection[0].useSkyregion = TRUE;
    set_skyregion (atof(argv[N]), atof(argv[N+1]), atof(argv[N+2]), atof(argv[N+3]));
    remove_argument (N, argc, argv);
    remove_argument (N, argc, argv);
    remove_argument (N, argc, argv);
    remove_argument (N, argc, argv);
    return selection;
  }    

  /* default to pre-defined sky region */
  selection[0].useSkyregion = TRUE;
  return selection;
}

/* given possible options (by name, by list, by graph region), select SkyRegions */
int SetSkyRegions (SkyRegionSelection *selection) {

  if (selection->name != NULL) {
    gprint (GP_ERR, "name-based selection not yet implemented (in parallel mode)\n");
    return FALSE;
  } 

  if (selection->list != NULL) {
    gprint (GP_ERR, "list-based selection not yet implemented (in parallel mode)\n");
    return FALSE;
  } 

  if (selection->useDisplay) {
    double Rmin, Rmax, Dmin, Dmax, Radius;
    Graphdata graphsky;

    if (!GetGraphdata (&graphsky, NULL, NULL)) {
      gprint (GP_ERR, "region display not available\n");
      return FALSE;
    }
    Radius = MAX (fabs(graphsky.xmax), fabs(graphsky.ymax));
    Dmin = graphsky.coords.crval2 - Radius;
    Dmax = graphsky.coords.crval2 + Radius;
    
    if ((Dmin <= -89) || (Dmax >= 89)) {
      Rmin = 0;
      Rmax = 360;
    } else {
      double Rmod = MAX (Radius / (cos(Dmin*RAD_DEG)), Radius / (cos(Dmax*RAD_DEG)));
      Rmin = graphsky.coords.crval1 - Rmod;
      Rmax = graphsky.coords.crval1 + Rmod;
    }

    set_skyregion (Rmin, Rmax, Dmin, Dmax);
    return TRUE;
  }
  if (selection->useSkyregion) {
    return TRUE;
  }
  return FALSE;
}

/* given possible options (by name, by list, by graph region), select SkyRegions */
SkyList *SelectRegions (SkyRegionSelection *selection) {

  SkyList *skylist;

  // the list of regions comes directly from a file
  if (selection->list != NULL) {
    skylist = SkyListLoadFile (selection->list);
    return (skylist);
  }

  // all other options require sky to be set
  if (!sky) {
    gprint (GP_ERR, "CATDIR not set\n");
    return NULL;
  }

  /* determine region-file names */
  if (selection->name != NULL) {
    skylist = SkyListByName (sky, selection->name);
    return (skylist);
  } 

  if (selection->useDisplay) {
    double Radius;
    Graphdata graphsky;

    if (!GetGraphdata (&graphsky, NULL, NULL)) {
      gprint (GP_ERR, "region display not available\n");
      return (NULL);
    }

    Radius = MAX (fabs(graphsky.xmax), fabs(graphsky.ymax));
    skylist = SkyListByRadius (sky, -1, graphsky.coords.crval1, graphsky.coords.crval2, Radius);
    return (skylist);
  }

  if (selection->useSkyregion) {
    double Rmin, Rmax, Dmin, Dmax;

    get_skyregion (&Rmin, &Rmax, &Dmin, &Dmax);
    skylist = SkyListByBounds (sky, -1, Rmin, Rmax, Dmin, Dmax);
    return (skylist);
  }    

  return NULL;
}

/* returns a list of region files names from file */
SkyList *SkyListLoadFile (char *filename) {
  
  FILE *f;
  int NREGIONS, Nregions;
  SkyList *skylist;

  ALLOCATE (skylist, SkyList, 1);

  f = fopen (filename, "r");
  if (f == NULL) {
    gprint (GP_ERR, "ERROR: can't find region list file %s\n", filename);
    skylist[0].Nregions = 0;
    skylist[0].regions = NULL;
    return (skylist);
  }
  
  Nregions = 0;
  NREGIONS = 50;
  ALLOCATE (skylist[0].regions, SkyRegion *, NREGIONS);
  ALLOCATE (skylist[0].filename, char *, NREGIONS);
  skylist[0].ownElements = TRUE; // free these elements when freeing the list

  char *CATDIR = dvo_get_catdir();

  while (fscanf (f, "%s", filename) != EOF) {
    ALLOCATE (skylist[0].regions[Nregions], SkyRegion, 1);
    strcpy (skylist[0].regions[Nregions][0].name, filename);
    sprintf (filename, "%s/%s.cpt", CATDIR, skylist[0].regions[Nregions][0].name);
    skylist[0].filename[Nregions] = strcreate (filename);
    Nregions ++;
    CHECK_REALLOCATE (skylist[0].regions, SkyRegion *, NREGIONS, Nregions, 50);
  }
  skylist[0].Nregions = Nregions;
  return (skylist);
}

