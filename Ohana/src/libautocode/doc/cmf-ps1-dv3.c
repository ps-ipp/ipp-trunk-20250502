# include "autocode.h"

int gfits_convert_CMF_PS1_V3 (CMF_PS1_V3 *data, off_t size, off_t nitems) {

  off_t i;
  unsigned char *byte, tmp;

  if (size != 224) { 
    fprintf (stderr, "WARNING: mismatch in data types CMF_PS1_V3: "OFF_T_FMT" vs %d\n",  size,  224);
    return (FALSE);
  }

  /* provide initial values to avoid compiler warnings for non-BYTE_SWAP arch */
  i = tmp = 0;
  byte = NULL;

# ifdef BYTE_SWAP
  byte = (unsigned char *) data;
  for (i = 0; i < nitems; i++, byte += 224) {
    /** BYTE SWAP **/
    SWAP_WORD (0); // IPP_IDET
    SWAP_WORD (4); // X_PSF
    SWAP_WORD (8); // Y_PSF
    SWAP_WORD (12); // X_PSF_SIG
    SWAP_WORD (16); // Y_PSF_SIG
    SWAP_WORD (20); // POSANGLE
    SWAP_WORD (24); // PLTSCALE
    SWAP_WORD (28); // PSF_INST_MAG
    SWAP_WORD (32); // PSF_INST_MAG_SIG
    SWAP_WORD (36); // PSF_INST_FLUX
    SWAP_WORD (40); // PSF_INST_FLUX_SIG
    SWAP_WORD (44); // AP_MAG
    SWAP_WORD (48); // AP_MAG_RAW
    SWAP_WORD (52); // AP_MAG_RADIUS
    SWAP_WORD (56); // AP_FLUX
    SWAP_WORD (60); // AP_FLUX_SIG
    SWAP_WORD (64); // PEAK_FLUX_AS_MAG
    SWAP_WORD (68); // CAL_PSF_MAG
    SWAP_WORD (72); // CAL_PSF_MAG_SIG
    SWAP_DBLE (76); // RA_PSF
    SWAP_DBLE (84); // DEC_PSF
    SWAP_WORD (92); // SKY
    SWAP_WORD (96); // SKY_SIGMA
    SWAP_WORD (100); // PSF_CHISQ
    SWAP_WORD (104); // CR_NSIGMA
    SWAP_WORD (108); // EXT_NSIGMA
    SWAP_WORD (112); // PSF_MAJOR
    SWAP_WORD (116); // PSF_MINOR
    SWAP_WORD (120); // PSF_THETA
    SWAP_WORD (124); // PSF_QF
    SWAP_WORD (128); // PSF_QF_PERFECT
    SWAP_WORD (132); // PSF_NDOF
    SWAP_WORD (136); // PSF_NPIX
    SWAP_WORD (140); // MOMENTS_XX
    SWAP_WORD (144); // MOMENTS_XY
    SWAP_WORD (148); // MOMENTS_YY
    SWAP_WORD (152); // MOMENTS_R1
    SWAP_WORD (156); // MOMENTS_RH
    SWAP_WORD (160); // KRON_FLUX
    SWAP_WORD (164); // KRON_FLUX_ERR
    SWAP_WORD (168); // KRON_FLUX_INNER
    SWAP_WORD (172); // KRON_FLUX_OUTER
    SWAP_WORD (176); // DIFF_NPOS
    SWAP_WORD (180); // DIFF_FRATIO
    SWAP_WORD (184); // DIFF_NRATIO_BAD
    SWAP_WORD (188); // DIFF_NRATIO_MASK
    SWAP_WORD (192); // DIFF_NRATIO_ALL
    SWAP_WORD (196); // DIFF_R_P
    SWAP_WORD (200); // DIFF_SN_P
    SWAP_WORD (204); // DIFF_R_M
    SWAP_WORD (208); // DIFF_SN_M
    SWAP_WORD (212); // FLAGS
    SWAP_WORD (216); // FLAGS2
    SWAP_BYTE (220); // N_FRAMES
    SWAP_BYTE (222); // PADDING
  }
# endif  

  return (TRUE);
} 

/*** add test of EXTNAME and header-defined columns? ***/
/* return internal structure representation */
CMF_PS1_V3 *gfits_table_get_CMF_PS1_V3 (FTable *ftable, off_t *Ndata, char *swapped) {

  int Ncols;
  CMF_PS1_V3 *data;

  Ncols = ftable[0].header[0].Naxis[0];
  if (Ncols != 224) {
    fprintf (stderr, "ERROR: mis-match in table size: width is %d but should be %d bytes\n", Ncols, 224);
    return NULL;
  }

  *Ndata = ftable[0].header[0].Naxis[1];
  data = (CMF_PS1_V3 *) ftable[0].buffer;
  if ((swapped == NULL) || (*swapped == FALSE)) {
    if (!gfits_convert_CMF_PS1_V3 (data, sizeof (CMF_PS1_V3), *Ndata)) {
      return NULL;
    }
    gfits_table_scale_data (ftable);
    if (swapped != NULL) *swapped = TRUE;
  }
  return (data);
}

