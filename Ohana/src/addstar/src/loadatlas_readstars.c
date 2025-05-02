# include "addstar.h"
# include "atlas.h"

/* this function reads the values of interest from the Gaia CSV files:

  data model description:
  http://gea.esac.esa.int/archive/documentation/GEDR3/Gaia_archive/chap_datamodel/sec_dm_main_tables/ssec_dm_gaia_source.html
  Name   |   Data Type   |   Unit   |   Description
-------------------------------------------------
  01 objid      bigint          none       Object ID
  02 RA 	   float  	   degrees    Right ascension from Gaia DR2, J2000, epoch 2015.5
  03 Dec 	   float  	   degrees    Declination from Gaia DR2, J2000, epoch 2015.5
  04 plx 	   real   	   mas	      Parallax from Gaia DR2
  05 dplx 	   real   	   mas	      Parallax uncertainty from Gaia DR2
  06 pmra 	   real   	   mas/yr     Proper motion in right ascension from Gaia DR2
  07 dpmra      real   	   mas/yr     Proper motion uncertainty in right ascension
  08 pmdec      real   	   mas/yr     Proper motion in declination from Gaia DR2
  09 dpmdec     real   	   mas/yr     Proper motion uncertainty in declination
  10 Gaia 	   real   	   mag	      Gaia G magnitude
  11 dGaia      real   	   mag	      Gaia G magnitude uncertainty
  12 BP 	   real   	   mag	      Gaia G_bp magnitude
  13 dBP 	   real   	   mag	      Gaia G_bp magnitude uncertainty
  14 RP 	   real   	   mag	      Gaia G_rp magnitude
  15 dRP 	   real   	   mag	      Gaia G_rp magnitude uncertainty
  16 Teff 	   int    	   K	      Gaia stellar effective temperature
  17 AGaia      real   	   mag	      Gaia estimate of G-band extinction for this star
  18 dupvar     int    	   none	      Gaia variability and duplicate flags, 0/1/2 for "CONSTANT"/"VARIABLE"/"NOT AVAILABLE" + 4*DUPLICATE
  19 Ag 	   real   	   mag	      SFD estimate of total g-band extinction
  20 rp1 	   real   	   arcsec     Radius where cummulative G flux exceeds 0.1 x this star
  21 r1 	   real   	   arcsec     Radius where cummulative G flux exceeds 1.0 x this star
  22 r10 	   real   	   arcsec     Radius where cummulative G flux exceeds 10.0 x this star
  23 g	   real   	   mag	      PanSTARRS g magnitude
  24 dg 	   real   	   mag	      PanSTARRS g magnitude uncertainty
  25 gchi 	   real   	   none	      chi^2 / DOF for contributors
  26 gcontrib   int    	   none	      Bitmap of conributing catalogs to g
  27 r 	   real   	   mag	      PanSTARRS r magnitude
  28 dr 	   real   	   mag	      PanSTARRS r magnitude uncertainty
  29 rchi 	   real   	   none	      chi^2 / DOF for contributors
  30 rcontrib   int    	   none	      Bitmap of conributing catalogs to r
  31 i 	   real   	   mag	      PanSTARRS i magnitude
  32 di 	   real   	   mag	      PanSTARRS i magnitude uncertainty
  33 ichi 	   real   	   none	      chi^2 / DOF for contributors
  34 icontrib   int    	   none	      Bitmap of conributing catalogs to i
  35 z 	   real   	   mag	      PanSTARRS z magnitude
  36 dz 	   real   	   mag	      PanSTARRS z magnitude uncertainty
  37 zchi 	   real   	   none	      chi^2 / DOF for contributors
  38 zcontrib   int    	   none	      Bitmap of conributing catalogs to z
  39 nstat      int    	   none	      Count of griz outliers rejected
  40 J 	   real   	   mag	      2MASS J magnitude
  41 dJ 	   real   	   mag	      2MASS J magnitude uncertainty
  42 H 	   real   	   mag	      2MASS H magnitude
  43 dH 	   real   	   mag	      2MASS H magnitude uncertainty
  44 K 	   real   	   mag	      2MASS K magnitude
  45 dK 	   real   	   mag	      2MASS K magnitude uncertainty

//  

 measure.psfChisq  : gchi/rchi/ichi/zchi
 photFlags : 
  name          hexadecimal  value   description
  Gaia DR2      0x00000000   0       Gaia DR2 contributed to griz magnitude
  GMP           0x00000001   1       GMP contributed to griz magnitude.
  Pan-STARRS    0x00000002   2       Pan-STARRS contributed to griz magnitude.
  SkyMapper     0x00000004   4       SkyMapper contributed to griz magnitude.
  Pathfinder    0x00000008   8       Pathfinder contributed to griz magnitude.
  APASS         0x00000010   16      APASS contributed to griz magnitude.
  APASS DR9     0x00000020   32      APASS DR9 contributed to griz magnitude.
  Tycho-2/BSC   0x00000040   64      Tycho-2/BSC contributed to griz magnitude.

*/

