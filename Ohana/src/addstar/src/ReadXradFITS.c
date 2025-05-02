# include "addstar.h"

// As a temporary hack, I am going to overload the following fields with radial aperture values
// from the convolved images:

# define  F_ApR5_C1 X11_sm_obj
# define dF_ApR5_C1  E1_sm_obj
# define  F_ApR6_C1 X22_sm_obj
# define dF_ApR6_C1  E2_sm_obj
# define  F_ApR7_C1 X11_sh_obj
# define dF_ApR7_C1  E2_sh_obj

# define  F_ApR5_C2 X11_sm_psf
# define dF_ApR5_C2  E1_sm_psf
# define  F_ApR6_C2 X22_sm_psf
# define dF_ApR6_C2  E2_sm_psf
# define  F_ApR7_C2 X11_sh_psf
# define dF_ApR7_C2  E2_sh_psf

// given a file with the pointer at the start of the table block and the 
// corresponding image header, load the xrad data from the table
int ReadXradFITS (FILE *f, Header *theader, Catalog *catalog) {

  FTable table;
  
  // we've already read in the header
  table.header = theader;
  off_t Nskip = theader[0].datasize;
  fseeko (f, Nskip, SEEK_CUR); 

  /* load the table data */
  if (!gfits_fread_ftable_data (f, &table, FALSE)) {
    fprintf (stderr, "WARNING: no xrad data, skipping\n");
    return TRUE;
  }

  if (theader[0].Naxis[1] == 0) {
    fprintf (stderr, "WARNING: no xrad data, skipping\n");
    return TRUE;
  }

  // I want to read the following columns from this table:
  // All of these fields should have the same number of rows
  // the APER_* fields should have the same number of columns (number of radii)
  
  off_t Nrow, NrowAlt;
  int Ncol, NcolAlt;
  char type[16], name[80];

  strcpy (name, "IPP_IDET");
  int *RadID = gfits_get_bintable_column_data (theader, &table, name, type, &Nrow, &Ncol);
  myAssert (!strcmp(type, "int"), "wrong column type for %s\n", name);

  strcpy (name, "PSF_FWHM");
  float *PSFfwhm = gfits_get_bintable_column_data (theader, &table, name, type, &NrowAlt, &Ncol);
  myAssert (!strcmp(type, "float"), "wrong column type for %s\n", name);
  myAssert (Nrow == NrowAlt, "row mismatch?");

  strcpy (name, "APER_FLUX");
  float *AperFlux = gfits_get_bintable_column_data (theader, &table, name, type, &NrowAlt, &Ncol);
  myAssert (!strcmp(type, "float"), "wrong column type for %s\n", name);
  myAssert (Nrow == NrowAlt, "row mismatch?");
  fprintf (stderr, "reading APER_FLUX for %d apertures\n", Ncol);

  strcpy (name, "APER_FLUX_ERR");
  float *AperFluxErr = gfits_get_bintable_column_data (theader, &table, name, type, &NrowAlt, &NcolAlt);
  myAssert (!strcmp(type, "float"), "wrong column type for %s\n", name); 
  myAssert (Nrow == NrowAlt, "row mismatch?");
  myAssert (Ncol == NcolAlt, "column mismatch?");

  strcpy (name, "APER_FLUX_STDEV");
  float *AperFluxStd = gfits_get_bintable_column_data (theader, &table, name, type, &NrowAlt, &NcolAlt);
  myAssert (!strcmp(type, "float"), "wrong column type for %s\n", name);
  myAssert (Nrow == NrowAlt, "row mismatch?");
  myAssert (Ncol == NcolAlt, "column mismatch?");

  strcpy (name, "APER_FILL");
  float *AperFill = gfits_get_bintable_column_data (theader, &table, name, type, &NrowAlt, &NcolAlt);
  myAssert (!strcmp(type, "float"), "wrong column type for %s\n", name);
  myAssert (Nrow == NrowAlt, "row mismatch?");
  myAssert (Ncol == NcolAlt, "column mismatch?");

  // XXX this is not a bug, it is guaranteed since we have 3 convolution targets (raw, 6pix, 8pix)
  // if (Nrow > catalog->Nmeasure) {
  //   myAbort("more radial measurements than stars?  seems like a bug\n");
  // }

  // myAssert (catalog->lensing, "lensing is not allocated");
  // we could allocate here, or just insist this is an error?
  if (!catalog->lensing) {
    ALLOCATE (catalog->lensing, Lensing, catalog->Nmeasure);
    catalog->Nlensing = catalog->Nmeasure;
  }

  // In the loop below, we need to assign the 3 fwhm values to the three sets of structure
  // elements. I believe the sequence is fixed (RAW, C1, C2).  Get the fwhm values from the
  // first entry and compare to the rest

  float fwhmValues[3];
  fwhmValues[0] = PSFfwhm[0];
  fwhmValues[1] = PSFfwhm[1];
  fwhmValues[2] = PSFfwhm[2];

  int i;
  int Nap = 0; 
  for (i = 0; i < catalog->Nmeasure; i++) {
    dvo_lensing_init (&catalog->lensing[i]);

    if (catalog->measure[i].detID < RadID[Nap]) {
      continue;
      // this is a psf measurement which does not have a radial aperture
    }
    if (catalog->measure[i].detID > RadID[Nap]) {
      myAbort("radial apertures for source not in psf list? sources out of order?  seems like a bug\n");
      // this could be a radial aperture which does not have a PSF source, but that is not possible
    }

    // confirm the 3 FWHM values:
    myAssert (fwhmValues[0] == PSFfwhm[Nap+0], "FWHM mismatch %f vs %f", fwhmValues[0], PSFfwhm[Nap+0]);
    myAssert (fwhmValues[1] == PSFfwhm[Nap+1], "FWHM mismatch %f vs %f", fwhmValues[1], PSFfwhm[Nap+1]);
    myAssert (fwhmValues[2] == PSFfwhm[Nap+2], "FWHM mismatch %f vs %f", fwhmValues[2], PSFfwhm[Nap+2]);

    // EAM 2022.02.17 : here is the comment for the PV3 load:
    // XXX this is all hard-wired and should make use of the headers. 
    // psphot cmfs have 5 radial apertures:
    // array 0, 1, 2, 3, 4
    // SDSS  3, 4, 5, 6, 7

    // EAM 2022.02.17 : here is the situation for UNIONS DR3:
    // we have 3 convolutions (raw, 6", 8")
    // for each we have 6 apertures with max radii of (4, 8, 16, 32, 48, 64) pixels = (1, 2, 4, 8, 12, 16) arcsec
    // I am going to save (4, 16, 32) which have index of (0, 2, 3)

    # define RAD_0 0
    # define RAD_1 2
    # define RAD_2 3
    catalog->lensing[i]. F_ApR5    = AperFlux   [(Nap + 0)*Ncol + RAD_0];
    catalog->lensing[i].dF_ApR5    = AperFluxErr[(Nap + 0)*Ncol + RAD_0];
    catalog->lensing[i].sF_ApR5    = AperFluxStd[(Nap + 0)*Ncol + RAD_0];
    catalog->lensing[i].fF_ApR5    = AperFill   [(Nap + 0)*Ncol + RAD_0];
				   
    catalog->lensing[i]. F_ApR6    = AperFlux   [(Nap + 0)*Ncol + RAD_1];
    catalog->lensing[i].dF_ApR6    = AperFluxErr[(Nap + 0)*Ncol + RAD_1];
    catalog->lensing[i].sF_ApR6    = AperFluxStd[(Nap + 0)*Ncol + RAD_1];
    catalog->lensing[i].fF_ApR6    = AperFill   [(Nap + 0)*Ncol + RAD_1];
				   
    catalog->lensing[i]. F_ApR7    = AperFlux   [(Nap + 0)*Ncol + RAD_2];
    catalog->lensing[i].dF_ApR7    = AperFluxErr[(Nap + 0)*Ncol + RAD_2];
    catalog->lensing[i].sF_ApR7    = AperFluxStd[(Nap + 0)*Ncol + RAD_2];
    catalog->lensing[i].fF_ApR7    = AperFill   [(Nap + 0)*Ncol + RAD_2];

    catalog->lensing[i]. F_ApR5_C1 = AperFlux   [(Nap + 1)*Ncol + RAD_0];
    catalog->lensing[i].dF_ApR5_C1 = AperFluxErr[(Nap + 1)*Ncol + RAD_0];
    catalog->lensing[i]. F_ApR6_C1 = AperFlux   [(Nap + 1)*Ncol + RAD_1];
    catalog->lensing[i].dF_ApR6_C1 = AperFluxErr[(Nap + 1)*Ncol + RAD_1];
    catalog->lensing[i]. F_ApR7_C1 = AperFlux   [(Nap + 1)*Ncol + RAD_2];
    catalog->lensing[i].dF_ApR7_C1 = AperFluxErr[(Nap + 1)*Ncol + RAD_2];

    catalog->lensing[i]. F_ApR5_C2 = AperFlux   [(Nap + 2)*Ncol + RAD_0];
    catalog->lensing[i].dF_ApR5_C2 = AperFluxErr[(Nap + 2)*Ncol + RAD_0];
    catalog->lensing[i]. F_ApR6_C2 = AperFlux   [(Nap + 2)*Ncol + RAD_1];
    catalog->lensing[i].dF_ApR6_C2 = AperFluxErr[(Nap + 2)*Ncol + RAD_1];
    catalog->lensing[i]. F_ApR7_C2 = AperFlux   [(Nap + 2)*Ncol + RAD_2];
    catalog->lensing[i].dF_ApR7_C2 = AperFluxErr[(Nap + 2)*Ncol + RAD_2];

    catalog->lensing[i].detID = catalog->measure[i].detID;

    // XXX set the measure, object, etc ID values here
    // catalog->lensing[i].objID : set in find_matches_closest.c
    // catalog->lensing[i].catID : set in find_matches_closest.c
    // catalog->lensing[i].averef : set in find_matches_closest.c
    // catalog->lensing[i].imageID : set in FilterStars.c and UpdateImageIDs.c

    Nap += 3;
  }
  myAssert (Nap == Nrow, "did we go too far???");

  gfits_free_table (&table);
  free (AperFlux);
  free (AperFluxErr);
  free (AperFluxStd);
  free (AperFill);
  free (PSFfwhm);
  free (RadID);

  return TRUE;
}

