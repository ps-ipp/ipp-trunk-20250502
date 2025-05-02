# include <ohana.h>
# include <gfitsio.h>

/*********************** fits find (and read) named header, leave file pointer and end of header *******/
int gfits_find_Xheader (FILE *f, Header *header, char *extname) {

  char tname[80];

  // start at the very beginning (no other info)
  fseeko (f, 0, SEEK_SET);

  // read the PHU header
  if (!gfits_load_header (f, header)) return FALSE;

  // skip to next header 
  off_t Nbytes = gfits_data_size (header);
  fseeko (f, Nbytes, SEEK_CUR);
  gfits_free_header (header);

  while (TRUE) {
    // load data for this header 
    if (!gfits_load_header (f, header)) return FALSE;

    // grab the EXTNAME field
    bzero (tname, 80);
    if (!gfits_scan (header, "EXTNAME", "%s", 1, tname)) goto next_header;

    // check if this is the correct extension or not 
    if (strcmp (tname, extname)) goto next_header;
    return TRUE;

  next_header:
    /* skip to next header */
    Nbytes = gfits_data_size (header);
    fseeko (f, Nbytes, SEEK_CUR);
    gfits_free_header (header);
  }
  myAbort("impossible");
}	
