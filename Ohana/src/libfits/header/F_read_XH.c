# include <ohana.h>
# include <gfitsio.h>

/*********************** fits read header ***********************************/
int gfits_read_Xheader (char *filename, Header *header, int N) {

  int status;
  FILE *f;

  f = fopen (filename, "r");
  if (f == NULL) {
    header[0].buffer = NULL;
    return (FALSE);
  }

  status = gfits_fread_Xheader (f, header, N);
  if (!status) {
    header[0].buffer = NULL;
    fclose (f);
    return (FALSE);
  }

  fclose (f);
  return (TRUE);

}

/*********************** fits read header ***********************************/
int gfits_fread_Xheader (FILE *f, Header *header, int N) {
  
  /* read header for extension number N */

  int j;
  off_t Nmatrix, Nskip;
  Header theader;
  
  /* set f to beginning of file */
  fseeko (f, 0, SEEK_SET);

  Nskip = 0;
  for (j = -1; j < N; j++) {
    /* load data for this header */
    if (!gfits_load_header (f, &theader)) {
      return (FALSE);
    }

    Nmatrix = gfits_data_size (&theader);

    /* skip to next header */
    fseeko (f, Nmatrix, SEEK_CUR);
    Nskip += (Nmatrix + theader.datasize);
    gfits_free_header (&theader);
  }
  
  if (!gfits_load_header (f, header)) {
    return (FALSE);
  }
  Nskip += header[0].datasize;
  return (Nskip);
}	

