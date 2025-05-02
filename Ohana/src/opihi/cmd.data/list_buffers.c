# include "data.h"

int list_buffers (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  if (argc == 3) PrintBuffers (TRUE);
  else PrintBuffers (FALSE);

  return (TRUE);

}

# if (0) 
int mtype (int argc, char **argv) {

  Variable = (char *) NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    Variable = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: mtype (buffer) [-var out]\n");
    return (FALSE);
  }

  Buffer *buf = SelectBuffer (argv[1], OLDBUFFER, FALSE);
  if (!buf) {
    gprint (GP_ERR, "unknown buffer %s\n", argv[1]);
    free (Variable);
    return FALSE;
  }

  if (buf->type == OPIHI_FLT) {
    if (Variable) {
      set_str_variable (Variable, "FLT");
    } else {
      gprint (GP_LOG, "%s : FLT\n", argv[1]);
    }
  } else {
    if (Variable) {
      set_str_variable (Variable, "INT");
    } else {
      gprint (GP_LOG, "%s : INT\n", argv[1]);
    }
  }
  if (Variable) free (Variable);

  return (TRUE);
}
# endif
