# include "markstar.h"
int *make_common_list ();
int *check_common_list ();

mark_trail (catstats, mark, Nave, i, m, b, axis, catalog)
     CatStats catstats[];
     char *mark;
     int i, Nave, axis;
     double m, b;
     Catalog catalog[];
{

  double *R, *D;
  int j, k, jj, kk, N, NPTS, marked, start, end, Nm;
  int *good, *seq, *N1;
  double *dist, *dist2;
  double d2, di, Di, dD, n, Dist, scale, norm;
  double spacing;
  int *list, Nlist;
  int M, Nmeas, thisimage;

  R = catstats[0].X;
  D = catstats[0].Y;
  N1 = catstats[0].N;
  spacing = catstats[0].spacing;

  NPTS = 200;
  ALLOCATE (good, int, NPTS);
  ALLOCATE (dist, double, NPTS);
  ALLOCATE (dist2, double, NPTS);
  ALLOCATE (seq, int, NPTS);

  /* Find all points which lie near line */
  /* save the entry number and distance along line */
  N = 0;
  scale = sqrt (1.0 + m*m);

  if (axis == 1) {
    for (j = 0; j < Nave; j++) {
      norm = scale * fabs(R[j] - m*D[j] - b);
      if (!mark[j] && (norm < TRAIL_WIDTH)) {
	good[N] = j;
	dist[N] = scale * (D[j] - D[i] + m*(R[j] - R[i]));
	seq[N] = N;
	N++;
	if (N == NPTS - 1) {
	  NPTS += 200;
	  REALLOCATE (good, int, NPTS);
	  REALLOCATE (dist, double, NPTS);
	  REALLOCATE (dist2, double, NPTS);
	  REALLOCATE (seq, int, NPTS);
	}
      }
    }
  } else {
    for (j = 0; j < Nave; j++) {
      norm = scale * fabs(D[j] - m*R[j] - b);
      if (!mark[j] && (norm < TRAIL_WIDTH)) {
	good[N] = j;
	dist[N] = scale * (R[j] - R[i] + m*(D[j] - D[i]));
	seq[N] = N;
	N++;
	if (N == NPTS - 1) {
	  NPTS += 200;
	  REALLOCATE (good, int, NPTS);
	  REALLOCATE (dist, double, NPTS);
	  REALLOCATE (dist2, double, NPTS);
	  REALLOCATE (seq, int, NPTS);
	}
      }
    }
  }
  
  if (N < NPTSINLINE) 
    return (0);

  sort_seq (dist, seq, N);
  
  start = -1; end = -1;
  for (j = 0; j < N-1; j++) {
    /* if we have part of a line, and next point is in the line, check for common images */
    if ((start != -1) && (fabs(dist[j] - dist[j+1]) < spacing)) {
      list = check_common_list (list, N1[good[seq[j+1]]], &Nlist, catalog);
      if (Nlist == 0) { /* if no common images, dump list and continue */
	end = j + 1;
	if (end - start > NPTSINLINE) {
	  if (axis == 0)
	    fprintf (stderr, "marking line %f %f  %d pts  %d  %d %d, %f %f\n", m, b, N, end-start, start, end, dist[start], dist[end-1]);
	  else
	    fprintf (stderr, "marking line %f %f  %d pts  %d  %d %d, %f %f\n", 1.0/m, -1.0*b/m, N, end-start, start, end, dist[start], dist[end-1]);
	  for (k = start; k < end; k++) {
	    M = N1[good[seq[k]]];
	    mark[good[seq[k]]] = TRUE;
	    /* we need to mark measurements from all images in common on the line */
	    Nm = 0;
	    for (jj = 0; jj < catalog[0].average[M].Nm; jj++) {
	      Nmeas = catalog[0].average[M].offset + jj;
	      thisimage = catalog[0].image[Nmeas];
	      for (kk = 0; kk < Nlist; kk++) {
		if (thisimage == list[kk]) {
		  catalog[0].measure[Nmeas].average |= PART_OF_TRAIL;
		  Nm ++;
		}
	      }
	    }
	    /* if there is only 1 measurement, mark object as bad */
	    if (catalog[0].average[M].Nm == Nm) {
	      catalog[0].average[M].code = ID_TRAIL;
	    }
	    catalog[0].average[M].code = ID_TRAIL;
	  }
	}
	start = -1;
	end = -1;
      }
    }
    /* if we haven't yet found a line segment, check for the beginning */
    if ((start < 0) && (fabs(dist[j] - dist[j+1]) < spacing)) {
      start = j;
      list = make_common_list (N1[good[seq[j]]], N1[good[seq[j+1]]], &Nlist, catalog);
      if (Nlist == 0) { /* if no common images, move on */
	start = -1;
	free (list);
      }
    }
    /* if we have a complete line, check for validity.  if it has enough members,
	 mark them and continue searching for lines */
    if ((start != -1) && ((fabs(dist[j] - dist[j+1]) >= spacing) || (j == N-2))) {
      end = j + 1;
      if (end - start > NPTSINLINE) {
	if (axis == 0)
	  fprintf (stderr, "marking line %f %f  %d pts  %d  %d %d, %f %f\n", m, b, N, end-start, start, end, dist[start], dist[end-1]);
	else
	  fprintf (stderr, "marking line %f %f  %d pts  %d  %d %d, %f %f\n", 1.0/m, -1.0*b/m, N, end-start, start, end, dist[start], dist[end-1]);
	for (k = start; k < end; k++) {
	  M = N1[good[seq[k]]];
	  mark[good[seq[k]]] = TRUE;
	  if (catalog[0].average[M].code == ID_BLEED) continue;
	  /* we need to mark measurements from all images in common on the line */
	  Nm = 0;
	  for (jj = 0; jj < catalog[0].average[M].Nm; jj++) {
	    Nmeas = catalog[0].average[M].offset + jj;
	    thisimage = catalog[0].image[Nmeas];
	    for (kk = 0; kk < Nlist; kk++) {
	      if (thisimage == list[kk]) {
		catalog[0].measure[Nmeas].average |= PART_OF_TRAIL;
		Nm ++;
	      }
	    }
	  }
	  /* if there is only 1 measurement, mark object as bad */
	  if (catalog[0].average[M].Nm == Nm) {
	    catalog[0].average[M].code = ID_TRAIL;
	  }
	  catalog[0].average[M].code = ID_TRAIL;
	}
      }
      free (list);
      start = -1;
      end = -1;
    }
  }
}


