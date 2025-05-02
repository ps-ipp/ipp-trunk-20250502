# include <kapa_internal.h>
# define NROTCHARS 256

// local functions
void PDF_DumpRotSegment (IOBuffer *buffer, char *segment, int *Nseg);
void PDF_SetFont (IOBuffer *buffer, char *name, int size);

/* writes commands to print string at location and angle using currently set font and size */
void PDFRotText (IOBuffer *buffer, int x, int y, char *string, int pos, double angle) {

  char *segment, basename[64], *currentname;
  int N, code, protect;
  int dX, dY, Xoff, Yoff, X, Y, Nseg, NSEG;
  double cs, sn, currentscale;
  int basesize, currentsize;
  RotFont *currentfont;
  
  currentname = GetRotFont (&currentsize);
  currentfont = GetRotFontData (&currentscale);
  strcpy (basename, currentname);
  basesize = currentsize;

  /* strip off leading WHITESPACE */
  stripwhite (string);
  if (*string == 0) return;
  
  /* compute string length */
  dX = PSRotStrlen (string); // common function for PDF & PS
  dY = currentfont[65].ascent;
  int dP = ceil(0.25 * dY);
  
  /* apply appropriate offset */
  Xoff = Yoff = 0;
  switch (pos) {
    case 0: Xoff =  -dX-dP; Yoff = -(dY+dP); break;
    case 1: Xoff = -0.5*dX; Yoff = -(dY+dP); break;
    case 2: Xoff =      dP; Yoff = -(dY+dP); break;
    case 3: Xoff =  -dX-dP; Yoff = -0.5*dY; break;
    case 4: Xoff = -0.5*dX; Yoff = -0.5*dY; break;
    case 5: Xoff =      dP; Yoff = -0.5*dY; break;
    case 6: Xoff =  -dX-dP; Yoff = dP; break;
    case 7: Xoff = -0.5*dX; Yoff = dP; break;
    case 8: Xoff =      dP; Yoff = dP; break;
  }
  cs = cos(angle*RAD_DEG);
  sn = sin(angle*RAD_DEG);
  X = x + Xoff*cs + Yoff*sn;
  Y = y - Xoff*sn + Yoff*cs;

  PDF_SetFont (buffer, currentname, currentsize);

  PrintIOBuffer (buffer, "q BT 0 0 m\n");
  if (fabs(angle) < 0.001) {
    PrintIOBuffer (buffer, " %d %d Td\n", X, Y);
  } else {
    PrintIOBuffer (buffer, " %5.3f %5.3f %5.3f %5.3f %d %d cm\n", cs, -sn, sn, cs, X, Y);
  }

  Nseg = 0;
  NSEG = strlen(string) + 2;
  ALLOCATE (segment, char, NSEG);
  bzero (segment, NSEG);

  code = FALSE;
  protect = FALSE;

  int YoffBase = 0;
  /* accumulate string segments with common state */

  unsigned int i;
  for (i = 0; i < strlen (string); i++) {
    N = (int)(string[i]);
    if ((N < 0) || (N >= NROTCHARS)) continue;

    if (N == 39) { // single-quote
      protect = protect ? FALSE : TRUE;
    } 

    // XXX I need to track the number of sub and super symbols
    // normal resets to the raw text
    // each sub or super scripts font and drops font

    /* check for special characters */
    if (!code && !protect) {
      /* subscript character (_) */
      if (N == 94) {
	PDF_DumpRotSegment (buffer, segment, &Nseg);
	PDF_SetFont (buffer, currentname, (int)(0.8*currentsize));
	currentfont = GetRotFontData (&currentscale);
	if (YoffBase > 0) YoffBase = 0;
	YoffBase --;
	PrintIOBuffer (buffer, "%d Ts\n", -5*YoffBase);
	continue;
      }
      /* superscript character (^) */
      if (N == 95) { 
	PDF_DumpRotSegment (buffer, segment, &Nseg);
	PDF_SetFont (buffer, currentname, (int)(0.8*currentsize));
	currentfont = GetRotFontData (&currentscale);
	if (YoffBase < 0) YoffBase = 0;
	YoffBase ++;
	PrintIOBuffer (buffer, "%d Ts\n", -5*YoffBase);
	continue;
      }
      /* end super/sub script (|) */
      if (N == 124) {
	PDF_DumpRotSegment (buffer, segment, &Nseg);
	PDF_SetFont (buffer, currentname, basesize);
	currentfont = GetRotFontData (&currentscale);

	YoffBase = 0;
	PrintIOBuffer (buffer, "0 Ts\n");
	continue;
      }
      /* escape char (\) */
      if (N == 92) {
	code = TRUE;
	continue;
      } 
      /* begin paren (insert \) */
      if (N == 40) {
	code = FALSE;
	segment[Nseg] = 92;
	Nseg ++;
	CHECK_REALLOCATE (segment, char, NSEG, Nseg, 64);
      }
      /* end paren (insert \) */
      if (N == 41) {
	code = FALSE;
	segment[Nseg] = 92;
	Nseg ++;
	CHECK_REALLOCATE (segment, char, NSEG, Nseg, 64);
      }
      /* font change character (&) */
      if (N == 38) {
	PDF_DumpRotSegment (buffer, segment, &Nseg);
	if (string[i+1] == 'h') {
	  PDF_SetFont (buffer, "helvetica", currentsize);
	  currentfont = GetRotFontData (&currentscale);
	}
	if (string[i+1] == 't') {
	  PDF_SetFont (buffer, "times", currentsize);
	  currentfont = GetRotFontData (&currentscale);
	}
	if (string[i+1] == 'c') {
	  PDF_SetFont (buffer, "courier", currentsize);
	  currentfont = GetRotFontData (&currentscale);
	}
	if (string[i+1] == 's') {
	  PDF_SetFont (buffer, "symbol", currentsize);
	  currentfont = GetRotFontData (&currentscale);
	}
	i++;
	continue;
      }
    }
    code = FALSE;
    segment[Nseg] = N;
    Nseg ++;
    CHECK_REALLOCATE (segment, char, NSEG, Nseg, 64);
  }
  PDF_DumpRotSegment (buffer, segment, &Nseg);

  // if (fabs(angle) >= 0.001) {
  PrintIOBuffer (buffer, " ET Q\n");
  PrintIOBuffer (buffer, " 0 Ts\n"); // clear any super- or subscript offsets

  free (segment);
  PDF_SetFont (buffer, basename, basesize);
}

void PDF_DumpRotSegment (IOBuffer *buffer, char *segment, int *Nseg) {
  segment[*Nseg] = 0;
  PrintIOBuffer (buffer, "(%s) Tj\n", segment);
  bzero (segment, *Nseg);
  *Nseg = 0;
}

void PDF_SetFont (IOBuffer *buffer, char *name, int size) {
  if (!strcmp (name, "times"))     PrintIOBuffer (buffer, "/Ft %d Tf\n", size);
  if (!strcmp (name, "helvetica")) PrintIOBuffer (buffer, "/Fh %d Tf\n", size);
  if (!strcmp (name, "courier"))   PrintIOBuffer (buffer, "/Fc %d Tf\n", size);
  if (!strcmp (name, "symbol"))    PrintIOBuffer (buffer, "/Fs %d Tf\n", size);
}
