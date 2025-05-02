# include <ohana.h>
# include <gfitsio.h>

/*********************** fits delete ****************************/
int gfits_delete (Header *header, char *field, int N) {
  
  int i;
  off_t Nbytes;
  char *p1, *p2;
  
  p1 = gfits_header_field (header, field, N);
  if (p1 == NULL) return (TRUE);

  p2 = gfits_header_field (header, "END", 1);
  if (p2 == NULL) return (FALSE); 

  /* pull everything from p1 + FT_LINE_LENGTH to p2 + 79 back 80 chars */
  Nbytes = (p2 - p1);
  memmove (p1, p1 + FT_LINE_LENGTH, Nbytes);

  for (i = 0; i < FT_LINE_LENGTH; i++) 
    *(p2 + i) = ' ';

  p2 = gfits_header_field (header, "END", 1);
  if (header[0].datasize - (p2 - header[0].buffer + FT_LINE_LENGTH) > FT_RECORD_SIZE) {
    header[0].datasize -= FT_RECORD_SIZE;
    REALLOCATE (header[0].buffer, char, header[0].datasize);
  }
  return (TRUE);

}


