# include "data.h"

/* there is some confusion in this function with the several possible options */
int rdseg (int argc, char **argv) {
  
  int x, y, nx, ny, status, blank;
  char region[512], *filename;
  FILE *f;
  Buffer *buf;

  if (argc != 7) {
    gprint (GP_ERR, "USAGE: rdseg <buffer> <filename> x y nx ny\n");
    return (FALSE);
  }
  x = atoi (argv[3]);
  y = atoi (argv[4]);
  nx = atoi (argv[5]);
  ny = atoi (argv[6]);

  /* test if file exists */
  f = fopen (argv[2], "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "file %s not found\n", argv[2]);
    return (FALSE);
  }
  fclose (f);

  /* find matrix, free old data */
  if ((buf = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  gfits_free_matrix (&buf[0].matrix);
  gfits_free_header (&buf[0].header);

  /* save file name */
  filename = filebasename (argv[2]);
  strcpy (buf[0].file, filename);
  free (filename);

  status = gfits_read_header (argv[2], &buf[0].header);
  sprintf (region, "%d %d %d %d 0 1", x, nx + x, y, ny + y);
  status = gfits_read_matrix_segment (argv[2], &buf[0].matrix, region);
  gfits_modify (&buf[0].header, "NAXIS1", "%d", 1, nx);
  gfits_modify (&buf[0].header, "NAXIS2", "%d", 1, ny);
  buf[0].header.Naxis[0] = nx;
  buf[0].header.Naxis[1] = ny;
  buf[0].matrix.Naxis[0] = nx;
  buf[0].matrix.Naxis[1] = ny;

  if (!status) {
    gprint (GP_ERR, "problem reading file, buffer not opened\n");
    DeleteBuffer (buf);
    return (FALSE);
  }
  if (buf[0].header.Naxes == 1) {
    /* we need to return an array, so make Naxis[1] = 1 */
    buf[0].header.Naxes = 2;
    buf[0].header.Naxis[1] = 1;
    buf[0].matrix.Naxis[1] = 1;
  }    

  buf[0].bitpix = buf[0].header.bitpix;    /* store the original values */
  buf[0].bscale = buf[0].header.bscale;    /* store the original values */
  buf[0].bzero  = buf[0].header.bzero;     /* store the original values */
  buf[0].unsign = buf[0].header.unsign;
  gprint (GP_LOG, "read "OFF_T_FMT" bytes from %s into buffer %s\n",  buf[0].header.datasize + buf[0].matrix.datasize, argv[2], argv[1]);

  gfits_scan (&buf[0].header, "BLANK", "%d", 1, &blank);

  /** now - convert the matrix values to floats for internal use **/
  gfits_convert_format (&buf[0].header, &buf[0].matrix, -32, 1.0, 0.0, blank, gfits_get_unsign_mode());
  
  return (TRUE);

}
