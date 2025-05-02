# include "addstar.h"

// XXX largely psphot specific

void HeaderSetFree (HeaderSet *headerSets, off_t NheaderSets) {
  int i;

  if (!headerSets) return;
  for (i = 0; i < NheaderSets; i++) {
    FREE (headerSets[i].exthead);
    FREE (headerSets[i].exttype);
    FREE (headerSets[i].extdata);
    FREE (headerSets[i].extxrad);
  }
  FREE (headerSets);
}

HeaderSet *MatchHeaders (off_t **extsize, off_t *nimage, int mode, Header **headers, int Nheaders) {

  int i, j, Nimage, NIMAGE;
  char extname[80], exttype[80], exthead[80], xtension[80];
  HeaderSet *headerSets;

  if (!Nheaders) return NULL;

  ALLOCATE (extsize[0], off_t, Nheaders);

  Nimage = 0;
  NIMAGE = 10;
  ALLOCATE (headerSets, HeaderSet, NIMAGE);

  // what is the mode of the first header (ie, do we have a PHU DIS image?)
  mode = GetFileMode (headers[0]);

  // mosaic mef can be identified from the PHU
  if (mode == MOSAIC_MEF) {
    headerSets[Nimage].exthead     = strcreate ("PHU");
    headerSets[Nimage].extdata     = strcreate ("NONE");
    headerSets[Nimage].exttype     = NULL;
    headerSets[Nimage].extxrad     = NULL;
    headerSets[Nimage].extnum_data = -1;
    headerSets[Nimage].extnum_head =  0;
    Nimage ++;
  }

  // sdss obj can be identified from the PHU
  if (mode == SDSS_OBJ) {
    // XXX these should have two headers (phu + table)
    assert (Nheaders == 2);
    headerSets[0].extdata     = strcreate ("SDSS_OBJ");
    headerSets[0].exttype     = strcreate ("SDSS_OBJ");
    headerSets[0].exthead     = strcreate ("PHU");
    headerSets[0].extxrad     = NULL;
    headerSets[0].extnum_head = 0;
    headerSets[0].extnum_data = 1;
    extsize[0][0] = headers[0][0].datasize;
    *nimage = 1;
    return headerSets;
  }

  // check for ukirt mode
  // check if headers[1] is UKIRT_OBJ (and all others)
  if (Nheaders > 1) { 
    int myMode = GetFileMode (headers[1]);
    if (myMode != UKIRT_OBJ) goto other_modes;
    for (i = 2; i < Nheaders; i++) {
      myMode = GetFileMode (headers[i]);
      if (myMode != UKIRT_OBJ) {
	fprintf (stderr, "inconsistent headers for UKIRT\n");
	exit (2);
      }
    }

    // the 0th entry is an empty PHU
    extsize[0][0] = headers[0][0].datasize + gfits_data_size (headers[0]);

    for (i = 1; i < Nheaders; i++) {
      extsize[0][i] = headers[i][0].datasize + gfits_data_size (headers[i]);

      gfits_scan (headers[i], ExtnameKeyword, "%s", 1, extname);
      headerSets[Nimage].exttype = strcreate ("UKIRT_OBJ");
      headerSets[Nimage].extdata = strcreate (extname);
      headerSets[Nimage].exthead = strcreate (extname);
      headerSets[Nimage].extxrad = NULL;
      headerSets[Nimage].extnum_data = i;
      headerSets[Nimage].extnum_head = i;
      headerSets[Nimage].extnum_xrad = -1;
      Nimage ++;
    }
    *nimage = Nimage;
    return headerSets;
  }

other_modes:

  // now examine the headers, count the table entries, find corresponding headers
  for (i = 0; i < Nheaders; i++) {
    if (mode == SIMPLE_CMP) {
      extsize[0][i] = headers[i][0].datasize;
    } else {
      extsize[0][i] = headers[i][0].datasize + gfits_data_size (headers[i]);
    }

    // is this a fits table? (otherwise we skip it)
    if (!gfits_scan (headers[i], "XTENSION", "%s", 1, xtension)) continue;
    if (strcmp (xtension, "BINTABLE")) continue;

    if (!gfits_scan (headers[i], "EXTTYPE", "%s", 1, exttype)) continue;
    if (!strcmp (exttype, "SMPDATA")) goto keep;
    if (!strcmp (exttype, "PS1_DEV_0")) goto keep;
    if (!strcmp (exttype, "PS1_DEV_1")) goto keep;
    if (!strcmp (exttype, "PS1_V1")) goto keep;
    if (!strcmp (exttype, "PS1_V2")) goto keep;
    if (!strcmp (exttype, "PS1_V3")) goto keep;
    if (!strcmp (exttype, "PS1_V4")) goto keep;
    if (!strcmp (exttype, "PS1_V5")) goto keep;

    if (!strcmp (exttype, "PS1_SV1")) goto keep;
    if (!strcmp (exttype, "PS1_SV2")) goto keep;
    if (!strcmp (exttype, "PS1_SV3")) goto keep;
    if (!strcmp (exttype, "PS1_SV4")) goto keep;

    if (!strcmp (exttype, "PS1_DV1")) goto keep;
    if (!strcmp (exttype, "PS1_DV2")) goto keep;
    if (!strcmp (exttype, "PS1_DV3")) goto keep;
    if (!strcmp (exttype, "PS1_DV4")) goto keep;
    if (!strcmp (exttype, "PS1_DV5")) goto keep;

    continue;

  keep:
    gfits_scan (headers[i], ExtnameKeyword, "%s", 1, extname);
    gfits_scan (headers[i], "EXTHEAD", "%s", 1, exthead);

    headerSets[Nimage].exttype     = strcreate (exttype);
    headerSets[Nimage].extdata     = strcreate (extname);
    headerSets[Nimage].exthead     = strcreate (exthead);
    headerSets[Nimage].extxrad     = NULL;
    headerSets[Nimage].extnum_data = i;
    headerSets[Nimage].extnum_head = -1;
    headerSets[Nimage].extnum_xrad = -1;

    // XXX a special case for fforce xrad data
    if (READ_XRAD_DATA) {
      // extname is foobar.psf, convert to foobar.xrad
      int Nchar = strlen (extname); // foobar.psf : Nchar = 10
      ALLOCATE (headerSets[Nimage].extxrad, char, Nchar + 2); // xrad is 1 longer than psf
      memcpy   (headerSets[Nimage].extxrad, extname, Nchar - 3); // Nchar - 3 = 7
      memcpy  (&headerSets[Nimage].extxrad[Nchar-3], "xrad", 4); 
      headerSets[Nimage].extxrad[Nchar + 1] = 0; // put the 0 at element 11 for foobar.xrad\0
    }

    // find the matching exthead entry
    for (j = 0; j < Nheaders; j++) {
      if (!gfits_scan (headers[j], ExtnameKeyword, "%s", 1, extname)) continue;
      if (!strcmp (extname, headerSets[Nimage].exthead)) { 
	headerSets[Nimage].extnum_head = j;
	if (!READ_XRAD_DATA) break;
      }
      if (READ_XRAD_DATA && !strcmp (extname, headerSets[Nimage].extxrad)) { 
	headerSets[Nimage].extnum_xrad = j;
      }
      if ((headerSets[Nimage].extnum_head > -1) && (headerSets[Nimage].extnum_xrad > -1)) {
	// we can only get here if READ_XRAD_DATA is true
	break;
      }
    }

    // skip or crash on table with missing matching header?
    if (headerSets[Nimage].extnum_head == -1) {
      return NULL;
    }
    Nimage ++;
    if (Nimage == NIMAGE) {
      NIMAGE += 10;
      REALLOCATE (headerSets, HeaderSet, NIMAGE);
    }
  }

  // some old format files did not write EXTTYPE.  they have a single table in the first
  // extension matched to the header in the PHU
  if (Nimage == 0) {
    extsize[0][0] = headers[0][0].datasize + gfits_data_size (headers[0]);
    extsize[0][1] = headers[1][0].datasize + gfits_data_size (headers[1]);
    gfits_scan (headers[1], ExtnameKeyword, "%s", 1, extname);
    if (!strcmp (extname, "SMPFILE")) {
      headerSets[Nimage].extdata     = strcreate (extname);
      headerSets[Nimage].exttype     = strcreate ("SMPDATA");
      headerSets[Nimage].exthead     = strcreate ("PHU");
      headerSets[Nimage].extxrad     = NULL;
      headerSets[Nimage].extnum_head = 0;
      headerSets[Nimage].extnum_data = 1;
      Nimage = 1;
    }
  }
  
  *nimage = Nimage;
  return (headerSets);
}

