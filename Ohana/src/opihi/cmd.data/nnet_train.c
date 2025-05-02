# include "data.h"

// these are local only
float nnet_sigmoid (float value);
int   nnet_onelayer (Nnet *nnet, int L);
void  nnet_reset_Nabla (Nnet *nnet);
void  nnet_update_Nabla (Nnet *nnet);
void  nnet_apply_Nabla (Nnet *nnet, int Nmini, float eta, float lambda, int Ntrial);
void  nnet_backprop (Nnet *nnet, Vector **inVec, Vector **outVec, int N);
void  nnet_descent_step (Nnet *nnet, Vector **inVec, Vector **outVec, int *seq, int pass, int Nmini, float eta, float lambda);
void  nnet_print_Nabla (Nnet *nnet);
void  nnet_write_Nabla (char *filename, Nnet *nnet);

static int QUADRATIC_COST = 0;

int nnet_train (int argc, char **argv) {

  int N;

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
  
  float eta = 1.0; // XXX how do I set this? does it need to update?
  if ((N = get_argument (argc, argv, "-eta"))) {
    remove_argument (N, &argc, argv);
    eta = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  float lambda = 0.0; // XXX how do I set this? does it need to update?
  if ((N = get_argument (argc, argv, "-lambda"))) {
    remove_argument (N, &argc, argv);
    lambda = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  Vector *resid = NULL;
  if ((N = get_argument (argc, argv, "-resid"))) {
    remove_argument (N, &argc, argv);
    if ((resid = SelectVector (argv[N], ANYVECTOR, FALSE)) == NULL) {
      gprint (GP_ERR, "USAGE: nnet train (nnet) [input] [input] ... [output] [output] ...\n");
      gprint (GP_ERR, "cannot assign residual vector %s\n", argv[N]);
      return FALSE;
    }
    remove_argument (N, &argc, argv);
  }
  
  Buffer *result = NULL;
  if ((N = get_argument (argc, argv, "-result"))) {
    remove_argument (N, &argc, argv);
    if ((result = SelectBuffer (argv[N], ANYBUFFER, FALSE)) == NULL) {
      gprint (GP_ERR, "USAGE: nnet train (nnet) [input] [input] ... [output] [output] ...\n");
      gprint (GP_ERR, "cannot assign result image %s\n", argv[N]);
      return FALSE;
    }
    remove_argument (N, &argc, argv);
  }
  
  QUADRATIC_COST = 0;
  if ((N = get_argument (argc, argv, "-quadratic-cost"))) {
    remove_argument (N, &argc, argv);
    QUADRATIC_COST = 1;    
  }

  if (argc < 4) {
    gprint (GP_ERR, "USAGE: nnet train (nnet) [input] [input] ... [output] [output] ...\n");
    gprint (GP_ERR, "OPTIONS: -Nepoch [N] -Nmini [N]\n");
    // FREE (resid);
    return FALSE;
  }

  Nnet *nnet = FindNnet (argv[1]);
  if (nnet == NULL) {
    gprint (GP_ERR, "nnet %s not found, create it first\n", argv[1]);
    // FREE (resid);
    return FALSE;
  }

  // save the number of input and output nodes; these need to match the supplied vectors
  int Nlayer   = nnet[0].Nlayer;
  int Ninput   = nnet[0].Nnodes[0];
  int Noutput  = nnet[0].Nnodes[Nlayer - 1];  // number of input + output nodes

  // we need to have the right number of input and output vectors
  if (argc != Ninput + Noutput + 2) {
    gprint (GP_ERR, "need %d input and %d output vectors, but we have %d total\n", nnet[0].Nnodes[0], nnet[0].Nnodes[Nlayer - 1], argc - 2);
    // FREE (resid);
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
      // FREE (resid);
      return FALSE;    
    }
    if (Ntrial && (inVec[i][0].Nelements != Ntrial)) {
      gprint (GP_ERR, "input vectors have inconsistent lengths: %d vs %d for %s\n", Ntrial, inVec[i][0].Nelements, inVec[i][0].name);
      free (inVec);
      free (outVec);
      // FREE (resid);
      return FALSE;    
    }
    if (!Ntrial) Ntrial = inVec[i][0].Nelements;
    if (!Ntrial) {
      gprint (GP_ERR, "trial vectors must be non-zero length\n");
      free (inVec);
      free (outVec);
      // FREE (resid);
      return FALSE;    
    }
  }    
  for (int i = 0; i < Noutput; i++) {
    if ((outVec[i] = SelectVector (argv[i + 2 + Ninput], OLDVECTOR, FALSE)) == NULL) {
      gprint (GP_ERR, "unknown output vector %s\n", argv[i+2+Ninput]);
      gprint (GP_ERR, "USAGE: nnet train (nnet) [input] [input] ... [output] [output] ...\n");
      free (inVec);
      free (outVec);
      // FREE (resid);
      return FALSE;    
    }
    if (outVec[i][0].Nelements != Ntrial) {
      gprint (GP_ERR, "output and input vectors have inconsistent lengths: %d vs %d for %s\n", Ntrial, outVec[i][0].Nelements, outVec[i][0].name);
      free (inVec);
      free (outVec);
      // FREE (resid);
      return FALSE;    
    }
  }    

  // core of the training process:

  // index to hold a random sequence
  ALLOCATE_PTR (seq, int,   Ntrial);
  ALLOCATE_PTR (rnd, float, Ntrial);

  if (resid) ResetVector (resid, OPIHI_FLT, Nepoch);

  if (1) {
    int Npts = 0;
    float s1 = 0.0, s2 = 0.0;
    for (int i = 0; i < Ntrial; i++) {

      // store the input values for this row (trial) in the input value vector
      for (int j = 0; j < Ninput; j++) {
	nnet[0].svalue[0][j] = inVec[j][0].elements.Flt[i];
      }
	  
      nnet_feedforward (nnet);
	  
      // store the output value vector values in output vectors for this row
      for (int j = 0; j < Noutput; j++) {
	float dS = outVec[j][0].elements.Flt[i] - nnet[0].svalue[Nlayer-1][j];
	s1 += dS;
	s2 += SQ(dS);
	Npts ++;
      }
    }
    float mean = s1 / Npts;
    float sigma = sqrt(s2 / Npts - mean*mean);
    gprint (GP_ERR, "start, %f +/- %f\n", mean, sigma);
  }

  // PrintNnet (nnet);

  // train for Nepochs
  // this recreates 'SGD' from http://neuralnetworksanddeeplearning.com/chap1.html
  for (int epoch = 0; epoch < Nepoch; epoch ++) {
    
    // generate a random sequence : used to select random mini batches
    for (int i = 0; i < Ntrial; i++) { seq[i] = i; rnd[i] = drand48(); }
    sort_float_index (rnd, seq, Ntrial);

    int Npass = Ntrial / Nmini;
    for (int pass = 0; pass < Npass; pass ++) {
      // for a given pass select seq elements pass*Nmini to pass*Nmini + Nmini - 1
      // update the weights and biases using the mini batch subset
      nnet_descent_step (nnet, inVec, outVec, seq, pass, Nmini, eta, lambda);
      // return TRUE; // XXX short-circuit at one step
    }
    // PrintNnet (nnet);

    if (resid) {
      int Npts = 0;
      float s1 = 0.0, s2 = 0.0;
      for (int i = 0; i < Ntrial; i++) {

	// store the input values for this row (trial) in the input value vector
	for (int j = 0; j < Ninput; j++) {
	  nnet[0].svalue[0][j] = inVec[j][0].elements.Flt[i];
	}
	  
	nnet_feedforward (nnet);
	  
	// store the output value vector values in output vectors for this row
	for (int j = 0; j < Noutput; j++) {
	  float dS = outVec[j][0].elements.Flt[i] - nnet[0].svalue[Nlayer-1][j];
	  s1 += dS;
	  s2 += SQ(dS);
	  Npts ++;
	}
      }
      float mean = s1 / Npts;
      float sigma = sqrt(s2 / Npts - mean*mean);
      // if (epoch % 10 == 0) gprint (GP_ERR, "epoch %d of %d, %f +/- %f\n", epoch, Nepoch, mean, sigma);
      gprint (GP_ERR, "epoch %d of %d, %f +/- %f\n", epoch, Nepoch, mean, sigma);
      resid[0].elements.Flt[epoch] = sigma;
    } else {
      // if (epoch % 10 == 0) gprint (GP_ERR, "epoch %d of %d\n", epoch, Nepoch);
      gprint (GP_ERR, "epoch %d of %d\n", epoch, Nepoch);
    }
  }  
  // PrintNnet (nnet);

  if (result) {

    gfits_free_matrix (&result[0].matrix);
    gfits_free_header (&result[0].header);
    if (!CreateBuffer (result, Noutput, Ntrial, -32, 1.0, 0.0)) return FALSE;


    float *value = (float *) result[0].matrix.buffer;

    for (int i = 0; i < Ntrial; i++) {
      // store the input values for this row (trial) in the input value vector
      for (int j = 0; j < Ninput; j++) {
	nnet[0].svalue[0][j] = inVec[j][0].elements.Flt[i];
      }
	  
      nnet_feedforward (nnet);
	  
      for (int j = 0; j < Noutput; j++) {
	value[j + i*Noutput] = nnet[0].svalue[Nlayer-1][j];
      }
    }
  }

  free (seq);
  free (rnd);
  free (inVec);
  free (outVec);
  return TRUE;
}

// this recreates 'update_mini_batch' from http://neuralnetworksanddeeplearning.com/chap1.html
void nnet_descent_step (Nnet *nnet, Vector **inVec, Vector **outVec, int *seq, int pass, int Nmini, float eta, float lambda) {

  int Ntrial = inVec[0][0].Nelements;

  nnet_reset_Nabla (nnet);

  for (int i = 0; (i < Nmini) && (pass*Nmini + i < Ntrial); i++) {

    // N is the element of the mini batch on which we are currently operating
    // int N = seq[pass*Nmini + i]; // XXX uncomment to turn on random shuffle
    int N = pass*Nmini + i;

    // backprop generates a dNabla_b, dNabla_w pair for the element N of the input and output vectors
    nnet_backprop (nnet, inVec, outVec, N);
    nnet_update_Nabla (nnet);
    // gprint (GP_ERR, ". ");

    // nnet_print_Nabla (nnet); // XXX print nabla for each epoch
    // XXX uncomment to dump nablas after one step, one element
    // nnet_write_Nabla ("test.nabla.op.dat", nnet); // XXX print nabla for each epoch
    // return; 
  }
  // gprint (GP_ERR, " done mini batch\n");

  // nnet_print_Nabla (nnet);
  nnet_apply_Nabla (nnet, Nmini, eta, lambda, Ntrial);

  // XXX uncomment to dump nablas after one mini batch
  // nnet_write_Nabla ("test.nabla.op.dat", nnet); // XXX print nabla for each epoch
  // PrintNnet (nnet);
}

void nnet_backprop (Nnet *nnet, Vector **inVec, Vector **outVec, int N) {

  // start with the input values
  int Nlayer   = nnet[0].Nlayer;

  // store the input values for this row (trial) in the input vector "svalue[0]"
  for (int j = 0; j < nnet[0].Nnodes[0]; j++) {
    nnet[0].svalue[0][j] = inVec[j][0].elements.Flt[N];
  }

  // z = w * activation + bias 
  // save z [one per non-input layer]
  // activation = sigmoid (z)

  // feedforward operates on the vector saved in svalue[0]
  // feedforward saves the zvalues [w * input + bias], svalues [sigmoid(z)], sprimes [sigmoid'(z)] as it runs
  nnet_feedforward (nnet);

  // backward pass
  for (int L = Nlayer - 1; L > 0; L--) {

    if (L == Nlayer - 1) {
      // starting point uses cost_derivative to compare last svalue set with truth output
      // delta = cost_derivative(svalue[L], output) * sprime[L];
      for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
	// cost_derivative (svalue[L][j], output[j]) = svalue[L][j] - output[j]
	// (because cost = (1/2)(svalue[L][j] - output[j])^2)
	// NOTE: the (1/Npt) factor is pushed to the apply_Nabla step
	// nnet[0].delta[L][j] = cost_derivative(nnet[0].svalue[L][j], outVec[j][0].elements.Flt[N]) * nnet[0].sprime[L][j];

	if (QUADRATIC_COST) {
	  nnet[0].delta[L][j] = (nnet[0].svalue[L][j] - outVec[j][0].elements.Flt[N]) * nnet[0].sprime[L][j];
	} else {
	  nnet[0].delta[L][j] = (nnet[0].svalue[L][j] - outVec[j][0].elements.Flt[N]);
	}

	// for quadratic cost, delta = (svalue[L] - output) * sprime[L]
	// for cross-entropy, delta = (svalue[L] - output)

      }
    } else {
      // XXX TEST PRINTS to catch code errors compared to python implementation
      // gprint (GP_ERR, "z: ");
      // for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
      // 	gprint (GP_ERR, "%f ", 	nnet[0].zvalue[L][j]);
      // } gprint (GP_ERR, "\n");
      // gprint (GP_ERR, "sp: ");
      // for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
      // 	gprint (GP_ERR, "%f ", 	nnet[0].sprime[L][j]);
      // } gprint (GP_ERR, "\n");

      // delta = DOT(delta, transpose(weight[L+1])) * sprime;
      for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
	float tmpdelta = 0.0;
	for (int i = 0; i < nnet[0].Nnodes[L+1]; i++) {
	  int k = j + i*nnet[0].Nnodes[L]; // note order of (i,j) : j is [L+1] direction 
	  myAssert (k < nnet[0].Nnodes[L]*nnet[0].Nnodes[L+1], "overflow");
	  tmpdelta += nnet[0].weight[L+1][k] * nnet[0].delta[L+1][i];
	  // gprint (GP_ERR, "%e %e\n", nnet[0].weight[L+1][k], nnet[0].delta[L+1][i]);
	}
	nnet[0].delta[L][j] = tmpdelta * nnet[0].sprime[L][j];
      }
    }					       

    // NOTE on weight array: for a given layer, L, the matrix weight[L] maps the nodes in
    // the previous layer (L-1) to those in L: Nnodes[L-1] -> Nnodes[L].  This is a matrix
    // with dimensions (Nnodes[L-1] x Nnodes[L]).  The Nnodes[L-1] is the fast dimension,
    // so for an element (i,j), the index k = i + j*Nnodes[L-1].   

    // Nabla_b[L] = delta;
    for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
      nnet[0]. dNabla_b[L][j] = nnet[0].delta[L][j];
    }
    
    // Nabla_w[L] = DOT(delta, transpose(svalue[L-1]));
    for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
      for (int i = 0; i < nnet[0].Nnodes[L-1]; i++) {
	int k = i + j*nnet[0].Nnodes[L-1];
	myAssert (k < nnet[0].Nnodes[L-1]*nnet[0].Nnodes[L], "overflow");
	nnet[0]. dNabla_w[L][k] = nnet[0].svalue[L-1][i] * nnet[0].delta[L][j];
      }
    }
  }
}

