# include "relphot.h"

// relphot_client is run on a remote host and is responsible for updating the catalogs
// owned by that host.  

// there are two modes:

// relphot_client -load : extract the bright catalog subset from the client's tables and
//                        save to a local FITS table for relphot to load and used

// (relphot loads the bright catalog subsets from all clients, then uses this to determine
// the per-image zero points (and potentially the flat-field corrections)

// relphot_client -update : load image table containing the update zero point information
//                          and apply this to the client's tables

int main (int argc, char **argv) {

  // get configuration info, args, lockfile (set CATDIR, HOST_ID, HOSTDIR, etc) 
  SetSignals ();
  initialize_client (argc, argv);
  client_logger_init (HOSTDIR);

  // load the current sky table (layout of all SkyRegions) if needed
  SkyTable *sky = SkyTableLoadOptimal (CATDIR, NULL, NULL, TRUE, -1, VERBOSE);
  if (!sky) {
    fprintf (stderr, "ERROR running loading sky table from %s\n", CATDIR);
    exit (2);
  }
  SkyTableSetFilenames (sky, CATDIR, "cpt");
      
  SkyList *skylist = SkyListByPatch (sky, -1, &UserPatch);
  if (!skylist) {
    fprintf (stderr, "ERROR setting up skylist for %s\n", CATDIR);
    exit (2);
  }
  client_logger_message ("loaded sky table and defined patch\n");
  
  switch (MODE) {
    case MODE_LOAD: {
      int Ncatalog;
      Catalog *catalog = load_catalogs (skylist, &Ncatalog, HOST_ID, HOSTDIR, NULL);
      if (!catalog) {
	  fprintf (stderr, "ERROR loading catalogs from %s\n", CATDIR);
	  exit (2);
      }
      client_logger_message ("loaded catalogs\n");

      BrightCatalog *bcatalog = BrightCatalogMerge (catalog, Ncatalog);
      client_logger_message ("generated subset data\n");

      if (!BrightCatalogSave (BCATALOG, bcatalog)) {
	  fprintf (stderr, "ERROR saving bright catalog from %s\n", CATDIR);
	  exit (2);
      }

      // free memory associated with the catalogs
      for (int i = 0; i < Ncatalog; i++) {
	free_tiny_values (&catalog[i]);
	dvo_catalog_free (&catalog[i]);
      }
      free (catalog);
      BrightCatalogFree (bcatalog);
      client_logger_message ("generated subset table\n");
      relphot_client_free (sky, skylist);
      break;
    }
      
    case MODE_UPDATE: {
      // load the image subset table from the specified location
      off_t Nimage;
      ImageSubset *image = ImageSubsetLoad (IMAGES, &Nimage);
      if (!image) {
	  fprintf (stderr, "ERROR loading image subset %s\n", CATDIR);
	  exit (2);
      }
      client_logger_message ("loaded image subset data\n");
      
      // save the available image information in the static array in ImageOps.c
      initImagesSubset (image, NULL, Nimage);

      // load grid corrections here
      GridCorrectionLoad (GRID_MEANFILE);

      reload_catalogs (skylist, HOST_ID, HOSTDIR);
      freeImages ((char *)image);
      free (image);
      freeGridBins ();
      client_logger_message ("updated catalogs\n");
      relphot_client_free (sky, skylist);
      break;
    }

/* deprecated (absorbed into MODE_UPDATE)
    case MODE_UPDATE_OBJECTS: {
      // take the current set of detections and set the mean magnitudes
      relphot_objects (skylist, HOST_ID, HOSTDIR);
      relphot_client_free (sky, skylist);
      client_logger_message ("updated objects\n");
      break;
    }
*/

    case MODE_SYNTH_PHOT:
      // set the mean magnitudes ONLY for SYNPHOT objects
      relphot_synthphot (skylist, HOST_ID, HOSTDIR);
      client_logger_message ("set synth photometry\n");
      break;

    case MODE_UPDATE_OBJECTS:
    default:
      fprintf (stderr, "impossible!");
      abort();
  }
  client_logger_message ("done with relphot_client\n");
  exit (0);
}

