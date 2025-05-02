# include "dvoImagesAtCoords.h"
# ifndef FLT_MAX
# define FLT_MAX 1e32
# endif
int ComputeLMLimits(double xmin, double xmax, double ymin, double ymax, 
            double *Lmin, double *Lmax, double *Mmin, double *Mmax, Coords *coords);

/* given coordinate, find images in list that contain the point */
off_t MatchCoords (Image *dbImages, off_t NdbImages, Point *points, int Npoints) {
  
  off_t i;
  int j;
  int totalMatches = 0;
  double Xi[4], Yi[4];  /* image and original corners */
  double xmin, xmax, ymin, ymax;
  double Lmin, Lmax, Mmin, Mmax;


  /* setup links for mosaic WRP and DIS entries */
  BuildChipMatch (dbImages, NdbImages);

  /* run through image table and search for overlaps
     also define the vtable entries for the images we keep  */

  int haveLimits = 0;
  for (i = 0; i < NdbImages; i++) {
      haveLimits = 0;

    if (!WITH_PHU && !strcmp (&dbImages[i].coords.ctype[4], "-DIS")) continue;
    if ( SOLO_PHU &&  strcmp (&dbImages[i].coords.ctype[4], "-DIS")) continue;

    /* define image corners */
    SetImageCorners (Xi, Yi, &dbImages[i]);
    // Xi[3] = Xi[0]; Yi[3] = Yi[0];

    ymin = xmin = +FLT_MAX;
    ymax = xmax = -FLT_MAX;
    for (j = 0; j < 4; j++) {
        xmin = MIN(xmin, Xi[j]);
        xmax = MAX(xmax, Xi[j]);
        ymin = MIN(ymin, Yi[j]);
        ymax = MAX(ymax, Yi[j]);
    }

    for (j = 0; j < Npoints; j++) {
        Point *pt = points + j;

        // transform input point to image coords
        double x, y;
        int status = RD_to_XY(&x, &y, pt->ra, pt->dec, &dbImages[i].coords);

        if (!status) {
            // avoid matching antipodal skycells
            continue;
        }
        
        if (VERBOSE) fprintf(stderr, OFF_T_FMT" %8.1f %8.1f %s\n", i, x, y, dbImages[i].name);

        if ((x >= xmin && x <= xmax) && (y >= ymin && y <= ymax)) {
            if (VERBOSE) fprintf(stderr, "\tpotential overlap for point %d image %i\n", j, (int) i);

            // We may be using the astrometry far outside it's range of validity.
            // As a sanity check make sure that the focal plane coords are in bounds.
            // See ticket 1381
            if (!haveLimits) {
                // We defer computing the focal plane limits until we need them
                status = ComputeLMLimits(xmin, xmax, ymin, ymax, 
                    &Lmin, &Lmax, &Mmin, &Mmax, &dbImages[i].coords);
                if (!status) {
                    continue;
                }
                haveLimits = 1;
            }

            double L, M;
            status = RD_to_LM(&L, &M, pt->ra, pt->dec, &dbImages[i].coords);
            if (!status) {
                // This won't happen because RD_to_XY succeeded above
                continue;
            }

            if (VERBOSE) fprintf(stderr, "L: %f M: %f\n", L, M);

            if (L < Lmin || L > Lmax || M < Mmin || M > Mmax) {
                if (VERBOSE) fprintf(stderr, "\trejecting match for because focal plane coordinates are out of range\n");
                continue;
            }
            if (VERBOSE) fprintf(stderr, "\tmatch confirmed\n");

            totalMatches++;

            pt->matches[pt->Nmatches].n = i;
            pt->matches[pt->Nmatches].x = x;
            pt->matches[pt->Nmatches].y = y;
            pt->Nmatches ++;

            if (pt->Nmatches == pt->arrayLength) {
                pt->arrayLength += 20;
                REALLOCATE (pt->matches, Match, pt->arrayLength);
            }
        }
    }
 }
 if (VERBOSE) fprintf (stderr, "found %d overlapping images\n", totalMatches);
  
 return (totalMatches);
}
  
void SetImageCorners (double *X, double *Y, Image *image) {

  if (!strcmp(&image[0].coords.ctype[4], "-DIS")) {
    X[0] = -0.5*image[0].NX; Y[0] = -0.5*image[0].NY;
    X[1] = +0.5*image[0].NX; Y[1] = -0.5*image[0].NY;
    X[2] = +0.5*image[0].NX; Y[2] = +0.5*image[0].NY;
    X[3] = -0.5*image[0].NX; Y[3] = +0.5*image[0].NY;
  } else {
    X[0] = 0;           Y[0] = 0;
    X[1] = image[0].NX; Y[1] = 0;
    X[2] = image[0].NX; Y[2] = image[0].NY;
    X[3] = 0;           Y[3] = image[0].NY;
  }
}

// compute bounding box of this image in focal plane coordinates
int ComputeLMLimits(double xmin, double xmax, double ymin, double ymax, 
            double *Lmin, double *Lmax, double *Mmin, double *Mmax, Coords *coords)
{
    double L0, L1, M0, M1;
    int status = XY_to_LM(&L0, &M0, xmin, ymin, coords);
    if (!status) {
        fprintf(stderr, "failed to transform image corner to LM\n");
        return 0;
    }
    status = XY_to_LM(&L1, &M1, xmax, ymax, coords);
    if (!status) {
        fprintf(stderr, "failed to transform image corner to LM\n");
        return 0;
    }

    if (L1 > L0) {
        *Lmin = L0;
        *Lmax = L1;
    } else {
        *Lmin = L1;
        *Lmax = L0;
    }
    if (M1 > M0) {
        *Mmin = M0;
        *Mmax = M1;
    } else {
        *Mmin = M1;
        *Mmax = M0;
    }
    if (VERBOSE) fprintf(stderr, "Lmin: %f Lmax: %f Mmin: %f Mmax: %f\n", *Lmin, *Lmax, *Mmin, *Mmax);
    return 1;
}
