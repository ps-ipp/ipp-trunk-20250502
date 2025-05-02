# include "data.h"

int kapamemory (int argc, char **argv) {

  int N, kapa;

  /* display source */
  char *name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetImage (NULL, &kapa, name)) return (FALSE);
  FREE (name);

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: kapamemory (mode)\n");
    gprint (GP_ERR, "  mode = dump, nlines, exit\n");
    return (FALSE);
  }

  if (!strcasecmp(argv[1], "dump")) {
    KapaMemoryDump (kapa);
    return TRUE;
  }

  if (!strcasecmp(argv[1], "exit")) {
    if (argc != 3) goto exit_usage;
    if (!strcasecmp(argv[2], "on")) {
      KapaMemoryDumpOnExit (kapa, TRUE);
      return TRUE;
    }
    if (!strcasecmp(argv[2], "off")) {
      KapaMemoryDumpOnExit (kapa, FALSE);
      return TRUE;
    }
  exit_usage:
    gprint (GP_ERR, "USAGE: kapamemory exit (status)\n");
    gprint (GP_ERR, "  statue = 'on' or 'off'\n");
    return FALSE;
  }

  if (!strcasecmp(argv[1], "nlines")) {
    if (argc != 3) {
      gprint (GP_ERR, "USAGE: kapamemory nlines (N)\n");
      return FALSE;
    }
    int Nlines = atoi (argv[2]);
    KapaMemoryDumpLines (kapa, Nlines);
    return TRUE;
  }
  gprint (GP_ERR, "ERROR: unknown kapamemory mode %s\n", argv[1]);
  return FALSE;
}

