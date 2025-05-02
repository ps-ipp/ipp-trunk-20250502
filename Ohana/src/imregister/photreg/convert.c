# include "imregister.h"
# include "photreg.h"

PhotPars *PhotParsOld_to_PhotPars (PhotParsOld *input, off_t Nphotpars) {

  off_t i;
  PhotPars *output;

  ALLOCATE (output, PhotPars, Nphotpars);

  for (i = 0; i < Nphotpars; i++) {
    output[i].ZP       = input[i].ZP;
    output[i].ZPo      = input[i].ZPo;
    output[i].dZP      = input[i].dZP;
    output[i].K        = input[i].K;
    output[i].X        = input[i].X;
    output[i].tstart   = input[i].tstart;
    output[i].tstop    = input[i].tstop;
    output[i].c1       = input[i].c1;
    output[i].c2       = input[i].c2;
    output[i].photcode = input[i].photcode;
    strcpy (output[i].label, input[i].label);
    output[i].Nmeas = 0;
    output[i].Ntime = 0;
  }
  return (output);
}
