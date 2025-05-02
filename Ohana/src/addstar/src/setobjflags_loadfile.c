# include "addstar.h"
# include "setobjflags.h"
void sort_exp (int *I, int *T, int *F, int *S, int N);
int expname_to_int (char *expname);
int value_to_sequence (int *sequence, int Nsequence, int value);

MyStars *setobjflags_loadfile_mops (char *metadata, char *detections, int mybit, int *Nstars);
MyStars *setobjflags_loadfile_hern (char *filename, int *Nstars);

/* read ASCII file with ref star data */
MyStars *setobjflags_loadfile (int *Nstars) {

  int Nmops;
  MyStars *mopsStars = setobjflags_loadfile_mops ("md.fix.dat", "mops_export.txt", ID_OBJ_HAS_SOLSYS_DET, &Nmops);

  int Nhern;
  MyStars *hernStars = setobjflags_loadfile_hern ("NH_qso_rrlyrae_candidates_catalog.fits", &Nhern);

  // merge into a single table
  int Ntotal = Nhern + Nmops;
  REALLOCATE (mopsStars, MyStars, Ntotal);
  for (int i = Nmops, j = 0; i < Ntotal; i++, j++) {
    mopsStars[i] = hernStars[j];
  }
  free (hernStars);

  *Nstars = Ntotal;
  return mopsStars;
}

MyStars *setobjflags_loadfile_mops (char *metadata, char *detections, int mybit, int *Nstars) {

  // code to read in the MOPS detection dump
  int Nexp = 0;
  int NEXP = 1000;
  ALLOCATE_PTR (expTIM,    int, NEXP);
  ALLOCATE_PTR (expSEQ,    int, NEXP);
  ALLOCATE_PTR (expFLT,    int, NEXP);
  ALLOCATE_PTR (expIDX,    int, NEXP);

  // load the metadata file:
  FILE *f = fopen (metadata, "r");
  if (f == NULL) Shutdown ("can't read data from %s", metadata);
    
  /* read in stars line-by-line */
  char line[1024];
  while (scan_line (f, line) != EOF) {
    if (Nexp % 1000 == 0) fprintf (stderr, ".");

    char filter[64], expname[64];
    double expMJD;

    int status = sscanf(line, "%lf %s %s", &expMJD, filter, expname);
    if (status != 3) {
      fprintf (stderr, "error reading line from MD: %s\n", line);
      continue;
    }

    // XXX NOTE this does not handle gpc2 images
    expIDX[Nexp] = expname_to_int (expname);

    switch (filter[0]) {
      case 'g': expFLT[Nexp] = 0; break;
      case 'r': expFLT[Nexp] = 1; break;
      case 'i': expFLT[Nexp] = 2; break;
      case 'z': expFLT[Nexp] = 3; break;
      case 'y': expFLT[Nexp] = 4; break;
      case 'w': expFLT[Nexp] = 5; break;
      default: myAbort ("oops");
    }

    expSEQ[Nexp] = Nexp;
    expTIM[Nexp] = ohana_mjd_to_sec (expMJD);

    Nexp++ ;
    if (Nexp == NEXP) {
      NEXP += 1000;
      REALLOCATE (expTIM,    int, NEXP);
      REALLOCATE (expSEQ,    int, NEXP);
      REALLOCATE (expFLT,    int, NEXP);
      REALLOCATE (expIDX,    int, NEXP);
    }
  }
  fclose (f);
  sort_exp (expIDX, expTIM, expFLT, expSEQ, Nexp);

  fprintf (stderr, "done reading exposure metadata\n");

  MyStars *stars = NULL;
  int NSTARS = 1000;
  ALLOCATE (stars, MyStars, NSTARS);

  // load the detection file:
  f = fopen (detections, "r");
  if (f == NULL) Shutdown ("can't read data from %s", detections);
    
  scan_line (f, line); // skip the first line

  /* read in stars line-by-line */
  int N = 0, Nbad = 0, Nmiss = 0;
  while (scan_line (f, line) != EOF) {
    if (N % 1000 == 0) fprintf (stderr, ".");
    stripwhite (line);
    if (line[0] == 0) continue;
    if (line[0] == '#') continue;
      
    char expname[64];
    int status = sscanf (line, "%s %lf %lf", expname, &stars[N].R, &stars[N].D);
    if (status != 3) {
      if (Nbad < 20) fprintf (stderr, "error reading line: %s\n", line);
      Nbad ++;
      continue;
    }
    stars[N].R = ohana_normalize_angle (stars[N].R);
      
    int expID = expname_to_int (expname);
    int seq = value_to_sequence (expIDX, Nexp, expID);
    if (seq < 0) {
      if (Nmiss < 20) fprintf (stderr, "error missing expname: %s\n", expname);
      Nmiss ++;
    }

    stars[N].myBit = mybit;
      
    // stars[N].time   = expTIM[seq];
    // stars[N].filter = expFLT[seq];
    stars[N].flag   = FALSE;
    stars[N].found  = FALSE;
      
    N++;
    CHECK_REALLOCATE (stars, MyStars, NSTARS, N, 1000);
  }
  fclose (f);
  *Nstars = N;

  fprintf (stderr, "done reading mops detections\n");

  FREE (expTIM);
  FREE (expSEQ);
  FREE (expFLT);
  FREE (expIDX);

  return (stars);
}

