# include "fakeastro.h"
# define SCALE 0.001

// sky[Nsecfilt], crude and hard-wired for now
float sky[] = {100.0, 150.0, 200.0, 300.0, 400.0};

// things to figure out:
// * which filter am I (image.photcode)
// * lookup table of sky counts / pixel or seeing disk
// * what about QSOs?

/* used in find_matches, find_matches_refstars */
# define IN_PATCH(REGION,R,D) (					\
    ((D) >= REGION[0].Dmin) && ((D) < REGION[0].Dmax) &&	\
    ((R) >= REGION[0].Rmin) && ((R) < REGION[0].Rmax))

// region corresponds to the catalog
Stars *make_fake_stars_catalog (Stars *stars, int *nstars, SkyRegion *imagePatch, Catalog *catalog, Image *image) {

  int Nstars = *nstars;
  
  // we have starpar.R,D, which represent the true positions at the reference epoch, FAKEASTRO_REF_EPOCH
  time_t timeRef = ohana_date_to_sec(FAKEASTRO_REF_EPOCH);
  
  if (!stars) {
    ALLOCATE (stars, Stars, catalog->Naverage);
  } else {
    REALLOCATE (stars, Stars, Nstars + catalog->Naverage);
  }
  
  int Nsecfilt = GetPhotcodeNsecfilt ();
  
  // use photcode to get zero point
  PhotCode *code = GetPhotcodebyCode (image->photcode);
  int Nsec = GetPhotcodeNsec (code->equiv);
  myAssert (Nsec >= 0, "undefined Nsec?");
  
  float Mtime = 2.5*log10(image->exptime);
  
  // XXX hard-wired plateScale for now?
  double plateScale = 0.257;

  // XXX put in airmass?
  float ZP  = SCALE*code->C - image->McalPSF + Mtime;
  float ZPo = 25.0 - image->McalPSF + Mtime;
  // float ZP = code[0].K*(measure[0].airmass - 1.000) + SCALE*code[0].C - measure[0].Mcal;
  
  // generate a set of measurements for each star entry

  Average *average = catalog->average;
  SecFilt *secfilt = catalog->secfilt;
  StarPar *starpar = catalog->starpar;
  
  double Rmid = 0.5*(imagePatch->Rmin + imagePatch->Rmax);

  int i;
  for (i = 0; i < catalog->Naverage; i++) {

    // true position from src catalog
    double ra = ohana_normalize_angle_to_midpoint (average[i].R, Rmid);

    // int show = !strcmp (image->name, "o6600g0127o.672655.cm.955522.smf[XY01]") && ((average[i].R < 1.0) || (average[i].R > 359.0));
    int show = FALSE;

    if (show) fprintf (stderr, "try: %f (%f) %f : %f %f , %f %f", average[i].R, ra, average[i].D, imagePatch->Rmin, imagePatch->Rmax, imagePatch->Dmin, imagePatch->Dmax);

    if (!IN_PATCH(imagePatch, ra, average[i].D)) {
      if (show) fprintf (stderr, "\n");
      continue;
    }
    if (show) fprintf (stderr, " * \n");

    int nStar = average[i].starparOffset;

    InitStar (&stars[Nstars]);

    // which filter?
    // float Mraw = Minst + measure[0].dt + 25.0;
    // float Minst = Msky - SCALE*code->C - measure[0].dt

    double Minst = secfilt[i*Nsecfilt + Nsec].MpsfChp - ZP;
    double Counts = pow(10.0, -0.4*Minst);
    double SkyCts = sky[Nsec];

    double SN = Counts / sqrt(SkyCts + Counts);

    // XXX skip stars with low S/N
    if ((SN < 5.0) || isnan (SN)) continue;

    // true position from src catalog
    double Rtru = average[i].R;
    double Dtru = average[i].D;
    
    stars[Nstars].starpar = starpar[nStar]; // keep this so we can save it on the generated stars

    // XXX what is the epoch for the src catalog? J2000?
    stars[Nstars].Rref = Rtru; // add in any deviations we want here 
    stars[Nstars].Dref = Dtru;

    // observed position is scattered from true position by:
    // * proper motion
    // * gaussian scatter (~ seeing) 
    double uR = starpar[nStar].uRA; // starpar are in arcsec / year
    double uD = starpar[nStar].uDEC;
    
    // tzero, timeRef are in UNIX seconds, Toffset should be in years
    double Toffset = (image->tzero - timeRef) / 365.25 / 86400.0;
    // Toffset = 0.0; // XXX TEST

    // uR,uD in linear (arcsec / yr)
    double dRpm = uR*Toffset;
    double dDpm = uD*Toffset;

    // uR,uD in linear arcsec
    double dRsee = ohana_gaussdev_rnd (0.0, 1.0 / SN);
    double dDsee = ohana_gaussdev_rnd (0.0, 1.0 / SN);

    // XXX TEST
    // dDsee = dRsee = 0.0;

    double dRoff = (dRpm + dRsee) / 3600.0;
    double dDoff = (dDpm + dDsee) / 3600.0;

    double Robs = Rtru + dRoff / cos(Dtru*RAD_DEG);
    double Dobs = Dtru + dDoff;

    Robs = ohana_normalize_angle (Robs);

    if (isnan(Robs)) continue;
    if (isnan(Dobs)) continue;

    double X, Y;
    RD_to_XY (&X, &Y, Robs, Dobs, &image->coords);

    if ((fabs(Robs) < 0.1) || (fabs(Robs - 360.0) < 0.1)) {
      // fprintf (stderr, "%f %f : %f %f : %f %f\n", Rtru, Dtru, Robs, Dobs, X, Y);
    }

    if (X < 0) continue;
    if (Y < 0) continue;
    if (X > image->NX) continue;
    if (Y > image->NY) continue;
    if (isnan(X)) continue;
    if (isnan(Y)) continue;

    if (!strcmp (image->name, "o6600g0127o.672655.cm.955522.smf[XY01]")) {
      // fprintf (stderr, "keep: %f %f\n", average[i].R, average[i].D);
    }

    stars[Nstars].measure.Xccd       = X;
    stars[Nstars].measure.Yccd       = Y;
    stars[Nstars].measure.dXccd      = 1.0 / SN / plateScale;
    stars[Nstars].measure.dYccd      = 1.0 / SN / plateScale;

    // stars[Nstars].measure.posangle   = ToShortDegrees(ps1data[i].posangle);
    // stars[Nstars].measure.pltscale   = ps1data[i].pltscale;

    stars[Nstars].measure.M      = Minst + ZPo; // XX I need to compensate for the internal zero point of 25.0
    stars[Nstars].measure.dM     = 1.0 / SN;

    // stars[Nstars].measure.dMcal      = ps1data[i].dMcal;
    stars[Nstars].measure.Sky        = sky[Nsec];
    stars[Nstars].measure.dSky       = sqrt(sky[Nsec]);
                        
    stars[Nstars].measure.photFlags  = 0;
    stars[Nstars].measure.photFlags2 = 0;

    stars[Nstars].average.R = Robs;
    stars[Nstars].average.D = Dobs;

    stars[Nstars].measure.photcode = image->photcode;

    stars[Nstars].measure.airmass = airmass (image->secz, stars[Nstars].average.R, stars[Nstars].average.D, image->sidtime, image->latitude);
    if (!isfinite(stars[Nstars].measure.airmass)) continue;

    stars[Nstars].measure.az      = azimuth (15.0*image->sidtime - stars[Nstars].average.R, stars[Nstars].average.D, image->latitude);
    stars[Nstars].measure.McalPSF = image->McalPSF;
    stars[Nstars].measure.McalAPER= image->McalAPER;
    stars[Nstars].measure.t       = image->tzero + 1e-4*stars[Nstars].measure.Yccd*image->trate;  // trate is in 0.1 msec / row 
    stars[Nstars].measure.dt      = Mtime;

    stars[Nstars].measure.imageID = image->imageID;

    // This is may optionally be replaced by the internal sequence (see FilterStars.c)
    stars[Nstars].measure.detID      = Nstars + 1;
    Nstars ++;
  }

  *nstars = Nstars;
  return stars;
}
