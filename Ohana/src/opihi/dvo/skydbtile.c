# include "dvo2.h"
SkyRegion *SkyDivideList (SkyRegion *list, int Nlist);

/* region names:

 depth Ntables  Name 
 0     1      	fullsky.cpt
 1     16     	n????.cpt, s????.cpt
 2     256    	n????/r????.cpt
 3     4096   	n????/t????.cpt
 4     65536  	n????/r????/r????.cpt
 5     1.0+06  	n????/t????/t????.cpt
 6     1.7+07  	n????/r????/r????/r????.cpt

 depth Ntables  Name 
 0     1      	fullsky
 1     16     	d%02
 2     256    	d%02/r%03d
 3     4096   	d%02/t%04d
 4     65536  	d%02/r%03d/r%03d
 5     1.0+06  	d%02/t%04d/t%04d
 6     1.7+07  	d%02/r%03d/r%03d/r%03d
 7     2.7+08  	d%02/t%04d/t%04d/t%04d

*/

/* a valid SkyRegion set must always start with the fullsky as the first entry, 
   with the sequence of entries ordered primarily by depth, followed by parent 
*/

int SkyMakeNames (SkyRegion *db, SkyRegion *ref, int depth) {

  int i, j, N, Ns, Ne, ns, ne;

  if (db == NULL) return (FALSE);
  if (depth == 0) {
    if (db[0].depth != 0) return (FALSE);
    strcpy (db[0].name, "fullsky");
    Ns = db[0].childS;
    Ne = db[0].childE;
    for (i = Ns; i < Ne; i++) {
      sprintf (db[i].name, "d%02d", i - 1);
      if (!SkyMakeNames (db, &db[i], 2)) return (FALSE);
      if (!SkyMakeNames (db, &db[i], 3)) return (FALSE);
    }
    return (TRUE);
  }

  if (ref == NULL) return (FALSE);
  
  if (depth == 2) {
    if (!ref[0].child) return (TRUE);
    Ns = ref[0].childS;
    Ne = ref[0].childE;
    N = 0;
    for (i = Ns; i < Ne; i++, N++) {
      sprintf (db[i].name, "%s/r%02d", ref[0].name, N);
      if (!SkyMakeNames (db, &db[i], depth + 2)) return (FALSE);
    }
  }
  
  if (depth > 2) {
    Ns = ref[0].childS;
    Ne = ref[0].childE;
    N = 0;
    for (i = Ns; i < Ne; i++) {
      if (!ref[0].child) continue;
      ns = db[i].childS;
      ne = db[i].childE;
      for (j = ns; j < ne; j++, N++) {
	if (depth % 2) {
	  sprintf (db[j].name, "%s/t%03d", ref[0].name, N);
	} else {				      			    
	  sprintf (db[j].name, "%s/r%02d", ref[0].name, N);
	}
	if (!SkyMakeNames (db, &db[j], depth + 2)) return (FALSE);
      }
    }  
  }
}

SkyRegion *SkyMakeRegions (int Nlevels, int *nlist) {

  int Nlist, Nnext, Ncurr, i, j;
  SkyRegion *list, *curr, *next;

  Nlist = 0;
  Nnext = 1;
  Ncurr = 1;
  ALLOCATE (list, SkyRegion, Ncurr);
  ALLOCATE (curr, SkyRegion, Ncurr);

  curr[0].Rmin =   0;
  curr[0].Rmax = 360;
  curr[0].Dmin = -90;
  curr[0].Dmax =  90;
  curr[0].depth = 0;
  curr[0].object = FALSE;
  curr[0].image = FALSE;
  curr[0].child = FALSE;
  curr[0].childS = 0;
  curr[0].childE = 0;

  for (i = 1; i < Nlevels; i++) {
    next  = SkyDivideRegions (curr, &Nnext);
    for (j = 0; j < Nnext; j++) {
      next[j].depth = i;
    }   
    REALLOCATE (list, SkyRegion, Nlist + Ncurr);
    memcpy (&list[Nlist], curr, Ncurr*sizeof(SkyRegion));
    for (j = Nlist; j < Nlist + Ncurr; j++) {
      list[j].childS += Nlist + Ncurr;
      list[j].childE += Nlist + Ncurr;
    }
    Nlist += Ncurr;
    free (curr);
    curr = next;
    Ncurr = Nnext;
  }
  REALLOCATE (list, SkyRegion, Nlist + Ncurr);
  memcpy (&list[Nlist], curr, Ncurr*sizeof(SkyRegion));
  Nlist += Ncurr;
  free (curr);

  list[0].object = TRUE;
  list[0].image = TRUE;
  *nlist = Nlist;
  return (list);
}

