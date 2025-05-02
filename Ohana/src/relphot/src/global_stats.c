# include "relphot.h"

void global_stats (Catalog *catalog, int Ncatalog, int nloop) {

  StatType stN, stX, stS, imN, imX, imM, imD, msM, msX, msN, msD, tgM, tgX, tgD;

  liststats_init (&stN);
  liststats_init (&stX);
  liststats_init (&stS);

  liststats_init (&imN);
  liststats_init (&imX);
  liststats_init (&imM);
  liststats_init (&imD);

  liststats_init (&msN);
  liststats_init (&msX);
  liststats_init (&msM);
  liststats_init (&msD);

  liststats_init (&tgX);
  liststats_init (&tgM);
  liststats_init (&tgD);

  fprintf (stderr, "\n");
  fprintf (stderr, "STATS            median     mean    sigma      min      max   Nmeas\n");

  int Ns;
  for (Ns = 0; (nloop % 3 == 2) && (Ns < Nphotcodes); Ns++) {

    int thisCode = photcodes[Ns][0].code;
    int Nsec = GetPhotcodeNsec(thisCode);
    int seccode = photcodes[Ns][0].code;

    stN = statsStarN (catalog, Ncatalog, Nsec, seccode);
    stX = statsStarX (catalog, Ncatalog, Nsec);
    stS = statsStarS (catalog, Ncatalog, Nsec);
  
    fprintf (stderr, "   --- stats for %s ---\n", photcodes[Ns][0].name);
    fprintf (stderr, "meas / star:    %7.0f  %7.1f  %7.1f  %7.0f  %7.0f  %6d\n", stN.median, stN.mean, stN.sigma, stN.min, stN.max, stN.Nmeas);
    fprintf (stderr, "dMrel star:     %7.4f  %7.4f  %7.4f  %7.4f  %7.4f  %6d\n", stS.median, stS.mean, stS.sigma, stS.min, stS.max, stS.Nmeas);
    fprintf (stderr, "chisq star:     %7.1f  %7.1f  %7.1f  %7.1f  %7.1f  %6d\n", stX.median, stX.mean, stX.sigma, stX.min, stX.max, stX.Nmeas);
  }
  
  imN = statsImageN (catalog);
  imX = statsImageX (catalog);
  imM = statsImageM (catalog);
  imD = statsImagedM (catalog);
  
  msN = statsMosaicN (catalog);
  msM = statsMosaicM (catalog);
  msD = statsMosaicdM (catalog);
  msX = statsMosaicX (catalog);
  
  tgM = statsMosaicM (catalog);
  tgD = statsMosaicdM (catalog);
  tgX = statsMosaicX (catalog);
  
  fprintf (stderr, "meas / image:   %7.0f  %7.0f  %7.0f  %7.0f  %7.0f  %6d\n",   imN.median, imN.mean, imN.sigma, imN.min, imN.max, imN.Nmeas);
  fprintf (stderr, "Mcal  image:    %7.4f  %7.4f  %7.4f  %7.4f  %7.4f  %6d\n",   imM.median, imM.mean, imM.sigma, imM.min, imM.max, imM.Nmeas);
  fprintf (stderr, "dMcal image:    %7.4f  %7.4f  %7.4f  %7.4f  %7.4f  %6d\n",   imD.median, imD.mean, imD.sigma, imD.min, imD.max, imD.Nmeas);
  fprintf (stderr, "chisq image:    %7.1f  %7.1f  %7.1f  %7.1f  %7.1f  %6d\n",   imX.median, imX.mean, imX.sigma, imX.min, imX.max, imX.Nmeas);

  fprintf (stderr, "meas / mosaic:  %7.0f  %7.0f  %7.0f  %7.0f  %7.0f  %6d\n",   msN.median, msN.mean, msN.sigma, msN.min, msN.max, msN.Nmeas);
  fprintf (stderr, "Mcal  mosaic:   %7.4f  %7.4f  %7.4f  %7.4f  %7.4f  %6d\n",   msM.median, msM.mean, msM.sigma, msM.min, msM.max, msM.Nmeas);
  fprintf (stderr, "dMcal mosaic:   %7.4f  %7.4f  %7.4f  %7.4f  %7.4f  %6d\n",   msD.median, msD.mean, msD.sigma, msD.min, msD.max, msD.Nmeas);
  fprintf (stderr, "chisq mosaic:   %7.1f  %7.1f  %7.1f  %7.1f  %7.1f  %6d\n",   msX.median, msX.mean, msX.sigma, msX.min, msX.max, msX.Nmeas);

  fprintf (stderr, "Mcal  tgroup:   %7.4f  %7.4f  %7.4f  %7.4f  %7.4f  %6d\n",   tgM.median, tgM.mean, tgM.sigma, tgM.min, tgM.max, tgM.Nmeas);
  fprintf (stderr, "dMcal tgroup:   %7.4f  %7.4f  %7.4f  %7.4f  %7.4f  %6d\n",   tgD.median, tgD.mean, tgD.sigma, tgD.min, tgD.max, tgD.Nmeas);
  fprintf (stderr, "chisq tgroup:   %7.1f  %7.1f  %7.1f  %7.1f  %7.1f  %6d\n",   tgX.median, tgX.mean, tgX.sigma, tgX.min, tgX.max, tgX.Nmeas);
}


