# include "dvoshell.h"

int RD_to_XYpic (double *x, double *y, double r, double d, Coords *coords, double Rmin, double Rmax, double Rmid, int *leftside);

int skycat (int argc, char **argv) {
  
  double Radius;
  int i, j, N, Nregions, kapa, ShowAll, NPTS, Npts, leftside, Depth, VERBOSE;
  struct stat filestat;
  Vector Xvec, Yvec;
  Graphdata graphmode;
  double X[4], Y[4], Rmin, Rmax, Rmid;
  SkyTable *sky;
  SkyList *skylist;
  SkyRegion **regions;

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }
  ShowAll = FALSE;
  if ((N = get_argument (argc, argv, "-all"))) {
    remove_argument (N, &argc, argv);
    ShowAll = TRUE;
  }
  Depth = -1;
  if ((N = get_argument (argc, argv, "-depth"))) {
    remove_argument (N, &argc, argv);
    Depth = atoi (argv[N]);
    remove_argument (N, &argc, argv);    
  }

  if (!style_args (&graphmode, &argc, argv, &kapa)) return FALSE;

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: skycat [-all] [-depth depth] [-v]\n");
    return (FALSE);
  }

  Radius = MAX (fabs(graphmode.xmax), fabs(graphmode.ymax));

  sky = GetSkyTable ();
  skylist = SkyListByRadius (sky, Depth, graphmode.coords.crval1, graphmode.coords.crval2, Radius);
  
  HostTable *table = NULL;  
  char *CATDIR = GetCATDIR();
  if (HostTableExists (CATDIR, sky->hosts)) {
    table = HostTableLoad (CATDIR, sky->hosts);
    if (!table) {
      gprint (GP_ERR, "ERROR: failure reading Host Table %s for parallel database %s\n", sky->hosts, CATDIR);
      return FALSE;
    }    
  }

  if (VERBOSE) gprint (GP_ERR, "region: %6.2f - %6.2f, %6.2f - %6.2f\n", 
			graphmode.coords.crval1 - Radius, graphmode.coords.crval1 + Radius, 
			graphmode.coords.crval2 - Radius, graphmode.coords.crval2 + Radius);

  Rmin = graphmode.coords.crval1 - 180.0;
  Rmax = graphmode.coords.crval1 + 180.0;
  Rmid = 0.5*(Rmin + Rmax);

  Npts = 0;
  NPTS = 200;
  SetVector (&Xvec, OPIHI_FLT, NPTS);
  SetVector (&Yvec, OPIHI_FLT, NPTS);
   
  regions = skylist[0].regions;
  Nregions = skylist[0].Nregions;

  // prepare to handle interrupt signals
  struct sigaction *old_sigaction = SetInterrupt();

  for (i = 0; (i < Nregions) && !interrupt; i++) {
    if (!ShowAll) {
      char hostfile[1024];
      if (table) {
	int hostID = (regions[i][0].hostFlags & DATA_USE_BCK) ? regions[i][0].backupID : regions[i][0].hostID;
	if (hostID) {
	  int index = table->index[hostID];
	  snprintf (hostfile, 1024, "%s/%s.cpt", table->hosts[index].pathname, regions[i][0].name);
	} else {
	  strcpy (hostfile, skylist[0].filename[i]);
	}
      } else {
	strcpy (hostfile, skylist[0].filename[i]);
      }
      if (stat (hostfile, &filestat) == -1) continue;
    }

    if (VERBOSE) gprint (GP_ERR, "%3d %s %6.2f - %6.2f, %6.2f - %6.2f\n", i, regions[i][0].name, 
			 regions[i][0].Rmin, regions[i][0].Rmax, regions[i][0].Dmin, regions[i][0].Dmax);
    
    leftside = -1;
    RD_to_XYpic (&X[0], &Y[0], regions[i][0].Rmin, regions[i][0].Dmin, &graphmode.coords, Rmin, Rmax, Rmid, &leftside);
    RD_to_XYpic (&X[1], &Y[1], regions[i][0].Rmin, regions[i][0].Dmax, &graphmode.coords, Rmin, Rmax, Rmid, &leftside);
    RD_to_XYpic (&X[2], &Y[2], regions[i][0].Rmax, regions[i][0].Dmax, &graphmode.coords, Rmin, Rmax, Rmid, &leftside);
    RD_to_XYpic (&X[3], &Y[3], regions[i][0].Rmax, regions[i][0].Dmin, &graphmode.coords, Rmin, Rmax, Rmid, &leftside);
    
    Xvec.elements.Flt[Npts] = X[0];
    Yvec.elements.Flt[Npts] = Y[0];
    for (j = 1; j < 4; j++) {
      Xvec.elements.Flt[Npts + j*2 - 0] = X[j];
      Yvec.elements.Flt[Npts + j*2 - 0] = Y[j];
      Xvec.elements.Flt[Npts + j*2 - 1] = X[j];
      Yvec.elements.Flt[Npts + j*2 - 1] = Y[j];
    }
    Xvec.elements.Flt[Npts+7] = Xvec.elements.Flt[Npts];
    Yvec.elements.Flt[Npts+7] = Yvec.elements.Flt[Npts];
    Npts += 8;
    if (Npts > NPTS - 1) {  /* this is OK because NPTS is made always a multiple of 8 */
      NPTS += 200;
      REALLOCATE (Xvec.elements.Flt, opihi_flt, NPTS);
      REALLOCATE (Yvec.elements.Flt, opihi_flt, NPTS);
    }
  }
  ClearInterrupt (old_sigaction);

  gprint (GP_ERR, "plotting %d catalogs\n", Npts/8);
  Xvec.Nelements = Yvec.Nelements = Npts;
  if (Npts > 0) {
    graphmode.style = KAPA_PLOT_POINTS; /* points */
    graphmode.ptype = KAPA_POINT_PAIR_CONNECT; /* connect pairs of points */
    graphmode.etype = 0;
    PlotVectorPair (kapa, &Xvec, &Yvec, NULL, &graphmode);
  }

  free (Xvec.elements.Ptr);
  free (Yvec.elements.Ptr);
  free (regions);

  return (TRUE);

}


int RD_to_XYpic (double *x, double *y, double r, double d, Coords *coords, double Rmin, double Rmax, double Rmid, int *leftside) {

  r = ohana_normalize_angle (r);
  while (r < Rmin) { r += 360.0; }
  while (r > Rmax) { r -= 360.0; }

  if (*leftside == -1) {
    *leftside = (r < Rmid);
  } else {
    if (  *leftside && (r > Rmid + 90)) { r -= 360.0; }
    if (! *leftside && (r < Rmid - 90)) { r += 360.0; }
  }

  RD_to_XY (x, y, r, d, coords);

  return (TRUE);
}
