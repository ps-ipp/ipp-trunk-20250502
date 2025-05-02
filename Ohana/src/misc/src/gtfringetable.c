# include <ohana.h>
# include <gfitsio.h>

static char *version = "gtfringetable $Revision: 1.5 $";

void get_version (int argc, char **argv, char *version);

int main (int argc, char **argv) {

  int i;
  double binning;
  float *xmin, *xmax, *ymin, *ymax;
  FILE *f;
  FTable table;
  Header header;

  get_version (argc, argv, version);
  if (argc != 4) {
    fprintf (stderr, "USAGE: (table) (ccd) (binning)\n");
    exit (2);
  }

  binning = atof (argv[3]);

  /* load data from fringe points file */
  f = fopen (argv[1], "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "error opening fringe file %s\n", argv[1]);
    exit (1);
  }
  table.header = &header;
  if (!gfits_fread_ftable (f, &table, argv[2])) {
    fprintf (stderr, "error reading table %s\n", argv[2]);
    exit (1);
  }
  fclose (f);

  gfits_get_table_column (&header, &table, "X_MIN", (void **) &xmin);
  gfits_get_table_column (&header, &table, "X_MAX", (void **) &xmax);
  gfits_get_table_column (&header, &table, "Y_MIN", (void **) &ymin);
  gfits_get_table_column (&header, &table, "Y_MAX", (void **) &ymax);

  for (i = 0; i < header.Naxis[1]; i++) {
    fprintf (stdout, "%f %f\n", xmin[i] / binning, ymin[i] / binning);
    fprintf (stdout, "%f %f\n", xmax[i] / binning, ymax[i] / binning);
  }
  exit (0);
}

/**** support functions ******/
void get_version (int argc, char **argv, char *version) {

  int N;
  char *p, *q, *line;

  if (get_argument (argc, argv, "-version")) {

    N = strlen (version) + 2;
    line = (char *) malloc (N);
    bzero (line, N);

    p = strstr (version, "$Revision: ");
    if (p != (char *) NULL) 
      p += strlen ("$Revision: ");
    else
      p = version;

    q = strstr (p, "$");
    if (q != (char *) NULL) 
      N = q - p; 
    else
      N = strlen (p);

    strncpy_nowarn (line, p, N);

    fprintf (stderr, "%s\n", line);
    exit (2);
  }
}  
