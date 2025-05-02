# include "dvorepair.h"

// I have updated this program to use a common arg-parsing function, with operation more similar to other ohana programs
// most of the modes here are not necessarily tested for current databases (parallel, compressed) and are disabled for now

int main (int argc, char **argv) {

  SetSignals ();
  dvorepair_help(argc, argv);
  dvorepair_args(&argc, argv);
  
  switch (MODE) {
    case DVOREPAIR_MODE_FixWarpIDs:
      dvorepairFixWarpIDs(argc, argv);
      break;
    case DVOREPAIR_MODE_FixStackIDs:
      dvorepairFixStackIDs(argc, argv);
      break;
    case DVOREPAIR_MODE_FixCPT:
      dvorepairFixCPT(argc, argv);
      break;
    case DVOREPAIR_MODE_BY_OBJ_ID:
      dvorepair_by_objID(argc, argv);
      break;
    case DVOREPAIR_MODE_ImagesVsMeasures:
      dvorepairImagesVsMeasures(argc, argv);
      break;
    case DVOREPAIR_MODE_DeleteImageList:
      dvorepairDeleteImageList(argc, argv);
      break;
    case DVOREPAIR_MODE_DeleteImagesByExternID:
      dvorepairDeleteImagesByExternID(argc, argv);
      break;
    case DVOREPAIR_MODE_DeleteImagesByExternID_v2:
      dvorepairDeleteImagesByExternID_v2(argc, argv);
      break;
    case DVOREPAIR_MODE_FixImages:
      dvorepairFixImages(argc, argv);
      break;
    default:
      fprintf (stderr, "unknown mode\n");
      exit (1);
  }

  exit (2);
}

  // fprintf (stderr, "this program needs to be updated to load old format Measure tables (pre PV1_V5) in which dR,dD are saved, not R,D\n");
  // fprintf (stderr, "reminder: relastro can re-construct R,D from X,Y; FtableToMeasure and vice versa could just NAN those values\n");
  // exit (2);

