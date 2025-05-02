# include "lightcurve.h"

/* this repairs headers screwed up in the very specific way of 
   astro_again: not 80 chars per line on new lines, and no padding
   of the buffer.

*/

void fix_header (Header *header)
{

  int i, j, Nbytes, N;
  char *p1, *p2;

  Nbytes = 2880 * (int) (header[0].datasize / 2880 + 1);
  reallocate (header[0].buffer, char, Nbytes);
  bzero (&header[0].buffer[header[0].datasize], Nbytes - header[0].datasize - 1);

  p1 = header[0].buffer - 1;
  p2 = strchr (p1 + 1, RETURN);

  while (p2 != NULL) {
    if ((N = p2 - p1) != 80) {
      bcopy (p2, p1 + 80, header[0].datasize - (p2 - header[0].buffer));
      for (i = 0; i < 80 - N; i++) {
	*(p2 + i) = ' ';
      }
      header[0].datasize += 80 - N;
    }
    p1 = strchr (p1 + 1, RETURN);
    p2 = strchr (p1 + 1, RETURN);
  }

  header[0].datasize = Nbytes;
  p1 = gfits_header_field (header, "END", 1);
  for (i = 3; i < 79; i++) 
    *(p1 + i) = ' ';
  *(p1 + 79) = RETURN;
  
  Nbytes = header[0].datasize - (p1 - header[0].buffer) - 80;
  
  for (i = 0; i < Nbytes; i++) {
    for (j = 0; j < 79; j++, i++) 
      *(p1 + i + 80) = '.';
    *(p1 + i + 80) = RETURN;
  }

  Nbytes = header[0].datasize - (p1 - header[0].buffer) - 80;
  while (Nbytes >= 2880) {
    header[0].datasize -= 2880;
    Nbytes = header[0].datasize - (p1 - header[0].buffer) - 80;
  }

  reallocate (header[0].buffer, char, header[0].datasize);
}


