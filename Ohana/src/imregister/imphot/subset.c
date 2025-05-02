# include "imregister.h"
# include "imphot.h"

off_t *subset (Image *image, off_t Nimage, off_t *nsubset) {

  off_t i, j, status;
  off_t Nsubset, NameSelectLength;
  off_t *index;

  NameSelectLength = 0;

  /* allocate space for reference lists */
  Nsubset = 0;
  ALLOCATE (index, off_t, Nimage);
  
  if (criteria.NameSelect) {
    NameSelectLength = strlen(criteria.Name);
  }

  for (i = 0; i < Nimage; i++) {
    for (j = 0, status = FALSE; !status && (j < criteria.Ntimes); j++) {
      status = (image[i].tzero >= criteria.tstart[j]) && (image[i].tzero <= criteria.tstop[j]);
    }
    if (!status && criteria.Ntimes) continue;
    if (criteria.PhotcodeSelect && (image[i].photcode != criteria.photcode)) continue;
    if (criteria.CodeSelect     && (image[i].flags != criteria.Code)) continue;
    if (criteria.NameSelect     && strncasecmp (image[i].name, criteria.Name, NameSelectLength)) continue;

    index[Nsubset] = i;
    Nsubset ++;
  }
  *nsubset = Nsubset;
  return (index);
}
