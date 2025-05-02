# include "setphot.h"

/* sort a coordinate pair (X,Y) and the associated index (S) */
static void sort_zpts_by_time (ZptTable *zpts, int Nzpts) {
  
# define SWAPFUNC(A,B){ ZptTable tmp;		\
    tmp = zpts[A]; zpts[A] = zpts[B]; zpts[B] = tmp;	\
  }
# define COMPARE(A,B)(zpts[A].time < zpts[B].time)

  OHANA_SORT (Nzpts, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
}

# define SCALE 0.001

int match_zpts_to_images (Image *image, off_t Nimage, ZptTable *zpts, int Nzpts) {

  // I have two lists.  I need to match images->tzero to zpts->time.  multiple images may match a single zpt
  
  // sort both lists by time (or at least get sorted indices)
  // sweep through both lists, advancing the one that is lagging
  // apply the matches

  int dT, NImatch, Nmatch;
  off_t i, Ni, Nz, *index;
  PhotCode *code;

  // create index and sort
  ALLOCATE (index, off_t, Nimage);
  for (i = 0; i < Nimage; i++) {
    index[i] = i;
  }
  sort_image_subset(image, index, Nimage);  // slightly misnamed sort function from libdvo

  // sort the zpts
  sort_zpts_by_time (zpts, Nzpts);

  if (RESET) {
    for (i = 0; i < Nimage; i++) {
      image[i].McalPSF  = 0.0;
      image[i].McalAPER = 0.0;
      image[i].dMcal    = NAN;
      image[i].flags   &= ~ID_IMAGE_PHOTOM_NOCAL; // clear the NOCAL flag
      if (UBERCAL) {
	image[i].flags &= ~ID_IMAGE_PHOTOM_UBERCAL; // clear the UBERCAL flag
      }
    }
  }

  NImatch = 0; // matched images
  for (i = Nz = 0; (i < Nimage) && (Nz < Nzpts); ) {

    // if (i % 1000 == 0) fprintf (stderr, ".");
    // if (Nz % 100 == 0) fprintf (stderr, "!");

    if (!isfinite(zpts[Nz].zpt)) {
      Nz++;
      continue;
    }

    Ni = index[i];

    // UBERCAL zero points are only applied to gpc1 exposures (not stacks)
    if (UBERCAL) {
      if (image[Ni].photcode < 10000) { 
	i++;
	continue;
      }
      if (image[Ni].photcode > 10600) { 
	i++;
	continue;
      }
    }

    dT = image[Ni].tzero - zpts[Nz].time;

    // negative dT, i is too small (allow a 1sec overlap window)
    if (dT < -1) {
      i++;
      continue;
    }

    // XXX careful about definition of image->tzero and zpt->time
    // negative dT, i is too small (allow a 1sec overlap window)
    if (dT > +1) {
      Nz++;
      continue;
    }

    // check that we have a valid photcode (skip mosaic images)
    code = GetPhotcodebyCode(image[Ni].photcode);
    if (!code) { 
      i++;
      continue;
    }

    // if secz is crazy, do not trust ubercal; skip and move along
    if (image[Ni].secz < 0.1) {
      i++;
      continue;
    }

    // we have a match: set zpt and record the match
    // is the zero point supplied nominally corrected or not?
    // UBERCAL includes 2.5log(exptime) + K*airmass in the zero point
    if (UBERCAL) {
      image[Ni].McalPSF  = SCALE*code[0].C - zpts[Nz].zpt + 2.5*log10(image[Ni].exptime) + code[0].K*(image[Ni].secz - 1.000);
      image[Ni].McalAPER = SCALE*code[0].C - zpts[Nz].zpt + 2.5*log10(image[Ni].exptime) + code[0].K*(image[Ni].secz - 1.000);
      myAssert (isfinite(image[Ni].McalPSF), "oops, ubercal made a nan image");
    } else {
      image[Ni].McalPSF  = SCALE*code[0].C - zpts[Nz].zpt;
      image[Ni].McalAPER = SCALE*code[0].C - zpts[Nz].zpt;
    }

    // if we have defined zero point offsets, then apply them here
    float offset = apply_zpt_offset (code[0].equiv);
    assert (isfinite(offset));
    image[Ni].McalPSF  += offset;
    image[Ni].McalAPER += offset;

    image[Ni].dMcal = zpts[Nz].zpt_err;
    image[Ni].flags &= ~ID_IMAGE_PHOTOM_NOCAL; // clear the NOCAL flag
    // image[Ni].flags |=  ID_IMAGE_PHOTOM_EXTERN; XXX do we want some flag like this?
    if (UBERCAL) {
      image[Ni].flags |=  ID_IMAGE_PHOTOM_UBERCAL;
    }
    zpts[Nz].found = TRUE;
    NImatch ++;

    // advance the image counter only -- a single zpt may match more than one image
    i++;
  }

  // how many zpts have we matched?
  Nmatch = 0;
  for (Nz = 0; Nz < Nzpts; Nz++) {
    if (zpts[Nz].found) Nmatch ++;
  }

  fprintf (stderr, "found %d zpt matches, %d image matches\n", Nmatch, NImatch);

  return (TRUE);
}