// support functions to loop over the Nabla entries
void nnet_reset_Nabla (Nnet *nnet) {
  for (int L = 1; L < nnet[0].Nlayer; L++) {

    for (int j = 0; j < nnet[0].Nnodes[L]; j++) {

      nnet[0]. Nabla_b[L][j] = 0;
      nnet[0].dNabla_b[L][j] = 0;

      for (int i = 0; i < nnet[0].Nnodes[L-1]; i++) {
	int k = i + j*nnet[0].Nnodes[L-1];
	myAssert (k < nnet[0].Nnodes[L-1]*nnet[0].Nnodes[L], "overflow");
	nnet[0]. Nabla_w[L][k] = 0;
	nnet[0].dNabla_w[L][k] = 0;
      }
    }
  }
}
void nnet_update_Nabla (Nnet *nnet) {
  for (int L = 1; L < nnet[0].Nlayer; L++) {

    for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
      nnet[0]. Nabla_b[L][j] += nnet[0].dNabla_b[L][j];

      for (int i = 0; i < nnet[0].Nnodes[L-1]; i++) {
	int k = i + j*nnet[0].Nnodes[L-1];
	myAssert (k < nnet[0].Nnodes[L-1]*nnet[0].Nnodes[L], "overflow");
	nnet[0]. Nabla_w[L][k] += nnet[0].dNabla_w[L][k];
      }
    }
  }
}
void nnet_apply_Nabla (Nnet *nnet, int Nmini, float eta, float lambda, int Ntrial) {
  for (int L = 1; L < nnet[0].Nlayer; L++) {

    for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
      nnet[0].biases[L][j] -= (eta / Nmini) * nnet[0].Nabla_b[L][j];

      for (int i = 0; i < nnet[0].Nnodes[L-1]; i++) {
	int k = i + j*nnet[0].Nnodes[L-1];
	myAssert (k < nnet[0].Nnodes[L-1]*nnet[0].Nnodes[L], "overflow");
	// nnet[0].weight[L][k] -= (eta / Nmini) * nnet[0].Nabla_w[L][k];
	// with lambda > 0.0, we have L2 regularization.  if lambda = 0.0, we recover the default implementation
	nnet[0].weight[L][k] = nnet[0].weight[L][k]*(1.0 - eta*lambda/Ntrial) - (eta / Nmini) * nnet[0].Nabla_w[L][k];
      }
    }
  }
}

