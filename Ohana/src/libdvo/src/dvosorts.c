# include <dvo.h>

/* several dvo-specific sorting functions used in a number of locations */

/* values are ave[i].R, ave[i].D, ave[i].M */
void sortave (Average *ave, off_t N) {

# define SWAPFUNC(A,B){ Average tmp; tmp = ave[A]; ave[A] = ave[B]; ave[B] = tmp; }
# define COMPARE(A,B)((!isfinite(ave[A].R) && isfinite(ave[B].R)) || (ave[A].R < ave[B].R))
// # define COMPARE(A,B)                                             (ave[A].R < ave[B].R)

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

/* sort subset by image[subset[i]].tzero */
void sort_image_subset (Image *image, off_t *subset, off_t N) {

# define SWAPFUNC(A,B){ int tmp; tmp = subset[A]; subset[A] = subset[B]; subset[B] = tmp; }
# define COMPARE(A,B)(image[subset[A]].tzero < image[subset[B]].tzero)

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

/* sort ImageMetadata array by image[i].imageID */
void sort_image_metadata (ImageMetadata *image, off_t Nimage) {

# define SWAPFUNC(A,B){ ImageMetadata tmp; tmp = image[A]; image[A] = image[B]; image[B] = tmp; }
# define COMPARE(A,B)(image[A].imageID < image[B].imageID)

  OHANA_SORT (Nimage, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

/* sort a coordinate pair (X,Y) and the associated index (S) */
void sort_coords_index (double *X, double *Y, off_t *S, off_t N) {
  
# define SWAPFUNC(A,B){ double dtmp; off_t itmp; \
  dtmp = X[A]; X[A] = X[B]; X[B] = dtmp; \
  dtmp = Y[A]; Y[A] = Y[B]; Y[B] = dtmp; \
  itmp = S[A]; S[A] = S[B]; S[B] = itmp; \
}
// # define COMPARE(A,B)                                     (X[A] < X[B])
# define COMPARE(A,B)((!isfinite(X[A]) && isfinite(X[B])) || (X[A] < X[B]))

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

/* sort a coordinate pair (X,Y) and the associated index (S) */
void sort_IDs_indexonly (opihi_int *X, off_t *S, off_t N) {
  
# define SWAPFUNC(A,B){ off_t itmp; \
  itmp = S[A]; S[A] = S[B]; S[B] = itmp; \
}
# define COMPARE(A,B)(X[S[A]] < X[S[B]])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

/* sort the index of a coordinate pair (X,Y) on X (vector pair stays unsorted) */
// XXX isn't this function equivalent to dsort_indexonly?
void sort_coords_indexonly (double *X, double *Y, off_t *S, off_t N) {
  OHANA_UNUSED_PARAM(Y);
  
# define SWAPFUNC(A,B){ off_t itmp; \
  itmp = S[A]; S[A] = S[B]; S[B] = itmp; \
}
# define COMPARE(A,B)((!isfinite(X[S[A]]) && isfinite(X[S[B]])) || (X[S[A]] < X[S[B]]))

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

void sort_regions (SkyRegion *region, off_t N) {

# define SWAPFUNC(A,B){ SkyRegion tmp; tmp = region[A]; region[A] = region[B]; region[B] = tmp; }
// # define COMPARE(A,B)(region[A].Dmin < region[B].Dmin)
# define COMPARE(A,B)((!isfinite(region[A].Dmin) && isfinite(region[B].Dmin)) || (region[A].Dmin < region[B].Dmin))

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}
