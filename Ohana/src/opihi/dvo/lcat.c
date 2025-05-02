# include "dvoshell.h"

int lcat (int argc, char **argv) {
  
  double Radius;
  int i, N, ShowAll;
  char exists;
  struct stat filestat;
  Graphdata graphmode;
  SkyTable *sky;
  SkyList *skylist;

  if (!GetGraphdata (&graphmode, NULL, NULL)) return (FALSE);

  ShowAll = FALSE;
  if ((N = get_argument (argc, argv, "-all"))) {
    remove_argument (N, &argc, argv);
    ShowAll = TRUE;
  }
  if (argc != 1) {
    gprint (GP_ERR, "USAGE: lcat [-all]\n");
    return (FALSE);
  }

  Radius = MAX (fabs(graphmode.xmax), fabs(graphmode.ymax));

  /* load sky from correct table */
  sky = GetSkyTable ();
  skylist = SkyListByRadius (sky, -1, graphmode.coords.crval1, graphmode.coords.crval2, Radius);

  for (i = 0; i < skylist[0].Nregions; i++) {
    exists = 'Y';
    if (stat (skylist[0].filename[i], &filestat) == -1) exists = 'N';
    if (ShowAll) {
      gprint (GP_ERR, "%3d %s  %c\n", i, skylist[0].regions[i][0].name, exists);
    } else {
      if (exists == 'Y') {
	gprint (GP_ERR, "%3d %s\n", i, skylist[0].regions[i][0].name);
      }
    }
  }

  SkyListFree (skylist);
  return (TRUE);
}

