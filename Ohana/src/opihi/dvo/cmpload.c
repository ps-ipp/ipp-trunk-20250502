# include "dvoshell.h"
# define D_NSTARS 1000
# define BYTES_STAR 66
# define BLOCK 1000

int cmpload (int argc, char **argv) {
  
  int i, Noverlay, NOVERLAY, Nstar, N, Nextra, Objtype, type;
  int doneread, done, Nskip, Nbytes, nbytes, Ninstar;
  char *c, *c2, *name;
  double dtmp;
  FILE *f;
  char *buffer;
  int kapa;
  Header header;
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
    gprint (GP_ERR, "USAGE: cmpload (overlay) <filename>\n");
    return (FALSE);
  }

  if (!gfits_read_header (argv[2], &header)) {
    gprint (GP_ERR, "ERROR: can't read header for %s\n", argv[2]);
    return (FALSE);
  }

  /* find expected number of stars */
  gfits_scan (&header, "NSTARS", "%d", 1, &Nstar);
  if (Nstar == 0) {
    gprint (GP_ERR, "ERROR: can't get NSTARS from header\n");
    gfits_free_header (&header);
    return (FALSE);
  }

  f = fopen (argv[2], "r");
  if (f == NULL) {
    gprint (GP_ERR, "ERROR: can't read data from %s\n", argv[2]);
    gfits_free_header (&header);
    return (FALSE);
  }
  fseeko (f, header.datasize, SEEK_SET); 

  Noverlay = 0;
  NOVERLAY = 1000;
  ALLOCATE (overlay, KiiOverlay, Noverlay);
  
  /* load in stars by blocks of 1000 */
  ALLOCATE (buffer, char, (BLOCK*BYTES_STAR) + 1);
  buffer[BLOCK*BYTES_STAR] = 0;
  Nextra = 0;
  doneread = FALSE;
  while (!doneread) {
    Nbytes = BYTES_STAR * BLOCK - Nextra;
    nbytes = fread (&buffer[Nextra], 1, Nbytes, f);
    if (nbytes == 0) {
      doneread = TRUE;
      continue;
    }
    nbytes += Nextra;
    /* check line-by-line integrity */
    c = buffer;
    done = FALSE;
    while ((c < buffer + nbytes) && (!done)) { 
      for (c2 = c; *c2 == '\n'; c2++);
      if (c2 > c) { /* extra return chars */
	memmove (c, c2, (int)(buffer + nbytes - c2));
	Nskip = c2 - c;
	nbytes -= Nskip;
	bzero (buffer + nbytes, Nskip);
      }
      c2 = strchr (c, '\n');
      if (c2 == (char *) NULL) {
	done = TRUE;	
	continue;
      }
      c2++;
      if ((c2 - c) != BYTES_STAR) { /* bad line, delete it */
	memmove (c, c2, (int)(buffer + nbytes - c2));
	Nskip = c2 - c;
	nbytes -= Nskip;
	bzero (buffer + nbytes, Nskip);
      } else {
	c = c2;
      }
    }
    Ninstar = nbytes / BYTES_STAR;
    Nextra = nbytes % BYTES_STAR;
    for (i = 0; i < Ninstar; i++) {
      if (Objtype) {
	dparse (&dtmp, 5, &buffer[i*BYTES_STAR]);
	type = dtmp;
	if (type != Objtype) continue;
      }
      # if (0)
      if (scale) {
	dparse (&mag,  3, &buffer[i*BYTES_STAR]);
	overlay[Noverlay].dx = mzero + mscale * mag;
	overlay[Noverlay].dy = mzero + mscale * mag;
      } else {
	overlay[Noverlay].dx = 5.0;
	overlay[Noverlay].dy = 5.0;
      }      
      # endif 

      fparse (&overlay[Noverlay].x,  1, &buffer[i*BYTES_STAR]);
      fparse (&overlay[Noverlay].y,  2, &buffer[i*BYTES_STAR]);
      overlay[Noverlay].type = KII_OVERLAY_BOX;
      overlay[Noverlay].dx = 5.0;
      overlay[Noverlay].dy = 5.0;
      Noverlay ++;
      CHECK_REALLOCATE (overlay, KiiOverlay, NOVERLAY, Noverlay, 1000);
    }
  }
  fclose (f);

  KiiLoadOverlay (kapa, overlay, Noverlay, argv[1]);
  free (overlay);

  gfits_free_header (&header);
  free (buffer);

  gprint (GP_ERR, "loaded %d objects\n", Noverlay);
  return (TRUE);
}

