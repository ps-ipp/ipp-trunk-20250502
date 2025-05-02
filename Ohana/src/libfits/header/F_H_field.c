# include <ohana.h>
# include <gfitsio.h>

/*********************** fits header field ****************************/
char *gfits_header_field (Header *header, char *field, int N) {

  char *buf;
  off_t i;
  int Nwant, Nfound;
  char keyword[10];

  if (header[0].buffer == NULL) return NULL;

  // regular keywords cannot be larger than 8 chars (use HIERARCH instead)
  if (strlen(field) > 8) return NULL; 

  /* create a blank-padded keyword with exactly 8 characters */
  strcpy (keyword, field);
  for (i = strlen(field); i < 8; i++) keyword[i] = ' ';
  keyword[8] = 0;

  buf = header[0].buffer;

  if (N > 0) {
    /* find the Nth entry */
    Nfound = 0;
    for (i = 0; i < header[0].datasize; i+= FT_LINE_LENGTH, buf += FT_LINE_LENGTH) {
      if (!strncmp (keyword, buf, 8)) {
	Nfound ++;
	if (Nfound == N) return (buf);
      }
    }
  }

  if (N <= 0) {
    /* count the entries */
    Nfound = 0;
    for (i = 0; i < header[0].datasize; i+= FT_LINE_LENGTH, buf += FT_LINE_LENGTH) {
      if (!strncmp (keyword, buf, 8)) {
	Nfound ++;
      }
    }

    Nwant = Nfound + N + 1;
    buf = header[0].buffer;

    /* find the Nwant entry */
    Nfound = 0;
    for (i = 0; i < header[0].datasize; i+= FT_LINE_LENGTH, buf += FT_LINE_LENGTH) {
      if (!strncmp (keyword, buf, 8)) {
	Nfound ++;
	if (Nwant == Nfound) return (buf);
      }
    }

  }

  return NULL;

/* 

   find the Nth entry of this keyword.
   if N < 0, find the last - N - 1 word (ie, -1 = last)

*/
}

// find the given field among the HIERARCH keywords
// these have the form: HIERARCH KEYWORD = value
// returns a pointer to the start of the field
char *gfits_header_hierarch_field (Header *header, char *field, int N) {

  char *buf, *ptr;
  off_t i;
  int Nwant, Nfound, Nfield;

  if (header[0].buffer == NULL) return NULL;

  Nfield = strlen (field);
  if (Nfield >= 71) return NULL;

  buf = header[0].buffer;

  /* find the Nth entry */
  if (N > 0) {
    Nfound = 0;
    for (i = 0; i < header[0].datasize; i+= FT_LINE_LENGTH, buf += FT_LINE_LENGTH) {
      // have we found a HIERARCH entry?
      if (strncmp ("HIERARCH", buf, 8)) continue;
      ptr = buf + 9; // start of following keyword
      if (strncmp (field, ptr, Nfield)) continue;
      if (ptr[Nfield + 1] != '=') continue; // the strncmp above will match a longer string which matches the subset of field (e.g., FOO will match FOOBAR and FOO).  test for the following '=' sign
      Nfound ++;
      if (Nfound == N) return (ptr);
    }
  }

  /* find the Ntotal - Nth entry */
  if (N < 0) {
    /* count the entries */
    Nfound = 0;
    for (i = 0; i < header[0].datasize; i+= FT_LINE_LENGTH, buf += FT_LINE_LENGTH) {
      if (strncmp ("HIERARCH", buf, 8)) continue;
      ptr = buf + 9; // start of following keyword
      if (strncmp (field, ptr, Nfield)) continue;
      Nfound ++;
    }

    Nwant = Nfound + N + 1;
    buf = header[0].buffer;

    /* find the Nwant entry */
    Nfound = 0;
    for (i = 0; i < header[0].datasize; i+= FT_LINE_LENGTH, buf += FT_LINE_LENGTH) {
      if (strncmp ("HIERARCH", buf, 8)) continue;
      ptr = buf + 9; // start of following keyword
      if (strncmp (field, ptr, Nfield)) continue;
      Nfound ++;
      if (Nwant == Nfound) return (ptr);
    }
  }

  return NULL;

/* 

   find the Nth entry of this keyword.
   if N < 0, find the last - N - 1 word (ie, -1 = last)

*/
}

/*********************** fits header field ****************************/
char *gfits_header_lineno (Header *header, off_t N) {

  char *buf;

  if (N*80 >= header[0].datasize) return NULL;

  buf = &header[0].buffer[N*80];

  return (buf);
}

int gfits_header_append_line_raw (Header *header, char *line) {

  // line: input pointer to string of at least 80 chars

  /* find the END of the header */
  char *p = gfits_header_field (header, "END", 1);
  if (p == NULL) return (FALSE); 

  /* is there enough space for 1 more line? */
  if (header[0].datasize - (p - (header[0].buffer)) < 2*FT_LINE_LENGTH) {
    header[0].datasize += FT_RECORD_SIZE;
    REALLOCATE (header[0].buffer, char, header[0].datasize);
    /* re-find the "END" marker, in case new memory block is used */
    p = gfits_header_field (header, "END", 1);
    if (p == NULL) return (FALSE); 
    // fill in the region after the END line with spaces
    memset (p + FT_LINE_LENGTH, ' ', FT_RECORD_SIZE);
  }

  /* push END line back 1 */
  memmove ((p + FT_LINE_LENGTH), p, FT_LINE_LENGTH);
  memcpy (p, line, FT_LINE_LENGTH);
  return TRUE;
}

