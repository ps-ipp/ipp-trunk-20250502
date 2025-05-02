# include "getstar.h"

int main (int argc, char **argv) {

  int i;
  SkyTable *sky;
  SkyList *skylist;
  Catalog catalog;
  Catalog output;
  FITS_DB db;
  int code, Nsec, needMeas;

  args (argc, argv);
  set_db (&db);

  sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, FALSE, SKY_DEPTH, VERBOSE);
  if (!sky) exit (1);
    
  SkyTableSetFilenames (sky, CATDIR, "cpt");

  code = photcode[0].code;
  Nsec = GetPhotcodeNsec (code);
  needMeas = (Nsec == -1);

  // create an output catalog with the desired name and format options
  dvo_catalog_init (&output, TRUE);

  dvo_catalog_init (&output, TRUE);
  output.filename  = OUTPUT;
  output.catformat = dvo_catalog_catformat (CATFORMAT);  // set the default catformat from config data
  output.catmode   = dvo_catalog_catmode (CATMODE);      // set the default catmode from config data
  output.catflags  = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
  output.Nsecfilt  = GetPhotcodeNsecfilt ();

  // this has to be here because the 'open' inits the catalog (perhaps not ideal)
  if (!strcmp (OUTFORMAT, "CATALOG")) {
      int length;
      char *filename, *path, *root;
      // we need to unlink the files, otherwise dvo_catalog_open ("w") will error if they exist
      unlink (output.filename);
      if (output.catmode == DVO_MODE_SPLIT) {
	  path = pathname (output.filename);
	  root = filerootname (output.filename);
	  length = strlen(path) + strlen(root) + 6;
	  ALLOCATE (filename, char, length);
	  sprintf (filename, "%s/%s.cpm", path, root);
	  unlink (filename);
	  sprintf (filename, "%s/%s.cpn", path, root);
	  unlink (filename);
	  sprintf (filename, "%s/%s.cps", path, root);
	  unlink (filename);
      }
      dvo_catalog_open   (&output, NULL, VERBOSE, "w");
  }

  switch (MODE) {

    case BY_REGION:
    case BY_RADIUS:

      /* load corresponding sky regions */
      skylist = SkyListByPatch (sky, -1, &REGION);
      for (i = 0; i < skylist[0].Nregions; i++) {
	// set the parameters which guide catalog open/load/create
	dvo_catalog_init (&catalog, TRUE);
	catalog.filename = skylist[0].filename[i];
	catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
	catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;
	catalog.catflags |= needMeas ? DVO_LOAD_MEASURE : DVO_SKIP_MEASURE;

	// an error exit status here is a significant error
	if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "r")) {
	  fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
	  exit (2);
	}
	if (!catalog.Naverage_disk) {
	  dvo_catalog_unlock (&catalog);
	  dvo_catalog_free (&catalog);
	  continue;
	}
	dvo_catalog_unlock (&catalog);

	/* skip empty catalogs */
	select_by_region (&output, &catalog, &REGION, 0, 0);
	dvo_catalog_free (&catalog);
      }
      break;

    case BY_IMLIST:
      /* load image list */
    case BY_IMAGE:

      # if (0)
      /* load corresponding sky regions */
      skylist = SkyListByImage (sky, -1, &image, &Nimage);
      for (i = 0; i < skylist[0].Nregions; i++) {
	// set the parameters which guide catalog open/load/create
	dvo_catalog_init (&catalog, TRUE);
	catalog.filename = skylist[0].filename[i];
	catalog.Nsecfilt  = GetPhotcodeNsecfilt ();
	catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_SECFILT;
	catalog.catflags |= needMeas ? DVO_LOAD_MEASURE : DVO_SKIP_MEASURE;

	// an error exit status here is a significant error
	if (!dvo_catalog_open (&catalog, skylist[0].regions[i], VERBOSE, "r")) {
	  fprintf (stderr, "ERROR: failure to open/create catalog file %s\n", catalog.filename);
	  exit (2);
	}
	/* skip empty catalogs */
	if (!catalog.Naverage_disk) continue;
	stars = select_by_image (&catalog, &image, 0, 0, stars, &Nstars);
      }
      # endif
      fprintf (stderr, "error: BY_IMAGE not implemented\n");
      exit (1);
      break;

    case BY_CATALOG:
      fprintf (stderr, "error: BY_CATALOG not implemented\n");
      exit (1);
      break;
      
    default:
      fprintf (stderr, "error: invalid options\n");
      exit (1);
  }

  if (!strcmp (OUTFORMAT, "CATALOG")) {
    write_catalog (&output);
  }
  if (!strcmp (OUTFORMAT, "PS1_DEV_0")) {
    write_getstar_PS1_DEV_0 (&output);
  }
  if (!strcmp (OUTFORMAT, "PS1_DEV_1")) {
    write_getstar_PS1_DEV_1 (&output);
  }
  if (!strcmp (OUTFORMAT, "PS1_DEV_2")) {
    write_getstar_PS1_DEV_2 (&output);
  }

  fprintf (stderr, "error: invalid output format %s\n", OUTFORMAT);
  exit (1);
}
