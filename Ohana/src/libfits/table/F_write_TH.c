# include <ohana.h>
# include <gfitsio.h>

/*********************** fits write header *********************************/
int gfits_write_Theader (char *filename, Header *header) {
  
  FILE *f = fopen (filename, "a+");
  if (!f) return (FALSE);
  
  int status = fseeko (f, 0LL, SEEK_END);  /* write header to end of file! */
  if (status) { perror ("fseeko: "); return (FALSE);  }
  status = gfits_fwrite_Theader (f, header);

  fclose (f);
  return (status);
}	

/*********************** fits write header *********************************/
int gfits_fwrite_Theader (FILE *f, Header *header) {
  
  off_t Nbytes;
  
# ifdef OHANA_MEMORY  
  { 
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
    return (FALSE);  
  }

  return (TRUE);
}	


