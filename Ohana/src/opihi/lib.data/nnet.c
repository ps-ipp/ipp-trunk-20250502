# include "data.h"

static Nnet **nnets  = NULL; /* nnet to store the list of all nnets */
static int    Nnnets = 0;    /* number of currently defined nnets */
static int    NNNETS = 0;    /* number of currently allocated nnets */

void InitNnets () {
  Nnnets = 0;
  NNNETS = 16;
  ALLOCATE (nnets, Nnet *, NNNETS); 
}

void FreeNnets () {

  int i;

  if (!nnets) InitNnets();

  for (i = 0; i < Nnnets; i++) {
    FreeNnetData (nnets[i]);
    free (nnets[i]);
  }
  free (nnets);
}

// InitNnetData requires nnet to unassigned or initialized
// if you have an existing Nnet, call FreeNnetData first
void InitNnetData (Nnet *nnet, char *name, int Nlayer) {

  nnet[0].name = strcreate (name);

  nnet[0].Nlayer  = 0;
  nnet[0].Nnodes  = NULL;
  nnet[0].weight  = NULL;
  nnet[0].biases  = NULL;
  nnet[0].svalue  = NULL;
  nnet[0].zvalue  = NULL;
  nnet[0].sprime  = NULL;
  nnet[0].delta   = NULL;

  nnet[0]. Nabla_b  = NULL;
  nnet[0].dNabla_b  = NULL;
  nnet[0]. Nabla_w  = NULL;
  nnet[0].dNabla_w  = NULL;

  nnet[0].Nlayer = Nlayer;
  ALLOCATE_ZERO (nnet[0].Nnodes, int, Nlayer);
  ALLOCATE_ZERO (nnet[0].weight, float *, Nlayer);
  ALLOCATE_ZERO (nnet[0].biases, float *, Nlayer);
  ALLOCATE_ZERO (nnet[0].svalue, float *, Nlayer);
  ALLOCATE_ZERO (nnet[0].zvalue, float *, Nlayer);
  ALLOCATE_ZERO (nnet[0].sprime, float *, Nlayer);
  ALLOCATE_ZERO (nnet[0].delta , float *, Nlayer);

  ALLOCATE_ZERO (nnet[0]. Nabla_b, float *, Nlayer);
  ALLOCATE_ZERO (nnet[0].dNabla_b, float *, Nlayer);
  ALLOCATE_ZERO (nnet[0]. Nabla_w, float *, Nlayer);
  ALLOCATE_ZERO (nnet[0].dNabla_w, float *, Nlayer);
}

void FreeNnetData (Nnet *nnet) {

    int i;

    FREE (nnet[0].name);
    for (i = 0; i < nnet[0].Nlayer; i++) {
      FREE (nnet[0].weight[i]);
      FREE (nnet[0].biases[i]);
      FREE (nnet[0].svalue[i]);
      FREE (nnet[0].zvalue[i]);
      FREE (nnet[0].sprime[i]);
      FREE (nnet[0].delta [i]);

      FREE (nnet[0]. Nabla_b[i]);
      FREE (nnet[0].dNabla_b[i]);
      FREE (nnet[0]. Nabla_w[i]);
      FREE (nnet[0].dNabla_w[i]);
    }
    FREE (nnet[0].Nnodes);
    FREE (nnet[0].weight);
    FREE (nnet[0].biases);
    FREE (nnet[0].svalue);
    FREE (nnet[0].zvalue);
    FREE (nnet[0].sprime);
    FREE (nnet[0].delta );

    FREE (nnet[0]. Nabla_b);
    FREE (nnet[0].dNabla_b);
    FREE (nnet[0]. Nabla_w);
    FREE (nnet[0].dNabla_w);
}

/* return the given nnet */
Nnet *GetNnet (int where) {

  if (!nnets) InitNnets();

  if (where < 0) where += Nnnets;
  if (where < 0) return NULL;
  if (where >= Nnnets) return NULL;
  return (nnets[where]);
}

/* return the given nnet */
Nnet *FindNnet (char *name) {

  int i;

  if (!nnets) InitNnets();

  for (i = 0; i < Nnnets; i++) {
    if (!strcmp (nnets[i][0].name, name)) {
      return (nnets[i]);
    }
  }
  return (NULL);
}

/* make a new named nnet with Nlayer */
Nnet *CreateNnet (char *name, int Nlayer) {

  int N;
  Nnet *nnet;

  if (!nnets) InitNnets();

  nnet = FindNnet (name);
  if (nnet != NULL) {
    FreeNnetData (nnet);
    InitNnetData (nnet, name, Nlayer);
    return (nnet);
  }

  N = Nnnets;
  Nnnets ++;
  CHECK_REALLOCATE (nnets, Nnet *, NNNETS, Nnnets, 16);
  ALLOCATE (nnet, Nnet, 1);
  InitNnetData (nnet, name, Nlayer);

  // save this nnet in the static array of nnets
  nnets[N] = nnet;
  return (nnet);
}

