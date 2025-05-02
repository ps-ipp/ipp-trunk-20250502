# include "imregister.h"
# include "imreg.h"
static char *version = "imsearch $Revision: 3.7 $";

int main (int argc, char **argv) {
 
  off_t Nmatch, Nimage, *match;
  RegImage *image;
  FITS_DB db;

  get_version (argc, argv, version);
  args (argc, argv);

  gfits_db_init (&db);
  db.lockstate = (output.modify || output.delete) ? LCK_HARD : LCK_SOFT;
  db.timeout   = 300.0;

  if (!gfits_db_lock (&db, ImageDB)) {
    fprintf (stderr, "ERROR: failure to lock db\n");
    gfits_db_close (&db);
    exit (1);
  }
  if (!gfits_db_load (&db)) {
    fprintf (stderr, "ERROR: failure to load db\n");
    gfits_db_close (&db);
    exit (1);
  }

  if (!output.modify && !output.delete) gfits_db_close (&db);

  image = gfits_table_get_RegImage (&db.ftable, &Nimage, &db.scaledValue, &db.nativeOrder);
  if (!image) {
    fprintf (stderr, "ERROR: failed to read images\n");
    exit (2);
  }
  
  match = match_criteria (image, Nimage, &Nmatch);
  match = unique_entries (image, Nimage, match, &Nmatch);

  if (output.modify) ModifySubset (&db, image, Nimage, match, Nmatch);
  if (output.delete) DeleteSubset (&db, image, Nimage, match, Nmatch);

  OutputSubset (image, Nimage, match, Nmatch);
  exit (0);
}

/* 

FITS table version:

   load_db - open, read in database, store as RegImage structure
   match   - return index 'match' to matched images
   modify  - change value of selected images
             write out subset of rows
   delete  - remove selected images from image structure
             write out entire table
   output  - write out subset in various formats

   get_images returns pointer to complete image structure


SQL version:

   load_db - set up connection
   match   - convert criteria to SQL where clause and do:
             'select from images [where clause]'
             images structure is filled with result from query
   modify  - change value of subset selection (identical)
             update selected rows
   delete  - user where clause and do 
             'delete from images where clause'
   output  - (identical)

   get_images returns pointer to image subset from query
   match[i] = i

*/
