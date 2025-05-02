# include "addstar.h"
# include "supercos.h"

int loadsupercos_getFilterInfo (char *line, char *emulsion, char *filterID) {
    
  int i;
  int Nchar = 0;
  char *p1 = line;

  // emulsion is field 11, filterID is field 12
  for (i = 1; i < 11; i++) {
    p1 = parse_nextword_csv (p1);
    if (!p1) {
      fprintf (stderr, "error parsing filter info for line %s\n", line);
      abort();
    }
  }
  if (*p1 == ',') {
    fprintf (stderr, "missing emulsion info for line %s\n", line);
    abort();
  }

  char *p2 = parse_nextword_csv (p1);
  if (!p2) {
    fprintf (stderr, "error parsing filter info for line %s\n", line);
    abort();
  }
  if (*p2 == ',') {
    fprintf (stderr, "missing filter ID for line %s\n", line);
    abort();
  }

  char *p3 = parse_nextword_csv (p2);
  if (!p3) {
    fprintf (stderr, "error parsing filter info for line %s\n", line);
    abort();
  }

  Nchar = p2 - p1 - 1;
  strncpy_nowarn (emulsion, p1, Nchar);
  stripwhite (emulsion);
	
  Nchar = p3 - p2 - 1;
  strncpy_nowarn (filterID, p2, Nchar);
  stripwhite (filterID);
	
  return TRUE;
}

int loadsupercos_getST (char *line, double *st) {
    
  int i;
  char *p1 = line;

  // lstObs is field 20
  for (i = 1; i < 20; i++) {
    p1 = parse_nextword_csv (p1);
    if (!p1) {
      fprintf (stderr, "error parsing filter info for line %s\n", line);
      abort();
    }
  }
  if (*p1 == ',') {
    fprintf (stderr, "missing sidereal time info for line %s\n", line);
    abort();
  }

  char hour[3], minute[3];
  strncpy_nowarn(hour, p1, 2);
  strncpy_nowarn(minute, p1+2, 2);

  double minuteF = atof (minute);
  double hourF = atof(hour);
  *st = hourF + minuteF / 60.0;
	
  return TRUE;
}

int *loadsupercos_image_index (Image *image, int Nimage) {

  int i, *index;

  // note that externID is unsigned, so we cannot set max to -1
  int maxIndex = 0;
  for (i = 0; i < Nimage; i++) {
    maxIndex = MAX (maxIndex, image[i].externID);
  }

  ALLOCATE (index, int, maxIndex + 1);
  for (i = 0; i < maxIndex + 1; i++) {
    index[i] = -1;
  }

  for (i = 0; i < Nimage; i++) {
    int plateID = image[i].externID;
    if (plateID < 0) abort();
    if (plateID > maxIndex) abort();
    if (index[plateID] != -1) abort();
    index[plateID] = i;
  }

  return index;

}
