# include "addstar.h"
# include "gaia_edr3.h"

/* this function reads the values of interest from the Gaia CSV files:

  data model description:
  http://gea.esac.esa.int/archive/documentation/GEDR3/Gaia_archive/chap_datamodel/sec_dm_main_tables/ssec_dm_gaia_source.html
# proper motion
  3 source_id (map to extID)
  5 ref_epoch
  6 ra
  7 ra_error
  8 dec
  9 dec_error
 10 parallax
 11 parallax_error
 12 parallax_over_error
 13 pm
 14 pmra
 15 pmra_error
 16 pmdec
 17 pmdec_error
# astrometric
 28 astrometric_n_obs_al
 30 astrometric_n_good_obs_al
 32 astrometric_gof_al
 33 astrometric_chi2_al
 34 astrometric_excess_noise
 35 astrometric_excess_noise_sig
 47 visibility_periods_used
 65 duplicated_source
# phot info
 66 phot_g_n_obs
 67 phot_g_mean_flux
 68 phot_g_mean_flux_error
 69 phot_g_mean_flux_over_error
 70 phot_g_mean_mag
 71 phot_bp_n_obs
 72 phot_bp_mean_flux
 73 phot_bp_mean_flux_error
 74 phot_bp_mean_flux_over_error
 75 phot_bp_mean_mag
 76 phot_rp_n_obs
 77 phot_rp_mean_flux
 78 phot_rp_mean_flux_error
 79 phot_rp_mean_flux_over_error
 80 phot_rp_mean_mag

 85 phot_proc_mode : use for a flag

//     - sqrt(astrometric_chi2_al / (astrometric_n_good_obs_al - 5)) < 1.2 * max(1.0, exp(-0.2 * (phot_g_mean_mag - 19.5)))

 we have 32 x 2 bits of photFlags potentially available for use. Gaia does have have a single flag
 value, so I will generate bits to represent some data quality cuts proposed in the Gaia docs.

 I'm also going to overload a couple of values as needed:

 measure.psfNpix   : astrometric_n_obs_al (nObs)
 measure.psfNdof   : astrometric_n_good_obs_al (nGood)
 measure.extNsigma : astrometric_gof_al (goFit)
 measure.psfChisq  : astrometric_chi2_al (chiSq)
 measure.psfQF     : astrometric_excess_noise (exNoise)
 measure.psfQFPerf : astrometric_excess_noise_sig (exNoiseSig)
 measure.FWx       : visibility_periods_used (nPeriods)

 photFlags : 
 0x0000.0001 : visibility_periods_used > 6 (nPeriods > 6)
 0x0000.0002 : visibility_periods_used > 8 (nPeriods > 8)
 0x0000.0004 : astrometric_excess_noise < 1.0 (exNoise < 1)
 0x0000.0008 : ChiSq < limit (

 0x0000.0010 : duplicated_source
 0x0000.0020 : phot_proc_mode == 0
 0x0000.0040 : phot_proc_mode == 1
 0x0000.0080 : phot_proc_mode == 2

*/

