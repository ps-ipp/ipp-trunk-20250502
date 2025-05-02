# include "dvoshell.h"

/* extract vectors giving delta mags for multiple measurements */ 
int dmt (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);
  OHANA_UNUSED_PARAM(argv);
  
  // XXX this needs to be fixed: how to access different graphs at once?
  gprint (GP_ERR, "ERROR: this function is currently disabled\n");
  return (FALSE);

# if (0)

  int kapa, SaveVectors;
  int Nsec, Nsecfilt;
  off_t i, m, k, N, NPTS;
  double Radius;
  float dt1, dt2, dmt1, dmt2;
  float M0, M1, M2, M3;
  PhotCode *code;
  Catalog catalog;
  Graphdata graphmode, graphsky;
  SkyTable *sky;
  SkyList *skylist;
  Vector Xvec, Yvec, Zvec, Rvec, Dvec;
  Vector *vec1, *vec2, *vec3, *vec4, *vec5;

  Dvec.elements = Rvec.elements = Zvec.elements = NULL;

  if (!InitPhotcodes ()) return (FALSE);

  vec1 = vec2 = vec3 = vec4 = vec5 = NULL;
  SaveVectors = FALSE;
  if ((N = get_argument (argc, argv, "-vect"))) {
    remove_argument (N, &argc, argv);
    if ((vec1 = SelectVector ("dmtdmt", ANYVECTOR, TRUE)) == NULL) return (FALSE);
    if ((vec2 = SelectVector ("dmtvar", ANYVECTOR, TRUE)) == NULL) return (FALSE);
    if ((vec3 = SelectVector ("dmtmag", ANYVECTOR, TRUE)) == NULL) return (FALSE);
    if ((vec4 = SelectVector ("dmtra",  ANYVECTOR, TRUE)) == NULL) return (FALSE);
    if ((vec5 = SelectVector ("dmtdec", ANYVECTOR, TRUE)) == NULL) return (FALSE);
    SaveVectors = TRUE;
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: dmags filter\n");
    return (FALSE);
  }

  if (!GetGraphdata (&graphsky, &kapa, NULL)) return (FALSE);
  if (!GetGraph (&graphmode, NULL, NULL)) return (FALSE);

  if ((code = GetPhotcodebyName (argv[1])) == NULL) {
    gprint (GP_ERR, "ERROR: photcode not found in photcode table\n");
    return (FALSE);
  }
  if (code[0].type != PHOT_SEC) {
    gprint (GP_ERR, "first filter must be a average photometry type\n");
    return (FALSE);
  }
  Nsecfilt = GetPhotcodeNsecfilt();
  Nsec = GetPhotcodeNsec (code[0].code);

  Radius = MAX (fabs(graphsky.xmax), fabs(graphsky.ymax));

  sky = GetSkyTable ();
  skylist = SkyListByRadius (sky, -1, graphsky.coords.crval1, graphsky.coords.crval2, Radius);
  
  N = 0;
  NPTS = catalog.Nmeasure;
  ALLOCATE (Xvec.elements, float, NPTS);
  ALLOCATE (Yvec.elements, float, NPTS);
  if (SaveVectors) {
    ALLOCATE (Zvec.elements, float, NPTS);
    ALLOCATE (Rvec.elements, float, NPTS);
    ALLOCATE (Dvec.elements, float, NPTS);
  }

  for (k = 0; k < skylist[0].Nregions; k++) {
    /* lock, load, unlock catalog */
    dvo_catalog_init (&catalog, TRUE);
    catalog.filename = skylist[0].filename[k];
    catalog.catflags = DVO_LOAD_AVERAGE | DVO_LOAD_MEASURE | DVO_LOAD_SECFILT;
    catalog.Nsecfilt = 0;

    // an error exit status here is a significant error
    if (!dvo_catalog_open (&catalog, NULL, FALSE, "r")) {
      fprintf (stderr, "ERROR: failure to open catalog file %s\n", catalog.filename);
      exit (2);
    }
    dvo_catalog_unlock (&catalog);

    for (i = 0; i < catalog.Naverage; i++) {
      if (catalog.average[i].Nm != 3) continue;
      m = catalog.average[i].offset;
      M0 = catalog.secfilt[i*Nsecfilt+Nsec].M;
      M1 = PhotCat (&catalog.measure[m+0]);
      M2 = PhotCat (&catalog.measure[m+1]);
      M3 = PhotCat (&catalog.measure[m+2]);

      dt1 = (catalog.measure[m+0].t < catalog.measure[m+1].t) ? catalog.measure[m+1].t - catalog.measure[m+0].t : -1 * ((float)(catalog.measure[m+1].t - catalog.measure[m+0].t));
      dt2 = (catalog.measure[m+1].t < catalog.measure[m+2].t) ? catalog.measure[m+2].t - catalog.measure[m+1].t : -1 * ((float)(catalog.measure[m+2].t - catalog.measure[m+1].t));
      dmt1 = (M2 - M1) / dt1;
      dmt2 = (M3 - M2) / dt2;
      Xvec.elements[N] = (dmt1 - dmt2) / (dmt1 + dmt2);
      Yvec.elements[N] = (dmt1 + dmt2) / 2.0;
      if (SaveVectors) {
	Rvec.elements[N] = catalog.average[i].R;
	Dvec.elements[N] = catalog.average[i].D;
	Zvec.elements[N] = M0;
      }
      N++;
      if (N == NPTS - 1) {
	NPTS += 2000;
	REALLOCATE (Xvec.elements, float, NPTS);
	REALLOCATE (Yvec.elements, float, NPTS);
	if (SaveVectors) {
	  REALLOCATE (Zvec.elements, float, NPTS);
	  REALLOCATE (Rvec.elements, float, NPTS);
	  REALLOCATE (Dvec.elements, float, NPTS);
	}
      }
    }
    dvo_catalog_free (&catalog);
  }
  Yvec.Nelements = Xvec.Nelements = N;
  REALLOCATE (Xvec.elements, float, MAX (1, N));
  REALLOCATE (Yvec.elements, float, MAX (1, N));
  if (SaveVectors) {
    Rvec.Nelements = Dvec.Nelements = Zvec.Nelements = N;
    REALLOCATE (Zvec.elements, float, MAX (1, N));
    REALLOCATE (Rvec.elements, float, MAX (1, N));
    REALLOCATE (Dvec.elements, float, MAX (1, N));
  }

  if (SaveVectors) {
    free (vec1[0].elements);
    vec1[0].elements = Yvec.elements;
    vec1[0].Nelements = Yvec.Nelements;
    free (vec2[0].elements);
    vec2[0].elements = Xvec.elements;
    vec2[0].Nelements = Xvec.Nelements;
    free (vec3[0].elements);
    vec3[0].elements = Zvec.elements;
    vec3[0].Nelements = Zvec.Nelements;
    free (vec4[0].elements);
    vec4[0].elements = Rvec.elements;
    vec4[0].Nelements = Rvec.Nelements;
    free (vec5[0].elements);
    vec5[0].elements = Dvec.elements;
    vec5[0].Nelements = Dvec.Nelements;
  } else {
    graphmode.style = KAPA_PLOT_POINTS; /* points */
    PlotVector (kapa, N, Xvec.elements, Yvec.elements, &graphmode);

    free (Xvec.elements);
    free (Yvec.elements);
    free (Zvec.elements);
  }
  return (TRUE);
# endif 
}