SkyRegion *SkyDivideRegions (SkyRegion *parents, int *N) {

  int i, Nchildren, Nparents, Np;
  SkyRegion *children, *temp;
  float *child, *index;

  Nchildren = Nparents = *N;

  /* copy input list to a new list, set child to be sequence number */
  ALLOCATE (children, SkyRegion, Nchildren);
  memcpy (children, parents, Nchildren*sizeof(SkyRegion));
  for (i = 0; i < Nchildren; i++) {
    children[i].childS = i;
    parents[i].childS = i;
  }

  /* double the number of children, dividing only the largest in half each pass */
  for (i = 0; i < 4; i++) {
    temp = SkyDivideList (children, Nchildren);
    free (children);
    children = temp;
    Nchildren *= 2;
  }

  /* sort children by childS (currently is parent entry) */
  ALLOCATE (child, float, Nchildren);
  ALLOCATE (index, float, Nchildren);
  for (i = 0; i < Nchildren; i++) {
    index[i] = i;
    child[i] = children[i].childS;
  }
  fsortpair (child, index, Nchildren);
  ALLOCATE (temp, SkyRegion, Nchildren);
  for (i = 0; i < Nchildren; i++) {
    temp[i] = children[(int)index[i]];
  }
  free (children);
  free (child);
  free (index);
  children = temp;
  
  /* look for all matching values of childS, setup parent values of childS, childE */
  Np = children[0].childS;
  parents[Np].childS = 0;
  parents[Np].child = TRUE;
  for (i = 0; i < Nchildren; i++) {
    if (children[i].childS != Np) {
      parents[Np].childE = i;
      Np = children[i].childS;
      parents[Np].childS = i;
      parents[Np].child = TRUE;
    }
    children[i].childS = 0;
    children[i].childE = 0;
    children[i].child = FALSE;
  }
  parents[Np].childE = i;
  *N = Nchildren;
  return (children);
}

/* always break the region with the largest area */
SkyRegion *SkyDivideBiggest (SkyRegion *list, int *nlist) {

  int i, N, Nlist;
  float *area, *indx, dec;
  SkyRegion *out, save;

  Nlist = *nlist;
  ALLOCATE (area, float, Nlist);
  ALLOCATE (indx, float, Nlist);
  for (i = 0; i < Nlist; i++) {
    dec = 0.5*(list[i].Dmax + list[i].Dmin);
    area[i] = (list[i].Rmax - list[i].Rmin)*(list[i].Dmax - list[i].Dmin)*cos(RAD_DEG*dec);
    indx[i] = i;
  }
  fsortpair (area, indx, Nlist);

  N = indx[Nlist - 1];
  out = SkyDivide (&list[N]);
  
  REALLOCATE (list, SkyRegion, Nlist + 1);
  list[N] = out[0];
  list[Nlist] = out[1];

  *nlist = Nlist + 1;
  free (out);
  free (area);
  free (indx);
  return (list);
}

SkyRegion *SkyDivide (SkyRegion *in) {

  SkyRegion *out;
  double RA, DEC, R_div, D_div;

  ALLOCATE (out, SkyRegion, 2);

  out[0] = in[0];
  out[1] = in[0];

  RA  = 0.5*(in[0].Rmin + in[0].Rmax);
  DEC = 0.5*(in[0].Dmin + in[0].Dmax);
  if ((DEC >= 90) || (DEC <= -90)) {
    gprint (GP_ERR, "error in Dmin");
    return (NULL);
  }
 
  D_div = (in[0].Rmax - in[0].Rmin) * cos (RAD_DEG*DEC);
  R_div = (in[0].Dmax - in[0].Dmin);
  if (R_div < D_div) {
    /* divide RA range */
    out[0].Rmax = RA;
    out[1].Rmin = RA;
  } else {
    out[0].Dmax = DEC;
    out[1].Dmin = DEC;
  }

  return (out);
}

/* return a list with 2x as many entries, each split in two */
SkyRegion *SkyDivideList (SkyRegion *in, int Nin) {

  SkyRegion *out;
  double RA, DEC, R_div, D_div;
  int i;

  ALLOCATE (out, SkyRegion, 2*Nin);

  for (i = 0; i < Nin; i++) {
    /* inherit properties from parent */
    out[2*i + 0] = in[i];
    out[2*i + 1] = in[i];

    RA  = 0.5*(in[i].Rmin + in[i].Rmax);
    DEC = 0.5*(in[i].Dmin + in[i].Dmax);
    
    /* split along longest axis */
    D_div = (in[i].Rmax - in[i].Rmin) * cos (RAD_DEG*DEC);
    R_div = (in[i].Dmax - in[i].Dmin);
    if (R_div < D_div) {
      out[2*i + 0].Rmax = RA;
      out[2*i + 1].Rmin = RA;
    } else {
      out[2*i + 0].Dmax = DEC;
      out[2*i + 1].Dmin = DEC;
    }
  }
  return (out);
}
