# include <ohana.h>
# include <gfitsio.h>

off_t gfits_heap_start (Header *header) {
  
  int i;
  off_t size;

  if (header[0].Naxes == 0) return (0);

  size = abs(header[0].bitpix / 8);

  for (i = 0; i < header[0].Naxes; i++)
    size *= header[0].Naxis[i];

  return (size);
}

off_t gfits_data_min_size (Header *header) {
  
  off_t size = gfits_heap_start (header);
  size += header[0].pcount;

  // XXX what do I do with gcount?

  return (size);
}

off_t gfits_data_pad_size (off_t rawsize) {
  
  off_t size = rawsize;

  /* round up to next complete block */
  if (rawsize % FT_RECORD_SIZE) {
    off_t Nrec = 1 + (int) (rawsize / FT_RECORD_SIZE);
    size = FT_RECORD_SIZE * Nrec;
  }

  return (size);
}

off_t gfits_data_size (Header *header) {
  off_t rawsize = gfits_data_min_size(header);
  off_t size = gfits_data_pad_size (rawsize);

  return (size);
}

