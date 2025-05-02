# include "skycells.h"
# include <libgen.h>

int main (int argc, char **argv) {

  int status;
  FITS_DB db;
  int simple = 0;
  char *tess_id = NULL;

  SetSignals ();
  // prevent creation of CATDIR if it doesn't exist
  READONLY = 1;
  ConfigInit_skycells (&argc, argv);
  
  int N;
  if ((N = get_argument (argc, argv, "-tess_id"))) {
    remove_argument (N, &argc, argv);
    tess_id = argv[N];
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-simple"))) {
    remove_argument (N, &argc, argv);
    simple = 1;
  }

  if (argc > 2) {
    fprintf(stderr, "usage: dumpskycells <filename> -D CATDIR <catdir>\n");
    exit(1);
  }
  char *filename = argc == 2 ? argv[1] : NULL;
  FILE *mdcfile;
  int closefile = 1;
  if (!filename || !strcmp(filename, "-")) {
    mdcfile = stdout;
    closefile = 0;
  } else {
    mdcfile = fopen(filename, "w");
  }
  if (!mdcfile) {
    fprintf(stderr, "failed to open %s\n", argv[1]);
    exit(1);
  }


  /*** open the image table ***/
  /* setup image table format and lock */
  gfits_db_init (&db);
  db.mode   = dvo_catalog_catmode (CATMODE);
  db.format = dvo_catalog_catformat (CATFORMAT);
  status    = dvo_image_lock (&db, ImageCat, 3600.0, LCK_SOFT);  // shorter timeout?
  if (!status) Shutdown ("ERROR: failure to lock image catalog %s", db.filename);

  /* load or create the image table */
  if (db.dbstate == LCK_EMPTY) {
    fprintf(stderr, "failed to find %s\n", ImageCat);
    exit(1);
  } else {
    if (!dvo_image_load (&db, VERBOSE, FALSE)) {
      Shutdown ("can't read image catalog %s", db.filename);
    }
  }

  skycells_to_mdc(mdcfile, simple, tess_id, &db);

  if (closefile) {
    fclose(mdcfile);
  }

  dvo_image_unlock (&db);
  exit (0);
}