void nnet_print_Nabla (Nnet *nnet) {

  for (int L = 1; L < nnet[0].Nlayer; L++) {
    gprint (GP_ERR, " ----- Nabla %d -----\n", L);
    for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
      for (int i = 0; i < nnet[0].Nnodes[L-1]; i++) {
	int k = j * nnet[0].Nnodes[L-1] + i;
	myAssert (k < nnet[0].Nnodes[L-1]*nnet[0].Nnodes[L], "overflow");
	gprint (GP_ERR, "%10.3e ", nnet[0].Nabla_w[L][k]);
      }
      gprint (GP_ERR, " : %10.3e\n", nnet[0].Nabla_b[L][j]);
    }
  }
  return;
}

void nnet_write_Nabla (char *filename, Nnet *nnet) {

  FILE *f = fopen (filename, "w");

  fprintf (f, "NLAYER %d\n", nnet[0].Nlayer);
  fprintf (f, "LAYERS ");
  for (int L = 0; L < nnet[0].Nlayer; L++) {
    fprintf (f, "%d ", nnet[0].Nnodes[L]);
  }
  fprintf (f, "\n");

  for (int L = 1; L < nnet[0].Nlayer; L++) {
    fprintf (f, "LAYER %d NX %d NY %d\n", L - 1, nnet[0].Nnodes[L-1], nnet[0].Nnodes[L]);
    for (int j = 0; j < nnet[0].Nnodes[L]; j++) {
      for (int i = 0; i < nnet[0].Nnodes[L-1]; i++) {
	int k = j * nnet[0].Nnodes[L-1] + i;
	myAssert (k < nnet[0].Nnodes[L-1]*nnet[0].Nnodes[L], "overflow");
	fprintf (f, "%.9f ", nnet[0].Nabla_w[L][k]);
      }
      fprintf (f, "%.9f\n", nnet[0].Nabla_b[L][j]);
    }
  }

  fclose (f);
  return;
}

