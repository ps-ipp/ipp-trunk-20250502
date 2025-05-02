# include <ohana.h>
# include <gfitsio.h>

static char *version = "mkfringetable $Revision: 1.9 $";

void get_version (int argc, char **argv, char *version);

int main (int argc, char **argv) {

  int i, j, Npts, NPTS, Nccd, status;
  char *layout, *config, *file;
  char filter[64], start[64], stop[64], camera[64], *datestr;
  char ImagetypeKeyword[64], CCDnumKeyword[64], FilterKeyword[64], CameraKeyword[64];
  char *row, line[512], field[64], extname[64], filename[512];
  double x, y, *xmin, *xmax, *ymin, *ymax;
  FILE *f, *g;

  Header header, theader;
  Matrix matrix;
  FTable table;

  get_version (argc, argv, version);

  /*** load ptolemy/elixir configuration info ***/
  file = SelectConfigFile (&argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (0);
  }
  ScanConfig (config, "IMAGETYPE-KEYWORD",           "%s", 0, ImagetypeKeyword);
  ScanConfig (config, "CCDNUM-KEYWORD",              "%s", 0, CCDnumKeyword);
  ScanConfig (config, "FILTER-KEYWORD",              "%s", 0, FilterKeyword);
  ScanConfig (config, "CAMERA-KEYWORD",              "%s", 0, CameraKeyword);
  free (config);
  free (file);

  if (argc != 3) {
    fprintf (stderr, "USAGE: (layout) (output)\n");
    exit (2);
  }

  g = fopen (argv[2], "w");
  if (g == (FILE *) NULL) { 
    fprintf (stderr, "cannot open %s for output\n", argv[2]);
    exit (1);
  }

  /* load info from layout file */
  layout = LoadConfigFile (argv[1]);
  if (layout == (char *) NULL) {
    fprintf (stderr, "cannot open layout file %s\n", argv[1]);
    exit (1);
  }
  status = TRUE;
  status = status && (NULL == ScanConfig (layout, "NCCD",    "%d", 1, &Nccd));
  status = status && (NULL == ScanConfig (layout, "FILTER",  "%s", 1, filter));
  status = status && (NULL == ScanConfig (layout, "CAMERA",  "%s", 1, camera));
  status = status && (NULL == ScanConfig (layout, "TVSTOP",  "%s", 1, stop));
  status = status && (NULL == ScanConfig (layout, "TVSTART", "%s", 1, start));
  if (!status) {
    fprintf (stderr, "error in layout file\n");
    fprintf (stderr, "Nccd: %d\n", Nccd);
    fprintf (stderr, "filter: %s\n", filter);
    fprintf (stderr, "camera: %s\n", camera);
    fprintf (stderr, "tvstart: %s\n", start);
    fprintf (stderr, "tvstop: %s\n", stop);
    exit (1);
  }

  { /* save file creation date */ 
    struct timeval now;
    gettimeofday (&now, (struct timezone *) NULL);
    datestr = ohana_sec_to_date (now.tv_sec);
  }

  /* make phu header (no matrix needed) */
  gfits_init_header (&header);    
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  
  gfits_modify (&header, "NEXTEND",        "%d", 1, Nccd);
  gfits_modify (&header, "DATE",           "%s", 1, datestr);
  gfits_modify (&header, "TVSTART",        "%s", 1, start);
  gfits_modify (&header, "TVSTOP",         "%s", 1, stop);
  gfits_modify (&header, "VERSION",        "%s", 1, version);
  gfits_modify (&header, CameraKeyword,    "%s", 1, camera);
  gfits_modify (&header, ImagetypeKeyword, "%s", 1, "FRPTS");
  gfits_modify (&header, FilterKeyword,    "%s", 1, filter);
    
  gfits_fwrite_header  (g, &header);
  gfits_fwrite_matrix  (g, &matrix);

  ALLOCATE (xmin, double, 1);
  ALLOCATE (xmax, double, 1);
  ALLOCATE (ymin, double, 1);
  ALLOCATE (ymax, double, 1);

  for (i = 0; i < Nccd; i++) {

    /* load entry from layout file */
    sprintf (field, "CCD%02d", i);
    if (!ScanConfig (layout, field, "%s", 1, line)) {
      fprintf (stderr, "missing entry for %s\n", field);
      exit (1);
    }
    sscanf (line, "%s %s", extname, filename);

    /* load data from fringe points file */
    f = fopen (filename, "r");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "error opening data file %s\n", filename);
      exit (1);
    }

    NPTS = 100;
    Npts = 0;
    REALLOCATE (xmin, double, NPTS);
    REALLOCATE (xmax, double, NPTS);
    REALLOCATE (ymin, double, NPTS);
    REALLOCATE (ymax, double, NPTS);

    while (fscanf (f, "%lf %lf", &x, &y) == 2) {
      xmin[Npts] = x;
      ymin[Npts] = y;
      if (fscanf (f, "%lf %lf", &x, &y) != 2) {
	fprintf (stderr, "Funny line at %d?", Npts);
	goto done;
      }
      xmax[Npts] = x;
      ymax[Npts] = y;
      Npts ++;
      if (Npts == NPTS) {
	NPTS += 100;
	REALLOCATE (xmin, double, NPTS);
	REALLOCATE (xmax, double, NPTS);
	REALLOCATE (ymin, double, NPTS);
	REALLOCATE (ymax, double, NPTS);
      }
    }
  done:

    /* create table header */
    gfits_create_table_header (&theader, "TABLE", extname);
      
    /* add current date/time to header */
    gfits_modify (&theader, "DATE",          "%s", 1, datestr);
    gfits_modify (&theader, "TVSTART",       "%s", 1, start);
    gfits_modify (&theader, "TVSTOP",        "%s", 1, stop);
    gfits_modify (&header, CameraKeyword,    "%s", 1, camera);
    gfits_modify (&header, ImagetypeKeyword, "%s", 1, "FRPTS");
    gfits_modify (&header, FilterKeyword,    "%s", 1, filter);
    gfits_modify (&header, CCDnumKeyword,    "%s", 1, extname);
    
    
    /* define table layout */
    gfits_define_table_column (&theader, "F6.1", "X_MIN", "min couple x", "pixels"); 
    gfits_define_table_column (&theader, "F6.1", "Y_MIN", "min couple y", "pixels"); 
    gfits_define_table_column (&theader, "F6.1", "X_MAX", "max couple x", "pixels"); 
    gfits_define_table_column (&theader, "F6.1", "Y_MAX", "max couple y", "pixels"); 
    
    /* create table, add data values */
    gfits_create_table (&theader, &table);
  
    for (j = 0; j < Npts; j++) {
      row = gfits_table_print (&table, xmin[j], ymin[j], xmax[j], ymax[j]);
      gfits_add_rows (&table, row, 1, strlen (row));
    }

    gfits_fwrite_Theader (g, &theader);
    gfits_fwrite_table   (g, &table);
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
