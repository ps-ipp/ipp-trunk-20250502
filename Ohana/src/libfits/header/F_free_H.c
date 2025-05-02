# include <ohana.h>
# include <gfitsio.h>

/*********************** fits free header ***********************************/
void gfits_free_header (Header *header) {

  // fprintf (stderr, "free data for %zx\n", (size_t) header->buffer);

  if (header[0].buffer == (char *) NULL) return;
  free (header[0].buffer);
  header[0].buffer = (char *) NULL;
  
}