Gaia_EDR3_Stars *loadgaia_edr3_readstars (char *filename, Gaia_EDR3_Stars *stars, int *nstars, AddstarClientOptions *options) {

  int codeG = GetPhotcodeCodebyName ("GAIA_G_EDR3"); if (!codeG) Shutdown ("missing photocde GAIA_G_EDR3");
  int codeB = GetPhotcodeCodebyName ("GAIA_B_EDR3"); if (!codeB) Shutdown ("missing photocde GAIA_B_EDR3");
  int codeR = GetPhotcodeCodebyName ("GAIA_R_EDR3"); if (!codeR) Shutdown ("missing photocde GAIA_R_EDR3");

  // GAIA EDR3 Epoch is 2016.0 == 2016/01/01,00:00:00
  time_t GAIA_EDR3_EPOCH = ohana_date_to_sec ("2016/01/01,00:00:00");

  // read in the full FITS files
  FILE *f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read gaia_edr3 file: %s", filename);

  int Nskip = 1; // Gaia CSV files have a single header row (and now special character to mark)

  int Nelem = 0;      // number of valid rows read (vector elements)
  int NELEM = 1000000;  // currently-allocated number of output rows

  // for initial testing, a minimal subset:
  ALLOCATE_PTR (srcID, uint64_t, NELEM);

  ALLOCATE_PTR (Rg, double, NELEM);
  ALLOCATE_PTR (dRg, double, NELEM);
  ALLOCATE_PTR (Dg, double, NELEM);
  ALLOCATE_PTR (dDg, double, NELEM);

  ALLOCATE_PTR (plx, double, NELEM);
  ALLOCATE_PTR (dplx, double, NELEM);

  ALLOCATE_PTR (uR, double, NELEM);
  ALLOCATE_PTR (duR, double, NELEM);

  ALLOCATE_PTR (uD, double, NELEM);
  ALLOCATE_PTR (duD, double, NELEM);

  ALLOCATE_PTR (nObs, int, NELEM);
  ALLOCATE_PTR (nGood, int, NELEM);

  ALLOCATE_PTR (goFit, double, NELEM);
  ALLOCATE_PTR (chiSq, double, NELEM);

  ALLOCATE_PTR (exNoise, double, NELEM);
  ALLOCATE_PTR (exNoiseSig, double, NELEM);

  ALLOCATE_PTR (nPeriods, int, NELEM);
  ALLOCATE_PTR (dupSource, int, NELEM);

  ALLOCATE_PTR (gg, double, NELEM);
  ALLOCATE_PTR (dgg, double, NELEM);
  ALLOCATE_PTR (ngg, int, NELEM);

  ALLOCATE_PTR (bg, double, NELEM);
  ALLOCATE_PTR (dbg, double, NELEM);
  ALLOCATE_PTR (nbg, int, NELEM);

  ALLOCATE_PTR (rg, double, NELEM);
  ALLOCATE_PTR (drg, double, NELEM);
  ALLOCATE_PTR (nrg, int, NELEM);

  ALLOCATE_PTR (procMode, int, NELEM);

  // we allocate one extra byte into which we never read so there will always be a NULL terminating the string
  ALLOCATE_PTR (buffer, char, 0x10001);
  bzero (buffer, 0x10001);

  int Nline_read = 0; // track number of lines read so far (use to skip lines as well)

  // we have a working buffer read from the file. we parse the lines in the working buffer
  // until we reach the last chunk without an EOL char.  at that point, we shift the start
  // of the last (partial) line to the start of the buffer and re-fill.

  // we treat \n\r pair as a single EOL char to handle mac files:

  int Nstart = 0; // location of the last valid byte in the buffer (start filling here)
  int EndOfFile = FALSE;
  while (!EndOfFile) {
    int Nbytes = 0x10000 - Nstart;
    // we have allocated one extra byte into which we never read so there will always be a NULL terminating the string
    bzero (&buffer[Nstart], Nbytes + 1);
    int Nread = fread (&buffer[Nstart], 1, Nbytes, f);
    if (ferror (f)) {
      perror ("error reading data file");
      break;
    }
    // we still need to parse the rest of the buffer, but there might not be an EOL on the last line
    if (Nread == 0) {
      EndOfFile = TRUE;
    }
    
    int bufferStatus = TRUE; 
    char *c0 = buffer; // c0 always marks the start of a line
    char *cA = NULL; // cA will carry the curr point within the line
    while (bufferStatus) {

      // find the end of this current line (\n or \r).  if we hit the end of the buffer (NULL),
      // attempt to read more data.  finish up when we hit the end of the file
      char *c1 = strchr (c0, '\n'); // find the end of this current line (also valid for a Mac: \r\n)
      if (!c1) {
	c1 = strchr (c0, '\r'); // try \r for Windows files
      }
      if (!c1) {
	Nstart = strlen (c0);
	if (EndOfFile) {
	  // if we have reached EOF, we need to do one last pass in case there is a line without a return
	  c1 = c0 + Nstart;
	  bufferStatus = FALSE;
	  if (Nstart == 0) continue; // if we have reached EOF and c0 points at the last valid character, we are done
	} else {
	  // if we have not reached EOF, we need to shift the buffer to the start of this line and read more data
	  memmove (buffer, c0, Nstart);
	  bufferStatus = FALSE;
	  continue;
	}
      }
      *c1 = 0; // mark the end of the line 
      Nline_read ++;

      // skip to the next line (but if EOF, do not overrun buffer)
      if (Nline_read <= Nskip) { if (!EndOfFile) { c0 = c1 + 1; } continue; }

      // these are not needed: Gaia CSV files do not have any commented-out lines
      if (*c0 == '#')          { if (!EndOfFile) { c0 = c1 + 1; } continue; }
      if (*c0 == '!')          { if (!EndOfFile) { c0 = c1 + 1; } continue; }

      // XXX for Gaia, we know which columns we want in advance

      int lineStatus = TRUE;
      int readStatus;
      double dvalue;
      int ivalue;
      uint64_t jvalue;

      // cA will follow the currently extracted field, c0 points to the start of the line
      cA = c0;

      // Tref == 2016.0 for EDR3

      // the start of the line is the 1st element (fields are 1-counting)
      cA = jparse_csv_rpt (&jvalue,  3,  1, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  3, "source_id", c0); }  	       lineStatus &= readStatus; srcID[Nelem]     = jvalue;
      cA = dparse_csv_rpt (&dvalue,  6,  3, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  6, "Ra", c0); }         	       lineStatus &= readStatus; Rg[Nelem]        = dvalue;
      cA = dparse_csv_rpt (&dvalue,  7,  6, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  7, "dRa", c0); }        	       lineStatus &= readStatus; dRg[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue,  8,  7, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  8, "Dec", c0); }        	       lineStatus &= readStatus; Dg[Nelem]        = dvalue;
      cA = dparse_csv_rpt (&dvalue,  9,  8, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  9, "dDec", c0); }       	       lineStatus &= readStatus; dDg[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 10,  9, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 10, "plx", c0); }        	       lineStatus &= readStatus; plx[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 11, 10, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 11, "dplx", c0); }       	       lineStatus &= readStatus; dplx[Nelem]      = dvalue;
      cA = dparse_csv_rpt (&dvalue, 14, 11, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 14, "uR", c0); }         	       lineStatus &= readStatus; uR[Nelem]        = dvalue;
      cA = dparse_csv_rpt (&dvalue, 15, 14, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 15, "duR", c0); }        	       lineStatus &= readStatus; duR[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 16, 15, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 16, "uD", c0); }         	       lineStatus &= readStatus; uD[Nelem]        = dvalue;
      cA = dparse_csv_rpt (&dvalue, 17, 16, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 17, "duD", c0); }        	       lineStatus &= readStatus; duD[Nelem]       = dvalue;
      cA = iparse_csv_rpt (&ivalue, 28, 17, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 28, "n_obs", c0); }      	       lineStatus &= readStatus; nObs[Nelem]      = ivalue;
      cA = iparse_csv_rpt (&ivalue, 30, 28, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 30, "n_good", c0); }     	       lineStatus &= readStatus; nGood[Nelem]     = ivalue;
      cA = dparse_csv_rpt (&dvalue, 32, 30, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 32, "GOF", c0); }        	       lineStatus &= readStatus; goFit[Nelem]     = dvalue;
      cA = dparse_csv_rpt (&dvalue, 33, 32, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 33, "chiSq", c0); }      	       lineStatus &= readStatus; chiSq[Nelem]     = dvalue;
      cA = dparse_csv_rpt (&dvalue, 34, 33, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 34, "exNoise", c0); }    	       lineStatus &= readStatus; exNoise[Nelem]   = dvalue;
      cA = dparse_csv_rpt (&dvalue, 35, 34, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 35, "exNoiseSig", c0); } 	       lineStatus &= readStatus; exNoiseSig[Nelem]= dvalue;
      cA = iparse_csv_rpt (&ivalue, 47, 35, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 47, "visibility_periods", c0); } lineStatus &= readStatus; nPeriods[Nelem]  = ivalue;
					
      // the duplicated_source field has the value false (or true)
      for (int i = 47; i < 65; i++) cA = parse_nextword_csv (cA);
      dupSource[Nelem] = strncmp (cA, "false", 5) ? 1 : 0; 
