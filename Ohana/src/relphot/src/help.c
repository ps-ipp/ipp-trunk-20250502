# include "relphot.h"

void relphot_usage (int argc, char **argv) {
  fprintf (stderr, "ERROR: USAGE: relphot (photcodes) -images\n");
  fprintf (stderr, "       or:    relphot -averages\n");
  fprintf (stderr, "       or:    relphot -apply-offsets\n");
  fprintf (stderr, "       or:    relphot -synthphot_means\n");
  fprintf (stderr, "       or:    relphot (photcodes) -parallel-regions -region-hosts (RegionFile)\n");
  fprintf (stderr, "       or:    relphot (photcodes) -parallel-images (ImageTable) -region-hosts (RegionFile)\n\n");
  fprintf (stderr, "  regions:    -region RA RA DEC DEC)\n");
  fprintf (stderr, "       or:    -catalog (name)\n");
  fprintf (stderr, "  use -h for more usage information\n");

  fprintf (stderr, "unparsed arguments: ");
  for (int i = 0; i < argc; i++) {
    fprintf (stderr, "%s ", argv[i]);
  }
  fprintf (stderr, "\n");

  exit (2);
} 

void relphot_help (int argc, char **argv) {

  /* check for help request */
  if (get_argument (argc, argv, "-help")) goto show_help;
  if (get_argument (argc, argv, "-h"))    goto show_help;
  if (argc == 1) relphot_usage(0, NULL);
  return;

show_help:
  fprintf (stderr, "ERROR: USAGE: relphot (photcodes) -images\n");
  fprintf (stderr, "       or:    relphot -averages\n");
  fprintf (stderr, "       or:    relphot -apply-offsets\n");
  fprintf (stderr, "       or:    relphot (photcodes) -parallel-regions -region-hosts (RegionFile)\n");
  fprintf (stderr, "       or:    relphot (photcodes) -parallel-images (ImageTable) -region-hosts (RegionFile)\n\n");
  fprintf (stderr, "  regions:    -region RA RA DEC DEC)\n");
  fprintf (stderr, "       or:    -catalog (name)\n");
  fprintf (stderr, "  options: \n");
  fprintf (stderr, "  -time (start) (stop)\n");
  fprintf (stderr, "  -v : verbose output\n");
  fprintf (stderr, "  -vv : more verbose output\n");
  fprintf (stderr, "  -outroot (outroot)\n");
  fprintf (stderr, "  -plot\n");
  fprintf (stderr, "  -plotdelay (seconds)\n");
  fprintf (stderr, "  -statmode (mode)\n");
  fprintf (stderr, "  -refcode (name) : give extra weight to this photcode\n");
  fprintf (stderr, "  -n (nloop)\n");
  fprintf (stderr, "  -reset\n");
  fprintf (stderr, "  -update\n");
  fprintf (stderr, "  -params\n");
  fprintf (stderr, "  -mosaic (mosaic)\n");
  fprintf (stderr, "  -imfreeze\n");
  fprintf (stderr, "  -grid\n");
  fprintf (stderr, "  -reset-ubercal : also reset ubercal-ed zero points (otherwise they are sacrosanct)\n");
  fprintf (stderr, "  -area Xmin Xmax Ymin Ymax\n");
  fprintf (stderr, "  -instmag min max\n");
  fprintf (stderr, "  \n");
  exit (2);
}

void relphot_client_usage (void) {
  fprintf (stderr, "ERROR: USAGE: relphot (photcodes) -load (filename) -hostID (hostID) -hostdir (hostdir) [options]\n");
  fprintf (stderr, "       or:    relphot -update (filename) -hostID (hostID) -hostdir (hostdir) [options]\n");
  fprintf (stderr, "       or:    relphot -update-objects -hostID (hostID) -hostdir (hostdir) [options]\n");
  fprintf (stderr, "  use -h for more usage information\n");
  exit (2);
} 

void relphot_client_help (int argc, char **argv) {

  /* check for help request */
  if (get_argument (argc, argv, "-help")) goto show_help;
  if (get_argument (argc, argv, "-h"))    goto show_help;
  if (argc == 1) relphot_client_usage();
  return;

show_help:
  fprintf (stderr, "USAGE: relphot_client [-load / -update] (db info)\n\n");
  fprintf (stderr, "       relphot_client -load (bcatalog) : extract the bright catalog subset from client's tables\n");
  fprintf (stderr, "                            (bcatalog) : location where the bright subset is saved\n");
  fprintf (stderr, "       relphot_client -update (images) : apply calculated zero points to the client's tables\n\n");
  fprintf (stderr, "                              (images) : location of the table with the image zero points\n");
  fprintf (stderr, "       relphot_client -update-objects  : determine average magnitudes for objects\n\n");
  fprintf (stderr, "       db info : -hostID (hostID) -hostdir (hostdir) -catdir (catdir)\n");
  fprintf (stderr, "other options:\n");
  fprintf (stderr, "  -v  : verbose output\n");
  fprintf (stderr, "  -vv : extra verbose output\n");
  fprintf (stderr, "  \n");
  exit (2);
}