// the input values must already be copied to the input layer svalue[]
void nnet_feedforward (Nnet *nnet) {

  for (int i = 1; i < nnet[0].Nlayer; i++) {
    nnet_onelayer (nnet, i);
  }
  return;
}

// calculate z, sigmoid(z) for each layer (z = w*value + bias)
int nnet_onelayer (Nnet *nnet, int L) {

  if (L < 1) return FALSE; // abort here?
  if (L >= nnet[0].Nlayer) return FALSE; // abort here?

  // evaluating a single layer [L], n > 0, n < Nlayer:
  int Ninput   = nnet[0].Nnodes[L-1];
  int Noutput  = nnet[0].Nnodes[L];

  // input layer is [L-1], output layer is [L]
  for (int j = 0; j < Noutput; j ++) {
    float sum = 0;
    for (int i = 0; i < Ninput; i++) {
      // weight matrix order is (0, 1, ... Ninput-1, Ninput, Ninput + 1, ... Ninput * Noutput - 1)
      int k = j * Ninput + i;
      myAssert (k < Ninput*Noutput, "overflow");
      sum += nnet[0].weight[L][k]*nnet[0].svalue[L-1][i];
    }
    sum += nnet[0].biases[L][j];
    nnet[0].zvalue[L][j] = sum;
    nnet[0].svalue[L][j] = nnet_sigmoid(sum);
    nnet[0].sprime[L][j] = nnet[0].svalue[L][j] * (1 - nnet[0].svalue[L][j]);
    // note that d sigmoid / dz = sigmoid * (1 - sigmoid)
  }
  return TRUE;
}

float nnet_sigmoid (float value) {
  return 1.0 / (1.0 + exp(-value));
}
