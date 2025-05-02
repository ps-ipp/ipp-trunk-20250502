# include "relastro.h"
// # include <sys/types.h>
// # include <unistd.h>

// relastro_client is run on a remote host and is responsible for updating the catalogs
// owned by that host.  

// there are four modes:

// relastro_client -load-objects : extract the bright catalog subset from the client's
//                                 tables and save to a local FITS table for relastro to
//                                 load and used

// relastro_client -update-offsets : load image table containing the updated astrometry
//                                   information and determine per-object astrometry

// relastro_client -update-objects : load image table containing the updated astrometry
//                                   information and determine per-object astrometry

// relastro loads the bright catalog subsets from all clients, then determines the
// astrometry for chips/mosaics, while iteratively improving the per-object mean positions

int main (int argc, char **argv) {

  my_memdump("start of program");

  // get configuration info, args, lockfile (set CATDIR, HOST_ID, HOSTDIR, etc) 
  SetSignals ();
  initialize_client (argc, argv);

  SkyTable *sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  SkyTableSetFilenames (sky, CATDIR, "cpt");
  SkyList *skylist = SkyListByPatch (sky, -1, &UserPatch);

  my_memdump("done config");

  switch (RELASTRO_OP) {

    case OP_LOAD_OBJECTS: {
      // USAGE: relastro_client -load-objects
      int Ncatalog;
      Catalog *catalog = load_catalogs (skylist, &Ncatalog, TRUE, HOST_ID, HOSTDIR, NULL);
      if (!catalog) {
	fprintf (stderr, "ERROR loading catalogs from %s\n", CATDIR);
	exit (2);
      }
      my_memdump("loaded catalogs");

      BrightCatalog *bcatalog = BrightCatalogMerge (catalog, Ncatalog);
      my_memdump("merged bcatalog");

      if (!BrightCatalogSave (BCATALOG, bcatalog)) {
	fprintf (stderr, "ERROR saving bright catalog from %s\n", CATDIR);
	exit (2);
      }

      int i;
      for (i = 0; i < Ncatalog; i++) {
	dvo_catalog_free (&catalog[i]);
      }
      FREE (catalog);
      BrightCatalogFree(bcatalog);

      relastro_client_free (sky, skylist);
      break;
    }
      
    case OP_UPDATE_OBJECTS: {
      // USAGE: relastro_client -update-objects
      relastro_objects (skylist, HOST_ID, HOSTDIR);
      relastro_client_free (sky, skylist);
      break;
    }

      // XXX loading the images is fairly costly -- see if we can do an image subset?
    case OP_UPDATE_OFFSETS: {
      FITS_DB db;
      
      set_db (&db);
      gfits_db_init (&db);

      /* lock and load the image db table */
      int status = dvo_image_lock (&db, ImageCat, 60.0, LCK_SOFT);
      if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
      if (db.dbstate == LCK_EMPTY) Shutdown ("ERROR: No images in catalog %s (1)", db.filename);
      if (!dvo_image_load (&db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db.filename);
      // the raw FITS data is freed by dvo_image_load, leaving only the Image structure data

      /* load regions and images based on specified sky patch (default depth) */
      load_images (&db, skylist, FALSE, USE_ALL_IMAGES);

      if (REPAIR_STACKS) MakeStackIndex ();

      UpdateObjectOffsets (skylist, HOST_ID, HOSTDIR);
      
      if (REPAIR_STACKS) FreeStackGroups ();

      freeImages (db.ftable.buffer);
      gfits_db_free (&db);
      freeMosaics ();

      relastro_client_free (sky, skylist);
      break;
    }

    case OP_REPAIR_OBJECT_ID: {

      // iterate over catalogs to make detection coordinates consistant
      RepairObjectIDs (skylist, HOST_ID, HOSTDIR);

      relastro_free (sky, skylist);
      exit (0);
    }

      // XXX loading the images is fairly costly -- see if we can do an image subset?
    case OP_REPAIR_STACKS: {
      FITS_DB db;
      
      set_db (&db);
      gfits_db_init (&db);

      /* lock and load the image db table */
      int status = dvo_image_lock (&db, ImageCat, 60.0, LCK_SOFT);
      if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
      if (db.dbstate == LCK_EMPTY) Shutdown ("ERROR: No images in catalog %s (1)", db.filename);
      if (!dvo_image_load (&db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db.filename);
      // the raw FITS data is freed by dvo_image_load, leaving only the Image structure data

      /* load regions and images based on specified sky patch (default depth) */
      load_images (&db, skylist, FALSE, USE_ALL_IMAGES);

      RepairStacks (skylist, HOST_ID, HOSTDIR);
      
      freeImages (db.ftable.buffer);
      gfits_db_free (&db);
      freeMosaics ();

      relastro_client_free (sky, skylist);
      break;
    }

      // XXX loading the images is fairly costly -- see if we can do an image subset?
    case OP_REPAIR_WARPS: {
      FITS_DB db;
      
      set_db (&db);
      gfits_db_init (&db);

      /* lock and load the image db table */
      int status = dvo_image_lock (&db, ImageCat, 60.0, LCK_SOFT);
      if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);
      if (db.dbstate == LCK_EMPTY) Shutdown ("ERROR: No images in catalog %s (1)", db.filename);
      if (!dvo_image_load (&db, VERBOSE, FALSE)) Shutdown ("can't read image catalog %s", db.filename);
      // the raw FITS data is freed by dvo_image_load, leaving only the Image structure data

      /* load regions and images based on specified sky patch (default depth) */
      load_images (&db, skylist, FALSE, USE_ALL_IMAGES);

      RepairWarps (skylist, HOST_ID, HOSTDIR);
      
      freeImages (db.ftable.buffer);
      gfits_db_free (&db);
      freeMosaics ();

      relastro_client_free (sky, skylist);
      break;
    }

    case OP_HIGH_SPEED: {
      // USAGE: relastro_client -high-speed
      high_speed_catalogs (sky, skylist, HOST_ID, HOSTDIR);
      break;
    }

    case OP_HPM: {
      // USAGE: relastro_client -high-speed
      hpm_catalogs (sky, skylist, HOST_ID, HOSTDIR);
      break;
    }

    default:
      fprintf (stderr, "impossible!");
      abort();
  }

  exit (0);
}
