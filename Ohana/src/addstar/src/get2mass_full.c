# include "addstar.h"
# include "2mass.h"

// XXX check to see if desired output format is PS1_V1 or later?  (use 16bit version if not?)

// fill in the data for a JHK triplet star.  takes a pointer to the start of the line the
// RA and DEC have already been set
int get2mass_3star_full (Measure *measure, char *line, int *Nmeasure) {

  char *ptr;
  double dMfull;
  double jd;

  if (line == NULL) Shutdown ("format error in 2mass");

  ptr = line;		      // ra  (set for average, but not yet measure)
  double R = atof(ptr);	      

  ptr = next2MASSfield (ptr); // dec (assumed to be already set)
  double D = atof(ptr);

  measure[2].R = measure[1].R = measure[0].R = R;
  measure[2].D = measure[1].D = measure[0].D = D;

  ptr = next2MASSfield (ptr); // err_maj
  measure[0].FWx = ToShortPixels(strtod (ptr, NULL));
  ptr = next2MASSfield (ptr); // err_min
  measure[0].FWy = ToShortPixels(strtod (ptr, NULL));
  ptr = next2MASSfield (ptr); // err_ang
  measure[0].theta = ToShortPixels(strtod (ptr, NULL));

  measure[2].FWx   = measure[1].FWx   = measure[0].FWx;
  measure[2].FWy   = measure[1].FWy   = measure[0].FWy;
  measure[2].theta = measure[1].theta = measure[0].theta;

  ptr = next2MASSfield (ptr); // designation (skip)

  ptr = next2MASSfield (ptr); // j_m
  measure[0].M  = strtod (ptr, NULL);
  ptr = next2MASSfield (ptr); // j_cmsig
  measure[0].dM = strtod (ptr, NULL);
  ptr = next2MASSfield (ptr); // j_msigcom
  dMfull = strtod (ptr, NULL);
  measure[0].dMcal = sqrt (SQ(dMfull) - SQ(measure[0].dM));
  ptr = next2MASSfield (ptr); // j_snr (skip)

  ptr = next2MASSfield (ptr); // h_m
  measure[1].M  = strtod (ptr, NULL);
  ptr = next2MASSfield (ptr); // h_cmsig
  measure[1].dM = strtod (ptr, NULL);
  ptr = next2MASSfield (ptr); // h_msigcom
  dMfull = strtod (ptr, NULL);
  measure[1].dMcal = sqrt (SQ(dMfull) - SQ(measure[1].dM));
  ptr = next2MASSfield (ptr); // h_snr (skip)

  ptr = next2MASSfield (ptr); // k_m
  measure[2].M  = strtod (ptr, NULL);
  ptr = next2MASSfield (ptr); // k_cmsig
  measure[2].dM = strtod (ptr, NULL);
  ptr = next2MASSfield (ptr); // k_msigcom
  dMfull = strtod (ptr, NULL);
  measure[2].dMcal = sqrt (SQ(dMfull) - SQ(measure[2].dM));
  ptr = next2MASSfield (ptr); // k_snr (skip)

  measure[2].photFlags = measure[1].photFlags = measure[0].photFlags = 0;

  ptr = next2MASSfield (ptr); // ph_qual
  set2MASS_ph_qual (&measure[0], ptr[0]);
  set2MASS_ph_qual (&measure[1], ptr[1]);
  set2MASS_ph_qual (&measure[2], ptr[2]);

  ptr = next2MASSfield (ptr); // rd_flg
  set2MASS_rd_flag (&measure[0], ptr[0]);
  set2MASS_rd_flag (&measure[1], ptr[1]);
  set2MASS_rd_flag (&measure[2], ptr[2]);

  ptr = next2MASSfield (ptr); // bl_flg
  set2MASS_bl_flag (&measure[0], ptr[0]);
  set2MASS_bl_flag (&measure[1], ptr[1]);
  set2MASS_bl_flag (&measure[2], ptr[2]);

  ptr = next2MASSfield (ptr); // cc_flg
  set2MASS_cc_flag (&measure[0], ptr[0]);
  set2MASS_cc_flag (&measure[1], ptr[1]);
  set2MASS_cc_flag (&measure[2], ptr[2]);

  ptr = next2MASSfield (ptr); // ndet (skip for now, XXX use somehow?)
  ptr = next2MASSfield (ptr); // prox (skip)
  ptr = next2MASSfield (ptr); // pxpa (skip)
  ptr = next2MASSfield (ptr); // pxcntr (skip)

  ptr = next2MASSfield (ptr); // gal_contam (one flag for all filters)
  set2MASS_gal_flag (&measure[0], ptr[0]);
  set2MASS_gal_flag (&measure[1], ptr[0]);
  set2MASS_gal_flag (&measure[2], ptr[0]);

  ptr = next2MASSfield (ptr); // mp_flg (one flag for all filters)
  set2MASS_mp_flag (&measure[0], ptr[0]);
  set2MASS_mp_flag (&measure[1], ptr[0]);
  set2MASS_mp_flag (&measure[2], ptr[0]);

  ptr = next2MASSfield (ptr); // pts_key (skip for now, XXX use somehow?)
  ptr = next2MASSfield (ptr); // hemis (skip)
  ptr = next2MASSfield (ptr); // date (skip)
  ptr = next2MASSfield (ptr); // scan (skip)
  ptr = next2MASSfield (ptr); // glon (skip)
  ptr = next2MASSfield (ptr); // glat (skip)

  ptr = next2MASSfield (ptr); // x_scan
  measure[0].Xccd = strtod (ptr, NULL);
  measure[2].Xccd = measure[1].Xccd = measure[0].Xccd;

  ptr = next2MASSfield (ptr); // jdate (julian date)
  jd = strtod (ptr, NULL);
  measure[0].t = ohana_jd_to_sec (jd);
  measure[2].t = measure[1].t = measure[0].t;

  ptr = next2MASSfield (ptr); // j_psfchi
  measure[0].psfChisq = strtod (ptr, NULL);
  ptr = next2MASSfield (ptr); // h_psfchi
  measure[1].psfChisq = strtod (ptr, NULL);
  ptr = next2MASSfield (ptr); // k_psfchi
  measure[2].psfChisq = strtod (ptr, NULL);

  ptr = next2MASSfield (ptr); // j_m_stdap
  measure[0].Map = strtod (ptr, NULL);
  ptr = next2MASSfield (ptr); // j_msig_stdap (skip?)

  ptr = next2MASSfield (ptr); // h_m_stdap
  measure[1].Map = strtod (ptr, NULL);
  ptr = next2MASSfield (ptr); // h_msig_stdap (skip?)

  ptr = next2MASSfield (ptr); // k_m_stdap
  measure[2].Map = strtod (ptr, NULL);
  ptr = next2MASSfield (ptr); // k_msig_stdap (skip?)

  ptr = next2MASSfield (ptr); // dist_edge_ns (skip)
  ptr = next2MASSfield (ptr); // dist_edge_ew (skip)
  ptr = next2MASSfield (ptr); // dist_edge_flg (skip)

  ptr = next2MASSfield (ptr); // dup_src (one flag for all filters)
  set2MASS_dup_flag (&measure[0], ptr[0]);
  set2MASS_dup_flag (&measure[1], ptr[0]);
  set2MASS_dup_flag (&measure[2], ptr[0]);

  ptr = next2MASSfield (ptr); // use_src (one flag for all filters)
  set2MASS_use_flag (&measure[0], ptr[0]);
  set2MASS_use_flag (&measure[1], ptr[0]);
  set2MASS_use_flag (&measure[2], ptr[0]);

  measure[0].photcode  = TM_J;
  measure[0].detID   = 0;
  measure[0].imageID = 0;

  measure[1].photcode  = TM_H;
  measure[1].detID   = 0;
  measure[1].imageID = 0;

  measure[2].photcode  = TM_K;
  measure[2].detID   = 0;
  measure[2].imageID = 0;

  *Nmeasure = 3;

  ptr = next2MASSfield (ptr); // a (optical source association)

  // if we have Tycho data, add as a new measurement
  if (*ptr == 'T') {
    ptr = next2MASSfield (ptr); // dist_opt (optical source association)
    float dR = atof(ptr) / 3600.0; // convert offset to degrees

    ptr = next2MASSfield (ptr); // phi_opt (optical source association)
    float phi = atof(ptr);

    ptr = next2MASSfield (ptr); // B_opt (optical source association)
    float B_j = atof(ptr);

    ptr = next2MASSfield (ptr); // V_opt (optical source association)
    float V_j = atof(ptr);

    *Nmeasure = 5;

    // inherit all info from the base measurement
    measure[3] = measure[0];
    measure[4] = measure[0];

    float dDEC = dR * cos(RAD_DEG*phi);
    float dRA  = dR * sin(RAD_DEG*phi) / cos(RAD_DEG*measure[3].D);

    // transformations from Bj,Vj to Bt,Vt from 2mass explanatory supplement:
    float BV_t = (B_j - V_j) / 0.85;
    float V_t = V_j + 0.09 * BV_t;
    float B_t = BV_t + V_t;

    measure[3].R -= dRA;
    measure[3].D -= dDEC;
    measure[3].photcode = TM_B;
    measure[3].M = B_t;
    measure[3].dM = 0.1;

    measure[4].R = measure[3].R;
    measure[4].D = measure[3].D;
    measure[4].photcode = TM_V;
    measure[4].M = V_t;
    measure[4].dM = 0.1;
  }

  return TRUE;
}

