# include "imregister.h"
# include "imphot.h"

int rtext (FITS_DB *db) {

  off_t Nimage, size, nimage;
  struct stat filestatus;
  Image *image;

  /* check that file size makes sense */
  Nimage = 0;
  gfits_scan (&db[0].header, "NIMAGES", OFF_T_FMT, 1,  &Nimage);
  if (stat (db[0].filename, &filestatus) == -1) {
    if (VERBOSE) fprintf (stderr, "ERROR: failed to get status of image catalog\n");
    exit (1);
  }
  size = Nimage*sizeof(Image) + db[0].header.datasize;
  if (size != filestatus.st_size) {
    int Ndata;

    Ndata = (filestatus.st_size - db[0].header.datasize) / sizeof (Image);
    if (VERBOSE) fprintf (stderr, "ERROR: image catalog has inconsistent size\n");
    if (VERBOSE) fprintf (stderr, "header: "OFF_T_FMT", data: %d\n",  Nimage, Ndata);
    if (!FORCE_READ) exit (1);
    Nimage = Ndata;
  } 

  /* create a dummy set of table information */
  /* (original table has NAXIS = 2, change to 0) */
  gfits_modify (&db[0].header, "NAXIS", "%d", 1, 0);
  gfits_create_matrix (&db[0].header, &db[0].matrix);
  gfits_table_mkheader_Image (&db[0].theader);
  db[0].ftable.header = &db[0].theader;

  /* alloc, read images */
  ALLOCATE (image, Image, MAX (Nimage, 1));
  nimage = fread (image, sizeof(Image), Nimage, db[0].f);
  if (nimage != Nimage) {
    if (VERBOSE) fprintf (stderr, "ERROR: problem loading image catalog\n");
    exit (1);
  } 
  db[0].ftable.buffer = (char *) image;
  gfits_modify (&db[0].theader, "NAXIS2", OFF_T_FMT, 1,  Nimage);
  db[0].theader.Naxis[1] = Nimage;
  db[0].ftable.datasize = gfits_data_size (&db[0].theader);
  
  return (TRUE);
}

/* the old Image.dat files used a fake FITS header defining a finite data block
   this function reads in the 
*/


