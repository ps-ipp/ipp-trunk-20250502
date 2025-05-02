# include "relphot.h"

// STATUS is value expected for success
# define CHECK_STATUS(STATUS,MSG,...)					\
  if (!(STATUS)) {							\
    fprintf (stderr, MSG, __VA_ARGS__);					\
    return FALSE;							\
  }

int DumpAllMags(char *filename, Catalog *catalog, int Ncatalog) {

  Header header;
  Header theader;
  Matrix matrix;
  FTable ftable;

  gfits_init_header (&header);
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);

  gfits_create_table_header (&theader, "BINTABLE", "DATA");

  gfits_define_bintable_column (&theader, "D", "RA",       "ra",                         "degree", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "DEC",      "dec",                        "degree", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MCAL",     "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MGRP",     "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MMOS",     "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MFLT",     "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MGRD",     "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MREL",     "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MSYS",     "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "MOFF",     "", "", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "AIRMASS",  "", "unitless", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "D", "MJD",      "", "unitless", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "E", "EXPTIME",  "", "unitless", 1.0, 0.0);
  gfits_define_bintable_column (&theader, "J", "PHOTCODE", "", "unitless", 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "IX",       "", "unitless", 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "IY",       "", "unitless", 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "XCCD",     "", "unitless", 1.0, FT_BZERO_INT32);
  gfits_define_bintable_column (&theader, "J", "YCCD",     "", "unitless", 1.0, FT_BZERO_INT32);

  // generate the output array that carries the data
  gfits_create_table (&theader, &ftable);

  // count the max number of measurements:
  int NptsMax = 0;
  for (int nc = 0; nc < Ncatalog; nc++) {
    for (int na = 0; na < catalog[nc].Naverage; na++) {

      int nm = catalog[nc].averageT[na].measureOffset;
      for (int k = 0; k < catalog[nc].averageT[na].Nmeasure; k++, nm++) {
	NptsMax++;
      }
    }
  }

  // create intermediate storage arrays
  ALLOCATE_PTR (Rs,       double, NptsMax);
  ALLOCATE_PTR (Ds,       double, NptsMax);
  ALLOCATE_PTR (Mcal,     float,  NptsMax);
  ALLOCATE_PTR (Mgrp,     float,  NptsMax);
  ALLOCATE_PTR (Mmos,     float,  NptsMax);
  ALLOCATE_PTR (Mflt,     float,  NptsMax);
  ALLOCATE_PTR (Mgrd,     float,  NptsMax);
  ALLOCATE_PTR (Mrel,     float,  NptsMax);
  ALLOCATE_PTR (Msys,     float,  NptsMax);
  ALLOCATE_PTR (Moff,     float,  NptsMax);
  ALLOCATE_PTR (airmass,  float,  NptsMax);
  ALLOCATE_PTR (mjd,      double, NptsMax);
  ALLOCATE_PTR (exptime,  float,  NptsMax);
  ALLOCATE_PTR (photcode, int,    NptsMax);
  ALLOCATE_PTR (iX,       int,    NptsMax);
  ALLOCATE_PTR (iY,       int,    NptsMax);
  ALLOCATE_PTR (Xccd,     int,    NptsMax);
  ALLOCATE_PTR (Yccd,     int,    NptsMax);

  // XXXX fill in the vectors here
  int Nsecfilt = GetPhotcodeNsecfilt ();

  int Npts = 0;
  for (int nc = 0; nc < Ncatalog; nc++) {
    for (int na = 0; na < catalog[nc].Naverage; na++) {

      int nm = catalog[nc].averageT[na].measureOffset;
      for (int k = 0; k < catalog[nc].averageT[na].Nmeasure; k++, nm++) {

	// skip measurements marked by AREA or TIME
	if (catalog[nc].measureT[nm].dbFlags & MEAS_BAD) continue;

	Mcal[Npts] = getMcal  (nm, nc, MAG_CLASS_PSF);
	if (isnan(Mcal[Npts])) continue;

	Mgrp[Npts] = getMgrp  (nm, nc, catalog[nc].measureT[nm].airmass, NULL);
	if (isnan(Mgrp[Npts])) continue;

	Mmos[Npts]  = getMmos  (nm, nc);
	if (isnan(Mmos[Npts])) continue;

	Mflt[Npts] = getMflat (nm, nc, catalog);
	if (isnan(Mflt[Npts])) continue;

	// Mrel* is the average magnitude for this star.  For PS1 stacks, we have too much
	// PSF variability.  We need to calibrate the PSF magnitudes separately from the
	// Aperture-like magnitues.  (We have an option to use the kron magnitudes or the
	// other apertures here).  I basically need to do this analysis separately for each
	// magnitude type
    
	// XXX works if we stop loop here
	// Npts ++;
	// continue;

	Mrel[Npts] = getMrel  (catalog, nm, nc, MAG_CLASS_PSF, MAG_SRC_CHP);
	if (isnan(Mrel[Npts])) continue;
      
	Msys[Npts] = PhotSysTiny (&catalog[nc].measureT[nm], &catalog[nc].averageT[na], &catalog[nc].secfilt[na*Nsecfilt], MAG_CLASS_PSF);
	if (isnan(Msys[Npts])) continue;

	Mgrd[Npts] = getMgridTiny (&catalog[nc].measureT[nm]); 
	if (isnan(Mgrd[Npts])) continue;

	Moff[Npts] = Mcal[Npts] + Mgrp[Npts] + Mmos[Npts] + Mflt[Npts] + Mgrd[Npts];

	photcode[Npts] = catalog[nc].measureT[nm].photcode;

	if (GRID_ZEROPT) {
	  GridCorrectionType *grid = getGridCorrByCode(photcode[Npts]);
	  if (!grid) continue; // does not match one of our images, skip

	  // edge effects could cause some positions to be slightly out of range
	  // probably should trap extreme outliers
	  iX[Npts] = MIN(MAX(0, (int)(catalog[nc].measureT[nm].Xccd * grid->dX)), grid->Nx - 1);
	  iY[Npts] = MIN(MAX(0, (int)(catalog[nc].measureT[nm].Yccd * grid->dY)), grid->Ny - 1);
	} else {
	  iX[Npts] = 0;
	  iY[Npts] = 0;
	}

	Xccd[Npts] = catalog[nc].measureT[nm].Xccd;
	Yccd[Npts] = catalog[nc].measureT[nm].Yccd;

	Rs[Npts] = catalog[nc].measureT[nm].R;
	Ds[Npts] = catalog[nc].measureT[nm].D;

	airmass[Npts] = catalog[nc].measureT[nm].airmass;

	exptime[Npts] = 0.0; // catalog[nc].measureT[nm].dt;
	mjd[Npts]     = 0.0; // ohana_sec_to_mjd (catalog[nc].measureT[nm].tzero);

	Npts ++;
      }
    }
  }

  // add the columns to the output array
  gfits_set_bintable_column (&theader, &ftable, "RA",       Rs,       Npts);
  gfits_set_bintable_column (&theader, &ftable, "DEC",      Ds,       Npts);
  gfits_set_bintable_column (&theader, &ftable, "MCAL",     Mcal,     Npts);
  gfits_set_bintable_column (&theader, &ftable, "MGRP",     Mgrp,     Npts);
  gfits_set_bintable_column (&theader, &ftable, "MMOS",     Mmos,     Npts);
  gfits_set_bintable_column (&theader, &ftable, "MFLT",     Mflt,     Npts);
  gfits_set_bintable_column (&theader, &ftable, "MGRD",     Mgrd,     Npts);
  gfits_set_bintable_column (&theader, &ftable, "MREL",     Mrel,     Npts);
  gfits_set_bintable_column (&theader, &ftable, "MSYS",     Msys,     Npts);
  gfits_set_bintable_column (&theader, &ftable, "MOFF",     Moff,     Npts);
  gfits_set_bintable_column (&theader, &ftable, "AIRMASS",  airmass,  Npts);
  gfits_set_bintable_column (&theader, &ftable, "MJD",      mjd,      Npts);
  gfits_set_bintable_column (&theader, &ftable, "EXPTIME",  exptime,  Npts);
  gfits_set_bintable_column (&theader, &ftable, "PHOTCODE", photcode, Npts);
  gfits_set_bintable_column (&theader, &ftable, "IX",       iX,       Npts);
  gfits_set_bintable_column (&theader, &ftable, "IY",       iY,       Npts);
  gfits_set_bintable_column (&theader, &ftable, "XCCD",     Xccd,     Npts);
  gfits_set_bintable_column (&theader, &ftable, "YCCD",     Yccd,     Npts);

  free (Rs);
  free (Ds);
  free (Mcal);
  free (Mgrp);
  free (Mmos);
  free (Mflt);
  free (Mgrd);
  free (Mrel);
  free (Msys);
  free (Moff);
  free (airmass);
  free (mjd);
  free (exptime);
  free (photcode);
  free (iX);
  free (iY);
  free (Xccd);
  free (Yccd);

  FILE *f = fopen (filename, "w");
  if (!f) {
    fprintf (stderr, "ERROR: cannot open meanmag file for output %s\n", filename);
    return FALSE;
  }

  int status;
  status = gfits_fwrite_header  (f, &header);
  CHECK_STATUS (status, "ERROR: cannot write header for meanmags %s\n", filename);

  status = gfits_fwrite_matrix  (f, &matrix);
  CHECK_STATUS (status, "ERROR: cannot write matrix for meanmags %s\n", filename);

  status = gfits_fwrite_Theader (f, &theader);
  CHECK_STATUS (status, "ERROR: cannot write table header for meanmags %s\n", filename);

  status = gfits_fwrite_table  (f, &ftable);
  CHECK_STATUS (status, "ERROR: cannot write table data for meanmags %s\n", filename);

  gfits_free_header (&header);
  gfits_free_matrix (&matrix);
  gfits_free_header (&theader);
  gfits_free_table (&ftable);

  int fd = fileno (f);

  status = fflush (f);
  CHECK_STATUS (!status, "ERROR: cannot flush file mags %s\n", filename);

  status = fsync (fd);
  CHECK_STATUS (!status, "ERROR: cannot flush file mags %s\n", filename);

  status = fclose (f);
  CHECK_STATUS (!status, "ERROR: problem closing mags file %s\n", filename);

  return TRUE;
}