/* return a pointer to the first char after the next field separator (|) */
char *next2MASSfield (char *line) {

  char *p, *q;

  p = line;
  q = strchr (p, '|');
  if (q == NULL) return (NULL);
  p = q + 1;
  if (*p == 0) return (NULL);
  return (p);
}

int set2MASS_ph_qual (Measure *measure, char qual) {

  switch (qual) {
    case 'A': measure[0].photFlags |= 0x00000001; break; // was: 0x0004
    case 'B': measure[0].photFlags |= 0x00000002; break; // was: 0x0005
    case 'C': measure[0].photFlags |= 0x00000004; break; // was: 0x0006
    case 'D': measure[0].photFlags |= 0x00000008; break; // was: 0x0007
    case 'E': measure[0].photFlags |= 0x00000010; break; // was: 0x0003
    case 'F': measure[0].photFlags |= 0x00000020; break; // was: 0x0002
    case 'U': measure[0].photFlags |= 0x00000040; break; // was: 0x0001
    case 'X': measure[0].photFlags |= 0x00000080; break; // was: 0x0000
    default: 
      fprintf (stderr, "error!\n");
      exit (2);
  }      
  return (TRUE);
}

int set2MASS_rd_flag (Measure *measure, char qual) {

  switch (qual) {
    case '0': measure[0].photFlags |= 0x00000100; break; // was: 0x0000 
    case '1': measure[0].photFlags |= 0x00000200; break; // was: 0x0010 
    case '2': measure[0].photFlags |= 0x00000400; break; // was: 0x0020 
    case '3': measure[0].photFlags |= 0x00000800; break; // was: 0x0030 
    case '4': measure[0].photFlags |= 0x00001000; break; // was: 0x0040 
    case '6': measure[0].photFlags |= 0x00002000; break; // was: 0x0050 
    case '9': measure[0].photFlags |= 0x00004000; break; // was: 0x0060 
    default: 
      fprintf (stderr, "error!\n");
      exit (2);
  }      
  return (TRUE);
}

