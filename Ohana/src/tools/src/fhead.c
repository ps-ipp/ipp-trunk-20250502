# include <ohana.h>
# include <gfitsio.h>
# include <regex.h>

int main (int argc, char **argv) {

  int N, Extnum, Nextend;
  int i, j;
  off_t nbytes;
  Header head;
  char *p, *CCDKeyword, *Extname, extname[80];
  FILE *f;
  off_t Nbytes;
  regex_t preg;

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
  Nextend = 0;
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

  for (i = 1; i < argc; i++) {
    if (argc != 2) 
      fprintf (stdout, "------> %s <------\n", argv[i]);
    
    if (!Extnum && !Extname) {
      if (!gfits_read_header (argv[i], &head)) {
	continue;
      }
    }
    if (Extnum) {
      if (!gfits_read_Xheader (argv[i], &head, Nextend)) {
	continue;
      }
    } 
    if (Extname) {
      /* keep reading headers until we reach the one we want */
      Nextend = 0;
      f = fopen (argv[i], "r");
      if (f == NULL) continue;
      while (gfits_fread_header (f, &head)) {
	/* extract the EXTNAME (or other CCDKeyword) for this component (set to PHU for 0th component) */
	if (!gfits_scan (&head, CCDKeyword, "%s", 1, extname)) {
	  if (Nextend == 0) {
	    strcpy (extname, "PHU");
	  } else {
	    strcpy (extname, "UNKNOWN");
	  }
	}
	if (!regexec (&preg, extname, 0, NULL, 0)) {
	  goto done;
	}
    
	Nbytes = gfits_data_size (&head);
	fseeko (f, Nbytes, SEEK_CUR);
	Nextend ++;
      }
      // failed to find the desired header
      continue;
    }

  done:
      for (j = 79; j < head.datasize; j+= 80) {
	head.buffer[j] = 10;
      }

      p = gfits_header_field (&head, "END", 1);
      nbytes = p - head.buffer;
      fwrite (head.buffer, nbytes, 1, stdout);
      gfits_free_header (&head);

    }
    exit (0);
  }
