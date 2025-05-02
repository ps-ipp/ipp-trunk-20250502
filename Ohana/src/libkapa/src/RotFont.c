# include <kapa_internal.h>
# include "alphabet.h"
  
static int Nrotfonts;
static FontSet *RotFonts;

static char currentname[64];
static int  currentsize;
static double currentscale;
static RotFont *currentfont;

static int RotFontInited = FALSE;

void InitRotFonts () {

  int i, Nhardwired;

  if (RotFontInited) return;
  RotFontInited = TRUE;

  Nhardwired = sizeof (HardwiredFonts) / sizeof (FontSet);
  
  Nrotfonts = Nhardwired;
  ALLOCATE (RotFonts, FontSet, Nrotfonts);
  
  for (i = 0; i < Nhardwired; i++) {
    RotFonts[i] = HardwiredFonts[i];
  }

  currentfont = RotFonts[DEFFONT].font;
  currentscale = 1.0;
  strncpy_nowarn (currentname, RotFonts[DEFFONT].name, 63); 
  currentsize = RotFonts[DEFFONT].size;
}

void FreeRotFonts (void) {
  free (RotFonts);
  RotFonts = FALSE;
  RotFontInited = FALSE;
}

int SetRotFont (char *name, int size) {
  
  int i, nsize, msize, bsize, bigger, dsize, match, good;

  // fprintf (stderr, "SetRotFont %s @ %d\n", name, size);

  InitRotFonts();

  bigger = good = match = -1;
  dsize = 10000;
  bsize = 10000;
  for (i = 0; i < Nrotfonts; i++) {
    if (!strcasecmp (RotFonts[i].name, name)) {
      // fprintf (stderr, "match name: %s %d\n", RotFonts[i].name, RotFonts[i].size);
      good = i;
      nsize = abs (RotFonts[i].size - size);
      if (nsize < dsize) {
	match = i;
	dsize = nsize;
      }
      msize = RotFonts[i].size - size;
      if ((msize < bsize) && (msize >= 0)) {
	bigger = i;
	bsize = msize;
      }
    }
  }
  
  // fprintf (stderr, "matched: %d %d %d\n", bigger, match, good);

  if ((match == -1) && (good != -1)) match = good;
  if (bigger != -1) match = bigger;

  // fprintf (stderr, "finally: %d %d %d\n", bigger, match, good);
  if (match != -1) {
    currentfont = RotFonts[match].font;
    currentscale = (double) size / RotFonts[match].size;
    currentsize = size;
    if (name != currentname) {
      strncpy_nowarn (currentname, name, 63); 
    }
    return (TRUE);
  } else {
    fprintf (stderr, "no matching font\n");
    return (FALSE);
  }

}
  
char *GetRotFont (int *size) {

  InitRotFonts();

  *size = currentsize;
  return (currentname);

}

RotFont *GetRotFontData (double *scale) {

  InitRotFonts();

  *scale = currentscale;
  return (currentfont);
}

int RotStrlen (char *c) {

  int N, dX, code;
  double scale; 
  
  InitRotFonts();

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
    dX += scale*currentfont[N].dx + 1;
  }
  return (dX);
}