int set2MASS_cc_flag (Measure *measure, char qual) {

  switch (qual) {
    case 'p': measure[0].photFlags |= 0x00010000; break; // was: 0x0000
    case 'c': measure[0].photFlags |= 0x00020000; break; // was: 0x0100
    case 'd': measure[0].photFlags |= 0x00040000; break; // was: 0x0200
    case 's': measure[0].photFlags |= 0x00080000; break; // was: 0x0300
    case 'b': measure[0].photFlags |= 0x00010000; break; // was: 0x0400
    case '0': measure[0].photFlags |= 0x00020000; break; // was: 0x0500
    default: 
      fprintf (stderr, "error!\n");
      exit (2);
  }      
  return (TRUE);
}

int set2MASS_bl_flag (Measure *measure, char qual) {

  switch (qual) {
    case '0': measure[0].photFlags &= ~0x00300000; break; // was: ~0x0008
    case '1': measure[0].photFlags |=  0x00100000; break; // was: ~0x0008
    default:  measure[0].photFlags |=  0x00200000; break; // was:  0x0008
  }      
  return (TRUE);
}

int set2MASS_gal_flag (Measure *measure, char qual) {

  switch (qual) {
    case '0': measure[0].photFlags &= ~0x00c00000; break; // was: ~0x0080 
    case '1': measure[0].photFlags |=  0x00400000; break; // was: ~0x0080 
    default:  measure[0].photFlags |=  0x00800000;	       // was:  0x0080
      measure[0].extNsigma = 100.0;
      break;
  }      
  return (TRUE);
}

int set2MASS_mp_flag (Measure *measure, char qual) {

  switch (qual) {
    case '0': measure[0].photFlags &= ~0x03000000; break; // was: ~0x0800
    case '1': measure[0].photFlags |=  0x01000000; break; // was: ~0x0800
    default:  measure[0].photFlags |=  0x02000000; break; // was:  0x0800
  }      
  return (TRUE);
}

int set2MASS_dup_flag (Measure *measure, char qual) {

  switch (qual) {
    case '0': measure[0].photFlags &= ~0x0c000000; break; // was: ~0x1000
    case '1': measure[0].photFlags |=  0x04000000; break; // was: ~0x1000
    default:  measure[0].photFlags |=  0x08000000; break; // was:  0x1000
  }      
  return (TRUE);
}

int set2MASS_use_flag (Measure *measure, char qual) {

  switch (qual) {
    case '0': measure[0].photFlags &= ~0x10000000; break; // was: ~0x2000
    case '1': measure[0].photFlags |=  0x10000000; break; // was:  0x2000
    default:  abort();
  }      
  return (TRUE);
}

// unused photFlags:
// 0x0000.8000
// 0x0004.0000
// 0x0008.0000
// 0x2000.0000
// 0x4000.0000
// 0x8000.0000
