# include "fakeastro.h"

// assume we are working with an empty database?  allow for matches?
int insert_fakestar (SkyRegion *region, FakeAstro_Stars *stars, int Nstars, Catalog *catalog) {
  
  off_t i, j;
  off_t Nave, NAVE, Nstarpar, NSTARPAR, Nmatch;

  /* internal counters */
  Nmatch = 0;
  NSTARPAR = Nstarpar = catalog[0].Nstarpar;

  // current max obj ID for this catalog
  unsigned int objID = catalog[0].objID;
  unsigned int catID = catalog[0].catID;

  int Nsecfilt = catalog[0].Nsecfilt;
  NAVE = Nave = catalog[0].Naverage;
  NSTARPAR = Nstarpar = catalog[0].Nstarpar;
  
  /** incorporate unmatched image stars? **/
  for (i = 0; i < Nstars; i++) {

    // skip already matched stars
    if (stars[i].found) continue;
    if (!IN_REGION (stars[i].R, stars[i].D)) continue;

    /* make sure there is space for next entry */
    if (Nstarpar >= NSTARPAR) {
      NSTARPAR = Nstarpar + 1000;
      REALLOCATE (catalog[0].starpar, StarPar, NSTARPAR);
    }
    if (Nave >= NAVE) {
      NAVE = Nave + 1000;
      REALLOCATE (catalog[0].average, Average, NAVE);
      REALLOCATE (catalog[0].secfilt, SecFilt, NAVE*Nsecfilt);
    }

    dvo_average_init (&catalog[0].average[Nave]);
    catalog[0].average[Nave].R         	   = stars[i].R;
    catalog[0].average[Nave].D         	   = stars[i].D;

    catalog[0].average[Nave].Nstarpar  	   = 1;
    catalog[0].average[Nave].starparOffset = Nstarpar;
    catalog[0].average[Nave].objID     	   = objID;
    catalog[0].average[Nave].catID     	   = catID;

    objID ++;

    for (j = 0; j < Nsecfilt; j++) {
      dvo_secfilt_init (&catalog[0].secfilt[Nave*Nsecfilt+j], SECFILT_RESET_ALL);
    }

    double m_r = NAN;
    double gr = NAN;
    double ri = NAN;
    double rz = NAN;
    double zy = NAN;

    // valid stars do not have crazy FeH values
    if (fabs(stars[i].starpar.FeH) < 50.0) {
      // Mr vs r-i, Mr vs r-z: linear fit from Bochanski et al 2010 Fig 7 & Fig 9 
      // http://iopscience.iop.org/1538-3881/139/6/2679/pdf/aj_139_6_2679.pdf
      // gr, zy relations from Magnier et al 2012 Fig 3
      double M_r = stars[i].starpar.M_r;
      m_r = M_r + stars[i].starpar.DistMag;
      ri = 0.215*M_r - 1.05;
      rz = 0.350*M_r - 1.82;
      gr = 2.27*ri + 0.09;
      zy = 0.37*(rz - ri) + 0.03; // zy vs iz
    }
    // ICRF QSOs have FeH = -100
    if (stars[i].starpar.FeH < -50.0) {
      m_r = stars[i].starpar.M_r;
      ri = 0.0;
      rz = 0.0;
      gr = 0.0;
      zy = 0.0;
    }
    // ICRF QSOs have FeH = -100
    if (stars[i].starpar.FeH > +50.0) {
      m_r = stars[i].starpar.M_r;
      ri = 0.0;
      rz = 0.0;
      gr = 0.0;
      zy = 0.0;
    }

    double m_g = m_r + gr;
    double m_i = m_r - ri;
    double m_z = m_r - rz;
    double m_y = m_z - zy;

    // I need a photcode for r-band
    catalog[0].secfilt[Nave*Nsecfilt+0].MpsfChp = m_g;
    catalog[0].secfilt[Nave*Nsecfilt+1].MpsfChp = m_r;
    catalog[0].secfilt[Nave*Nsecfilt+2].MpsfChp = m_i;
    catalog[0].secfilt[Nave*Nsecfilt+3].MpsfChp = m_z;
    catalog[0].secfilt[Nave*Nsecfilt+4].MpsfChp = m_y;

    catalog[0].starpar[Nstarpar]        = stars[i].starpar;
    catalog[0].starpar[Nstarpar].averef = Nave;
    catalog[0].starpar[Nstarpar].objID  = catalog[0].average[Nave].objID;
    catalog[0].starpar[Nstarpar].catID  = catID;

    stars[i].found = TRUE;
    Nstarpar ++;
    Nave ++;
  }

  REALLOCATE (catalog[0].average, Average, Nave);
  REALLOCATE (catalog[0].starpar, StarPar, Nstarpar);
 
  catalog[0].sorted = FALSE;

  /* check if the catalog has changed?  if no change, no need to write */
  catalog[0].objID     = objID; // new max value, save on catalog close
  catalog[0].Naverage  = Nave;
  catalog[0].Nstarpar  = Nstarpar;
  catalog[0].Nsecfilt_mem = Nave*Nsecfilt;
  if (VERBOSE) fprintf (stderr, "Nstars, Nave, Nstarpar: %d "OFF_T_FMT" "OFF_T_FMT" ("OFF_T_FMT" matches)\n",  Nstars, Nave, Nstarpar, Nmatch);

  return (Nmatch);
}
