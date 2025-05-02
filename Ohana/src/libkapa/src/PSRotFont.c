# include <kapa_internal.h>
# define NROTCHARS 256

// local functions:
void PSDumpRotSegment (FILE *f, char *segment, int *Nseg);
void PSSetFont (FILE *f, char *name, int size);

/* writes commands to print string at location and angle using
   currently set font and size */
void PSRotText (FILE *f, int x, int y, char *string, int pos, double angle) {

  char *segment, basename[64], *currentname;
  int N, code, protect;
  int dX, dY, Xoff, Yoff, X, Y, Nseg, NSEG, YoffBase;
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
  dX = PSRotStrlen (string);
  dY = currentfont[65].ascent;
  int dP = ceil(0.25 * dY);
  
  /***

      update this to use the following PS code:

      /ceshow { % (string) fontsize fontname x y 
      gsave 
      moveto findfont exch scalefont setfont % s 
      gsave 
      dup false charpath flattenpath pathbbox % s x0 y0 x1 y1 
      grestore 
      3 -1 roll sub % s x0 x1 dy 
      3 1 roll sub % s dy -dx 
      2 div exch % s -dx/2 dy 
      -2 div % s -dx/2 -dy/2 
      rmoveto show 
      grestore 
      } bind def 

   ***/

  /* apply appropriate offset */
  Xoff = Yoff = 0;
  switch (pos) {
  // case 0: Xoff =     -dX; Yoff = -dY; break;
  // case 1: Xoff = -0.5*dX; Yoff = -dY; break;
  // case 2: Xoff =       0; Yoff = -dY; break;
  // case 3: Xoff =     -dX; Yoff = -0.5*dY; break;
  // case 4: Xoff = -0.5*dX; Yoff = -0.5*dY; break;
  // case 5: Xoff =       0; Yoff = -0.5*dY; break;
  // case 6: Xoff =     -dX; Yoff = 0; break;
  // case 7: Xoff = -0.5*dX; Yoff = 0; break;
  // case 8: Xoff =       0; Yoff = 0; break;
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

  PSSetFont (f, currentname, currentsize);
  fprintf (f, "gsave\n");
  fprintf (f, " %d %d moveto %f rotate\n", X, Y, -angle);

  Nseg = 0;
  NSEG = strlen(string) + 2;
  ALLOCATE (segment, char, NSEG);
  bzero (segment, NSEG);

  code = FALSE;
  protect = FALSE;

  YoffBase = 0;
  /* accumulate string segments with common state */

  unsigned int i;
  for (i = 0; i < strlen (string); i++) {
    N = (int)(string[i]);
    if ((N < 0) || (N >= NROTCHARS)) continue;

    if (N == 39) { // single-quote
      protect = protect ? FALSE : TRUE;
    } 

    /* check for special characters */
    if (!code && !protect) {
      /* subscript character (_) */
      if (N == 94) {
	PSDumpRotSegment (f, segment, &Nseg);
	PSSetFont (f, currentname, (int)(0.8*currentsize));
	currentfont = GetRotFontData (&currentscale);
	Yoff = 0.75*currentscale*dY;
	fprintf (f, "0 %d rmoveto\n", Yoff);
	YoffBase += Yoff;
	// PSSetFont (f, currentname, currentsize);
	continue;
      }
      /* superscript character (^) */
      if (N == 95) { 
	PSDumpRotSegment (f, segment, &Nseg);
	PSSetFont (f, currentname, (int)(0.8*currentsize));
	currentfont = GetRotFontData (&currentscale);
	Yoff = -0.5*currentscale*dY;
	fprintf (f, "0 %d rmoveto\n", Yoff);
	YoffBase += Yoff;
	// PSSetFont (f, currentname, currentsize);
	continue;
      }
      /* end super/sub script (|) */
      if (N == 124) {
	PSDumpRotSegment (f, segment, &Nseg);
	PSSetFont (f, currentname, basesize);
	currentfont = GetRotFontData (&currentscale);
	fprintf (f, "0 %d rmoveto\n", -YoffBase);
	YoffBase = 0;
	// PSSetFont (f, currentname, currentsize);
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
	PSDumpRotSegment (f, segment, &Nseg);
	if (string[i+1] == 'h') {
	  PSSetFont (f, "helvetica", currentsize);
	  currentfont = GetRotFontData (&currentscale);
	  // PSSetFont (f, currentname, currentsize);
	}
	if (string[i+1] == 't') {
	  PSSetFont (f, "times", currentsize);
	  currentfont = GetRotFontData (&currentscale);
	  // PSSetFont (f, currentname, currentsize);
	}
	if (string[i+1] == 'c') {
	  PSSetFont (f, "courier", currentsize);
	  currentfont = GetRotFontData (&currentscale);
	  // PSSetFont (f, currentname, currentsize);
	}
	if (string[i+1] == 's') {
	  PSSetFont (f, "symbol", currentsize);
	  currentfont = GetRotFontData (&currentscale);
	  // PSSetFont (f, currentname, currentsize);
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
  PSDumpRotSegment (f, segment, &Nseg);
  fprintf (f, "stroke grestore\n");
  free (segment);
  PSSetFont (f, basename, basesize);
}

void PSDumpRotSegment (FILE *f, char *segment, int *Nseg) {
  segment[*Nseg] = 0;
  fprintf (f, "(%s) show\n", segment);
  bzero (segment, *Nseg);
  *Nseg = 0;
}

void PSSetFont (FILE *f, char *name, int size) {
  if (!strcmp (name, "times")) 
    fprintf (f, "/Times-Roman findfont %d scalefont setfont\n", size);
  if (!strcmp (name, "helvetica")) 
    fprintf (f, "/Helvetica findfont %d scalefont setfont\n", size);
  if (!strcmp (name, "courier")) 
    fprintf (f, "/Courier findfont %d scalefont setfont\n", size);
  if (!strcmp (name, "symbol")) 
    fprintf (f, "/Symbol findfont %d scalefont setfont\n", size);
}

# define NROT 256
int PSRotStrlen (char *c) {

  int N, dX, code;
  double currentscale, scale; 

  RotFont *currentfont = GetRotFontData (&currentscale);
  scale = currentscale;

  /* find string length */
  dX = 0;

  code = FALSE;

  unsigned int i;
  for (i = 0; i < strlen (c); i++) {
    N = (int)(c[i]);
    /* skip non-printing characters */
    if ((N < 0) || (N >= NROT)) continue;

    /* check for special characters */
    if (!code) {
      if (N == 94) { /* super-script */
	scale *= 0.8;
	continue;
      }
      if (N == 95) { /* sub-script */
	scale *= 0.8;
	continue;
      }
      if (N == 124) { /* normal-script */
	scale = currentscale;
	continue;
      }
      if (N == 92) { /* backslash */
	code = TRUE;
	continue;
      } 
      if (N == 38) { /* font-code */
	i++;
	continue;
      }
    }
    code = FALSE;
    dX += scale*currentfont[N].dXps + 1;
  }
  return (dX);
}


  /* test cross hair
  fprintf (f, "%d %d %d %d L\n", x-10, y, x+10, y);
  fprintf (f, "%d %d %d %d L\n", x, y-10, x, y+10);
  */