int gfits_table_set_CMF_PS1_V3 (FTable *ftable, CMF_PS1_V3 *data, off_t Ndata) {

  Header *header;

  header = ftable[0].header;

  /* create table header */
  if (!gfits_create_table_header (header, "BINTABLE", "CMF_PS1_V3")) return (FALSE);

  /* define table layout */
  /** TABLE DEFINITION **/
  gfits_define_bintable_column (header, "J",    "IPP_IDET",         "detection ID                     ", "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "X_PSF",            "x coord",                         "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "Y_PSF",            "y coord",                         "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "X_PSF_SIG",        "x coord error",                   "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "Y_PSF_SIG",        "y coord error",                   "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "POSANGLE",         "Posangle at source",              "degrees",           1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PLTSCALE",         "Plate Scale at source",           "arcsec/pixel",      1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_INST_MAG",     "inst mags",                       "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_INST_MAG_SIG", "inst mag error",                  "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_INST_FLUX",    "psf flux",                        "counts",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_INST_FLUX_SIG", "psf flux error",                  "counts      ",      1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "AP_MAG",           "standard aperture mag",           "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "AP_MAG_RAW",       "raw aperture mag",                "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "AP_MAG_RADIUS",    "radius used for fit",             "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "AP_FLUX",          "ap flux",                         "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "AP_FLUX_SIG",      "ap flux err",                     "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PEAK_FLUX_AS_MAG", "peak flux as a mag",              "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "CAL_PSF_MAG",      "calibrated psf mag",              "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "CAL_PSF_MAG_SIG",  "zero point scatter",              "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "D",    "RA_PSF",           "PSF RA coord",                    "degrees",           1.0, 0.0);
  gfits_define_bintable_column (header, "D",    "DEC_PSF",          "PSF DEC coord",                   "degrees",           1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "SKY",              "sky flux",                        "cnts/sec",          1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "SKY_SIGMA",        "sky flux error",                  "cnts/sec",          1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_CHISQ",        "psf fit chisq",                   "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "CR_NSIGMA",        "Nsigma deviations from PSF to CF", "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "EXT_NSIGMA",       "Nsigma deviations from PSF to EXT", "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_MAJOR",        "psf fit major axis",              "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_MINOR",        "psf fit minor axis",              "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_THETA",        "ellipse angle",                   "degrees",           1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_QF",           "quality factor",                  "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_QF_PERFECT",   "quality factor perfect",          "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "PSF_NDOF",         "psf degrees of freedom",          "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "PSF_NPIX",         "psf number of pixels",            "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "MOMENTS_XX",       "second moment X",                 "pixels^2",          1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "MOMENTS_XY",       "second moment Y",                 "pixels^2",          1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "MOMENTS_YY",       "second moment XY",                "pixels^2",          1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "MOMENTS_R1",       "first radial moment",             "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "MOMENTS_RH",       "half radial moment",              "pixels^1/2",        1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "KRON_FLUX",        "kron flux",                       "counts",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "KRON_FLUX_ERR",    "kron flux error",                 "counts",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "KRON_FLUX_INNER",  "kron flux 1<R<2.5",               "counts",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "KRON_FLUX_OUTER",  "kron flux 2.5<R<4",               "counts",            1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "DIFF_NPOS",        "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_FRATIO",      "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_NRATIO_BAD",  "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_NRATIO_MASK", "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_NRATIO_ALL",  "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_R_P",         "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_SN_P",        "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_R_M",         "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_SN_M",        "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "FLAGS",            "analysis flags",                  "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "FLAGS2",           "analysis flags (2)",              "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "N_FRAMES",         "images overlapping peak",         "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "PADDING",          "padding for 8byte records",       "",                  1.0, 0.0);

  /* create table */
  if (!gfits_create_table (header, ftable)) return (FALSE);

  /* add data values */
  if (!gfits_table_scale_data (ftable)) return (FALSE);
  if (!gfits_convert_CMF_PS1_V3 (data, sizeof (CMF_PS1_V3), Ndata)) return (FALSE);
  if (!gfits_add_rows (ftable, (char *) data, Ndata, sizeof (CMF_PS1_V3))) return (FALSE);

  return (TRUE);
}

int gfits_table_mkheader_CMF_PS1_V3 (Header *header) {

  /* create table header */
  if (!gfits_create_table_header (header, "BINTABLE", "CMF_PS1_V3")) return (FALSE);

  /* define table layout */
  /** TABLE DEFINITION **/
  gfits_define_bintable_column (header, "J",    "IPP_IDET",         "detection ID                     ", "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "X_PSF",            "x coord",                         "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "Y_PSF",            "y coord",                         "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "X_PSF_SIG",        "x coord error",                   "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "Y_PSF_SIG",        "y coord error",                   "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "POSANGLE",         "Posangle at source",              "degrees",           1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PLTSCALE",         "Plate Scale at source",           "arcsec/pixel",      1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_INST_MAG",     "inst mags",                       "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_INST_MAG_SIG", "inst mag error",                  "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_INST_FLUX",    "psf flux",                        "counts",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_INST_FLUX_SIG", "psf flux error",                  "counts      ",      1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "AP_MAG",           "standard aperture mag",           "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "AP_MAG_RAW",       "raw aperture mag",                "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "AP_MAG_RADIUS",    "radius used for fit",             "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "AP_FLUX",          "ap flux",                         "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "AP_FLUX_SIG",      "ap flux err",                     "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PEAK_FLUX_AS_MAG", "peak flux as a mag",              "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "CAL_PSF_MAG",      "calibrated psf mag",              "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "CAL_PSF_MAG_SIG",  "zero point scatter",              "mags",              1.0, 0.0);
  gfits_define_bintable_column (header, "D",    "RA_PSF",           "PSF RA coord",                    "degrees",           1.0, 0.0);
  gfits_define_bintable_column (header, "D",    "DEC_PSF",          "PSF DEC coord",                   "degrees",           1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "SKY",              "sky flux",                        "cnts/sec",          1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "SKY_SIGMA",        "sky flux error",                  "cnts/sec",          1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_CHISQ",        "psf fit chisq",                   "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "CR_NSIGMA",        "Nsigma deviations from PSF to CF", "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "EXT_NSIGMA",       "Nsigma deviations from PSF to EXT", "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_MAJOR",        "psf fit major axis",              "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_MINOR",        "psf fit minor axis",              "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_THETA",        "ellipse angle",                   "degrees",           1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_QF",           "quality factor",                  "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "PSF_QF_PERFECT",   "quality factor perfect",          "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "PSF_NDOF",         "psf degrees of freedom",          "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "PSF_NPIX",         "psf number of pixels",            "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "MOMENTS_XX",       "second moment X",                 "pixels^2",          1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "MOMENTS_XY",       "second moment Y",                 "pixels^2",          1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "MOMENTS_YY",       "second moment XY",                "pixels^2",          1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "MOMENTS_R1",       "first radial moment",             "pixels",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "MOMENTS_RH",       "half radial moment",              "pixels^1/2",        1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "KRON_FLUX",        "kron flux",                       "counts",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "KRON_FLUX_ERR",    "kron flux error",                 "counts",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "KRON_FLUX_INNER",  "kron flux 1<R<2.5",               "counts",            1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "KRON_FLUX_OUTER",  "kron flux 2.5<R<4",               "counts",            1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "DIFF_NPOS",        "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_FRATIO",      "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_NRATIO_BAD",  "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_NRATIO_MASK", "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_NRATIO_ALL",  "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_R_P",         "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_SN_P",        "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_R_M",         "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "E",    "DIFF_SN_M",        "diff param",                      "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "FLAGS",            "analysis flags",                  "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "J",    "FLAGS2",           "analysis flags (2)",              "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "N_FRAMES",         "images overlapping peak",         "",                  1.0, 0.0);
  gfits_define_bintable_column (header, "I",    "PADDING",          "padding for 8byte records",       "",                  1.0, 0.0);

  return (TRUE);
}

int Send_CMF_PS1_V3 (int device, CMF_PS1_V3 *data, int Ndata, int copy) {

  int Nwrite, Nbytes;
  CMF_PS1_V3 *tmpdata;

  Nbytes = Ndata * sizeof (CMF_PS1_V3);

  if (copy) {
    ALLOCATE (tmpdata, CMF_PS1_V3, Ndata);
    memcpy (tmpdata, data, Nbytes);
  } else {
    tmpdata = data;
  }

  if (!gfits_convert_CMF_PS1_V3 (tmpdata, sizeof (CMF_PS1_V3), Ndata)) return (FALSE);

  SendCommand (device, 16, "NVALUE: %6d", Ndata);
  SendCommand (device, 16, "NBYTES: %6d", Nbytes);
  Nwrite = write (device, tmpdata, Nbytes);
  if (Nwrite != Nbytes) {
    return (FALSE);
  }
  
  /* perform handshaking? */

  return (TRUE);
}

int Recv_CMF_PS1_V3 (int device, CMF_PS1_V3 **data, int *Ndata) {

  int ndata;
  IOBuffer message;
  CMF_PS1_V3 *tmpdata;

  ExpectCommand (device, 16, 1.0, &message);
  sscanf (message.buffer, "%*s %d", &ndata);
  FreeIOBuffer (&message);
  
  /* what is reasonable for timeout? */
  ExpectMessage (device, 1.0, &message);
  
  tmpdata = (CMF_PS1_V3 *) message.buffer;
  if (!gfits_convert_CMF_PS1_V3 (tmpdata, sizeof (CMF_PS1_V3), ndata)) return (FALSE);

  /* double-check data length? */
  /* perform handshaking? */

  *Ndata = ndata;
  *data = tmpdata;

  return (TRUE);
}
