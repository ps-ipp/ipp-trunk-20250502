# include "relphot.h"

static int Ntess = 0;
static TessellationTable *tess = NULL;

int TessellationIDsByImageName (int *tessID, int *projID, int *skycellID, char *name) {

  int i;

  *tessID = -1;
  *projID = -1;
  *skycellID = -1;

  if (!tess) return FALSE;

  for (i = 0; i < Ntess; i++) {
    // do this with a strhash of some kind?
    if (!strncmp (name, tess[i].basename, tess[i].Nbasename)) {
      *tessID    = i;
      if (tess[i].projectIDoff >= 0) {
	*projID    = atoi(&name[tess[i].projectIDoff]);
      } else {
	*projID    = 0;
      }
      if (tess[i].skycellIDoff >= 0) {
	*skycellID = atoi(&name[tess[i].skycellIDoff]);
      } else {
	*skycellID = 0;
      }
      return TRUE;
    }
  }
  return FALSE;
}

int load_tess (char *tessfile) {

  tess = TessellationTableLoad (tessfile, &Ntess);
  if (!tess) {
    fprintf (stderr, "failed to load tessellation boundary file %s\n", tessfile);
    exit (2);
  }

  return TRUE;
}

int get_tess_ids (int *tessID, int *projID, int *skycellID, double ra, double dec) {

  int status;

  if (!tess) return FALSE;

  double R = ohana_normalize_angle(ra);
  status = TessellationPrimaryCellIDs(tess, Ntess, tessID, projID, skycellID, R, dec);
  return status;
}

void free_tess () {

  if (!tess) return;
  TessellationTableFree (tess, Ntess);
  free (tess);
}