void CreateNnetData (Nnet *nnet, int LargeWeightInit) {

  ohana_gaussdev_init ();

  // NOTE : none of these elements are used for the first layer (the input layer)
  // EXCEPT svalue[0]
  
  nnet[0].weight[0] = NULL;
  nnet[0].biases[0] = NULL;
  nnet[0].zvalue[0] = NULL;
  nnet[0].sprime[0] = NULL;
  nnet[0].delta [0] = NULL;
  nnet[0]. Nabla_b[0] = NULL;
  nnet[0].dNabla_b[0] = NULL;
  nnet[0]. Nabla_w[0] = NULL;
  nnet[0].dNabla_w[0] = NULL;

  ALLOCATE (nnet[0].svalue[0], float, nnet[0].Nnodes[0]);

  for (int i = 1; i < nnet[0].Nlayer; i++) {
    ALLOCATE (nnet[0].biases[i], float, nnet[0].Nnodes[i]);  // biases for each node in the hidden and output layers only
    for (int j = 0; j < nnet[0].Nnodes[i]; j++) {
      nnet[0].biases[i][j] = ohana_gaussdev_rnd (0.0, 1.0);
    }

    float sigma = LargeWeightInit ? 1.0 : 1.0 / sqrt(nnet[0].Nnodes[i-1]);

    ALLOCATE (nnet[0].weight[i], float, nnet[0].Nnodes[i-1]*nnet[0].Nnodes[i]);  // weight connected each node in the previous layer to the current layer (excludes input layer)
    for (int j = 0; j < nnet[0].Nnodes[i-1]*nnet[0].Nnodes[i]; j++) {
      nnet[0].weight[i][j] = ohana_gaussdev_rnd (0.0, sigma);
    }

    ALLOCATE (nnet[0].svalue[i], float, nnet[0].Nnodes[i]);  // vectors for holding results / values for each node in the input, hidden, output layers
    ALLOCATE (nnet[0].zvalue[i], float, nnet[0].Nnodes[i]);  // vectors for holding results / values for each node in the input, hidden, output layers
    ALLOCATE (nnet[0].sprime[i], float, nnet[0].Nnodes[i]);  // vectors for holding results / values for each node in the input, hidden, output layers
    ALLOCATE (nnet[0].delta [i], float, nnet[0].Nnodes[i]);  // vectors for holding results / values for each node in the input, hidden, output layers

    ALLOCATE (nnet[0]. Nabla_b[i], float, nnet[0].Nnodes[i]);
    ALLOCATE (nnet[0].dNabla_b[i], float, nnet[0].Nnodes[i]);
    ALLOCATE (nnet[0]. Nabla_w[i], float, nnet[0].Nnodes[i-1]*nnet[0].Nnodes[i]);
    ALLOCATE (nnet[0].dNabla_w[i], float, nnet[0].Nnodes[i-1]*nnet[0].Nnodes[i]);
  }
}

/* delete a nnet */
int DeleteNnet (Nnet *nnet) {

  int i, N, NNNETS_2;

  if (!nnets) InitNnets();

  /* find nnet in nnet list */
  N = -1;
  for (i = 0; i < Nnnets; i++) {
    if (nnets[i] == nnet) {
      N = i;
      break;
    }
  }
  if (N == -1) return (FALSE);

  for (i = N; i < Nnnets - 1; i++) {
    nnets[i] = nnets[i + 1];
  }
  Nnnets --;
  NNNETS_2 = MAX (16, NNNETS / 2);
  if (Nnnets < NNNETS_2) {
    NNNETS = NNNETS_2;
    REALLOCATE (nnets, Nnet *, NNNETS);
  }

  FreeNnetData (nnet);
  free (nnet);
  return (TRUE);
}

/* list known nnets */
void ListNnets () {

  int i, j;

  if (!nnets) InitNnets();

  for (i = 0; i < Nnnets; i++) {
    gprint (GP_ERR, "%-15s :", nnets[i][0].name);
    for (j = 0; j < nnets[i][0].Nlayer; j++) {
      gprint (GP_ERR, " %3d", nnets[i][0].Nnodes[j]);
    }
    gprint (GP_ERR, "\n");
  }  
  return;
}

void PrintNnet (Nnet *nnet) {

  for (int L = 1; L < nnet[0].Nlayer; L++) {
    gprint (GP_ERR, " ----- Layer %d -----\n", L);
    for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
      for (int i = 0; i < nnet[0].Nnodes[L-1]; i++) {
	int k = j * nnet[0].Nnodes[L-1] + i;
	myAssert (k < nnet[0].Nnodes[L-1]*nnet[0].Nnodes[L], "overflow");
	gprint (GP_ERR, "%7.4f ", nnet[0].weight[L][k]);
      }
      gprint (GP_ERR, " : %7.4f\n", nnet[0].biases[L][j]);
    }
  }
  return;
}

