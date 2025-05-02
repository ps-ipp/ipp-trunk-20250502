# include "data.h"

static Command nnet_commands[] = {
  {1, "list",     nnet_list,     "list nnets"},
  {1, "delete",   nnet_delete,   "delete a nnet"},
  {1, "print",    nnet_print,    "display nnet values"},
  {1, "create",   nnet_create,   "create a nnet"},
  {1, "set",      nnet_set,      "set nnet node values"},
  {1, "get",      nnet_get,      "get nnet node values"},
  {1, "read",     nnet_read,     "read nnet values from a file"},
  {1, "write",    nnet_write,    "write nnet values to a file"},
  {1, "train",    nnet_train,    "train nnet on a set of data"},
  {1, "apply",    nnet_apply,    "apply nnet to a set of data"},
};

int nnet_command (int argc, char **argv) {

  int i, N, status;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: nnet (command)\n");
    gprint (GP_ERR, "    nnet list          : list nnets\n");
    gprint (GP_ERR, "    nnet delete (nnet) : delete a nnet\n");
    gprint (GP_ERR, "    nnet print  (nnet) : print values for a nnet\n");
    gprint (GP_ERR, "    nnet create (nnet) (Ninput) [Nnodes] [Nnodes] ... (Noutput) : create a nnet\n");
    gprint (GP_ERR, "    nnet set    (nnet) [weights] [biases] ... [weights] [biases] : set nnet weights (images) and biases (vectors)\n");
    gprint (GP_ERR, "    nnet get    (nnet) [weights] [biases] ... [weights] [biases] : get nnet weights (images) and biases (vectors)\n");
    gprint (GP_ERR, "    nnet read   (nnet) (filename) : set nnet weights and biases using a data file\n");
    gprint (GP_ERR, "    nnet write  (nnet) (filename) : save nnet weights and biases to a data file\n");
    gprint (GP_ERR, "    nnet train  (nnet) [input] [input] ... [output] [output] ... : train nnet on data from a set of vectors\n");
    gprint (GP_ERR, "    nnet apply  (nnet) [input] [input] ... [output] [output] ... : apply nnet to input data and generate output\n");
    return (FALSE);
  }

  N = sizeof (nnet_commands) / sizeof (Command);

  /* find the nnet sub-command which matches */
  for (i = 0; i < N; i++) {
    if (!strcmp (nnet_commands[i].name, argv[1])) {
      status = (*nnet_commands[i].func) (argc - 1, argv + 1);
      return (status);
    }
  }

  gprint (GP_ERR, "unknown nnet command %s\n", argv[1]);
  return (FALSE);
}

/* nnet is called with the command "nnet".  
   the command line word "nnet" is meant to be followed the one of several 
   possible options list above */
