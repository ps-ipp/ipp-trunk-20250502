# include "photdbc.h"

void initialize (int argc, char **argv) {

  /* are these set correctly? */
  if (get_argument (argc, argv, "-h")) usage();
  if (get_argument (argc, argv, "--h")) usage();
  if (get_argument (argc, argv, "-help")) usage();
  if (get_argument (argc, argv, "--help")) usage();

  ConfigInit (&argc, argv);
  args (argc, argv);

  photcodesDrop = ParsePhotcodeList (PHOTCODE_DROP_LIST, &NphotcodesDrop, FALSE);
  photcodesKeep = ParsePhotcodeList (PHOTCODE_KEEP_LIST, &NphotcodesKeep, FALSE);

  if (SHOW_PARAMS) {
    fprintf (stderr, "current parameter settings:\n");
    fprintf (stderr, "VERBOSE:                %d\n", VERBOSE);
    fprintf (stderr, "JOIN_RADIUS:           %lf\n", JOIN_RADIUS);
    fprintf (stderr, "UNIQ_RADIUS:           %lf\n", UNIQ_RADIUS);
						     
    fprintf (stderr, "XMIN:                  %lf\n", XMIN);
    fprintf (stderr, "XMAX:                  %lf\n", XMAX);
    fprintf (stderr, "YMIN:                  %lf\n", YMIN);
    fprintf (stderr, "YMAX:                  %lf\n", YMAX);
    fprintf (stderr, "MMIN:                  %lf\n", MMIN);
    fprintf (stderr, "MMAX:                  %lf\n", MMAX);
    fprintf (stderr, "DMCAL_MIN:             %lf\n", DMCAL_MIN);
    fprintf (stderr, "DMSYS:                 %lf\n", DMSYS);
						     
    fprintf (stderr, "CHISQ_MAX:             %lf\n", CHISQ_MAX);
    fprintf (stderr, "NMEAS_MIN:             %d\n",  NMEAS_MIN);

    fprintf (stderr, "IMAGE_CATALOG          %s\n",  ImageCat);
    fprintf (stderr, "GSCFILE                %s\n",  GSCFILE);
    fprintf (stderr, "CATDIR                 %s\n",  CATDIR);
    fprintf (stderr, "PHOTCODE_FILE          %s\n",  PhotCodeFile);

    exit (0);
  }
}

void initialize_client (int argc, char **argv) {

  /* are these set correctly? */
  if (get_argument (argc, argv, "-h")) usage();
  if (get_argument (argc, argv, "--h")) usage();
  if (get_argument (argc, argv, "-help")) usage();
  if (get_argument (argc, argv, "--help")) usage();

  ConfigInit (&argc, argv);
  args_client (argc, argv);

  photcodesDrop = ParsePhotcodeList (PHOTCODE_DROP_LIST, &NphotcodesDrop, FALSE);
  photcodesKeep = ParsePhotcodeList (PHOTCODE_KEEP_LIST, &NphotcodesKeep, FALSE);

  if (SHOW_PARAMS) {
    fprintf (stderr, "current parameter settings:\n");
    fprintf (stderr, "VERBOSE:                %d\n", VERBOSE);
    fprintf (stderr, "JOIN_RADIUS:           %lf\n", JOIN_RADIUS);
    fprintf (stderr, "UNIQ_RADIUS:           %lf\n", UNIQ_RADIUS);
						     
    fprintf (stderr, "XMIN:                  %lf\n", XMIN);
    fprintf (stderr, "XMAX:                  %lf\n", XMAX);
    fprintf (stderr, "YMIN:                  %lf\n", YMIN);
    fprintf (stderr, "YMAX:                  %lf\n", YMAX);
    fprintf (stderr, "MMIN:                  %lf\n", MMIN);
    fprintf (stderr, "MMAX:                  %lf\n", MMAX);
    fprintf (stderr, "DMCAL_MIN:             %lf\n", DMCAL_MIN);
    fprintf (stderr, "DMSYS:                 %lf\n", DMSYS);
						     
    fprintf (stderr, "CHISQ_MAX:             %lf\n", CHISQ_MAX);
    fprintf (stderr, "NMEAS_MIN:             %d\n",  NMEAS_MIN);

    fprintf (stderr, "IMAGE_CATALOG          %s\n",  ImageCat);
    fprintf (stderr, "GSCFILE                %s\n",  GSCFILE);
    fprintf (stderr, "CATDIR                 %s\n",  CATDIR);
    fprintf (stderr, "PHOTCODE_FILE          %s\n",  PhotCodeFile);

    exit (0);
  }
}
