# include "data.h"

int nnet_list (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);
  if (argc != 1) {
    gprint (GP_ERR, "USAGE: nnet list\n");
    return FALSE;
  }

  ListNnets();
  return TRUE;
}

int nnet_delete (int argc, char **argv) {

  int status;
  Nnet *nnet;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: nnet delete (nnet)\n");
    return FALSE;
  }

  nnet = FindNnet (argv[1]);
  if (nnet == NULL) {
    gprint (GP_ERR, "nnet %s not found\n", argv[1]);
    return FALSE;
  }

  status = DeleteNnet (nnet);
  if (!status) abort ();

  return TRUE;
}

int nnet_create (int argc, char **argv) {

  int N;

  int LargeWeightInit = FALSE;
  if ((N = get_argument (argc, argv, "-large-weight-initializer"))) {
    remove_argument (N, &argc, argv);
    LargeWeightInit = TRUE;    
  }

  if (argc < 4) {
    gprint (GP_ERR, "USAGE: nnet create (nnet) (Ninput) [Nnodes] [Nnodes] ... (Noutput)\n");
    return FALSE;
  }

  int Nlayer = argc - 2;

  // nnet is guaranteed to be initialized with only name and the containers (but not vectors) allocated
  Nnet *nnet = CreateNnet (argv[1], Nlayer);

  // define the size of each layer
  for (int i = 0; i < Nlayer; i++) {
    nnet[0].Nnodes[i] = atoi (argv[i + 2]); // input (0), hidden layer nodes, output (Nlayer)
  }

  // allocate the vector and matrix data arrays (and init)
  CreateNnetData (nnet, LargeWeightInit);

  return TRUE;
}

int nnet_print (int argc, char **argv) {

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: nnet print (nnet)\n");
    return FALSE;
  }

  Nnet *nnet = FindNnet (argv[1]);
  if (nnet == NULL) {
    gprint (GP_ERR, "nnet %s not found, create it first\n", argv[1]);
    return FALSE;
  }

  PrintNnet (nnet);
  return TRUE;
}

int nnet_set (int argc, char **argv) {

  Buffer *matrix;
  Vector *vector;

  if (argc < 4) {
    gprint (GP_ERR, "USAGE: nnet set (nnet) [weights] [biases] ... [weights] [biases] : set nnet weights (images) and biases (vectors)\n");
    return FALSE;
  }

  Nnet *nnet = FindNnet (argv[1]);
  if (nnet == NULL) {
    gprint (GP_ERR, "nnet %s not found, create it first\n", argv[1]);
    return FALSE;
  }

  // compare argc and nnet[0].Nlayer 
  if (argc - 2 != (nnet[0].Nlayer - 1) * 2) {
    gprint (GP_ERR, "ERROR: invalid number of arguments\n");
    gprint (GP_ERR, "USAGE: nnet set (nnet) [weights] [biases] ... [weights] [biases] : set nnet weights (images) and biases (vectors)\n");
    return FALSE;
  }

  // check for existence of each matrix & vector (and check sizes)
  for (int L = 1; L < nnet[0].Nlayer; L++) {
    int Narg = 2*(L - 1) + 2;
    if ((matrix = SelectBuffer (argv[Narg + 0], OLDBUFFER, FALSE)) == NULL) {
      gprint (GP_ERR, "unknown input image %s\n", argv[Narg + 0]);
      gprint (GP_ERR, "USAGE: nnet set (nnet) [weights] [biases] ... [weights] [biases] : set nnet weights (images) and biases (vectors)\n");
      return (FALSE);    
    }
    if ((vector = SelectVector (argv[Narg + 1], OLDVECTOR, FALSE)) == NULL) {
      gprint (GP_ERR, "unknown input vector %s\n", argv[Narg + 1]);
      gprint (GP_ERR, "USAGE: nnet set (nnet) [weights] [biases] ... [weights] [biases] : set nnet weights (images) and biases (vectors)\n");
      return (FALSE);    
    }

    if (vector->Nelements != nnet[0].Nnodes[L]) {
      gprint (GP_ERR, "invalid bias vector length %s (%d vs %d)\n", argv[Narg + 1], vector->Nelements, nnet[0].Nnodes[L]);
      gprint (GP_ERR, "USAGE: nnet set (nnet) [weights] [biases] ... [weights] [biases] : set nnet weights (images) and biases (vectors)\n");
      return (FALSE);    
    }

    if (matrix->matrix.Naxis[0] != nnet[0].Nnodes[L-1]) {
      gprint (GP_ERR, "invalid weight image size for %s (Nx = %d vs %d)\n", argv[Narg + 0], (int) matrix->matrix.Naxis[0], nnet[0].Nnodes[L-1]);
      gprint (GP_ERR, "USAGE: nnet set (nnet) [weights] [biases] ... [weights] [biases] : set nnet weights (images) and biases (vectors)\n");
      return (FALSE);    
    }

    if (matrix->matrix.Naxis[1] != nnet[0].Nnodes[L]) {
      gprint (GP_ERR, "invalid weight image size for %s (Ny = %d vs %d)\n", argv[Narg + 0], (int) matrix->matrix.Naxis[1], nnet[0].Nnodes[L]);
      gprint (GP_ERR, "USAGE: nnet set (nnet) [weights] [biases] ... [weights] [biases] : set nnet weights (images) and biases (vectors)\n");
      return (FALSE);    
    }

  }

  for (int L = 1; L < nnet[0].Nlayer; L++) {

    int Narg = 2*(L - 1) + 2;
    matrix = SelectBuffer (argv[Narg + 0], OLDBUFFER, FALSE); // I've checked the argv entry above so I do not need to check again...
    vector = SelectVector (argv[Narg + 1], OLDVECTOR, FALSE);

    opihi_flt *vecvalue = vector[0].elements.Flt;
    for (int i = 0; i < nnet[0].Nnodes[L]; i++) {
      nnet[0].biases[L][i] = vecvalue[i];
    }

    float *matvalue = (float *) matrix[0].matrix.buffer;
    for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
      for (int i = 0; i < nnet[0].Nnodes[L-1]; i++) {
	int k = i + j*nnet[0].Nnodes[L-1];
	nnet[0].weight[L][k] = matvalue[k];
      }
    }
  
  }

  return TRUE;
}