Atlas_Stars *loadatlas_readstars (char *filename, Atlas_Stars *stars, int *nstars, AddstarClientOptions *options) {

  int codeG = GetPhotcodeCodebyName ("g_PS"); if (!codeG) Shutdown ("missing photocde g_PS");
  int codeR = GetPhotcodeCodebyName ("r_PS"); if (!codeR) Shutdown ("missing photocde r_PS");
  int codeI = GetPhotcodeCodebyName ("i_PS"); if (!codeI) Shutdown ("missing photocde i_PS");
  int codeZ = GetPhotcodeCodebyName ("z_PS"); if (!codeZ) Shutdown ("missing photocde z_PS");
  int codeJ = GetPhotcodeCodebyName ("2MASS_J"); if (!codeJ) Shutdown ("missing photocde 2MASS_J");
  int codeH = GetPhotcodeCodebyName ("2MASS_H"); if (!codeH) Shutdown ("missing photocde 2MASS_H");
  int codeK = GetPhotcodeCodebyName ("2MASS_K"); if (!codeK) Shutdown ("missing photocde 2MASS_K");

  // Atlas Epoch is 2016.0 == 2016/01/01,00:00:00 (? this is for GAIA edr3)
  time_t ATLAS_EPOCH = ohana_date_to_sec ("2016/01/01,00:00:00");

  // read in the full FITS files
  FILE *f = fopen (filename, "r");
  if (f == NULL) Shutdown ("can't read atlas file: %s", filename);

  int Nskip = 0; // ATLAS CSV files have no header row (and now special character to mark)

  int Nelem = 0;      // number of valid rows read (vector elements)
  int NELEM = 5000000;  // currently-allocated number of output rows

  // for initial testing, a minimal subset:
  ALLOCATE_PTR (srcID, uint64_t, NELEM);
  ALLOCATE_PTR (Rg, double, NELEM);
  ALLOCATE_PTR (Dg, double, NELEM);
  ALLOCATE_PTR (plx, double, NELEM);
  ALLOCATE_PTR (dplx, double, NELEM);
  ALLOCATE_PTR (uR, double, NELEM);
  ALLOCATE_PTR (duR, double, NELEM);
  ALLOCATE_PTR (uD, double, NELEM);
  ALLOCATE_PTR (duD, double, NELEM);

  ALLOCATE_PTR (gg, double, NELEM);
  ALLOCATE_PTR (rr, double, NELEM);
  ALLOCATE_PTR (ii, double, NELEM);
  ALLOCATE_PTR (zz, double, NELEM);
  ALLOCATE_PTR (jj, double, NELEM);
  ALLOCATE_PTR (hh, double, NELEM);
  ALLOCATE_PTR (kk, double, NELEM);

  ALLOCATE_PTR (dgg, double, NELEM);
  ALLOCATE_PTR (drr, double, NELEM);
  ALLOCATE_PTR (dii, double, NELEM);
  ALLOCATE_PTR (dzz, double, NELEM);
  ALLOCATE_PTR (djj, double, NELEM);
  ALLOCATE_PTR (dhh, double, NELEM);
  ALLOCATE_PTR (dkk, double, NELEM);

  ALLOCATE_PTR (cgg, double, NELEM);
  ALLOCATE_PTR (crr, double, NELEM);
  ALLOCATE_PTR (cii, double, NELEM);
  ALLOCATE_PTR (czz, double, NELEM);

  ALLOCATE_PTR (fgg, int, NELEM);
  ALLOCATE_PTR (frr, int, NELEM);
  ALLOCATE_PTR (fii, int, NELEM);
  ALLOCATE_PTR (fzz, int, NELEM);

 //ALLOCATE_PTR (procMode, int, NELEM);
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

      // XXX we know which columns we want in advance

      int lineStatus = TRUE;
      int readStatus;
      double dvalue;
      int ivalue;
      uint64_t jvalue;

      // cA will follow the currently extracted field, c0 points to the start of the line
      cA = c0;

      // Tref == 2016.0 for EDR3

      // the start of the line is the 1st element (fields are 1-counting)
      cA = jparse_csv_rpt (&jvalue,  1,  1, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  1, "source_id", c0); }  	       lineStatus &= readStatus; srcID[Nelem]     = jvalue;
      cA = dparse_csv_rpt (&dvalue,  2,  1, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  2, "Ra", c0); }         	       lineStatus &= readStatus; Rg[Nelem]        = dvalue;
      cA = dparse_csv_rpt (&dvalue,  3,  2, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  3, "Dec", c0); }        	       lineStatus &= readStatus; Dg[Nelem]        = dvalue;
      cA = dparse_csv_rpt (&dvalue,  4,  3, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  4, "plx", c0); }        	       lineStatus &= readStatus; plx[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue,  5,  4, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  5, "dplx", c0); }       	       lineStatus &= readStatus; dplx[Nelem]      = dvalue;
      cA = dparse_csv_rpt (&dvalue,  6,  5, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  6, "uR", c0); }         	       lineStatus &= readStatus; uR[Nelem]        = dvalue;
      cA = dparse_csv_rpt (&dvalue,  7,  6, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  7, "duR", c0); }        	       lineStatus &= readStatus; duR[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue,  8,  7, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  8, "uD", c0); }         	       lineStatus &= readStatus; uD[Nelem]        = dvalue;
      cA = dparse_csv_rpt (&dvalue,  9,  8, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n",  9, "duD", c0); }        	       lineStatus &= readStatus; duD[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 23,  9, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 23, "phot_g", c0); }             lineStatus &= readStatus;  gg[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 24, 23, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 24, "dphot_g", c0); }            lineStatus &= readStatus;  dgg[Nelem]      = dvalue;
      cA = dparse_csv_rpt (&dvalue, 25, 24, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 25, "cphot_g", c0); }            lineStatus &= readStatus;  cgg[Nelem]      = dvalue;
      cA = iparse_csv_rpt (&ivalue, 26, 25, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 26, "fphot_g", c0); }            lineStatus &= readStatus;  fgg[Nelem]      = ivalue;
      cA = dparse_csv_rpt (&dvalue, 27, 26, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 27, "phot_r", c0); }             lineStatus &= readStatus;  rr[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 28, 27, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 28, "dphot_r", c0); }            lineStatus &= readStatus;  drr[Nelem]      = dvalue;
      cA = dparse_csv_rpt (&dvalue, 29, 28, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 29, "cphot_r", c0); }            lineStatus &= readStatus;  crr[Nelem]      = dvalue;
      cA = iparse_csv_rpt (&ivalue, 30, 29, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 30, "fphot_r", c0); }            lineStatus &= readStatus;  frr[Nelem]      = ivalue;
      cA = dparse_csv_rpt (&dvalue, 31, 30, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 31, "phot_i", c0); }             lineStatus &= readStatus;  ii[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 32, 31, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 32, "dphot_i", c0); }            lineStatus &= readStatus;  dii[Nelem]      = dvalue;
      cA = dparse_csv_rpt (&dvalue, 33, 32, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 33, "cphot_i", c0); }            lineStatus &= readStatus;  cii[Nelem]      = dvalue;
      cA = iparse_csv_rpt (&ivalue, 34, 33, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 34, "fphot_i", c0); }            lineStatus &= readStatus;  fii[Nelem]      = ivalue;
      cA = dparse_csv_rpt (&dvalue, 35, 34, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 35, "phot_z", c0); }             lineStatus &= readStatus;  zz[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 36, 35, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 36, "dphot_z", c0); }            lineStatus &= readStatus;  dzz[Nelem]      = dvalue;
      cA = dparse_csv_rpt (&dvalue, 37, 36, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 37, "cphot_z", c0); }            lineStatus &= readStatus;  czz[Nelem]      = dvalue;
      cA = iparse_csv_rpt (&ivalue, 38, 37, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 38, "fphot_z", c0); }            lineStatus &= readStatus;  fzz[Nelem]      = ivalue;
      cA = dparse_csv_rpt (&dvalue, 40, 38, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 40, "phot_j", c0); }             lineStatus &= readStatus;  jj[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 41, 40, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 41, "dphot_j", c0); }            lineStatus &= readStatus;  djj[Nelem]      = dvalue;
      cA = dparse_csv_rpt (&dvalue, 42, 41, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 42, "phot_h", c0); }             lineStatus &= readStatus;  hh[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 43, 42, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 43, "dphot_h", c0); }            lineStatus &= readStatus;  dhh[Nelem]      = dvalue;
      cA = dparse_csv_rpt (&dvalue, 44, 43, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 44, "phot_k", c0); }             lineStatus &= readStatus;  kk[Nelem]       = dvalue;
      cA = dparse_csv_rpt (&dvalue, 45, 44, cA, &readStatus); if (!readStatus && VERBOSE) { gprint (GP_ERR, "suspect field: %d (%s) in %s\n", 45, "dphot_k", c0); }            lineStatus &= readStatus;  dkk[Nelem]      = dvalue;

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
        REALLOCATE (Dg, double, NELEM);
        REALLOCATE (plx, double, NELEM);
        REALLOCATE (dplx, double, NELEM);
        REALLOCATE (uR, double, NELEM);
        REALLOCATE (duR, double, NELEM);
        REALLOCATE (uD, double, NELEM);
        REALLOCATE (duD, double, NELEM);
        REALLOCATE (gg, double, NELEM);
        REALLOCATE (rr, double, NELEM);
        REALLOCATE (ii, double, NELEM);
        REALLOCATE (zz, double, NELEM);
        REALLOCATE (jj, double, NELEM);
        REALLOCATE (hh, double, NELEM);
        REALLOCATE (kk, double, NELEM);
        REALLOCATE (dgg, double, NELEM);
        REALLOCATE (drr, double, NELEM);
        REALLOCATE (dii, double, NELEM);
        REALLOCATE (dzz, double, NELEM);
        REALLOCATE (djj, double, NELEM);
        REALLOCATE (dhh, double, NELEM);
        REALLOCATE (dkk, double, NELEM);
        REALLOCATE (cgg, double, NELEM);
        REALLOCATE (crr, double, NELEM);
        REALLOCATE (cii, double, NELEM);
        REALLOCATE (czz, double, NELEM);
        REALLOCATE (fgg, int, NELEM);
        REALLOCATE (frr, int, NELEM);
        REALLOCATE (fii, int, NELEM);
        REALLOCATE (fzz, int, NELEM);
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
    ALLOCATE (stars, Atlas_Stars, NSTARS);
  } else {
    REALLOCATE (stars, Atlas_Stars, NSTARS);
  }

  for (int i = 0; i < NstarsIn; i++) {

    Rmin = MIN (Rmin, Rg[i]);
    Rmax = MAX (Rmax, Rg[i]);
    Dmin = MIN (Dmin, Dg[i]);
    Dmax = MAX (Dmax, Dg[i]);

    // each atlas source corresponds to 7 measurements: g, r, i, z, j, h, k:
    dvo_average_init (&stars[Nstars].average);
    for (int j = 0; j < 7; j++) {
      dvo_measure_init (&stars[Nstars].measure[j]);
    }

    stars[Nstars].average.R = Rg[i];
    stars[Nstars].average.D = Dg[i];

    if (ACCEPT_MOTION) {
      // atlas values are reported in milliarcseconds, but DVO wants arcseconds
      stars[Nstars].average.uR    =   uR[i] * 0.001;
      stars[Nstars].average.uD    =   uD[i] * 0.001;
      stars[Nstars].average.duR   =  duR[i] * 0.001;
      stars[Nstars].average.duD   =  duD[i] * 0.001;
      stars[Nstars].average.P     =  plx[i] * 0.001;
      stars[Nstars].average.dP    = dplx[i] * 0.001;
      stars[Nstars].average.Tmean = ATLAS_EPOCH;
    }

    stars[Nstars].flag  = FALSE;
    stars[Nstars].found = FALSE;

    for (int j = 0; j < 7; j++) {
      stars[Nstars].measure[j].R = Rg[i];
      stars[Nstars].measure[j].D = Dg[i];
    }

    // measure->M         = NAN;
    // measure->dM        = NAN;
    // measure->Map       = NAN;
    // measure->dMap      = NAN;
    // measure->Mkron     = NAN;
    // measure->dMkron    = NAN;
    // measure->McalPSF   = NAN;
    // measure->McalAPER  = NAN;
    // measure->dMcal     = NAN;
    // measure->dt        = NAN;
    stars[Nstars].measure[0].M         = gg[i];
    stars[Nstars].measure[0].dM        = dgg[i];
    stars[Nstars].measure[0].psfChisq  = cgg[i]; // not sure how to extract this value from avextract 
    stars[Nstars].measure[0].photFlags = fgg[i];
    stars[Nstars].measure[0].Mkron     = cgg[i]; // for chisq that can be loaded from avextract photcode:kron

    stars[Nstars].measure[1].M         = rr[i];
    stars[Nstars].measure[1].dM        = drr[i];
    stars[Nstars].measure[1].psfChisq  = crr[i];
    stars[Nstars].measure[1].photFlags = frr[i];
    stars[Nstars].measure[1].Mkron     = crr[i]; // for chisq that can be loaded from avextract photcode:kron

    stars[Nstars].measure[2].M         = ii[i];
    stars[Nstars].measure[2].dM        = dii[i];
    stars[Nstars].measure[2].psfChisq  = cii[i];
    stars[Nstars].measure[2].photFlags = fii[i];
    stars[Nstars].measure[2].Mkron     = cii[i]; // for chisq that can be loaded from avextract photcode:kron

    stars[Nstars].measure[3].M         = zz[i];
    stars[Nstars].measure[3].dM        = dzz[i];
    stars[Nstars].measure[3].psfChisq  = czz[i];
    stars[Nstars].measure[3].photFlags = fzz[i];
    stars[Nstars].measure[3].Mkron     = czz[i]; // for chisq that can be loaded from avextract photcode:kron

    stars[Nstars].measure[4].M         = jj[i];  // WARNING: there are many duplicate values between J and H
    stars[Nstars].measure[4].dM        = djj[i];
    stars[Nstars].measure[5].M         = hh[i];
    stars[Nstars].measure[5].dM        = dhh[i];
    stars[Nstars].measure[6].M         = kk[i];
    stars[Nstars].measure[6].dM        = dkk[i];

    stars[Nstars].measure[0].photcode = codeG;
    stars[Nstars].measure[1].photcode = codeR;
    stars[Nstars].measure[2].photcode = codeI;
    stars[Nstars].measure[3].photcode = codeZ;
    stars[Nstars].measure[4].photcode = codeJ;
    stars[Nstars].measure[5].photcode = codeH;
    stars[Nstars].measure[6].photcode = codeK;

    stars[Nstars].average.Nmeasure = 7;

    //printf ("%18ld %10.5f %10.5f %7.3f %6.2f %6.2f %3d %7.3f %7.3f %7.3f %7.3f\n", srcID[i], Rg[i], Dg[i], gg[i], dgg[i], cgg[i], fgg[i], rr[i], ii[i], zz[i], jj[i]);
    Nstars ++;

    CHECK_REALLOCATE (stars, Atlas_Stars, NSTARS, Nstars, 10000);
  }

  free (srcID);
  free (Rg);
  free (Dg);
  free (plx);
  free (dplx);
  free (uR);
  free (duR);
  free (uD);
  free (duD);
  free (gg);
  free (rr);
  free (ii);
  free (zz);
  free (jj);
  free (hh);
  free (kk);
  free (dgg);
  free (drr);
  free (dii);
  free (dzz);
  free (djj);
  free (dhh);
  free (dkk);
  free (cgg);
  free (crr);
  free (cii);
  free (czz);
  free (fgg);
  free (frr);
  free (fii);
  free (fzz);
  free (buffer);

  *nstars = Nstars;
  return (stars);
}

int loadatlas_sortStars (Atlas_Stars *stars, int Nstars) {

# define SWAPFUNC(A,B){ Atlas_Stars temp = stars[A]; stars[A] = stars[B]; stars[B] = temp; }
# define COMPARE(A,B)(stars[A].average.R < stars[B].average.R)

  OHANA_SORT (Nstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
  return TRUE;
}

