# include "data.h"

int medimage_list (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);

  if (argc != 1) {
    gprint (GP_ERR, "USAGE: medimage list\n");
    return FALSE;
  }

  ListMedImages();
  return TRUE;
}

int medimage_add (int argc, char **argv) {

  int N;
  Buffer *image;
  Buffer *var = NULL;

  if ((N = get_argument (argc, argv, "-variance"))) {
    remove_argument (N, &argc, argv);
    if ((var = SelectBuffer (argv[N], OLDBUFFER, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: medimage add (name) (image) [-variance variance]\n");
    gprint (GP_ERR, "       add the given image to the set of images to be medianed\n");
    gprint (GP_ERR, "       optionally supply variance image (for weighted calculations)\n");
    return FALSE;
  }

  if ((image = SelectBuffer (argv[2], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  if (var) {
    if ((var->matrix.Naxis[0] != image->matrix.Naxis[0]) ||
	(var->matrix.Naxis[1] != image->matrix.Naxis[1])) {
      gprint (GP_ERR, "variance buffer does not match image buffer dimensions\n");
      return FALSE;
    }
  }

  MedImageType *median = FindMedImage (argv[1]);
  if (!median) {
    // create a new median with this name
    median = CreateMedImage (argv[1], image->matrix.Naxis[0], image->matrix.Naxis[1]);
    if (!median) {
      gprint (GP_ERR, "failed to generate a new median image\n");
      return FALSE;
    }
  }

  if (median->Nx != image->matrix.Naxis[0]) {
    gprint (GP_ERR, "image does not match medimage dimensions\n");
    return FALSE;
  }
  if (median->Ny != image->matrix.Naxis[1]) {
    gprint (GP_ERR, "image does not match medimage dimensions\n");
    return FALSE;
  }

  // new image should match existing medimage dimensions

  // AddMedImage (median, image, var);
  int Ninput = median->Ninput;
  median->Ninput ++;
  REALLOCATE (median->flx, float *, median->Ninput);
  REALLOCATE (median->var, float *, median->Ninput);

  ALLOCATE (median->flx[Ninput], float, median->Nx*median->Ny);
  memcpy (median->flx[Ninput], image->matrix.buffer, sizeof(float)*median->Nx*median->Ny);

  median->var[Ninput] = NULL;
  if (var) {
    ALLOCATE (median->var[Ninput], float, median->Nx*median->Ny);
    memcpy (median->var[Ninput], var->matrix.buffer, sizeof(float)*median->Nx*median->Ny);
  }

  return TRUE;
}

/* 
   int medimage_save (int argc, char **argv) {

   int N;

   int APPEND = FALSE;
   if ((N = get_argument (argc, argv, "-append"))) {
   APPEND = TRUE;
   remove_argument (N, &argc, argv);
   }

   if (argc != 3) {
   gprint (GP_ERR, "USAGE: medimage save (name) (filename) [-append]\n");
   return FALSE;
   }

   if (!SaveMedImage(argv[2], argv[1], APPEND)) {
   gprint (GP_ERR, "failed to save medimage %s\n", argv[1]);
   return (FALSE);
   }
   return TRUE;
   }

   int medimage_load (int argc, char **argv) {

   if (argc != 3) {
   gprint (GP_ERR, "USAGE: medimage load (name) (filename)\n");
   return FALSE;
   }

   if (!LoadMedImage(argv[2], argv[1])) {
   gprint (GP_ERR, "failed to load medimage %s\n", argv[1]);
   return (FALSE);
   }
   return TRUE;
   }
*/

int medimage_delete (int argc, char **argv) {

  int N, status;
  MedImageType *medimage;

  int QUIET = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    QUIET = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: medimage delete (medimage)\n");
    return FALSE;
  }

  medimage = FindMedImage (argv[1]);
  if (medimage == NULL) {
    if (QUIET) return TRUE;
    gprint (GP_ERR, "medimage %s not found\n", argv[1]);
    return FALSE;
  }

  status = DeleteMedImage (medimage);
  if (!status) abort ();
  return TRUE;
}

int medimage_rename (int argc, char **argv) {

  MedImageType *medimage;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: medimage rename (medimage) (newname)\n");
    return FALSE;
  }

  medimage = FindMedImage (argv[1]);
  if (medimage == NULL) {
    gprint (GP_ERR, "medimage %s not found\n", argv[1]);
    return FALSE;
  }

  free (medimage->name);
  medimage->name = strcreate (argv[2]);
  return TRUE;
}
