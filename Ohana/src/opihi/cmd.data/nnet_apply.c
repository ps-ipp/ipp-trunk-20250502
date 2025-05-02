# include "data.h"

int nnet_apply (int argc, char **argv) {

  if (argc < 4) {
    gprint (GP_ERR, "USAGE: nnet apply (nnet) [input] [input] ... [output] [output] ...\n");
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
    if ((outVec[i] = SelectVector (argv[i + 2 + Ninput], ANYVECTOR, FALSE)) == NULL) {
      gprint (GP_ERR, "invalid output vector %s\n", argv[i+2+Ninput]);
      gprint (GP_ERR, "USAGE: nnet train (nnet) [input] [input] ... [output] [output] ...\n");
      free (inVec);
      free (outVec);
      return (FALSE);    
    }
    ResetVector (outVec[i], OPIHI_FLT, Ntrial);
  }    

  for (int i = 0; i < Ntrial; i++) {

    // store the input values for this row (trial) in the input value vector
    for (int j = 0; j < Ninput; j++) {
      nnet[0].svalue[0][j] = inVec[j][0].elements.Flt[i];
    }
    
    nnet_feedforward (nnet);

    // store the output value vector values in output vectors for this row
    for (int j = 0; j < Noutput; j++) {
      outVec[j][0].elements.Flt[i] = nnet[0].svalue[Nlayer-1][j];
    }
  }    

  free (inVec);
  free (outVec);
  return TRUE;
}
