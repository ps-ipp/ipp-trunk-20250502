# include "dvomerge.h"
# define PSPS_ID TRUE
# define D_ITEM 100000

# define IN_REGION(R,D) (					\
    ((D) >= region[0].Dmin) && ((D) < region[0].Dmax) &&	\
    ((R) >= region[0].Rmin) && ((R) < region[0].Rmax))

// merge the input data into the output catalog
// input entries are matched by position to the new objects
int merge_catalogs_old (SkyRegion *region, Catalog *output, Catalog *input, double RADIUS, int *secfiltMap) {

  off_t i, j, k, Nin, offset, J, Jmin, status, Nstars;
  double RADIUS2, Rmin;
  double *X1, *Y1, *X2, *Y2;
  double dX, dY, dR;
  off_t *N1, *N2, *next_measure, *next_lensing, *next_lensobj, *next_starpar, *next_galphot;
  off_t Nave, NAVE, Nmeasure, NMEASURE, Nmatch, Nlensing, NLENSING, Nlensobj, NLENSOBJ, Nstarpar, NSTARPAR, Ngalphot, NGALPHOT;
  int NsecfiltIn;
  int NsecfiltOut;
  unsigned int objID, catID;
  Coords tcoords;
  
  // INITTIME;

  NsecfiltOut = output[0].Nsecfilt;
  NsecfiltIn  = input[0].Nsecfilt;
  assert (secfiltMap || (NsecfiltOut == NsecfiltIn));

  /** allocate local arrays (stars) **/
  ALLOCATE (X1, double, input[0].Naverage);
  ALLOCATE (Y1, double, input[0].Naverage);
  ALLOCATE (N1, off_t,  input[0].Naverage);
  if (!input[0].found_t) {
    ALLOCATE (input[0].found_t, off_t, input[0].Naverage);
  } else {
    REALLOCATE (input[0].found_t, off_t, input[0].Naverage);
  }

  /** allocate local arrays (catalog) **/
  NAVE = Nave = output[0].Naverage;
  ALLOCATE (X2, double, NAVE);
  ALLOCATE (Y2, double, NAVE);
  ALLOCATE (N2, off_t,    NAVE);
  if (!output[0].found_t) {
    ALLOCATE (output[0].found_t, off_t, NAVE);
  } else {
    REALLOCATE (output[0].found_t, off_t, NAVE);
  }
  /* for secfilt j and star i, secfilt[i*Nsecfilt+j] */

  /* internal counters */
  Nmatch = 0;
  NMEASURE = Nmeasure = output[0].Nmeasure;
  NLENSING = Nlensing = output[0].Nlensing;
  NLENSOBJ = Nlensobj = output[0].Nlensobj;
  NSTARPAR = Nstarpar = output[0].Nstarpar;
  NGALPHOT = Ngalphot = output[0].Ngalphot;

  // current max obj ID for this catalog
  objID = output[0].objID;
  catID = output[0].catID;

  /* project onto rectilinear grid with 1 arcsec pixels. the choice of ZEA projection has the
   * advantage that every point in R,D has a mapping to a unique X,Y.  However, note that not all
   * possible X,Y points map back to R,D and the local plate scale changes substantially far from
   * the projection pole.  a better mapping might be ARC, not yet implemented (see
   * coordops.update.c).  We use the center of the region (catalog) for crval1,2. 
   */

  InitCoords (&tcoords, "DEC--ARC");
  tcoords.crval1 = 0.5*(region[0].Rmin + region[0].Rmax);
  if (region[0].Dmax < 90) {
    tcoords.crval2 = 0.5*(region[0].Dmin + region[0].Dmax);
  } else {
    tcoords.crval2 = 90.0;
  }
  tcoords.cdelt1 = tcoords.cdelt2 = 1.0 / 3600.0;

  if (VERBOSE) fprintf (stderr, "merging %s into %s\n", input[0].filename, output[0].filename);

  /* build spatial index (RA sort) referencing input array sequence */
  Nstars = 0;
  for (i = 0; i < input[0].Naverage; i++) {
    status = RD_to_XY (&X1[Nstars], &Y1[Nstars], input[0].average[i].R, input[0].average[i].D, &tcoords);
    if (!status) continue;
    N1[Nstars] = i;
    Nstars ++;
    input[0].found_t[i] = FALSE;
  }
  if (Nstars < 1) {
    if (VERBOSE) fprintf (stderr, "skipping %s, no overlapping stars\n", output[0].filename);
    free (X1);
    free (Y1);
    free (N1);
    free (X2);
    free (Y2);
    free (N2);
    return (0);
  }
  if (Nstars > 1) sort_coords_index (X1, Y1, N1, Nstars);

  /* build spatial index (RA sort) */
  for (i = 0; i < Nave; i++) {
    RD_to_XY (&X2[i], &Y2[i], output[0].average[i].R, output[0].average[i].D, &tcoords);
    output[0].found_t[i] = -1;
    N2[i] = i;
  }
  if (Nave > 1) sort_coords_index (X2, Y2, N2, Nave);

  if (REPLACE_BY_PHOTCODE && output[0].Nmeasure && !output[0].sorted) {
    fprintf (stderr, "ERROR: attempt to merge with replace into an unsorted database\n");
    exit (3);
  }

  /* set up pointers for linked list of measure */
  if (output[0].sorted) {
    // this version is only valid if we have done a full catalog load, and if the catalog
    // is sorted while processed
    next_measure = init_measure_links (output[0].average, Nave, output[0].measure, Nmeasure);
    next_lensing = init_lensing_links (output[0].average, Nave, output[0].lensing, Nlensing);
    next_lensobj = init_lensobj_links (output[0].average, Nave, output[0].lensobj, Nlensobj);
    next_starpar = init_starpar_links (output[0].average, Nave, output[0].starpar, Nstarpar);
    next_galphot = init_galphot_links (output[0].average, Nave, output[0].galphot, Ngalphot);
  } else {
    next_measure = build_measure_links (output[0].average, Nave, output[0].measure, Nmeasure);
    next_lensing = build_lensing_links (output[0].average, Nave, output[0].lensing, Nlensing);
    next_lensobj = build_lensobj_links (output[0].average, Nave, output[0].lensobj, Nlensobj);
    next_starpar = build_starpar_links (output[0].average, Nave, output[0].starpar, Nstarpar);
    next_galphot = build_galphot_links (output[0].average, Nave, output[0].galphot, Ngalphot);
  }    

  /* choose a radius for matches */
  RADIUS2 = RADIUS*RADIUS;

  // MARKTIME("set up structures: %f sec\n", dtime);

  /** find matched stars **/
  for (i = j = 0; (i < Nstars) && (j < Nave); ) {
    if (!finite(X1[i]) || !finite(Y1[i])) { 
      i++; 
      continue;
    }
    if (!finite(X2[j]) || !finite(Y2[j])) { 
      j++; 
      continue;
    }
    
    /* negative dX: j is too large */
    dX = X1[i] - X2[j];
    if (dX <= -1.02*RADIUS) {
      i++;
      continue;
    }
    /* positive dX, i is too large */
    if (dX >= 1.02*RADIUS) {
      j++;
      continue;
    }

    Jmin = -1;
    Rmin = RADIUS2;
    for (J = j; (dX > -1.02*RADIUS) && (J < Nave); J++) {
      /* find closest match for this detection */
      dX = X1[i] - X2[J];
      dY = Y1[i] - Y2[J];
      dR = dX*dX + dY*dY;
      if (dR > RADIUS2) continue;
      if (dR < Rmin) {
	Rmin = dR;
	Jmin  = J;
      }
    }

    /* no match, try next detection */ 
    if (Jmin == -1) {
      i++;
      continue;
    }

    /*** a match is found, add to average, measure ***/
    Nmatch ++;
    off_t n = N2[Jmin];
    off_t N = N1[i];

    input[0].found_t[N] = TRUE;

    /* make sure there is space for next Nmeasure entries */
    if (Nmeasure + input[0].average[N].Nmeasure >= NMEASURE) {
      NMEASURE = Nmeasure + input[0].average[N].Nmeasure + D_ITEM;
      REALLOCATE (next_measure, off_t, NMEASURE);
      REALLOCATE (output[0].measure, Measure, NMEASURE);
    }
    if (Nlensing + input[0].average[N].Nlensing >= NLENSING) {
      NLENSING = Nlensing + input[0].average[N].Nlensing + D_ITEM;
      REALLOCATE (next_lensing, off_t, NLENSING);
      REALLOCATE (output[0].lensing, Lensing, NLENSING);
    }
    if (Nlensobj + input[0].average[N].Nlensobj >= NLENSOBJ) {
      NLENSOBJ = Nlensobj + input[0].average[N].Nlensobj + D_ITEM;
      REALLOCATE (next_lensobj, off_t, NLENSOBJ);
      REALLOCATE (output[0].lensobj, Lensobj, NLENSOBJ);
    }
    if (Nstarpar + input[0].average[N].Nstarpar >= NSTARPAR) {
      NSTARPAR = Nstarpar + input[0].average[N].Nstarpar + D_ITEM;
      REALLOCATE (next_starpar, off_t, NSTARPAR);
      REALLOCATE (output[0].starpar, StarPar, NSTARPAR);
    }
    if (Ngalphot + input[0].average[N].Ngalphot >= NGALPHOT) {
      NGALPHOT = Ngalphot + input[0].average[N].Ngalphot + D_ITEM;
      REALLOCATE (next_galphot, off_t, NGALPHOT);
      REALLOCATE (output[0].galphot, GalPhot, NGALPHOT);
    }

    // 4) average properties from the input and the output db need to be properly merged.

    /** add ALL measurements for this input average object **/
    if (output[0].measure && !SKIP_MEASURE) {

      int Nreplace = 0;
      if (REPLACE_TYCHO) {
	int Minp =  input[0].average[N].measureOffset;
	Nreplace = replace_tycho (&output[0].average[n], output[0].measure, next_measure, &input[0].average[N], &input[0].measure[Minp]);
	if (Nreplace == 6) {
	  output[0].found_t[n] = Nmeasure;
	  i++;
	  continue;
	  // XXX this is probably a bad idea: breaks other tables
	}
      }

      for (Nin = Nreplace; Nin < input[0].average[N].Nmeasure; Nin ++) {
	offset = input[0].average[N].measureOffset + Nin;

	if (REPLACE_BY_PHOTCODE) {
	  // index to first measure for this object
	  // XXX this does not support lensing, starpar, or galphot measurements
	  if (replace_match (&output[0].average[n], output[0].measure, next_measure, &input[0].average[N], &input[0].measure[offset])) {
	    continue;
	  }
	}
	/* add to end of measurement list */
	add_measure_link (&output[0].average[n], next_measure, Nmeasure, NMEASURE);
	
	// set the new measurements
	output[0].measure[Nmeasure] = input[0].measure[offset];

	// old code: find R,D using average_in[0], the get offset relative to average_out[0].  no longer
	// needed since we carry around R,D
	// Rin = input[0].average[N].R - input[0].measure[offset].dR / 3600.0;
	// Din = input[0].average[N].D - input[0].measure[offset].dD / 3600.0;
	// output[0].measure[Nmeasure].dR = 3600.0*(output[0].average[n].R - Rin);
	// output[0].measure[Nmeasure].dD = 3600.0*(output[0].average[n].D - Din);

	output[0].measure[Nmeasure].dbFlags  = 0;  // XXX why reset these?
	output[0].measure[Nmeasure].averef   = n;
	output[0].measure[Nmeasure].objID    = output[0].average[n].objID;
	output[0].measure[Nmeasure].catID    = output[0].catID;

	assert (output[0].measure[Nmeasure].averef < Nave);

	// fprintf (stderr, "Nave : "OFF_T_FMT", Nmeasure : "OFF_T_FMT", dR: %f, dD: %f, catID: %d\n",  n,  Nmeasure, output[0].measure[Nmeasure].dR, output[0].measure[Nmeasure].dD, output[0].measure[i].catID);

	float dRoff = dvoOffsetR(&output[0].measure[Nmeasure], &output[0].average[n]);

	// rationalize R
	if (dRoff > +180.0*3600.0) {
	  // average on high end of boundary, move star up
	  output[0].measure[Nmeasure].R += 360.0;
	  dRoff -= 360.0*3600.0;
	}
	if (dRoff < -180.0*3600.0) {
	  // average on low end of boundary, move star down
	  output[0].measure[Nmeasure].R -= 360.0;
	  dRoff += 360.0*3600.0;
	}
	if (fabs(dRoff) > 10*RADIUS) {
	  // take declination into account and check again.
	  double cosD = cos(RAD_DEG*output[0].average[n].D);
	  if (fabs(dRoff*cosD) > 10*RADIUS) {
	    fprintf (stderr, "error: %10.6f,%10.6f vs %10.6f,%10.6f (%f,%f vs %f,%f)\n", 
		     output[0].average[n].R, output[0].average[n].D, 
		     output[0].measure[Nmeasure].R, output[0].measure[Nmeasure].D,
		     X1[i], X2[Jmin], Y1[i], Y2[Jmin]);
	    // XXX abort on this? -- this is a bad failure...
	  }
	}
	output[0].average[n].Nmeasure ++;
	Nmeasure ++;
      }
    }

    // if lensing measurements exist, add them too
    if (output[0].lensing && !SKIP_LENSING) {
      for (Nin = 0; Nin < input[0].average[N].Nlensing; Nin++) {
	/* add to end of lensing list */
	add_lensing_link (&output[0].average[n], next_lensing, Nlensing, NLENSING);
	
	// set the new lensing
	off_t lensoff = input[0].average[N].lensingOffset + Nin;
	output[0].lensing[Nlensing] = input[0].lensing[lensoff];

	output[0].lensing[Nlensing].averef   = n;
	output[0].lensing[Nlensing].objID    = output[0].average[n].objID;
	output[0].lensing[Nlensing].catID    = output[0].catID;
	output[0].average[n].Nlensing ++;
	Nlensing ++;
      }
    }

    // if lensobj measurements exist, add them too
    if (output[0].lensobj && !SKIP_LENSOBJ) {
      for (Nin = 0; Nin < input[0].average[N].Nlensobj; Nin++) {
	/* add to end of lensobj list */
	add_lensobj_link (&output[0].average[n], next_lensobj, Nlensobj, NLENSOBJ);
	
	// set the new lensobj
	off_t lensoff = input[0].average[N].lensobjOffset + Nin;
	output[0].lensobj[Nlensobj] = input[0].lensobj[lensoff];

	// output[0].lensobj[Nlensobj].averef   = n;
	output[0].lensobj[Nlensobj].objID    = output[0].average[n].objID;
	output[0].lensobj[Nlensobj].catID    = output[0].catID;
	output[0].average[n].Nlensobj ++;
	Nlensobj ++;
      }
    }

    // if lensing measurements exist, add them too
    if (output[0].starpar && !SKIP_STARPAR) {
      for (Nin = 0; Nin < input[0].average[N].Nstarpar; Nin++) {
	/* add to end of lensing list */
	add_starpar_link (&output[0].average[n], next_starpar, Nstarpar, NSTARPAR);
	
	// set the new starpar
	off_t staroff = input[0].average[N].starparOffset + Nin;
	output[0].starpar[Nstarpar] = input[0].starpar[staroff];

	output[0].starpar[Nstarpar].averef   = n;
	output[0].starpar[Nstarpar].objID    = output[0].average[n].objID;
	output[0].starpar[Nstarpar].catID    = output[0].catID;
	output[0].average[n].Nstarpar ++;
	Nstarpar ++;
      }
    }

    // if galphot measurements exist, add them too
    if (output[0].galphot && !SKIP_GALPHOT) {
      for (Nin = 0; Nin < input[0].average[N].Ngalphot; Nin++) {
	/* add to end of galphot list */
	add_galphot_link (&output[0].average[n], next_galphot, Ngalphot, NGALPHOT);
	
	// set the new galphot
	off_t galpoff = input[0].average[N].galphotOffset + Nin;
	output[0].galphot[Ngalphot] = input[0].galphot[galpoff];

	output[0].galphot[Ngalphot].averef   = n;
	output[0].galphot[Ngalphot].objID    = output[0].average[n].objID;
	output[0].galphot[Ngalphot].catID    = output[0].catID;
	output[0].average[n].Ngalphot ++;
	Ngalphot ++;
      }
    }

    // XXX: add choice of secfilt
    // update the average properties to reflect the incoming entries:
    // if RETAIN_AVE_PHOTOMETRY is true and the original value is NAN, but the input value is not, accept the input:
    // if RETAIN_AVE_PHOTOMETRY is false and the input value is not NAN, accept the input
    // if RETAIN_AVE_PHOTOMETRY is true and the original value is not NAN, the keep the existing average photometry
    
    if (!secfiltMap) {
      // secfilt tables of input and output are the same
      for (k = 0; k < NsecfiltIn; k++) {
        if (RETAIN_AVE_PHOTOMETRY && isfinite(output[0].secfilt[n*NsecfiltIn+k].MpsfChp)) continue;
        if (!isfinite( input[0].secfilt[N*NsecfiltIn+k].MpsfChp)) continue;
        output[0].secfilt[n*NsecfiltIn+k] = input[0].secfilt[N*NsecfiltIn+k];
      }
    } else {
      for (k = 0; k < NsecfiltIn; k++) {
	if (secfiltMap[k] < 0) continue;  // skip secfilt entries from input that are not in the output

        // index for this entry in output's secfilt list
        int outputIndex = n * NsecfiltOut + secfiltMap[k];
        
        if (RETAIN_AVE_PHOTOMETRY && isfinite(output[0].secfilt[outputIndex].MpsfChp)) continue;
        if (!isfinite( input[0].secfilt[N*NsecfiltIn+k].MpsfChp)) continue;
        output[0].secfilt[outputIndex] = input[0].secfilt[N*NsecfiltIn+k];
      }
    }

    // we can choose to accept the proper-motion and parallax from the reference tgtcat
    if (ACCEPT_MOTION || ACCEPT_ASTROM) {
      output[0].average[n].dR         = input[0].average[N].dR;
      output[0].average[n].dD         = input[0].average[N].dD;
      output[0].average[n].uR         = input[0].average[N].uR;
      output[0].average[n].uD         = input[0].average[N].uD;
      output[0].average[n].duR        = input[0].average[N].duR;
      output[0].average[n].duD        = input[0].average[N].duD;
      output[0].average[n].P          = input[0].average[N].P;
      output[0].average[n].dP         = input[0].average[N].dP;
      output[0].average[n].Tmean      = input[0].average[N].Tmean;
    }
    if (ACCEPT_ASTROM) {
      output[0].average[n].R          = input[0].average[N].R;
      output[0].average[n].D          = input[0].average[N].D;
    }

    /* Nm is updated, but not written out in -update mode (for existing entries)
       Nm is recalculated in build_meas_links if loaded table is not sorted */
    output[0].found_t[n] = Nmeasure;
    i++;
  }
  // MARKTIME("find matched stars: %f sec for "OFF_T_FMT","OFF_T_FMT" stars ("OFF_T_FMT" meas)\n", dtime, Nstars, Nave, Nmeasure);

  /** incorporate unmatched image stars, if this star is in field of this catalog **/
  /* these new entries are all written out in UPDATE mode */ 
  for (i = 0; (i < Nstars) && !ONLY_MATCHES; i++) {
    off_t N = N1[i];

    /* make sure there is space for next entry */
    if (Nmeasure + input[0].average[N].Nmeasure >= NMEASURE) {
      NMEASURE = Nmeasure + input[0].average[N].Nmeasure + D_ITEM;
      REALLOCATE (next_measure, off_t, NMEASURE);
      REALLOCATE (output[0].measure, Measure, NMEASURE);
    }
    if (Nlensing + input[0].average[N].Nlensing >= NLENSING) {
      NLENSING = Nlensing + input[0].average[N].Nlensing + D_ITEM;
      REALLOCATE (next_lensing, off_t, NLENSING);
      REALLOCATE (output[0].lensing, Lensing, NLENSING);
    }
    if (Nlensobj + input[0].average[N].Nlensobj >= NLENSOBJ) {
      NLENSOBJ = Nlensobj + input[0].average[N].Nlensobj + D_ITEM;
      REALLOCATE (next_lensobj, off_t, NLENSOBJ);
      REALLOCATE (output[0].lensobj, Lensobj, NLENSOBJ);
    }
    if (Nstarpar + input[0].average[N].Nstarpar >= NSTARPAR) {
      NSTARPAR = Nstarpar + input[0].average[N].Nstarpar + D_ITEM;
      REALLOCATE (next_starpar, off_t, NSTARPAR);
      REALLOCATE (output[0].starpar, StarPar, NSTARPAR);
    }
    if (Ngalphot + input[0].average[N].Ngalphot >= NGALPHOT) {
      NGALPHOT = Ngalphot + input[0].average[N].Ngalphot + D_ITEM;
      REALLOCATE (next_galphot, off_t, NGALPHOT);
      REALLOCATE (output[0].galphot, GalPhot, NGALPHOT);
    }
    if (Nave >= NAVE) {
      NAVE = Nave + D_ITEM;
      REALLOCATE (output[0].average, Average, NAVE);
      REALLOCATE (output[0].secfilt, SecFilt, NAVE*NsecfiltOut);
    }

    if (input[0].found_t[N]) continue;

    // if we are using MATCHED_TABLES, we are going to leave the edge cases in their
    // source catalog, even if they have leaked beyond the edge
    if (!MATCHED_TABLES && !IN_REGION (input[0].average[N].R, input[0].average[N].D)) continue;

    input[0].found_t[N] = TRUE;

    // XXX should we accept the input measurements for these fields?
    dvo_average_init (&output[0].average[Nave]);
    output[0].average[Nave].R         	   = input[0].average[N].R;
    output[0].average[Nave].D         	   = input[0].average[N].D;
    output[0].average[Nave].objID     	   = objID; // we create objID values in the context of the output db
    output[0].average[Nave].catID     	   = catID; // we create catID values in the context of the output db

    // we can choose to accept the proper-motion and parallax from the reference tgtcat
    if (ACCEPT_MOTION || ACCEPT_ASTROM) {
      output[0].average[Nave].dR         = input[0].average[N].dR;
      output[0].average[Nave].dD         = input[0].average[N].dD;
      output[0].average[Nave].uR         = input[0].average[N].uR;
      output[0].average[Nave].uD         = input[0].average[N].uD;
      output[0].average[Nave].duR        = input[0].average[N].duR;
      output[0].average[Nave].duD        = input[0].average[N].duD;
      output[0].average[Nave].P          = input[0].average[N].P;
      output[0].average[Nave].dP         = input[0].average[N].dP;
      output[0].average[Nave].Tmean      = input[0].average[N].Tmean;
    }
    if (ACCEPT_ASTROM) {
      output[0].average[Nave].R          = input[0].average[N].R;
      output[0].average[Nave].D          = input[0].average[N].D;
    }

    if (PSPS_ID) {
      output[0].average[Nave].extID = CreatePSPSObjectID(output[0].average[Nave].R, output[0].average[Nave].D);
    } 

    objID ++;

    // init the new secfilt entries
    for (j = 0; j < NsecfiltOut; j++) {
        int outputIndex = (Nave * NsecfiltOut) + j;
	dvo_secfilt_init (&output[0].secfilt[outputIndex], SECFILT_RESET_ALL);
    }

    for (j = 0; j < NsecfiltIn; j++) {
      int outputIndex;
      if (secfiltMap) {
        outputIndex = (Nave * NsecfiltOut) + secfiltMap[j];
      } else {
        outputIndex = (Nave * NsecfiltIn) + j;
      }
    
      if (isfinite(input[0].secfilt[N*NsecfiltIn+j].MpsfChp)) {
	output[0].secfilt[outputIndex] = input[0].secfilt[N*NsecfiltIn+j];
      }
    }

    /** add measurements for this input average object **/
    if (output[0].measure && !SKIP_MEASURE && input[0].average[N].Nmeasure) {
      output[0].average[Nave].measureOffset  = Nmeasure;
      for (Nin = 0; Nin < input[0].average[N].Nmeasure; Nin ++) {
	offset = input[0].average[N].measureOffset + Nin;
	
	// supply the measurments from this detection
	output[0].measure[Nmeasure]           = input[0].measure[offset];
	
	// the following measure elements cannot be set until here:
	output[0].measure[Nmeasure].dbFlags  = 0;
	output[0].measure[Nmeasure].averef   = Nave;
	output[0].measure[Nmeasure].objID    = output[0].average[Nave].objID;
	output[0].measure[Nmeasure].catID    = output[0].catID;
	
	// as we add measurements, update Nmeasure to match
	output[0].average[Nave].Nmeasure ++;

	/* we set next[Nmeasure] to -1 here, and update correctly below */
	next_measure[Nmeasure] = -1;
	Nmeasure ++;
      }
      int Ngroup = input[0].average[N].Nmeasure;
      for (j = 0; j < Ngroup - 1; j++) {
	next_measure[Nmeasure - Ngroup + j] = Nmeasure - Ngroup + j + 1;
      }
    }

    /** add lensing for this input average object **/
    if (output[0].lensing && !SKIP_LENSING && input[0].average[N].Nlensing) {
      output[0].average[Nave].lensingOffset  = Nlensing;
      for (Nin = 0; Nin < input[0].average[N].Nlensing; Nin ++) {
	// supply the lensing values from this detection
	off_t lensoff = input[0].average[N].lensingOffset + Nin;
	output[0].lensing[Nlensing]           = input[0].lensing[lensoff];

	// the following lensing elements cannot be set until here:
	output[0].lensing[Nlensing].averef   = Nave;
	output[0].lensing[Nlensing].objID    = output[0].average[Nave].objID;
	output[0].lensing[Nlensing].catID    = output[0].catID;

	// as we add lensing, update Nlensing to match
	output[0].average[Nave].Nlensing ++;

	/* we set next[Nlensing] to -1 here, and update correctly below */
	next_lensing[Nlensing] = -1;
	Nlensing ++;
      }
      int Ngroup = input[0].average[N].Nlensing;
      for (j = 0; j < Ngroup - 1; j++) {
	next_lensing[Nlensing - Ngroup + j] = Nlensing - Ngroup + j + 1;
      }
    }

    /** add lensobj for this input average object **/
    if (output[0].lensobj && !SKIP_LENSOBJ && input[0].average[N].Nlensobj) {
      output[0].average[Nave].lensobjOffset  = Nlensobj;
      for (Nin = 0; Nin < input[0].average[N].Nlensobj; Nin ++) {
	// supply the lensobj values from this detection
	off_t lensoff = input[0].average[N].lensobjOffset + Nin;
	output[0].lensobj[Nlensobj]           = input[0].lensobj[lensoff];

	// the following lensobj elements cannot be set until here:
	// output[0].lensobj[Nlensobj].averef   = Nave;
	output[0].lensobj[Nlensobj].objID    = output[0].average[Nave].objID;
	output[0].lensobj[Nlensobj].catID    = output[0].catID;

	// as we add lensobj, update Nlensobj to match
	output[0].average[Nave].Nlensobj ++;

	/* we set next[Nlensobj] to -1 here, and update correctly below */
	next_lensobj[Nlensobj] = -1;
	Nlensobj ++;
      }
      int Ngroup = input[0].average[N].Nlensobj;
      for (j = 0; j < Ngroup - 1; j++) {
	next_lensobj[Nlensobj - Ngroup + j] = Nlensobj - Ngroup + j + 1;
      }
    }

    /** add starpar for this input average object **/
    if (output[0].starpar && !SKIP_STARPAR && input[0].average[N].Nstarpar) {
      output[0].average[Nave].starparOffset  = Nstarpar;
      for (Nin = 0; Nin < input[0].average[N].Nstarpar; Nin ++) {
	// supply the starpar values from this detection
	off_t staroff = input[0].average[N].starparOffset + Nin;
	output[0].starpar[Nstarpar]           = input[0].starpar[staroff];

	// the following starpar elements cannot be set until here:
	output[0].starpar[Nstarpar].averef   = Nave;
	output[0].starpar[Nstarpar].objID    = output[0].average[Nave].objID;
	output[0].starpar[Nstarpar].catID    = output[0].catID;

	// as we add starpar, update Nstarpar to match
	output[0].average[Nave].Nstarpar ++;

	/* we set next[Nstarpar] to -1 here, and update correctly below */
	next_starpar[Nstarpar] = -1;
	Nstarpar ++;
      }
      int Ngroup = input[0].average[N].Nstarpar;
      for (j = 0; j < Ngroup - 1; j++) {
	next_starpar[Nstarpar - Ngroup + j] = Nstarpar - Ngroup + j + 1;
      }
    }

    /** add galphot for this input average object **/
    if (output[0].galphot && !SKIP_GALPHOT && input[0].average[N].Ngalphot) {
      output[0].average[Nave].galphotOffset  = Ngalphot;
      for (Nin = 0; Nin < input[0].average[N].Ngalphot; Nin ++) {
	// supply the galphot values from this detection
	off_t galpoff = input[0].average[N].galphotOffset + Nin;
	output[0].galphot[Ngalphot]           = input[0].galphot[galpoff];

	// the following galphot elements cannot be set until here:
	output[0].galphot[Ngalphot].averef   = Nave;
	output[0].galphot[Ngalphot].objID    = output[0].average[Nave].objID;
	output[0].galphot[Ngalphot].catID    = output[0].catID;

	// as we add galphot, update Ngalphot to match
	output[0].average[Nave].Ngalphot ++;

	/* we set next[Ngalphot] to -1 here, and update correctly below */
	next_galphot[Ngalphot] = -1;
	Ngalphot ++;
      }
      int Ngroup = input[0].average[N].Ngalphot;
      for (j = 0; j < Ngroup - 1; j++) {
	next_galphot[Ngalphot - Ngroup + j] = Ngalphot - Ngroup + j + 1;
      }
    }

    Nave ++;
  }
      
  // MARKTIME("save unmatched stars: %f sec\n", dtime);

  REALLOCATE (output[0].average, Average, Nave);
  if (!SKIP_MEASURE) { REALLOCATE (output[0].measure, Measure, Nmeasure); }
  if (!SKIP_LENSING) { REALLOCATE (output[0].lensing, Lensing, Nlensing); }
  if (!SKIP_LENSOBJ) { REALLOCATE (output[0].lensobj, Lensobj, Nlensobj); }
  if (!SKIP_STARPAR) { REALLOCATE (output[0].starpar, StarPar, Nstarpar); }
  if (!SKIP_GALPHOT) { REALLOCATE (output[0].galphot, GalPhot, Ngalphot); }
 
# define NOSORT 0
  if (NOSORT) {
    output[0].sorted = FALSE;
  } else {
    output[0].sorted = TRUE;
    if (!SKIP_MEASURE) { output[0].measure = sort_measure (output[0].average, Nave, output[0].measure, Nmeasure, next_measure); }
    if (!SKIP_LENSING) { output[0].lensing = sort_lensing (output[0].average, Nave, output[0].lensing, Nlensing, next_lensing); }
    if (!SKIP_LENSOBJ) { output[0].lensobj = sort_lensobj (output[0].average, Nave, output[0].lensobj, Nlensobj, next_lensobj); }
    if (!SKIP_STARPAR) { output[0].starpar = sort_starpar (output[0].average, Nave, output[0].starpar, Nstarpar, next_starpar); }
    if (!SKIP_GALPHOT) { output[0].galphot = sort_galphot (output[0].average, Nave, output[0].galphot, Ngalphot, next_galphot); }
  }

  /* check if the catalog has changed?  if no change, no need to write */
  output[0].objID    = objID; // new max value, save on catalog close
  output[0].Naverage = Nave;
  if (!SKIP_MEASURE) { output[0].Nmeasure = Nmeasure; }
  if (!SKIP_LENSING) { output[0].Nlensing = Nlensing; }
  if (!SKIP_LENSOBJ) { output[0].Nlensobj = Nlensobj; }
  if (!SKIP_STARPAR) { output[0].Nstarpar = Nstarpar; }
  if (!SKIP_GALPHOT) { output[0].Ngalphot = Ngalphot; }
  output[0].Nsecfilt_mem = Nave*NsecfiltOut;
  if (VERBOSE) fprintf (stderr, "Nstars, Nave, Nmeasure, Nlensing, Ngalphot: "OFF_T_FMT" "OFF_T_FMT" "OFF_T_FMT" "OFF_T_FMT" "OFF_T_FMT", ("OFF_T_FMT" matches)\n",  Nstars,  Nave,  Nmeasure,  Nlensing, Ngalphot, Nmatch);

  free (next_measure);
  free (next_lensing);
  free (next_lensobj);
  free (next_starpar);
  free (next_galphot);

  free (X2);
  free (Y2);
  free (N2);
  free (X1);
  free (Y1);
  free (N1);

  // MARKTIME("cleanup: %f sec\n", dtime);
  return (Nmatch);
}

/* 
   notes:
   
   for finding if a catalog star is in an image or an image star is in the catalog:
   
   catalogs have boundaries defined by RA and DEC, but they may curve in projection
   images have boundaries which are lines in pixels coords, but curve in RA and DEC
   
   output[0].found_t[Ncat] but stars[Nstars].found
   
*/

