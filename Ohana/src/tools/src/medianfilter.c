# include <ohana.h>
# include <gfitsio.h>

# define D_NFILES 100

int main (int argc, char **argv) {

  int i, j, k, n, Npixels, Npixin, Npixrd, Npixlast, N;
  int Npass, NFILES, Nfiles, Nvalid;

  char **filename;
  Header head, *tmphead;
  Matrix out,  *tmpmatr;
  float *list, *v, *O;
  float fmin, fmax, Nval, sum, MinValid;

  if ((N = get_argument (argc, argv, "-unsign"))) {
    remove_argument (N, &argc, argv);
    gfits_set_unsign_mode (TRUE);
  }

  MinValid = 1.0;
  if ((N = get_argument (argc, argv, "-bad"))) {
    remove_argument (N, &argc, argv);
    MinValid = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc < 4) {
    fprintf (stderr, "USAGE: minmaxfilter outfile fmin fmax\n");
    exit (0);
  }
  fmin = atof (argv[2]); 
  fmax = atof (argv[3]); 

  /* read in the filenames */
  NFILES = D_NFILES;
  ALLOCATE (filename, char *, NFILES);
  ALLOCATE (filename[0], char, 1024);
  for (i = 0; fscanf (stdin, "%s", filename[i]) != EOF; i++) {
    if (i == NFILES - 1) {
      NFILES += D_NFILES;
      REALLOCATE (filename, char *, NFILES);
    }
    ALLOCATE (filename[i+1], char, 1024);
  }
  Nfiles = i;
  REALLOCATE (filename, char *, Nfiles);

  /* load a header, setup output header, matrix */
  gfits_read_header (filename[0], &head);
  gfits_create_matrix (&head, &out);
  gfits_convert_format (&head, &out, -32, 1.0, 0.0, 0xffff, FALSE);

  /* find size of temporary images */
  Npixels = head.Naxis[0]*head.Naxis[1];
  Npixin = Npixels / Nfiles;
  Npixlast = Npixels - Npixin*Nfiles;
  if (Npixlast == 0) {
    Npass = Nfiles;
    Npixlast = Npixin;
    fprintf (stderr, "operating on %d pixels per pass\n", Npixin);
  } else {
    Npass = Nfiles + 1;
    fprintf (stderr, "operating on %d pixels per pass, %d in last\n", Npixin, Npixlast);
  }
  
  ALLOCATE (tmphead, Header, Nfiles);
  ALLOCATE (tmpmatr, Matrix, Nfiles);
  ALLOCATE (list, float, Nfiles);

  /*
  Nval = 0;
  for (k = fmin*Nfiles; k < fmax*Nfiles; k++) {
    Nval += 1.0;
  }
  */

  O = (float *) out.buffer;
  for (n = 0; n < Npass; n++) {
    fprintf (stderr, "pass %d\n", n);
    Npixrd = (n == Npass - 1) ? Npixlast : Npixin;
    for (i = 0; i < Nfiles; i++) {
      fprintf (stderr, ".");
      if (!gfits_read_header  (filename[i], &tmphead[i])) {
	fprintf (stderr, "trouble reading file %s\n", filename[i]);
	exit (1);
      }
      gfits_read_portion (filename[i], &tmpmatr[i], n*Npixin, Npixrd);
      tmphead[i].Naxis[0] = 1;
      tmphead[i].Naxis[1] = Npixrd;
      gfits_convert_format (&tmphead[i], &tmpmatr[i], -32, 1.0, 0.0, 0xffff, FALSE);
    }
    
    fprintf (stderr, "starting sorts\n");
    for (j = 0; j < Npixrd; j++, O++) {
      Nvalid = 0;
      int debug = FALSE && (n == 5) && (j >= 1776779) && (j <= 1776789);
      for (k = 0; k < Nfiles; k++) {
	v = (float *)tmpmatr[k].buffer;
	if (debug) {
	    fprintf (stderr, "%8.2f ", v[j]);
	}
	if (v[j] < MinValid) continue;
	if (isnan(v[j])) {
	    continue;
	}
	if (isinf(v[j])) {
	    continue;
	}
	list[Nvalid] = v[j];
	Nvalid ++;
      }
      if (debug) fprintf (stderr, "\n");
      if (Nvalid == 0) {
	*O = NAN;
	if (debug) {
	    fprintf (stderr, "Nvalid is 0\n");
	}
	continue;
      }
      fsort (list, Nvalid);
      sum = 0;
      Nval = 0;
      if (debug) fprintf (stderr, "list : ");
      for (k = fmin*Nvalid; k < fmax*Nvalid; k++) {
	if (debug) {
	    fprintf (stderr, "%8.2f ", list[k]);
	}
	sum += list[k];
	Nval ++;
      }
      *O = (sum / Nval);
      if (debug) {
	  fprintf (stderr, " = %f / %f = %f\n", sum, Nval, *O);
	  fprintf (stderr, "\n");
      }
    }

    for (i = 0; i < Nfiles; i++) {
      fprintf (stderr, ",");
      gfits_free_header (&tmphead[i]);
      gfits_free_matrix (&tmpmatr[i]);
    }
    
  }

  gfits_write_header (argv[1], &head);
  gfits_write_matrix (argv[1], &out);

  return (0);

}