int nnet_read (int argc, char **argv) {

  # define D_LINE 0x10000
  char word[128];
  char line[D_LINE];

  if (argc < 3) {
    gprint (GP_ERR, "USAGE: nnet read (nnet) (filename) : set nnet weights and biases based on a file\n");
    gprint (GP_ERR, "the first line of the file specifies the number of layers, each layer in the network is written as a matrix of numbers (weights) and a vector\n");
    return FALSE;
  }

  // open the file and read the number of layers
  FILE *f = fopen (argv[2], "r");
  if (f == NULL) {
    gprint (GP_ERR, "USAGE: nnet read (nnet) (filename) : set nnet weights and biases based on a file\n");
    gprint (GP_ERR, "file %s could not be opened\n", argv[2]);
    return FALSE;
  }

  // read the number of layers
  // NLAYER 3
  int Nlayer;
  scan_line_maxlen (f, line, D_LINE);
  sscanf (line, "%127s %d", word, &Nlayer);
  if (strcmp(word, "NLAYER")) {
    gprint (GP_ERR, "warning: NLAYER keyword not found\n");
  }

  Nnet *nnet = CreateNnet (argv[1], Nlayer);

  // read the number of nodes
  // LAYERS 2 4 2
  scan_line_maxlen (f, line, D_LINE);
  char *tmpword = getword (line);
  if (strcmp (tmpword, "LAYERS")) {
    gprint (GP_ERR, "warning: LAYERS keyword not found\n");
  }
  FREE (tmpword);
  for (int i = 0; i < Nlayer; i++) {
    int Nnode;
    int status = iparse (&Nnode, i + 2, line); // numbering is fields 1 2 3
    if (!status) {
      gprint (GP_ERR, "error: failed to find all Nnode values\n");
      fclose (f);
      DeleteNnet (nnet);
      return FALSE;
    }
    nnet[0].Nnodes[i] = Nnode;
  }

  // this creates the data for each node and inits with gaussian weights
  CreateNnetData (nnet, FALSE);

  // read the weights and biases from the data file
  for (int L = 1; L < nnet[0].Nlayer; L++) {
    
    char word1[128], word2[128], word3[128];
    int Nx, Ny, layer;
    scan_line_maxlen (f, line, D_LINE);
    sscanf (line, "%127s %d %127s %d %127s %d", word1, &layer, word2, &Nx, word3, &Ny);
    gprint (GP_ERR, "LAYER: %d, Nx: %d, Ny: %d\n", layer, Nx, Ny);

    if (layer != L - 1) {
      gprint (GP_ERR, "warning: expect layer = %d, got %d\n", L - 1, layer);
    }
    if (Nx != nnet[0].Nnodes[L - 1]) {
      gprint (GP_ERR, "warning: expect Nx = %d, got %d\n", nnet[0].Nnodes[L - 1], Nx);
    }
    if (Ny != nnet[0].Nnodes[L]) {
      gprint (GP_ERR, "warning: expect Ny = %d, got %d\n", nnet[0].Nnodes[L], Ny);
    }

    double value;
    for (int j = 0; j < Ny; j++) {
      scan_line_maxlen (f, line, D_LINE);
      for (int i = 0; i < Nx; i++) {
	int k = i + j*nnet[0].Nnodes[L-1];
	int status = dparse (&value, i + 1, line);
	if (!status) {
	  gprint (GP_ERR, "error: failed to read entry from a line (layer: %d, i: %d, j: %d)\n", layer, i, j);
	  fclose (f);
	  DeleteNnet (nnet);
	  return FALSE;
	}
	nnet[0].weight[L][k] = value;
      }
      dparse (&value, Nx + 1, line);
      nnet[0].biases[L][j] = value;
    }
  }

  return TRUE;
}

