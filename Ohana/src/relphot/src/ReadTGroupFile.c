# include "relphot.h"

# define NBUFFER 0x10000
# define IsCSV FALSE
# define COLUMN_NUMBER 1

unsigned int *ReadTGroupFile (FILE *f, int *nelem) {

  // we allocate one extra byte into which we never read so there will always be a NULL terminating the string
  ALLOCATE_PTR (buffer, char, NBUFFER + 1);
  bzero (buffer, NBUFFER + 1);

  int Nstart = 0; // location of the last valid byte in the buffer (start filling here)
  int EndOfFile = FALSE;

  // int TimeFormat = TIME_MJD;
  // time_t TimeReference = 0;

  int Nelem = 0;
  int NELEM = 1000;
  ALLOCATE_PTR (elem, unsigned int, NELEM);

  while (!EndOfFile) {

    int Nbytes = NBUFFER - Nstart;
    bzero (&buffer[Nstart], Nbytes + 1);
    // ^-- we have allocated one extra byte into which we never read so there will always be a NULL terminating the string

    int nread = fread (&buffer[Nstart], 1, Nbytes, f);
    if (ferror (f)) {
      perror ("error reading data file");
      return NULL;
    }

    // we still need to parse the rest of the buffer, but there might not be an EOL on the last line
    if (nread == 0) {
      EndOfFile = TRUE;
    }
    
    int bufferStatus = TRUE; 

    char *c0 = buffer; // c0 always marks the start of a line
    while (bufferStatus) {
      char *c1 = strchr (c0, '\n'); // find the end of this current line (also valid for a Mac: \r\n)
      if (!c1) {
	c1 = strchr (c0, '\r'); // try \r for Windows files
      }
      if (!c1) {
	Nstart = strlen (c0);
	if (EndOfFile) {
	  // if we have reached EOF, we need to do one last pass in case there is a line without a return
	  c1 = c0 + Nstart;
	  bufferStatus = FALSE;
	  if (Nstart == 0) continue; // if we have reached EOF and c0 points at the last valid character, we are done
	} else {
	  // if we have not reached EOF, we need to shift the buffer to the start of this line and read more data
	  memmove (buffer, c0, Nstart);
	  bufferStatus = FALSE;
	  continue;
	}
      }
      *c1 = 0; // mark the end of the line 

      // skip to the next line (but if EOF, do not overrun buffer)
      if (*c0 == '#')          { if (!EndOfFile) { c0 = c1 + 1; } continue; }
      if (*c0 == '!')          { if (!EndOfFile) { c0 = c1 + 1; } continue; }
  
      // the times are written as MJD in the file:
      double dvalue;
      int readStatus = IsCSV ? dparse_csv (&dvalue, COLUMN_NUMBER, c0) : dparse (&dvalue, COLUMN_NUMBER, c0);
      if (!readStatus) continue; // skip invalid entries

      // convert the (double) mjd value to (unsigned int) time
      elem[Nelem] = ohana_mjd_to_sec (dvalue);

      Nelem ++;
      if (Nelem == NELEM) {
	NELEM += 1000;
	REALLOCATE (elem, unsigned int, NELEM);
      }

      if (!EndOfFile) {
	c0 = c1 + 1;
      }
    }
  }
  FREE (buffer);

  *nelem = Nelem;
  return elem;
}

/* 
      time_t tvalue;
      int readStatus = IsCSV ? tparse_csv (&tvalue, COLUMN_NUMBER, c0) : tparse (&tvalue, COLUMN_NUMBER, c0);
      if (!readStatus) continue; // skip invalid entries

      dvalue = TimeValue (tvalue, TimeReference, TimeFormat);
      elem[Nelem] = (int) dvalue;
*/
