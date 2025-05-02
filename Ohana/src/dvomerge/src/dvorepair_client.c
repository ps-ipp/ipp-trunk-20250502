# include "dvorepair.h"

int main (int argc, char **argv) {

  // check various options
  SetSignals ();

  SkyRegion UserPatch[2]; // 1 or 2 UserPatch areas are set by FindDeleteRegion
  int nUserPatch = dvorepair_client_args (&argc, argv, UserPatch);
  
  if (argc != 2) dvorepair_client_args (0, NULL, NULL);
  CATDIR = argv[1];

  DeleteImageDataType deleteImageData;

  char filename[DVO_MAX_PATH];

  sprintf (filename, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (filename, NULL, FALSE)) {
    fprintf (stderr, "error reading photcodes from %s\n", CATDIR);
    exit (1);
  }	

  snprintf (filename, DVO_MAX_PATH, "%s/DeleteImages.fits", CATDIR);
  if (!DeleteImagesLoad (filename, &deleteImageData)) {
    fprintf (stderr, "ERROR: failure to save delete image info\n");
    exit (2);
  }

  int status = dvorepairDeleteImagesByExternID_catalogs (UserPatch, nUserPatch, NULL, deleteImageData.Nimage, deleteImageData.deleteImage, deleteImageData.imageIDindex);

  if (!status) {
    fprintf (stderr, "ERROR: failure on hostID %d\n", HOST_ID);
    exit (1);
  }
  fprintf (stderr, "SUCCESS on hostID %d\n", HOST_ID);
  exit (0);
}