//    cA = dparse_csv_rpt (&dvalue, 65, 47, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 65, "phot_r", c0); } lineStatus &= readStatus;  dupSource[Nelem] = dvalue;

      cA = iparse_csv_rpt (&ivalue, 66, 65, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 66, "nphot_g", c0); }            lineStatus &= readStatus;  ngg[Nelem]       = ivalue;
      cA = dparse_csv_rpt (&dvalue, 69, 66, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 69, "dphot_g", c0); }            lineStatus &= readStatus;  dgg[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 70, 69, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 70, "phot_g", c0); }             lineStatus &= readStatus;  gg[Nelem]        = dvalue;
      cA = iparse_csv_rpt (&ivalue, 71, 70, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 71, "nphot_b", c0); }            lineStatus &= readStatus;  nbg[Nelem]       = ivalue;
      cA = dparse_csv_rpt (&dvalue, 74, 71, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 74, "dphot_b", c0); }            lineStatus &= readStatus;  dbg[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 75, 74, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 75, "phot_b", c0); }             lineStatus &= readStatus;  bg[Nelem]        = dvalue;
      cA = iparse_csv_rpt (&ivalue, 76, 75, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 76, "nphot_r", c0); }            lineStatus &= readStatus;  nrg[Nelem]       = ivalue;
      cA = dparse_csv_rpt (&dvalue, 79, 76, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 79, "dphot_r", c0); }            lineStatus &= readStatus;  drg[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 80, 79, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 80, "phot_r", c0); }             lineStatus &= readStatus;  rg[Nelem]        = dvalue;
      cA = iparse_csv_rpt (&ivalue, 85, 80, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 85, "phot_mode", c0); }          lineStatus &= readStatus;  procMode[Nelem]  = ivalue;

      if (!lineStatus && VERBOSE) {
	// why do I need to copy temp here, does gprint modify the value of temp?
	char temp[32];
	strncpy_nowarn (temp, c0, 31);
	gprint (GP_ERR, "skip line %s\n\n", temp);
      }

      Nelem ++;
      if (Nelem == NELEM) {
	NELEM += 1000;
        REALLOCATE (srcID, uint64_t, NELEM);
        REALLOCATE (Rg, double, NELEM);
        REALLOCATE (dRg, double, NELEM);
        REALLOCATE (Dg, double, NELEM);
        REALLOCATE (dDg, double, NELEM);
        REALLOCATE (plx, double, NELEM);
        REALLOCATE (dplx, double, NELEM);
        REALLOCATE (uR, double, NELEM);
        REALLOCATE (duR, double, NELEM);
        REALLOCATE (uD, double, NELEM);
        REALLOCATE (duD, double, NELEM);
        REALLOCATE (nObs, int, NELEM);
        REALLOCATE (nGood, int, NELEM);
        REALLOCATE (goFit, double, NELEM);
        REALLOCATE (chiSq, double, NELEM);
        REALLOCATE (exNoise, double, NELEM);
        REALLOCATE (exNoiseSig, double, NELEM);
        REALLOCATE (nPeriods, int, NELEM);
        REALLOCATE (dupSource, int, NELEM);
        REALLOCATE (gg, double, NELEM);
        REALLOCATE (dgg, double, NELEM);
        REALLOCATE (ngg, int, NELEM);
        REALLOCATE (bg, double, NELEM);
        REALLOCATE (dbg, double, NELEM);
        REALLOCATE (nbg, int, NELEM);
        REALLOCATE (rg, double, NELEM);
        REALLOCATE (drg, double, NELEM);
        REALLOCATE (nrg, int, NELEM);
        REALLOCATE (procMode, int, NELEM);
      }
      if (!EndOfFile) {
	c0 = c1 + 1;
      }
    }
  }

  // Nelem is now the number of items (objects,stars) read from the Gaia CSV file
  int NstarsIn = Nelem;

  double Rmin = +360.0;
  double Rmax = -360.0;
  double Dmin = +360.0;
  double Dmax = -360.0;

  // start off where we finished on a previous read
  int Nstars = *nstars;
  int NSTARS = Nstars + 0.1*NstarsIn;

  if (!stars) {
    ALLOCATE (stars, Gaia_EDR3_Stars, NSTARS);
  } else {
    REALLOCATE (stars, Gaia_EDR3_Stars, NSTARS);
  }

  for (int i = 0; i < NstarsIn; i++) {

    Rmin = MIN (Rmin, Rg[i]);
    Rmax = MAX (Rmax, Rg[i]);
    Dmin = MIN (Dmin, Dg[i]);
    Dmax = MAX (Dmax, Dg[i]);

    // each Gaia EDR3 source corresponds to 3 measurements: Gg, Gr, Gb:

    dvo_average_init (&stars[Nstars].average);
    for (int j = 0; j < 3; j++) {
      dvo_measure_init (&stars[Nstars].measure[j]);
    }

    stars[Nstars].average.Nmeasure = 3;
    stars[Nstars].average.R = Rg[i];
    stars[Nstars].average.D = Dg[i];
    stars[Nstars].average.dR = 0.001*dRg[i];
    stars[Nstars].average.dD = 0.001*dDg[i];

    if (ACCEPT_MOTION) {
      // GAIA values are reported in milliarcseconds, but DVO wants arcseconds
      stars[Nstars].average.uR    =   uR[i] * 0.001;
      stars[Nstars].average.uD    =   uD[i] * 0.001;
      stars[Nstars].average.duR   =  duR[i] * 0.001;
      stars[Nstars].average.duD   =  duD[i] * 0.001;
      stars[Nstars].average.P     =  plx[i] * 0.001;
      stars[Nstars].average.dP    = dplx[i] * 0.001;
      stars[Nstars].average.Tmean = GAIA_EDR3_EPOCH;
    }

    stars[Nstars].flag  = FALSE;
    stars[Nstars].found = FALSE;

    // for the following cut, see https://arxiv.org/pdf/1804.09378.pdf (Gaia Collaboration et al 2018)
    double ChiSqLimit = (nGood[i] > 5) ? sqrt(chiSq[i] / (nGood[i] - 5)) : NAN;
    double ChiCompare = 1.2 * MAX (1.0, exp(-0.2*(gg[i] - 19.5)));

    int NVis6   = (nPeriods[i] >  6)        ? 0x00000001 : 0x0;
    int NVis8   = (nPeriods[i] >  8)        ? 0x00000002 : 0x0;
    int ExNoise = (exNoise[i] < 1.0)        ? 0x00000004 : 0x0;
    int ChiGood = (ChiSqLimit < ChiCompare) ? 0x00000008 : 0x0;

    int isDup = dupSource[i]                ? 0x00000010 : 0x0;
    int Mode0 = (procMode[i] == 0)          ? 0x00000020 : 0x0;
    int Mode1 = (procMode[i] == 1)          ? 0x00000040 : 0x0;
    int Mode2 = (procMode[i] == 2)          ? 0x00000080 : 0x0;

    int photFlags = NVis6 | NVis8 | ExNoise | ChiGood | isDup | Mode0 | Mode1 | Mode2;

    for (int j = 0; j < 3; j++) {
      stars[Nstars].measure[j].R = Rg[i];
      stars[Nstars].measure[j].D = Dg[i];

      stars[Nstars].measure[j].psfNpix = nPeriods[i]; // number of observation for period calculation
      stars[Nstars].measure[j].extNsigma = goFit[i];
      stars[Nstars].measure[j].psfChisq = chiSq[i];
      stars[Nstars].measure[j].psfQF = exNoise[i];
      stars[Nstars].measure[j].psfQFperf = exNoiseSig[i];
      stars[Nstars].measure[j].Xccd = nObs[i]; // number of observation for astrometry
      stars[Nstars].measure[j].Xfix = nGood[i]; // number of observations for astrometry fitting
      stars[Nstars].measure[j].XoffKH  = ngg[i]; // number of observations for g band
      stars[Nstars].measure[j].XoffDCR = nbg[i]; // number of observation for bp band
      stars[Nstars].measure[j].XoffCAM = nrg[i]; // number of observations for rp band
      stars[Nstars].measure[j].photFlags = photFlags;
      stars[Nstars].measure[j].t = GAIA_EDR3_EPOCH;
    }

    // the flux reported by Gaia is in electrons / second; we just want to use the mags:
    // dgg, dbg, drg store (flux / error), i.e., S/N.  mag error is 2.5/ln(10) ~ 1.0857 / (S/N)
    // ngg, nbg, nrg: number of measurement for the photcode
    stars[Nstars].measure[0].M  = gg[i];
    stars[Nstars].measure[0].dM = 1.08573620476 / dgg[i];
    stars[Nstars].measure[0].psfNdof = ngg[i];

    stars[Nstars].measure[1].M = bg[i];
    stars[Nstars].measure[1].dM = 1.08573620476 / dbg[i];
    stars[Nstars].measure[1].psfNdof = nbg[i];

    stars[Nstars].measure[2].M = rg[i];
    stars[Nstars].measure[2].dM = 1.08573620476 / drg[i];
    stars[Nstars].measure[2].psfNdof = nrg[i];

    stars[Nstars].measure[0].photcode = codeG;
    stars[Nstars].measure[1].photcode = codeB;
    stars[Nstars].measure[2].photcode = codeR;

    // Not sure how to make these work
    // stars[Nstars].average.Npos     = nGood[i];
    // stars[Nstars].average.Nmissing = nObs[i] - nGood[i];

    //printf ("%5d %5d %5d %5d %5d %5d\n", nObs[i], nGood[i], ngg[i], nbg[i], nrg[i], nPeriods[i]);
    Nstars ++;

    CHECK_REALLOCATE (stars, Gaia_EDR3_Stars, NSTARS, Nstars, 10000);
  }

  free (srcID);
  free (Rg);
  free (dRg);
  free (Dg);
  free (dDg);
  free (plx);
  free (dplx);
  free (uR);
  free (duR);
  free (uD);
  free (duD);
  free (nObs);
  free (nGood);
  free (goFit);
  free (chiSq);
  free (exNoise);
  free (exNoiseSig);
  free (nPeriods);
  free (dupSource);
  free (gg);
  free (dgg);
  free (ngg);
  free (bg);
  free (dbg);
  free (nbg);
  free (rg);
  free (drg);
  free (nrg);
 // free (procMode);

  free (buffer);

  *nstars = Nstars;
  return (stars);
}

int loadgaia_edr3_sortStars (Gaia_EDR3_Stars *stars, int Nstars) {

# define SWAPFUNC(A,B){ Gaia_EDR3_Stars temp = stars[A]; stars[A] = stars[B]; stars[B] = temp; }
# define COMPARE(A,B)(stars[A].average.R < stars[B].average.R)

  OHANA_SORT (Nstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
  return TRUE;
}

