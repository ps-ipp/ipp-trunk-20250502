# include "imregister.h"
# include "imreg.h"

int SubmitImages (RegImage *image) {

  int i;
  char line[1024], *root, *path;

  /* send these images to FIFOs for imstat and ptolemy */ 

  if (image[0].mode == M_MEF) {
    for (i = 0; i < image[0].ccd; i++) {
      root = filerootname (image[0].filename);
      if (i < Nccd) {
	sprintf (line, "%s/%s %s/%s%s %s %s", image[0].pathname, image[0].filename, root, root, ccdn[i], ccds[i], "MEF");
      } else {
	sprintf (line, "%s/%s %s/%sxx none %s", image[0].pathname, image[0].filename, root, root, "MEF");
      }
      if (!WriteFIFO (ImstatFifo, line)) return (FALSE);
      if (image[0].type == T_OBJECT) {
	if (!WriteFIFO (PtolemyFifo, line)) return (FALSE);
      }
    }
  } 

  if (image[0].mode == M_SINGLE) {
    root = filerootname (image[0].filename);
    sprintf (line, "%s/%s %s %02d %s", image[0].pathname, image[0].filename, root, 0, "SINGLE");
    if (!WriteFIFO (ImstatFifo, line)) return (FALSE);
    if (image[0].type == T_OBJECT) {
      if (!WriteFIFO (PtolemyFifo, line)) return (FALSE);
    }
  } 

  if (image[0].mode == M_CUBE) {
    root = filerootname (image[0].filename);
    sprintf (line, "%s/%s %s %02d %s", image[0].pathname, image[0].filename, root, 0, "CUBE");
    if (!WriteFIFO (ImstatFifo, line)) return (FALSE);
    if (image[0].type == T_OBJECT) {
      if (!WriteFIFO (PtolemyFifo, line)) return (FALSE);
    }
  } 

  if (image[0].mode == M_SPLIT) {
    path = basename (image[0].pathname);
    root = filerootname (image[0].filename);
    sprintf (line, "%s/%s %s/%s %02d %s", image[0].pathname, image[0].filename, path, root, image[0].ccd, "SPLIT");
    if (!WriteFIFO (ImstatFifo, line)) return (FALSE);
    if (image[0].type == T_OBJECT) {
      if (!WriteFIFO (PtolemyFifo, line)) return (FALSE);
    }
  }    

  return (TRUE);
}
  
