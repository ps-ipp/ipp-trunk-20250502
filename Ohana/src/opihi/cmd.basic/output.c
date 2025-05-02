# include "basic.h"

int output (int argc, char **argv) {
  
  int N, Noutput;
  gpDest dest;
  IOBuffer *buffer;
  char *output, *current;

  dest = GP_LOG;
  if ((N = get_argument (argc, argv, "-err"))) {
    dest = GP_ERR;
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-current"))) {
    current = gprintGetName (dest);
    remove_argument (N, &argc, argv);
    set_str_variable (argv[N], current);
    remove_argument (N, &argc, argv);
    return (TRUE);
  }

  if ((N = get_argument (argc, argv, "-buffer"))) {
    remove_argument (N, &argc, argv);
    gprintSetBuffer (dest);
    return (TRUE);
  }
    
  /* set the output target and dump the current buffer there */
  if ((N = get_argument (argc, argv, "-dump"))) {
    buffer = gprintGetBuffer (dest);
    if (buffer == NULL) return (FALSE);

    /* save the current buffer contents */
    Noutput = buffer[0].Nbuffer;
    ALLOCATE (output, char, Noutput);
    memcpy (output, buffer[0].buffer, Noutput);
    
    /* set the output target to the specified name */
    remove_argument (N, &argc, argv);
    gprintSetFileAllThreads (dest, argv[N]);
    remove_argument (N, &argc, argv);

    /* send the output to the appropriate destination */
    gwrite (output, 1, Noutput, dest);
    free (output);
    return (TRUE);
  }
    
  if (argc != 2) {
    gprint (GP_ERR, "USAGE: output <filename> [-err] [-buffer] [-current var] [-dump filename]\n");
    return (FALSE);
  }

  gprintSetFileAllThreads (dest, argv[1]);
  return (TRUE);
}
