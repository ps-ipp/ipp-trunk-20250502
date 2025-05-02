# include "relastro.h"

int main (int argc, char **argv) {

  /* get configuration info, args */
  SetSignals ();
  initialize (argc, argv);

  SkyTable *sky = NULL;
  SkyList *skylist = NULL;

  if ((RELASTRO_OP != OP_PARALLEL_IMAGES) && (RELASTRO_OP != OP_PARALLEL_REGIONS)) {
    sky = SkyTableLoadOptimal (CATDIR, SKY_TABLE, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
    SkyTableSetFilenames (sky, CATDIR, "cpt");
    skylist = SkyListByPatch (sky, -1, &UserPatch);
  }

  switch (RELASTRO_OP) {
    case OP_UPDATE_OBJECTS:
      /* the object analysis is a separate process iterating over catalogs */
      relastro_objects (skylist, 0, NULL);
      relastro_free (sky, skylist);
      exit (0);

    case OP_IMAGES:
      relastro_images (skylist);
      relastro_free (sky, skylist);
      exit (0);

    case OP_UPDATE_OFFSETS: {
      FITS_DB db;
      if (!PARALLEL) {
      
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
      }

      if (REPAIR_STACKS && !PARALLEL) MakeStackIndex ();

      // iterate over catalogs to make detection coordinates consistant
      UpdateObjectOffsets (skylist, 0, NULL);

      if (REPAIR_STACKS && !PARALLEL) FreeStackGroups ();

      if (!PARALLEL) {
	freeImages (db.ftable.buffer);
	gfits_db_free (&db);
	freeMosaics ();
      }	

      relastro_free (sky, skylist);
      exit (0);
    }

    case OP_PARALLEL_REGIONS:
      // run image updates in parallel across multiple remote machines
      relastro_parallel_regions ();
      exit (0);

    case OP_PARALLEL_IMAGES:
      // operation on the remote machines in the PARALLEL_REGION mode
      relastro_parallel_images ();
      exit (0);

    case OP_REPAIR_WARPS: {
      FITS_DB db;
      if (!PARALLEL) {
      
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
      }

      // iterate over catalogs to make detection coordinates consistant
      RepairWarps (skylist, 0, NULL);

      if (!PARALLEL) {
	freeImages (db.ftable.buffer);
	gfits_db_free (&db);
	freeMosaics ();
      }	

      relastro_free (sky, skylist);
      exit (0);
    }

    case OP_REPAIR_STACKS: {
      FITS_DB db;
      if (!PARALLEL) {
      
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
      }

      // iterate over catalogs to make detection coordinates consistant
      RepairStacks (skylist, 0, NULL);

      if (!PARALLEL) {
	freeImages (db.ftable.buffer);
	gfits_db_free (&db);
	freeMosaics ();
      }	

      relastro_free (sky, skylist);
      exit (0);
    }

    case OP_REPAIR_OBJECT_ID: {

      // iterate over catalogs to make detection coordinates consistant
      RepairObjectIDs (skylist, 0, NULL);

      relastro_free (sky, skylist);
      exit (0);
    }

    case OP_HIGH_SPEED:
      /* high-speed is a 2pt cross-correlation process for linking moving objects (high PM) */
      high_speed_catalogs (sky, skylist, 0, NULL);
      exit (0);

    case OP_HPM:
      hpm_catalogs (sky, skylist, 0, NULL);
      exit (0);

    case OP_MERGE_SOURCE:
      /* a special method to manually merge unlinked detections of sources togther (not parallel) */
      relastro_merge_source (sky);
      exit (0);

    default:
      fprintf (stderr, "impossible!\n");
      abort();
  }
  exit (1);
}
