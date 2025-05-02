# include "addstar.h"

// load all of the headers, jump in file to skip data segments
Header **LoadHeaders (FILE *f, int *mode, int *Nheaders) {

  off_t Nskip;
  int i, status, NHEADERS;
  Header **headers;

  /* we need to examine the extensions to determine the headers and the data */
  NHEADERS = 10;
  ALLOCATE (headers, Header *, NHEADERS);

  // load all headers into memory
  for (i = 0;; i++) {
    ALLOCATE (headers[i], Header, 1);
    status = gfits_fread_header (f, headers[i]);
    if (!status) { 
      gfits_free_header (headers[i]);
      FREE (headers[i]);
      *Nheaders = i;
      return (headers);
    }

    // check the mode for this file
    if (i == 0) {
      *mode = GetFileMode (headers[0]);
      // CMP mode has no binary table, just image header and text blocks
      if ((*mode == SIMPLE_CMP) || (*mode == MOSAIC_CMP)) {
	*Nheaders = i;
	return (headers);
      }
    }

    // advance to the next header
    Nskip = gfits_data_size (headers[i]);
    fseeko (f, Nskip, SEEK_CUR); 
    if (i == NHEADERS - 1) {
      NHEADERS += 10;
      REALLOCATE (headers, Header *, NHEADERS);
    }
  }
}

