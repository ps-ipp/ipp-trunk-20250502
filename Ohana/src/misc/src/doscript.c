# include <ohana.h>

void args(), error();
int COLDSTART, OUTIMAGE;
double THRESHOLD;

main (argc, argv) 
     int argc;
     char **argv;
{
  
  FILE *f, *ft;
  char name[128], root[128], line[256];
  double psfx, psfy, sky, tilt;

  args (argc, argv);

  while (fscanf (stdin, "%s", root) != EOF) {
    if (COLDSTART) {
      fscanf (stdin, "%lf %lf", &psfx, &sky);
    }
    else {
      fscanf (stdin, "%lf %lf %lf %lf", &psfx, &psfy, &tilt, &sky);
    }

    /* we demand that the image names have the *.f extension */
    if (strncmp (&root[strlen(root) - 2], ".f", 2)) {
      fprintf (stderr, "valid image names are *.f\n");
      exit (0);
    }

    ft = fopen (root, "r");
    if (ft == NULL) {
      fprintf (stderr, "image %s does not exist\n", root);
      continue;
    }
    fclose(ft);

    /* root contains *.f, remove the .f portion: */
    root[strlen(root) - 2] = 0;

    strcpy (name, root);
    strcat (name, ".par_in");
    f = fopen (name, "w");
    if (f == (FILE *) NULL) {
      fprintf (stderr, "couldn't write to parameter file %s\n", name);
      continue;
    }
    fprintf (f, "FWHM = %f\n", psfx);
    fprintf (f, "SKY = %f\n", sky);
    if (!COLDSTART) {
      fprintf (f, "AXIS_RATIO = %f\n", psfy/psfx);
      fprintf (f, "TILT = %f\n", tilt);
    }
    fprintf (f, "=\n");
    fprintf (f, "PARAMS_DEFAULT = 'dophot.params'\n");
    fprintf (f, "PARAMS_OUT = '%s.par_out'\n", root);
    if (THRESHOLD) {
      fprintf (f, "AUTOTHRESH = 'NO'\n");     
      fprintf (f, "THRESHMIN = %f\n", THRESHOLD);
    }
    else {
      fprintf (f, "AUTOTHRESH = 'YES'\n");     
    }
    fprintf (f, "=\n");
    fprintf (f, "IMAGE_IN = '%s.f'\n", root);
    if (OUTIMAGE) 
      fprintf (f, "IMAGE_OUT = '%s.s'\n", root);
    else
      fprintf (f, "IMAGE_OUT = ' '\n");
    if (!COLDSTART) {
      strcpy (name, root);
      strcat (name, ".obj_out");
      ft = fopen (name, "r");
      if (ft != (FILE *) NULL) {
	fclose (ft);
	sprintf (line, "mv %s.obj_out %s.obj_in\0", root, root);
	system (line);
	fprintf (f, "OBJECTS_IN = '%s.obj_in'\n", root);
      }
      else {
	fprintf (f, "OBJECTS_IN = ' '\n");
      }
    }
    else {
      fprintf (f, "OBJECTS_IN = ' '\n");
    }
    fprintf (f, "OBJECTS_OUT = '%s.obj_out'\n", root);
    fprintf (f, "=\n");

    if (!COLDSTART) {
      strcpy (name, root);
      strcat (name, ".shd_out");
      ft = fopen (name, "r");
      if (ft != (FILE *) NULL) {
	fclose (ft);
	sprintf (line, "mv %s.shd_out %s.shd_in\0", root, root);
	system (line);
	fprintf (f, "SHADOWFILE_IN = '%s.shd_in'\n", root);
      }
      else {
	fprintf (f, "SHADOWFILE_IN = ' '\n");
      }
    }
    else {
      fprintf (f, "SHADOWFILE_IN = ' '\n");
    }
    fprintf (f, "SHADOWFILE_OUT = '%s.shd_out'\n", root);
    
    fprintf (f, "END\n");
    fclose (f);

    fprintf (stdout, "dophot << END > %s.dout\n", root);
    fprintf (stdout, "%s.par_in\n", root);
    fprintf (stdout, "END\n");
  }
}

/* 
  original image: image
  flattened im:   image.f
  star sub image: image.s
  dp params in:   image.par_in
  dp params out:  image.par_out
  dp objects in:  image.obj_in
  dp objects out: image.obj_out
  dp shadow in:   image.shd_in
  dp shadow out:  image.shd_out
*/

void args (argc, argv)
     int      argc;
     char   **argv;
{
  
  int N;
  
  if (get_argument (argc, argv, "-h") || get_argument (argc, argv, "-help")) {
    error ();
  }

  COLDSTART = TRUE;
  if (N = get_argument (argc, argv, "-w")) {
    remove_argument (N, &argc, argv);
    COLDSTART = FALSE;
  }
  if (N = get_argument (argc, argv, "-warm")) {
    remove_argument (N, &argc, argv);
    COLDSTART = FALSE;
  }
  
  OUTIMAGE = FALSE;
  if (N = get_argument (argc, argv, "-sub")) {
    remove_argument (N, &argc, argv);
    OUTIMAGE = TRUE;
  }

  THRESHOLD = 0;
  if (N = get_argument (argc, argv, "-t")) {
    remove_argument (N, &argc, argv);
    if (N < argc) {
      THRESHOLD = atof (argv[N]);
    } 
    else {
      error ();
    }   
  }
}

void error ()
{
  fprintf (stderr, "USAGE:\n");
  fprintf (stderr, "  doscript [-w/-warm] [-sub] [-t threshold]\n");
  fprintf (stderr, "    default is coldstart -- takes from stdin: filename FWHM sky\n");
  fprintf (stderr, "    -w, -warm: warmstart -- takes from stdin: filename FWHMx FWHMy angle sky'\n");
  fprintf (stderr, "    -t: set minimum threshold, also sets AUTOTHRESH = 'NO'\n");
  fprintf (stderr, "    -sub: generate star-substracted images\n");
  exit (0);
}
