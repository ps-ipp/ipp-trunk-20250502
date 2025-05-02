# include "dvopsps.h"

int main (int argc, char **argv) {

  int status;

  /* get configuration info, args, lockfile */
  initialize_dvopsps (argc, argv);

  status = -1;
  if (!strcasecmp (argv[1], "detections")) {
    status = insert_detections_dvopsps ();
  }
  if (!strcasecmp (argv[1], "objects")) {
    status = insert_objects_dvopsps ();
  }
  if (!strcasecmp (argv[1], "diffobj")) {
    status = insert_diffobj_dvopsps ();
  }
  if (!strcasecmp (argv[1], "forced_warp_objects")) {
    status = insert_FWobjects_dvopsps ();
  }
  if (!strcasecmp (argv[1], "forced_galaxy_shape")) {
    status = insert_FGshape_dvopsps ();
  }
  if (!strcasecmp (argv[1], "skytable")) {
    status = insert_skytable ();
  }
  
  if (status == -1) {
    fprintf (stderr, "invalid mode, should be : detections, skytable\n");
    exit (2);
  }

  if (!status) {
    fprintf (stdout, "ERROR running dvopsps!\n");
    exit (1);
  }
  fprintf (stdout, "SUCCESS!\n");
  exit (0);
}
  

/* dvopsps : extract the ids and other needed fields from dvo and insert in the mysql db
 */