# define GET_COLUMN(OUT,NAME,TYPE)					\
  TYPE *OUT = gfits_get_bintable_column_data (&theader, &ftable, NAME, type, &Nrow, &Ncol); \
  myAssert (!strcmp(type, #TYPE), "wrong column type");

# define MIN_QSO_P60_VALUE 0.60
# define MIN_QSO_P05_VALUE 0.05

# define MIN_RRLYRA_P60_VALUE 0.60
# define MIN_RRLYRA_P05_VALUE 0.05

MyStars *setobjflags_loadfile_hern (char *filename, int *Nstars) {

  int i, Ncol;
  off_t Nrow;
  char type[16];

  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  header.buffer = NULL;
  matrix.buffer = NULL;
  ftable.buffer = NULL;
  theader.buffer = NULL;
  
  FILE *f = fopen (filename, "r");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open image subset file %s\n", filename);
    return NULL;
  }

  /* load in PHU segment (ignore) */
  if (!gfits_fread_header (f, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset header\n");
    goto escape;
  }
  if (!gfits_fread_matrix (f, &matrix, &header)) {
    if (VERBOSE) fprintf (stderr, "can't read image subset matrix\n");
    goto escape;
  }

  ftable.header = &theader;

  // load data for this header 
  if (!gfits_load_header (f, &theader)) goto escape;
  
  // read the fits table bytes
  if (!gfits_fread_ftable_data (f, &ftable, FALSE)) goto escape;
  
  // need to create and assign to flat-field correction
  GET_COLUMN(R,         "ra",   	float);
  GET_COLUMN(D,         "dec",  	float);
  GET_COLUMN(pQSO,      "p_QSO",  	float);
  GET_COLUMN(pRRLyra,   "p_RRLyrae",  	float);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);

  MyStars *stars = NULL;
  ALLOCATE (stars, MyStars, Nrow);

  for (i = 0; i < Nrow; i++) {
    stars[i].R      = R[i];         
    stars[i].D      = D[i];
    // stars[i].time   = 0;
    // stars[i].filter = 0;
    stars[i].myBit  = ID_OBJ_HERN_VARIABLE; 

    if (pQSO[i] >= MIN_QSO_P60_VALUE) {
      stars[i].myBit |= ID_OBJ_HERN_QSO_P60;
    }
    if (pQSO[i] >= MIN_QSO_P05_VALUE) {
      stars[i].myBit |= ID_OBJ_HERN_QSO_P05;
    }
    if (pRRLyra[i] >= MIN_RRLYRA_P60_VALUE) {
      stars[i].myBit |= ID_OBJ_HERN_RRL_P60;
    }
    if (pRRLyra[i] >= MIN_RRLYRA_P05_VALUE) {
      stars[i].myBit |= ID_OBJ_HERN_RRL_P05;
    }
    stars[i].flag   = FALSE;
    stars[i].found  = FALSE;
  }

  FREE (R);
  FREE (D);
  FREE (pQSO);
  FREE (pRRLyra);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  fclose (f);
  
  *Nstars = Nrow;
  return (stars);

 escape:
  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table  (&ftable);

  fclose (f);
  return NULL;
}

int expname_to_int (char *expname) {
  
  // XXX NOTE this does not handle gpc2 images
  myAssert (expname[ 0] == 'o', "invalid exposure name");
  myAssert (expname[ 5] == 'g', "invalid exposure name");
  myAssert (expname[10] == 'o', "invalid exposure name");
  int mjdExp = atoi(&expname[1]);
  int seqExp = atoi(&expname[6]);
  int value  = mjdExp * 10000 + seqExp;
  return value;
}

void sort_exp (int *I, int *T, int *F, int *S, int N) {

# define SWAPFUNC(A,B){ int itmp; 	\
    itmp = I[A]; I[A] = I[B]; I[B] = itmp;	\
    itmp = T[A]; T[A] = T[B]; T[B] = itmp;	\
    itmp = F[A]; F[A] = F[B]; F[B] = itmp;	\
    itmp = S[A]; S[A] = S[B]; S[B] = itmp;	\
  }
# define COMPARE(A,B)(I[A] < I[B])
  
  OHANA_SORT (N, COMPARE, SWAPFUNC);
  
# undef SWAPFUNC
# undef COMPARE
  
}

// find the entry by bisection:
int value_to_sequence (int *sequence, int Nsequence, int value) {

  int Nlo = 0; 
  int Nhi = Nsequence - 1;

  if (value < sequence[Nlo]) return -1;
  if (value > sequence[Nhi]) return -1;

  int N;
  while (Nhi - Nlo > 4) {
    N = 0.5*(Nlo + Nhi);
    if (sequence[N] < value) {
      Nlo = MAX(N, 0);
    } else {
      Nhi = MIN(N + 1, Nsequence - 1);
    }
  }
  // mySequence->value[Nlo] <  value 
  // mySequence->value[Nhi] >= value

  for (N = Nlo; N <= Nhi; N++) {
    if (sequence[N] == value) {
      return N;
    }
  }
  return -1;
}

