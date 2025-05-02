# include "data.h"

int medimage_help (int argc, char **argv);
int medimage_list (int argc, char **argv);
int medimage_add (int argc, char **argv);
int medimage_calc (int argc, char **argv);
int medimage_delete (int argc, char **argv);
int medimage_rename (int argc, char **argv);

static Command medimage_commands[] = {
  {1, "help",       medimage_help,       "list medimage help info"},
  {1, "list",       medimage_list,       "list medimages"},
  {1, "add",        medimage_add,        "add an image to the given medimage (creates new one if needed)"},
  {1, "calc",       medimage_calc,       "measure the median image and save to a buffer"},
  {1, "delete",     medimage_delete,     "delete a medimage"},
  {1, "rename",     medimage_rename,     "rename a medimage"},
};

int medimage_help (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);
  OHANA_UNUSED_PARAM(argv);

  gprint (GP_ERR, "USAGE: medimage (command)\n");
  gprint (GP_ERR, "    medimage help                       : this listing\n");
  gprint (GP_ERR, "    medimage list                       : list medimages\n");
  gprint (GP_ERR, "    medimage add    (medimage) (image)  : add the given image to a medimage\n");
  gprint (GP_ERR, "    medimage calc   (medimage) (output) : calculate the median image\n");
  gprint (GP_ERR, "    medimage delete (medimage)          : delete named medimage\n");
  gprint (GP_ERR, "    medimage rename (medimage) (new)    : change medimage name to new name\n");

  return FALSE;
}

int medimage_command (int argc, char **argv) {

  int i, N, status;

  if (argc < 2) {
    medimage_help(0,NULL);
    return (FALSE);
  }

  N = sizeof (medimage_commands) / sizeof (Command);

  /* find the medimage sub-command which matches */
  for (i = 0; i < N; i++) {
    if (!strcmp (medimage_commands[i].name, argv[1])) {
      status = (*medimage_commands[i].func) (argc - 1, argv + 1);
      return (status);
    }
  }

  gprint (GP_ERR, "unknown medimage command %s\n", argv[1]);
  return (FALSE);
}