int nnet_write (int argc, char **argv) {

  if (argc < 3) {
    gprint (GP_ERR, "USAGE: nnet write (nnet) (filename) : save nnet weights and biases to a file\n");
    return FALSE;
  }

  Nnet *nnet = FindNnet (argv[1]);
  if (nnet == NULL) {
    gprint (GP_ERR, "nnet %s not found, create it first\n", argv[1]);
    return FALSE;
  }

  // open the file and read the number of layers
  FILE *f = fopen (argv[2], "w");
  if (f == NULL) {
    gprint (GP_ERR, "USAGE: nnet write (nnet) (filename) : save nnet weights and biases to a file\n");
    gprint (GP_ERR, "file %s could not be opened\n", argv[2]);
    return FALSE;
  }

  // write the number of layers
  // NLAYER 3
  fprintf (f, "NLAYER %d\n", nnet->Nlayer);

  // write the number of nodes for each layer
  // LAYERS 2 4 2
  fprintf (f, "LAYERS");
  for (int i = 0; i < nnet->Nlayer; i++) {
    fprintf (f, " %d", nnet->Nnodes[i]);
  }
  fprintf (f, "\n");

  // read the weights and biases from the data file
  for (int L = 1; L < nnet[0].Nlayer; L++) {
    
    fprintf (f, "LAYER %d NX %d NY %d\n", L - 1, nnet->Nnodes[L-1], nnet->Nnodes[L]);

    for (int j = 0; j < nnet->Nnodes[L]; j++) {
      for (int i = 0; i < nnet->Nnodes[L-1]; i++) {
	int k = i + j*nnet[0].Nnodes[L-1];
	fprintf (f, "%f ", nnet[0].weight[L][k]);
      }
      fprintf (f, "%f\n", nnet[0].biases[L][j]);
    }
  }
  fclose (f);

  return TRUE;
}

int nnet_get (int argc, char **argv) {

  Buffer *matrix;
  Vector *vector;

  if (argc < 4) {
    gprint (GP_ERR, "USAGE: nnet get (nnet) [weights] [biases] ... [weights] [biases] : get nnet weights (images) and biases (vectors)\n");
    return FALSE;
  }

  Nnet *nnet = FindNnet (argv[1]);
  if (nnet == NULL) {
    gprint (GP_ERR, "nnet %s not found, create it first\n", argv[1]);
    return FALSE;
  }

  // compare argc and nnet[0].Nlayer 
  if (argc - 2 != (nnet[0].Nlayer - 1) * 2) {
    gprint (GP_ERR, "ERROR: invalid number of arguments\n");
    gprint (GP_ERR, "USAGE: nnet set (nnet) [weights] [biases] ... [weights] [biases] : set nnet weights (images) and biases (vectors)\n");
    return FALSE;
  }

  // check for existence of each matrix & vector (and check sizes)
  for (int L = 1; L < nnet[0].Nlayer; L++) {
    int Narg = 2*(L - 1) + 2;
    if ((matrix = SelectBuffer (argv[Narg + 0], ANYBUFFER, FALSE)) == NULL) {
      gprint (GP_ERR, "unknown input image %s\n", argv[Narg + 0]);
      gprint (GP_ERR, "USAGE: nnet set (nnet) [weights] [biases] ... [weights] [biases] : set nnet weights (images) and biases (vectors)\n");
      return (FALSE);    
    }
    if ((vector = SelectVector (argv[Narg + 1], ANYVECTOR, FALSE)) == NULL) {
      gprint (GP_ERR, "unknown input vector %s\n", argv[Narg + 1]);
      gprint (GP_ERR, "USAGE: nnet set (nnet) [weights] [biases] ... [weights] [biases] : set nnet weights (images) and biases (vectors)\n");
      return (FALSE);    
    }
  }

  for (int L = 1; L < nnet[0].Nlayer; L++) {

    int Narg = 2*(L - 1) + 2;
    matrix = SelectBuffer (argv[Narg + 0], OLDBUFFER, FALSE); // I've checked the argv entry above so I do not need to check again...
    vector = SelectVector (argv[Narg + 1], OLDVECTOR, FALSE);

    ResetVector (vector, OPIHI_FLT, nnet[0].Nnodes[L]);
    ResetBuffer (matrix, nnet[0].Nnodes[L-1], nnet[0].Nnodes[L], -32, 0.0, 1.0);

    opihi_flt *vecvalue = vector[0].elements.Flt;
    for (int i = 0; i < nnet[0].Nnodes[L]; i++) {
      vecvalue[i] = nnet[0].biases[L][i];
    }

    float *matvalue = (float *) matrix[0].matrix.buffer;
    for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
      for (int i = 0; i < nnet[0].Nnodes[L-1]; i++) {
	int k = i + j*nnet[0].Nnodes[L-1];
	matvalue[k] = nnet[0].weight[L][k];
      }
    }
  
  }

  return TRUE;
}

