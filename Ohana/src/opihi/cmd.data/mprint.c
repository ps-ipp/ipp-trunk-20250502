# include "data.h"

enum {OUTPUT_TEXT, OUTPUT_HTML, OUTPUT_LATEX};

int mprint (int argc, char **argv) {

  Buffer *buf;
  int N;

  int OUTPUT_FMT = OUTPUT_TEXT;
  if ((N = get_argument (argc, argv, "-html"))) {
    remove_argument (N, &argc, argv);
    OUTPUT_FMT = OUTPUT_HTML;
  }
  if ((N = get_argument (argc, argv, "-latex"))) {
    if (OUTPUT_FMT == OUTPUT_HTML) {
      gprint (GP_ERR, "ERROR: only one of -html and -latex may be used for mprint\n");
      return (FALSE);
    }
    remove_argument (N, &argc, argv);
    OUTPUT_FMT = OUTPUT_LATEX;
  }

  char *format = NULL;
  if ((N = get_argument (argc, argv, "-fmt"))) {
    remove_argument (N, &argc, argv);
    format = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!format) format = strcreate ("%f");

  if (argc < 6) {
    gprint (GP_ERR, "USAGE: mprint [-fmt format] [-html | -latex] (buffer) [Xs] [Ys] [Nx] [Ny]\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  int Xs = atoi(argv[2]);
  int Ys = atoi(argv[3]);
  int Nx = atoi(argv[4]);
  int Ny = atoi(argv[5]);

  // (silently?) truncate to valid region on the image
  Xs = MIN (MAX (0, Xs), buf->matrix.Naxis[0]); // Xs : 0 to Nx-1
  Ys = MIN (MAX (0, Ys), buf->matrix.Naxis[1]);

  // if: Naxis[0] = 100, Xs = 50, Nx = 50, Nx -> 50
  // if: Naxis[0] = 100, Xs = 100, Nx = 1, Nx -> 0
  Nx = MIN (buf->matrix.Naxis[0] - Xs, MAX (0, Nx));
  Ny = MIN (buf->matrix.Naxis[1] - Ys, MAX (0, Ny));

  int NX = buf->matrix.Naxis[0];

  float *bufValue = (float *) buf->matrix.buffer;

  if (OUTPUT_FMT == OUTPUT_HTML) {
    gprint (GP_LOG, " <table>\n");
  }
  if (OUTPUT_FMT == OUTPUT_LATEX) {
    char tablespec[512];
    strcpy (tablespec, "{ | l || ");
    for (int ix = 0; ix < Nx; ix++) {
      strcat (tablespec, "r | ");
    }
    strcat (tablespec, "}");
    gprint (GP_LOG, "\\begin{tabular}%s\n", tablespec);
    gprint (GP_LOG, "\\hline\n");
  }

  // top row (column labels)
  if (OUTPUT_FMT == OUTPUT_HTML) gprint (GP_LOG, "<tr>");
  if (OUTPUT_FMT == OUTPUT_HTML) gprint (GP_LOG, " <th> ");
  gprint (GP_LOG, "%s", "Y X ");
  if (OUTPUT_FMT == OUTPUT_HTML) gprint (GP_LOG, " </th> ");
  if (OUTPUT_FMT == OUTPUT_LATEX) gprint (GP_LOG, " & ");

  for (int ix = Xs; ix < Xs + Nx; ix++) {
    if (OUTPUT_FMT == OUTPUT_HTML) gprint (GP_LOG, " <th> ");
    gprint (GP_LOG, format, (float) ix);
    if (OUTPUT_FMT == OUTPUT_TEXT) gprint (GP_LOG, " ");
    if (OUTPUT_FMT == OUTPUT_HTML) gprint (GP_LOG, " </th> ");
    if (OUTPUT_FMT == OUTPUT_LATEX) {
      if (ix == Xs + Nx - 1) gprint (GP_LOG, " \\\\ ");
      else gprint (GP_LOG, " & ");
    }
  }
  if (OUTPUT_FMT == OUTPUT_HTML) gprint (GP_LOG, "</tr>");
  gprint (GP_LOG, "\n");
  if (OUTPUT_FMT == OUTPUT_LATEX) gprint (GP_LOG, "\\hline \\hline\n");

  for (int iy = Ys; iy < Ys + Ny; iy++) {
    if (OUTPUT_FMT == OUTPUT_HTML) gprint (GP_LOG, "<tr>");

    // add row label 
    if (OUTPUT_FMT == OUTPUT_HTML) gprint (GP_LOG, " <td> ");
    gprint (GP_LOG, format, (float) iy);
    if (OUTPUT_FMT == OUTPUT_LATEX) gprint (GP_LOG, " & ");
    if (OUTPUT_FMT == OUTPUT_TEXT) gprint (GP_LOG, " ");
    if (OUTPUT_FMT == OUTPUT_HTML) gprint (GP_LOG, " </td> ");
    
    for (int ix = Xs; ix < Xs + Nx; ix++) {
      if (OUTPUT_FMT == OUTPUT_HTML) gprint (GP_LOG, " <td> ");
      gprint (GP_LOG, format, bufValue[ix + iy*NX]);
      if (OUTPUT_FMT == OUTPUT_TEXT) gprint (GP_LOG, " ");
      if (OUTPUT_FMT == OUTPUT_HTML) gprint (GP_LOG, " </td> ");
      if (OUTPUT_FMT == OUTPUT_LATEX) {
	if (ix == Xs + Nx - 1) gprint (GP_LOG, " \\\\ ");
	else gprint (GP_LOG, " & ");
      }
    }
    if (OUTPUT_FMT == OUTPUT_HTML) gprint (GP_LOG, "</tr>");
    gprint (GP_LOG, "\n");
    if (OUTPUT_FMT == OUTPUT_LATEX) gprint (GP_LOG, "\\hline\n");
  }

  if (OUTPUT_FMT == OUTPUT_LATEX) {
    gprint (GP_LOG, "\\end{tabular}\n");
  }

  if (OUTPUT_FMT == OUTPUT_HTML) {
    gprint (GP_LOG, " </table>\n");
  }

  gprint (GP_LOG, "\n");

  return (TRUE);
}
