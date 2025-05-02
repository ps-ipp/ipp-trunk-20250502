# include "dvoshell.h"
# define CHAR_LINE 104
# define NBLOCK 100

int objload (int argc, char **argv) {
  
  int i, N, Objtype, type, Nline, status;
  FILE *f;
  char *buffer, *line, *name;
  int kapa, Noverlay, NOVERLAY;
  KiiOverlay *overlay;
  
  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetImage (NULL, &kapa, name)) return (FALSE);
  FREE (name);

  int channel = 0;
  if ((N = get_argument (argc, argv, "-ch"))) {
    channel = GetKapaChannelFromString (argv[N]);
    if (!channel) return FALSE;
    KiiSetChannel (kapa, channel - 1);
  }

  Objtype = 0;
  if ((N = get_argument (argc, argv, "-t"))) {
    remove_argument (N, &argc, argv);
    Objtype = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: objload (overlay) <filename>\n");
    return (FALSE);
  }

  f = fopen (argv[2], "r");
  if (f == (FILE *) NULL) {
    gprint (GP_ERR, "ERROR: can't find object file %s\n", argv[2]);
    return (FALSE);
  }

  /* read average values from first line */
  ALLOCATE (line, char, 129);
  scan_line (f, line);

  ALLOCATE (buffer, char, CHAR_LINE*NBLOCK);

  Noverlay = 0;
  NOVERLAY = 1000;
  ALLOCATE (overlay, KiiOverlay, Noverlay);
  
  /* read in data from obj file */
  while ((Nline = fread (buffer, CHAR_LINE, NBLOCK, f)) > 0) {
    for (i = 0; i < Nline; i++) {
      /* we are now using all entries on the *.obj line */
      status = sscanf (&buffer[i*CHAR_LINE], "%d %f %f",  &type, &overlay[Noverlay].x, &
overlay[Noverlay].y);
      if (status != 3) continue;
      if (Objtype && (Objtype != type)) continue;
      overlay[Noverlay].type = KII_OVERLAY_BOX;
      overlay[Noverlay].dx = 5.0;
      overlay[Noverlay].dy = 5.0;
      Noverlay ++;
      CHECK_REALLOCATE (overlay, KiiOverlay, NOVERLAY, Noverlay, 1000);
    }
  }
  fclose (f);
  free (buffer);

  KiiLoadOverlay (kapa, overlay, Noverlay, argv[1]);

  free (overlay);
  free (buffer);
  free (line);

  gprint (GP_ERR, "loaded %d objects\n", Noverlay);
  return (TRUE);
}

