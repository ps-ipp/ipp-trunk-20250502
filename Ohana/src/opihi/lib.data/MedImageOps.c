# include "data.h"

/* this file contains functions to manage the median image data
 */

MedImageType **medimages = NULL; /* book to store the list of all splines */
int     Nmedimages;   /* number of currently defined medimages */
int     NMEDIMAGES;   /* number of currently allocated medimages */

void InitMedImages () {
  Nmedimages = 0;
  NMEDIMAGES = 16;
  ALLOCATE (medimages, MedImageType *, NMEDIMAGES); 
}

void FreeMedImages () {

  int i;

  for (i = 0; i < Nmedimages; i++) {
    FreeMedImage (medimages[i]);
  }
  free (medimages);
}

void FreeMedImage (MedImageType *medimage) {

  int i;

  if (!medimage) return;

  free (medimage[0].name);
  for (i = 0; i < medimage[0].Ninput; i++) {
    free (medimage[0].flx[i]);
    FREE (medimage[0].var[i]);
  }
  free (medimage[0].flx);
  free (medimage[0].var);
  free (medimage);
}

/* return the given medimage */
MedImageType *FindMedImage (char *name) {

  int i;

  if (!medimages) return NULL;

  for (i = 0; i < Nmedimages; i++) {
    if (!strcmp (medimages[i][0].name, name)) {
      return (medimages[i]);
    }
  }
  return (NULL);
}

/* make a new named medimage */
MedImageType *CreateMedImage (char *name, int Nx, int Ny) {

  int N;
  MedImageType *medimage;

  // do not create an image if one exists
  medimage = FindMedImage (name);
  if (medimage != NULL) {
    return NULL;
  }

  if (!medimages) InitMedImages ();

  N = Nmedimages;
  Nmedimages ++;
  CHECK_REALLOCATE (medimages, MedImageType *, NMEDIMAGES, Nmedimages, 16);
  ALLOCATE (medimage, MedImageType, 1);
  medimage->name = strcreate (name);
  medimage->Ninput = 0;
  medimage->Nx = Nx;
  medimage->Ny = Ny;
  ALLOCATE (medimage->flx, float *, 1);
  ALLOCATE (medimage->var, float *, 1);

  medimages[N] = medimage;
  return (medimage);
}

/* delete a medimage */
int DeleteMedImage (MedImageType *medimage) {

  int i, N, NMEDIMAGES_2;

  if (!medimages) return FALSE;

  /* find medimage in medimage list */
  N = -1;
  for (i = 0; i < Nmedimages; i++) {
    if (medimages[i] == medimage) {
      N = i;
      break;
    }
  }
  if (N == -1) return (FALSE);

  for (i = N; i < Nmedimages - 1; i++) {
    medimages[i] = medimages[i + 1];
  }
  Nmedimages --;
  NMEDIMAGES_2 = MAX (16, NMEDIMAGES / 2);
  if (Nmedimages < NMEDIMAGES_2) {
    NMEDIMAGES = NMEDIMAGES_2;
    REALLOCATE (medimages, MedImageType *, NMEDIMAGES);
  }

  FreeMedImage (medimage);
  return (TRUE);
}

/* list known medimages */
void ListMedImages () {

  int i;

  if (!medimages) return;

  for (i = 0; i < Nmedimages; i++) {
    gprint (GP_ERR, "%-15s %4d x %4d : %3d inputs\n", medimages[i][0].name, medimages[i][0].Nx, medimages[i][0].Ny, medimages[i][0].Ninput);
  }
  return;
}