/* I is Average seq number for star 1, J for star 2 */
/* make a list of images in common between two measurements */
int *make_common_list (I, J, Nlist, catalog)
int I, J, *Nlist;
Catalog catalog[];
{

  int i, j, N1, N2, nlist, NLIST;
  int *list;
  int k, already;

  NLIST = 50;
  ALLOCATE (list, int, NLIST);
  nlist = 0;

  for (i = 0; i < catalog[0].average[I].Nm; i++) {
    N1 = catalog[0].average[I].offset + i;
    if (catalog[0].image[N1] == -1)
      continue; /* not a real measurement */
    for (j = 0; j < catalog[0].average[J].Nm; j++) {
      N2 = catalog[0].average[J].offset + j;
      if (catalog[0].image[N2] == -1)
	continue; /* not a real measurement */
      if (catalog[0].image[N1] == catalog[0].image[N2]) {
	already = FALSE; 
	for (k = 0; !already && (k < nlist); k++) {
	  if (catalog[0].image[N1] == list[k]) { 
	    already = TRUE;
	  }
	}
	if (!already) {
	  list[nlist] = catalog[0].image[N1];
	  nlist ++;
	  if (nlist == NLIST - 1) {
	    NLIST += 50;
	    REALLOCATE (list, int, NLIST);
	  }
	}
      }
    }
  }
  
  REALLOCATE (list, int, MAX(nlist, 1));
  *Nlist = nlist;
  return (list);

}



/* J is Average seq number for star */

int *check_common_list (inlist, J, Nlist, catalog)
int *inlist, J, *Nlist;
Catalog catalog[];
{

  int i, j, N2, Ninlist;
  int *list, NLIST, nlist;
  int already, found, k;

  NLIST = 50;
  ALLOCATE (list, int, NLIST);
  nlist = 0;
  Ninlist = *Nlist;

  for (j = 0; j < catalog[0].average[J].Nm; j++) {
    N2 = catalog[0].average[J].offset + j;
    found = FALSE;
    for (i = 0; !found && (i < Ninlist); i++) {
      if (catalog[0].image[N2] == -1)
	continue; /* not a real measurement */
      if (inlist[i] == catalog[0].image[N2]) {
	found = TRUE;
	already = FALSE; 
	for (k = 0; !already && (k < nlist); k++) {
	  if (inlist[i] == list[k]) {
	    already = TRUE;
	  }
	}
	if (!already) {
	  list[nlist] = inlist[i];
	  nlist ++;
	  if (nlist == NLIST - 1) {
	    NLIST += 50;
	    REALLOCATE (list, int, NLIST);
	  }
	}
      }
    }
  }
  
  /* if there are no common images, return the input list */
  if (nlist != 0) {
    free (inlist);
    REALLOCATE (list, int, MAX(nlist, 1));
    *Nlist = nlist;
    return (list);
  } else {
    return (inlist);
  }

}


/* measurements associated with image number -1 are not 
   measurements we made, but are added, like USNO */
