# include "Ximage.h"
# define XOFFSET 0
# define YOFFSET 0

static char *name = "$Name: not supported by cvs2svn $";
static Graphic *graphic;

int PScommand (int sock) {

  int status, scaleMode, pageMode;
  char filename[1024], pagename[1024];

  /* expect a line telling the number of bytes and a filename */
  KiiScanMessage (sock, "%s %s %d %d", filename, pagename, &scaleMode, &pageMode);
  status = PSit (filename, pagename, scaleMode, pageMode);
  return (status);
}

int PSit (char *filename, char *pagename, int scaleMode, int pageMode) {

  int i, Nsection;
  double scale;
  FILE *f;
  char *version;
  Section *section;

  graphic = GetGraphic();

  if (pageMode == KAPA_PS_NEWPAGE) {
    f = fopen (filename, "a+");
  } else {
    f = fopen (filename, "w");
  }
  if (f == NULL) {
    fprintf (stderr, "can't open output file %s\n", filename);
    return (TRUE);  /* true because otherwise it quits kapa! */
  }

  /* two scaling options: expand to fit page / keep absolute size */ 
  if (scaleMode) {
    scale = MIN (fabs(500.0 / graphic->dx), fabs (700.0 / graphic->dy));
  } else {
    scale = 72.0 / 96.0; /* ratio of screen pixels to points */
  }

  switch (pageMode) {
    case KAPA_PS_NEWPLOT:
      fprintf (f, "%%!PS-Adobe-2.0 EPSF-2.0\n");
      fprintf (f, "%%%%Title: %s\n", filename);
      version = strip_version (name);
      fprintf (f, "%%%%Creator: Kapa (%s)\n", version);
      free (version);
      fprintf (f, "%%%%BoundingBox: %d %d %.0f %.0f\n", 
	       XOFFSET, YOFFSET, XOFFSET + scale*graphic->dx, YOFFSET + scale*graphic->dy);
      fprintf (f, "%%%%Pages: 1\n");
      fprintf (f, "%%%%DocumentFonts:\n");
      fprintf (f, "%%%%EndComments\n");
      fprintf (f, "%%%%EndProlog\n");
      fprintf (f, "%%%%Page: %s\n\n", pagename);
      break;

    case KAPA_PS_NEWPAGE:
      fprintf (f, "%%%%Page: %s\n\n", pagename);
      break;

    case KAPA_PS_RAWPAGE:
      break;
  } 
  fprintf (f, "gsave %% encloses picture\n");
  fprintf (f, "%% local abbreviations\n");
  fprintf (f, "/Times-Roman findfont 14 scalefont setfont\n");
  fprintf (f, "/T {moveto show stroke} def\n");
  fprintf (f, "/B { newpath moveto dup 0 exch rlineto exch dup 0 rlineto exch -1 mul\n");
  fprintf (f, " 0 exch rlineto -1 mul 0 rlineto closepath stroke } def\n");
  fprintf (f, "/F { newpath moveto dup 0 exch rlineto exch dup 0 rlineto exch -1 mul\n");
  fprintf (f, " 0 exch rlineto -1 mul 0 rlineto closepath fill stroke } def\n");
  fprintf (f, "/C {0 360 arc stroke} def\n");
  fprintf (f, "/FC {0 360 arc fill stroke} def\n");
  fprintf (f, "/RGB {setrgbcolor} def\n");
  fprintf (f, "/L {newpath moveto lineto stroke} def\n\n");
  fprintf (f, "/TF {newpath moveto lineto lineto fill stroke} def\n\n");

  if (pageMode != KAPA_PS_RAWPAGE) {
    fprintf (f, " %d %d translate\n", XOFFSET, YOFFSET);
    fprintf (f, "  %f  %f scale\n", scale, scale);
  }

  Nsection = GetNumberOfSections ();
  for (i = 0; i < Nsection; i++) {
    section = GetSectionByNumber (i);
    if (section->image) {
      PSimage (section->image, f);
    }
    if (section->graph) {
      PSFrame (section->graph, f); 
      PSObjects (section->graph, f);
      PSLabels (section->graph, f);
      PSTextlines (section->graph, f);
    }
  }
  
  fprintf (f, "grestore %% end of picture\n");

  if (pageMode != KAPA_PS_RAWPAGE) fprintf (f, "showpage\n");

  fclose (f);
  return (TRUE);
}

