# include <ohana.h>
# include <gfitsio.h>
# include <sys/types.h>
# include <regex.h>

int print_fields (char *filename, char *extname, Header *header, int argc, char **argv);

int usage () {

  fprintf (stderr, "USAGE: fields [options] [KEYWORD] [KEYWORD]...\n");
  fprintf (stderr, " reads filenames from stdin, writes header values to stdout\n");
  fprintf (stderr, " options:\n");
  fprintf (stderr, " -x (extnum)  : PHU is -1, followed by 0,1,2...\n");
  fprintf (stderr, " -n (extname) : name may include a regex for EXTNAME values\n");

  exit (2);
}

int main (int argc, char **argv) {

  FILE *f;
  Header header;
  char filename[1000], *CCDKeyword, *Extname, extname[80];
  int N, Extnum, Nextend, status, GotFile, GotField, GotExtension;
  off_t Nbytes;
  regex_t preg;

  Nextend = 0;
  GotExtension = FALSE;

  if (get_argument (argc, argv, "-h")) usage();
  if (get_argument (argc, argv, "--help")) usage();

  CCDKeyword = NULL;
  if ((N = get_argument (argc, argv, "-keyword"))) {
    remove_argument (N, &argc, argv);
    CCDKeyword = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (CCDKeyword == NULL) {
    CCDKeyword = strcreate ("EXTNAME");
  }

  Extnum = FALSE;
  if ((N = get_argument (argc, argv, "-x"))) {
    Extnum = TRUE;
    remove_argument (N, &argc, argv);
    Nextend = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  Extname = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    Extname = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    regcomp (&preg, Extname, REG_EXTENDED);
  }

  GotFile  = TRUE; 
  GotField = TRUE;

  while (fscanf (stdin, "%s", filename) != EOF) {
    if (!Extnum && !Extname) {
      if (!gfits_read_header (filename, &header)) continue;
      GotFile = TRUE;
      GotField &= print_fields (filename, NULL, &header, argc, argv);
      GotExtension = TRUE;
      continue;
    }
    if (Extnum) {
      if (!gfits_read_Xheader (filename, &header, Nextend)) continue;
      GotFile = TRUE;
      GotField &= print_fields (filename, NULL, &header, argc, argv);
      GotExtension = TRUE;
      continue;
    } 

    if (Extname) {
      /* keep reading headers, only parse fields for matching headers */
      Nextend = 0;
      GotExtension = FALSE;
      f = fopen (filename, "r");
      if (f == NULL) {
	GotFile = FALSE;
	continue;
      }
      while (gfits_fread_header (f, &header)) {
	/* extract the EXTNAME (or other CCDKeyword) for this component (set to PHU for 0th component) */
	status = gfits_scan (&header, CCDKeyword, "%s", 1, extname);
	if (!status) {
	  if (Nextend == 0) {
	    strcpy (extname, "PHU");
	  } else {
	    strcpy (extname, "UNKNOWN");
	  }
	}
	// we are checking each extension to see if it matches the regex.
	if (!regexec (&preg, extname, 0, NULL, 0)) {
	  GotField &= print_fields (filename, extname, &header, argc, argv);
	  GotExtension = TRUE;
	}   
    
	Nbytes = gfits_data_size (&header);
	fseeko (f, Nbytes, SEEK_CUR);
	Nextend ++;

	// this implementation is very inefficient!
	// since we are checking each header for a regex, we should
	// loop over all extensions once.  the function below reads the
	// entire file a second time
	GotFile &= gfits_read_Xheader (filename, &header, Nextend);
	continue;
      } 
      fclose (f);
      if (Nextend == 0) {
	GotFile = FALSE;
      }
    }
  }

  if (Extname) regfree (&preg);

  if (!GotFile) exit (1);
  if (!GotField) exit (2);
  if (!GotExtension) exit (3);
  exit (0);
}

int print_fields (char *filename, char *extname, Header *header, int argc, char **argv) {

  int i, GotField;
  char buffer[1000];

  GotField = TRUE;

  if (extname) {
    fprintf (stdout, "%s[%s]  ", filename, extname);
  } else {
    fprintf (stdout, "%s  ", filename);
  }
  for (i = 1; i < argc; i++) {
    bzero (buffer, 1000);
    GotField &= gfits_scan (header, argv[i], "%s", 1, buffer);
    stripwhite (buffer);
    fprintf (stdout, "%s  ", buffer);
  }
  fprintf (stdout, "\n");
  gfits_free_header (header);
  return (GotField);
}
