# include "data.h"
void sortseq (float *X, int *Y, int N);

int nnet_train (int argc, char **argv) {

  int Nepoch = 10;
  if ((N = get_argument (argc, argv, "-Nepoch"))) {
    remove_argument (N, &argc, argv);
    Nepoch = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  int Nmini = 100;
  if ((N = get_argument (argc, argv, "-Nmini"))) {
    remove_argument (N, &argc, argv);
    Nmini = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  if (argc < 4) {
    gprint (GP_ERR, "USAGE: nnet train (nnet) [input] [input] ... [output] [output] ...\n");
    gprint (GP_ERR, "OPTIONS: -Nepoch [N] -Nmini [N]\n");
    return FALSE;
  }

  Nnet *nnet = FindNnet (argv[1]);
  if (nnet == NULL) {
    gprint (GP_ERR, "nnet %s not found, create it first\n", argv[1]);
    return FALSE;
  }

  // save the number of input and output nodes; these need to match the supplied vectors
  int Nlayer   = nnet[0].Nlayer;
  int Ninput   = nnet[0].Nnodes[0];
  int Noutput  = nnet[0].Nnodes[Nlayer - 1];  // number of input + output nodes

  // we need to have the right number of input and output vectors
  if (argc != Ninput + Noutput + 2) {
    gprint (GP_ERR, "need %d input and %d output vectors, but we have %d total\n", nnet[0].Nnodes[0], nnet[0].Nnodes[Nlayer - 1], argc - 2);
    return FALSE;
  }

  // grab the input and output vectors from the argument list
  ALLOCATE_PTR (inVec, Vector *, Ninput);
  ALLOCATE_PTR (outVec, Vector *, Noutput);

  // check that the vectors exist and that lengths match
  int Ntrial = 0;
  for (int i = 0; i < Ninput; i++) {
    if ((inVec[i] = SelectVector (argv[i + 2], OLDVECTOR, FALSE)) == NULL) {
      gprint (GP_ERR, "unknown input vector %s\n", argv[i+2]);
      gprint (GP_ERR, "USAGE: nnet train (nnet) [input] [input] ... [output] [output] ...\n");
      free (inVec);
      free (outVec);
      return (FALSE);    
    }
    if (Ntrial && (inVec[i][0].Nelements != Ntrial)) {
      gprint (GP_ERR, "input vectors have inconsistent lengths: %d vs %d for %s\n", Ntrial, inVec[i][0].Nelements, inVec[i][0].name);
      free (inVec);
      free (outVec);
      return (FALSE);    
    }
    if (!Ntrial) Ntrial = inVec[i][0].Nelements;
    if (!Ntrial) {
      gprint (GP_ERR, "trial vectors must be non-zero length\n");
      free (inVec);
      free (outVec);
      return (FALSE);    
    }
  }    
  for (int i = 0; i < Noutput; i++) {
    if ((outVec[i] = SelectVector (argv[i + 2 + Ninput], OLDVECTOR, FALSE)) == NULL) {
      gprint (GP_ERR, "unknown output vector %s\n", argv[i+2+Ninput]);
      gprint (GP_ERR, "USAGE: nnet train (nnet) [input] [input] ... [output] [output] ...\n");
      free (inVec);
      free (outVec);
      return (FALSE);    
    }
    if (outVec[i][0].Nelements != Ntrial) {
      gprint (GP_ERR, "output and input vectors have inconsistent lengths: %d vs %d for %s\n", Ntrial, outVec[i][0].Nelements, outVec[i][0].name);
      free (inVec);
      free (outVec);
      return (FALSE);    
    }
  }    

  // core of the training process:

  // index to hold a random sequence
  ALLOCATE_PTR (seq, int,   Ntrial);
  ALLOCATE_PTR (rnd, float, Ntrial);

  // train for Nepochs
  // this recreates 'SGD' from http://neuralnetworksanddeeplearning.com/chap1.html
  for (int epoch = 0; epoch < Nepoch; epoch ++) {
    
    // generate a random sequence : used to select random mini batches
    for (int i = 0; i < Ntrial; i++) { seq[i] = i; rnd[i] = drand48(); }
    sortseq (rnd, seq, Ntrial);

    int Npass = Ntrial / Nmini;
    for (int pass = 0; pass < Npass; pass ++) {
      // for a given pass select seq elements pass*Nmini to pass*Nmini + Nmini - 1
      // update the weights and biases using the mini batch subset
      nnet_descent_step (nnet, inVec, outVec, seq, pass, Nmini, eta);
      gprint (GP_ERR, "epoch %d of %d, pass %d of %d\n", epoch, Nepoch, pass, Npass);
    }
  }  

  free (seq);
  free (rnd);
  free (inVec);
  free (outVec);
  return TRUE;
}

// this recreates 'update_mini_batch' from http://neuralnetworksanddeeplearning.com/chap1.html
void nnet_descent_step (NNet *nnet, Vector **inVec, Vector **outVec, int *seq, int pass, int Nmini, float eta) {

  int Ntrial = inVec[0][0].Nelements;

  nnet_reset_Nabla (nnet);

  for (i = 0; (i < Nmini) && (pass*Nmini + i < Ntrial); i++) {

    // N is the element of the mini batch on which we are currently operating
    N = seq[pass*Nmini + i];

    // backprop generates a dNabla_b, dNabla_w pair for the element N of the input and output vectors
    nnet_backprop (nnet, inVec, outVec, N);

    nnet_update_Nabla (nnet);
  }

  nnet_apply_Nabla (nnet, Nmini, eta);
}

void nnet_backprop (NNet *nnet, Vector **inVec, Vector **outVec, int N) {

  // start with the input values

  // store the input values for this row (trial) in the input value vector
  int Nlayer   = nnet[0].Nlayer;
  int Ninput   = nnet[0].Nnodes[0];
  int Noutput  = nnet[0].Nnodes[Nlayer - 1];  // number of input + output nodes

  for (int j = 0; j < Ninput; j++) {
    nnet[0].svalue[0][j] = inVec[j][0].elements.Flt[N];
  }

  // z = w * activation + bias 
  // save z [one per non-input layer]
  // activation = sigmoid (z)

  nnet_feedforward (nnet);

  // backward pass (note for now these are vector operations:
  for (L = Nlayer - 1; L > 0; L--) {

    // sp is array of same size as zvalue for each layer
    // sp = sigmoid_prime (nnet[0].zvalue[L]); sprime is precalculated in feedforward

    if (L == Nlayer - 1) {
      // starting point uses cost_derivative to compare last svalue set with truth output
      // delta = cost_derivative(nnet[0].svalue[L], outVec, N) * sprime[L];
      for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
	nnet[0].delta[L][j] = cost_derivative(nnet[0].svalue[L][j], outVec[j][0].elements.Flt[N]) * nnet[0].sprime[L][j];
      }
    } else {
      // delta = (delta DOT transpose(weights[L+1])) * sp;
      for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
	tmpdelta[L][j] = 0;
	for (int i = 0; i < nnet[0].Nnodes[L+1]; i++) {
	  int k = j + i*nnet[0].Nnodes[L];
	  tmpdelta[L][j] += nnet[0].weight[L][k] * nnet[0].sprime[L][i]; // XXX check on the index values
	}
	tmpdelta[L][j] * nnet[0].sprime[L][j];
      }
    }					       

    // UPDATE
    Nabla_b[L] = delta;
    
    // Nabla_w[L] = delta DOT transpose(nnet[0].svalue[L-1]);
    for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
      for (int i = 0; i < nnet[0].Nnodes[L-1]; i++) {
	int k = i + j*nnet[0].Nnodes[n-1];
	nnet[0]. Nabla_w[n][k] = nnet[0].svalue[L-1][i] * nnet[0].delta[L][j];
      }
    }

  }
}

