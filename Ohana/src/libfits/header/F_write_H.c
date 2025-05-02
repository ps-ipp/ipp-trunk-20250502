# include <ohana.h>
# include <gfitsio.h>

/*********************** fits write header ***********************************/
int gfits_write_header (char *filename, Header *header) {

  FILE *f = fopen (filename, "w");
  if (!f) return (FALSE);

  int status = gfits_fwrite_header (f, header);
  fclose (f);

  return (status);
}	

/*********************** fits write header ***********************************/
int gfits_save_header (FILE *f, Header *header) {

  off_t Nbytes; 

# ifdef OHANA_MEMORY  
  { // check memory before writing:
    // memblock of supplied pointer
    OhanaMemblock *myBlock = (OhanaMemblock *) header[0].buffer - 1;
    myAssert (myBlock->startblock == OHANA_MEMMAGIC, "bad memory");
    myAssert (myBlock->endblock == OHANA_MEMMAGIC, "bad memory");
    myAssert (myBlock->size >= (size_t) header[0].datasize, "overflow");
  }
# endif

  Nbytes = fwrite (header[0].buffer, sizeof(char), header[0].datasize, f);
  if (Nbytes != header[0].datasize) { 
    perror ("fwrite: "); 
    return FALSE; 
  }
  return TRUE;
}	


int gfits_fwrite_header (FILE *f, Header *header) {
  int status = gfits_save_header (f, header);
  return (status);
}

