# include "ppSim.h"

psArray *ppSimMergeSources (psArray *in1, psArray *in2) {

  psArray *out = psArrayAlloc (in1->n + in2->n);

  for (int i = 0; i < in1->n; i++) {
    out->data[i] = psMemIncrRefCounter (in1->data[i]);
  }

  int nOff = in1->n;
  for (int i = nOff; i < out->n; i++) {
    out->data[i] = psMemIncrRefCounter (in2->data[i-nOff]);
  }

  return out;
}