// support functions to loop over the Nabla entries
void nnet_apply_Nabla (NNet *nnet, int Nmini, float eta) {
  for (int n = 1; n < nnet[0].Nlayer; n++) {

    for (int j = 0; j < nnet[0].Nnodes[n]; j++) {
      nnet[0].biases[n][j] -= (eta / Nmini) * nnet[0].Nabla_b[n][j];

      for (int i = 0; i < nnet[0].Nnodes[n-1]; i++) {
	int k = i + j*nnet[0].Nnodes[n-1];
	nnet[0].weights[n][k] -= (eta / Nmini) * nnet[0].Nabla_w[n][k];
      }
    }
  }
}
void nnet_update_Nabla (NNet *nnet) {
  for (int n = 1; n < nnet[0].Nlayer; n++) {

    for (int j = 0; j < nnet[0].Nnodes[n]; j++) {
      nnet[0]. Nabla_b[n][j] += nnet[0].dNabla_b[n][j];

      for (int i = 0; i < nnet[0].Nnodes[n-1]; i++) {
	int k = i + j*nnet[0].Nnodes[n-1];
	nnet[0]. Nabla_w[n][k] += nnet[0].dNabla_w[n][k];
      }
    }
  }
}
void nnet_reset_Nabla (NNet *nnet) {
  for (int n = 1; n < nnet[0].Nlayer; n++) {

    for (int j = 0; j < nnet[0].Nnodes[n]; j++) {

      nnet[0]. Nabla_b[n][j] = 0;
      nnet[0].dNabla_b[n][j] = 0;

      for (int i = 0; i < nnet[0].Nnodes[n-1]; i++) {
	int k = i + j*nnet[0].Nnodes[n-1];
	nnet[0]. Nabla_w[n][k] = 0;
	nnet[0].dNabla_w[n][k] = 0;
      }
    }
  }
}

