# include "imregister.h"
# include "imreg.h"

RegImage *newimages (RegImage *image, off_t *Nimage) {

  off_t i, Nnew;
  RegImage *regimage;

  Nnew = 0;
  regimage = NULL;

  /* identify new entries based on mode */
  switch (image[0].mode) {
  case M_CUBE:
    Nnew = image[0].seq + 1;
    ALLOCATE (regimage, RegImage, Nnew);
    regimage[0] = image[0];
    for (i = 1; i < Nnew; i++) {
      regimage[i] = image[0]; 
      regimage[i].seq = i - 1;
      regimage[i].mode = M_SLICE;
      regimage[i].obstime += image[0].seqtime*regimage[i].seq;
    }
    break;
  case M_MEF:
    Nnew = image[0].ccd;
    ALLOCATE (regimage, RegImage, Nnew);
    for (i = 0; i < Nnew; i++) {
      regimage[i] = image[0]; 
      regimage[i].ccd = i;
    }
    break;
  case M_SPLIT:
  case M_SINGLE:
    Nnew = 1;
    regimage = &image[0];
    break;
  }
  *Nimage = Nnew;
  return (regimage);
}


/* meaning of image fields for different data modes:

   mode      ccd                    seq
   SINGLE    0                      0
   SPLIT     ccd seq number         0
   MEF       Nccd                   0
   CUBE      ccd seq number         Nseq
   SLICE     ccd seq number         seq number
   CUBE-MEF  Nccd                   Nseq
   SLICE-MEF Nccd                   seq number

*/

/*
 we need to consider modifying this to separately ID MEF/SPLIT/SINGLE and CUBE/SLICE 
 for the moment, we are keeping this the same as the old style
*/
