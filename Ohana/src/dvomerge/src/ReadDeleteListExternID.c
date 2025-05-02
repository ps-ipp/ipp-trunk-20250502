# include "dvomerge.h"

/* Delete List Format:
   image o5252g0035o.140669.cm.51017.smf[XY11.hdr] (219974) : 368 vs 375
   image o5252g0051o.140685.cm.51123.smf[XY11.hdr] (226417) : 350 vs 356
   image o5399g0207o.194950.cm.98351.smf[XY46.hdr] (1683277) : 10341 vs 10347
   image o5464g0284o.230438.cm.123774.smf[XY21.hdr] (2634913) : 214 vs 242
   image o5464g0305o.230459.cm.123795.smf[XY21.hdr] (2635758) : 232 vs 257
   image o5465g0306o.231171.cm.124201.smf[XY01.hdr] (2654941) : 0 vs 494
   image o5465g0306o.231171.cm.124201.smf[XY02.hdr] (2654942) : 10 vs 813
   image o5465g0306o.231171.cm.124201.smf[XY10.hdr] (2654947) : 0 vs 534
   image o5465g0306o.231171.cm.124201.smf[XY11.hdr] (2654948) : 0 vs 447
*/

int *ReadDeleteListExternID(char *filename, int *nindex) {

  int j, index, Nindex, NINDEX, *indexList, Nline;
  int Nstart, Nbytes, Nread, status;
  char *c0, *c1, *space, *buffer;

  char *indexPoint = NULL;
  FILE *f = fopen (filename, "r");
  myAssert(f, "failed to open delete list");
  
  Nindex = 0;
  NINDEX = 1000;
  ALLOCATE(indexList, int, NINDEX);

  ALLOCATE (buffer, char, 0x10001);
  bzero (buffer, 0x10001);

  Nstart = 0;
  Nline = 0;
  while (1) {
    Nbytes = 0x10000 - Nstart;
    bzero (&buffer[Nstart], Nbytes);
    Nread = fread (&buffer[Nstart], 1, Nbytes, f);
    if (ferror (f)) {
      perror ("error reading data file");
      exit (1);
    }
    if (Nread == 0) break; // end of data
    // nbytes = Nread + Nstart;
    
    status = TRUE;
    c0 = buffer; 
    while (status) {
      // identify the range of the line
      c1 = strchr (c0, '\n');
      if (c1 == (char *) NULL) {
	Nstart = strlen (c0);
	memmove (buffer, c0, Nstart);
	break;
      } else {
	*c1 = 0;
      }      
      if (*c0 == '#') goto next_line;

      // confirm we have 9 fields broken by 8 spaces:
      space = c0; // pointer to track the space-separated words
      indexPoint = strchr(c0, ' '); // pointer to the ID on this line

      for (j = 0; j < 9; j++) {
	space = strchr(space, ' '); 
	if (!space) {
	  fprintf (stderr, "line only has %d word(s)\n", j + 1);
	  goto next_line;
	}
	space ++;
      }

      // save the value we have found:
      index = atoi(indexPoint);
      indexList[Nindex] = index;

      fprintf (stderr, "index: %d, line: %s\n", index, c0);

      Nindex ++;
      if (Nindex == NINDEX) {
	NINDEX += 1000;
	REALLOCATE (indexList, int, NINDEX);
      }
      
    next_line:
      Nline ++;
      c0 = c1 + 1;
    }
  }

  *nindex = Nindex;
  return (indexList);
}

/* Delete List Format:
  duplicate 78853625 2
  duplicate 78853626 2
  duplicate 78853627 2
*/

int *ReadDeleteListExternID_v2(char *filename, int *nindex) {

  int j, index, Nindex, NINDEX, *indexList, Nline;
  int Nstart, Nbytes, Nread, status;
  char *c0, *c1, *space, *buffer;

  char *indexPoint = NULL;
  FILE *f = fopen (filename, "r");
  myAssert(f, "failed to open delete list");
  
  Nindex = 0;
  NINDEX = 1000;
  ALLOCATE(indexList, int, NINDEX);

  ALLOCATE (buffer, char, 0x10001);
  bzero (buffer, 0x10001);

  Nstart = 0;
  Nline = 0;
  while (1) {
    Nbytes = 0x10000 - Nstart;
    bzero (&buffer[Nstart], Nbytes);
    Nread = fread (&buffer[Nstart], 1, Nbytes, f);
    if (ferror (f)) {
      perror ("error reading data file");
      exit (1);
    }
    if (Nread == 0) break; // end of data
    // nbytes = Nread + Nstart;
    
    status = TRUE;
    c0 = buffer; 
    while (status) {
      // identify the range of the line
      c1 = strchr (c0, '\n');
      if (c1 == (char *) NULL) {
	Nstart = strlen (c0);
	memmove (buffer, c0, Nstart);
	break;
      } else {
	*c1 = 0;
      }      
      if (*c0 == '#') goto next_line;

      // confirm we have 3 fields broken by 2 spaces:
      space = c0; // pointer to track the space-separated words
      indexPoint = strchr(c0, ' '); // pointer to the ID on this line

      for (j = 0; j < 2; j++) {
	space = strchr(space, ' '); 
	if (!space) {
	  fprintf (stderr, "line only has %d word(s)\n", j + 1);
	  goto next_line;
	}
	space ++;
      }

      // save the value we have found:
      index = atoi(indexPoint);
      indexList[Nindex] = index;

      fprintf (stderr, "index: %d, line: %s\n", index, c0);

      Nindex ++;
      if (Nindex == NINDEX) {
	NINDEX += 1000;
	REALLOCATE (indexList, int, NINDEX);
      }
      
    next_line:
      Nline ++;
      c0 = c1 + 1;
    }
  }

  *nindex = Nindex;
  return (indexList);
}