// the input values must already be copied to the input layer svalue[]
void nnet_feedforward (NNet *nnet) {

  for (int i = 1; i < nnet[0].Nlayer; i++) {
    nnet_onelayer (nnet, i);
  }
  return;
}

// calcularte z, sigmoid(z) for each layer (z = w*value + bias)
int nnet_onelayer (NNet *nnet, int n) {

  if (n < 1) return FALSE; // abort here?
  if (n >= nnet[0].Nlayer) return FALSE; // abort here?

  // evaluating a single layer [n], n > 0, n < Nlayer:
  int Ninput   = nnet[0].Nnodes[n - 1];
  int Noutput  = nnet[0].Nnodes[n];

  // input layer is [n-1], output layer is [n]
  for (int j = 0; j < Noutput; j ++) {
    float sum = 0;
    for (int i = 0; i < Ninput; i++) {
      // weight matrix order is (0, 1, ... Ninput-1, Ninput, Ninput + 1, ... Ninput * Noutput - 1)
      int k = j * Ninput + i;
      sum += nnet[0].weight[n][k]*nnet[0].svalue[n-1][i];
    }
    sum += nnet[0].biases[n][j];
    nnet[0].zvalue[n][j] = sum;
    nnet[0].svalue[n][j] = nnet_sigmoid(sum);
    nnet[0].sprime[n][j] = nnet[0].svalue[n][j] * (1 - nnet[0].svalue[n][j]);
    // note that d sigmoid / dz = sigmoid * (1 - sigmoid)
  }
  return TRUE;
}

float nnet_sigmoid (float value) {
  return 1.0 / (1.0 + exp(-sum));
}

void sortseq (float *X, int *Y, int N) {

# define SWAPFUNC(A,B){ float ftmp; int itmp; \
    ftmp = X[A]; X[A] = X[B]; X[B] = ftmp;      \
    itmp = Y[A]; Y[A] = Y[B]; Y[B] = itmp;       \
}
# define COMPARE(A,B)(X[A] < X[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

