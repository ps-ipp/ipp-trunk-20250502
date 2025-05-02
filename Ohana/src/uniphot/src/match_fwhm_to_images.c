# include "uniphot.h"

/* sort a coordinate pair (X,Y) and the associated index (S) */
static void sort_fwhm_by_time (FWHMTable *fwhm, int Nfwhm) {
  
# define SWAPFUNC(A,B){ FWHMTable tmp;		\
    tmp = fwhm[A]; fwhm[A] = fwhm[B]; fwhm[B] = tmp;	\
  }
# define COMPARE(A,B)(fwhm[A].time < fwhm[B].time)

  OHANA_SORT (Nfwhm, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
}

# define SCALE 25.0

int match_fwhm_to_images (Image *image, off_t Nimage, FWHMTable *fwhm, int Nfwhm) {

  // I have two lists.  I need to match images->tzero to fwhm->time.  multiple images may match a single fwhm
  
  // sort both lists by time (or at least get sorted indices)
  // sweep through both lists, advancing the one that is lagging
  // apply the matches

  int dT, NImatch, Nmatch;
  off_t i, Ni, Nf, *index;
  float *fwhmMajorSet, *fwhmMinorSet;

  // create index and sort
  ALLOCATE (index, off_t, Nimage);
  for (i = 0; i < Nimage; i++) {
    index[i] = i;
  }
  sort_image_subset(image, index, Nimage);  // slightly misnamed sort function from libdvo

  // XXX GPC1 hack: accumulate the values within a single exposure (time) to provide the 
  // mosaic exposure with a mean or median
  int Ncode = 0;
  int NCODE = 100;
  ALLOCATE (fwhmMajorSet, float, NCODE);
  ALLOCATE (fwhmMinorSet, float, NCODE);

  // sort the fwhm
  sort_fwhm_by_time (fwhm, Nfwhm);

  NImatch = 0; // matched images
  for (i = Nf = 0; (i < Nimage) && (Nf < Nfwhm); ) {

    if (i % 1000 == 0) fprintf (stderr, ".");
    if (Nf % 100 == 0) fprintf (stderr, "!");

    Ni = index[i];
    dT = image[Ni].tzero - fwhm[Nf].time;

    // negative dT, i is too small (allow a 1sec overlap window)
    if (dT < -1) {
      i++;
      continue;
    }

    // XXX careful about definition of image->tzero and fwhm->time
    // negative dT, i is too small (allow a 1sec overlap window)
    if (dT > +1) {
      Nf++;
      continue;
    }

    {
	// loop over the exposure in this time block to get a median fwhm
	float fwhmMajorMedian = 0;
	float fwhmMinorMedian = 0;

	int NfS;
	int dTf = 0;
	Ncode = 0;
	for (NfS = Nf; (dTf < +1) && (NfS < Nfwhm); NfS++) {
	    dTf = image[Ni].tzero - fwhm[NfS].time;
	    if (dTf < -1 ) {
		break; // too far in fwhm sequence
	    }
	    
	    fwhmMajorSet[Ncode] = fwhm[NfS].fwhm_major;
	    fwhmMinorSet[Ncode] = fwhm[NfS].fwhm_minor;
	    Ncode ++;
	    if (Ncode == NCODE) {
		NCODE += 100;
		REALLOCATE(fwhmMajorSet, float, NCODE);
		REALLOCATE(fwhmMinorSet, float, NCODE);
	    }
	}

	// find the medians
	fsort (fwhmMajorSet, Ncode);
	fsort (fwhmMinorSet, Ncode);
	if (Ncode) {
	    if (Ncode % 2) {
		fwhmMajorMedian = fwhmMajorSet[(int)(Ncode/2)];
		fwhmMinorMedian = fwhmMinorSet[(int)(Ncode/2)];
	    } else {
		fwhmMajorMedian = 0.5*(fwhmMajorSet[(int)(Ncode/2)-1] + fwhmMajorSet[(int)(Ncode/2)]);
		fwhmMinorMedian = 0.5*(fwhmMinorSet[(int)(Ncode/2)-1] + fwhmMinorSet[(int)(Ncode/2)]);
	    }
	}

	// now find the mosaic image and set the median values
	int iS;
	int dTi = 0;
	for (iS = i; (dTi < +1) && (iS < Nimage); iS++) {
	    int NiS = index[iS];
	    dTi = image[NiS].tzero - fwhm[Nf].time;
	    if (dTi > 1) {
		break; // too far in image sequence
	    }
	    // XXX possible hack: if the mosaics ever get a photcode, this will break
	    if (image[NiS].photcode) continue;
	    image[NiS].fwhm_x = fwhmMajorMedian * SCALE;
	    image[NiS].fwhm_y = fwhmMinorMedian * SCALE;
	    break;
	}
    }

    // we have two sets: images[Ni,Ni+N] and fwhm[Nf,Nf+M], now we need to match them by photcode
    int iNext = i + 1;
    int NfNext = Nf + 1;
    int dTi = 0;
    int iS;
    for (iS = i; (dTi < +1) && (iS < Nimage); iS++) {
	int NiS = index[iS];
	dTi = image[NiS].tzero - fwhm[Nf].time;
	if (dTi > 1) {
	    iNext = iS;
	    break; // too far in image sequence
	}

	int dTf = 0;
	int NfS;
	for (NfS = Nf; (dTf < +1) && (NfS < Nfwhm); NfS++) {
	    dTf = image[Ni].tzero - fwhm[NfS].time;
	    if (dTf < -1 ) {
		NfNext = NfS;
		break; // too far in fwhm sequence
	    }

	    if (image[NiS].photcode == fwhm[NfS].photcode) {
		// we have a match: set zpt and record the match
		image[NiS].fwhm_x = fwhm[NfS].fwhm_major * SCALE;
		image[NiS].fwhm_y = fwhm[NfS].fwhm_minor * SCALE;
		fwhm[NfS].found = TRUE;
		NImatch ++;
	    }
	}
    }

    // advance the image counter only -- a single zpt may match more than one image
    Nf = NfNext;
    i = iNext;
  }

  // how many fwhm have we matched?
  Nmatch = 0;
  for (Nf = 0; Nf < Nfwhm; Nf++) {
      if (fwhm[Nf].found) {
	  Nmatch ++;
      } else {
	  // fprintf (stderr, "%d %d\n", fwhm[Nf].time, fwhm[Nf].photcode);
      }
  }

  fprintf (stderr, "found %d zpt matches, %d image matches\n", Nmatch, NImatch);

  return (TRUE);
}

