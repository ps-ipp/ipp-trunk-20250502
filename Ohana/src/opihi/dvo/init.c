# include "dvoshell.h"

int avextract       PROTO((int, char **));
int avmatch         PROTO((int, char **));
int avselect        PROTO((int, char **));
int avperiodogram   PROTO((int, char **));
int avperiodomatch  PROTO((int, char **));
int badimages       PROTO((int, char **));
int calextract      PROTO((int, char **));
int calmextract     PROTO((int, char **));
int catlog          PROTO((int, char **));
int catname         PROTO((int, char **));
int catid           PROTO((int, char **));
int catdir_define   PROTO((int, char **));
int ccd             PROTO((int, char **));
int cimages         PROTO((int, char **));
int cmatch          PROTO((int, char **));
int cmd             PROTO((int, char **));
int cmpload         PROTO((int, char **));
int cmpread         PROTO((int, char **));
int coordimage      PROTO((int, char **));
int coordmosaic     PROTO((int, char **));
int psastro_model   PROTO((int, char **));
int ddmags          PROTO((int, char **));
int detrend         PROTO((int, char **));
int dmagaves        PROTO((int, char **));
int dmagmeas        PROTO((int, char **));
int dmags           PROTO((int, char **));
int dmt             PROTO((int, char **));
int elixir          PROTO((int, char **));
int fitcolors       PROTO((int, char **));
int fitsed          PROTO((int, char **));
int gcat            PROTO((int, char **));
int catlist         PROTO((int, char **));
int getxtra         PROTO((int, char **));
int gimages         PROTO((int, char **));
int gstar           PROTO((int, char **));
int gtypes          PROTO((int, char **));
int hosts           PROTO((int, char **));
int images          PROTO((int, char **));
int imbox           PROTO((int, char **));
int imdata          PROTO((int, char **));
int imdense         PROTO((int, char **));
int imextract       PROTO((int, char **));
int imlist          PROTO((int, char **));
int imphot          PROTO((int, char **));
int imrough         PROTO((int, char **));
int imsearch        PROTO((int, char **));
int lcat            PROTO((int, char **));
int lcurve          PROTO((int, char **));
int lightcurve      PROTO((int, char **));
int mextract        PROTO((int, char **));
int mmatch          PROTO((int, char **));
int mmextract       PROTO((int, char **));
int objectcoverage  PROTO((int, char **));
int pcat            PROTO((int, char **));
int photcodes       PROTO((int, char **));
int pmeasure        PROTO((int, char **));
int paverage        PROTO((int, char **));
int procks          PROTO((int, char **));
int remote          PROTO((int, char **));
int showtile        PROTO((int, char **));
int skycat          PROTO((int, char **));
int skycoverage     PROTO((int, char **));
int skyregion       PROTO((int, char **));
int simage          PROTO((int, char **));
int subpix          PROTO((int, char **));
int version         PROTO((int, char **));

static Command cmds[] = {  
  {1, "avextract",   avextract,    "extract average data values"},
  {1, "avmatch",     avmatch,      "extract average data values matched to RA,DEC points"},
  {1, "avselect",    avselect,     "extract average data values within range of a list of RA,DEC points"},
  {1, "avperiodogram",  avperiodogram, "perform periodogram on objects based on restrictions"},
  {1, "avperiodomatch", avperiodomatch, "perform periodogram on objects based on ra,dec list"},
  {1, "badimages",   badimages,    "look for images with anomalous astrometry"},
//  {1, "calextract",  calextract,   "extract photometry calibration"},
//  {1, "calmextract", calmextract,  "extract photometry calibration"},
  {1, "catname",     catname,      "list catalog files by name"},
  {1, "catid",       catid,        "list catalog files by catID"},
  {1, "catdir",      catdir_define,"re-define CATDIR"},
//  {1, "ccd",         ccd,          "plot color-color diagram"},
  {1, "cimages",     cimages,      "fill image boxes with a color"},
  {1, "cmatch",      cmatch,       "match two catalogs"},
//  {1, "cmd",         cmd,          "plot cmd of stars in current region"},
  {1, "cmpload",     cmpload,      "load cmp file into ?"},
  {1, "cmpread",     cmpread,      "read data from cmp format files"},
  {1, "coordimage",  coordimage,   "generate a map of the transformation residuals"},
  {1, "coordmosaic", coordmosaic,  "generate a map of the distortion"},
  {1, "psastro_model", psastro_model, "save psastro-format astrometry model"},
//  {1, "ddmags",      ddmags,       "plot magnitude differences"},
  {1, "detrend",     detrend,      "extract from detrend database?"},
//  {1, "dmagaves",    dmagaves,     "foo"},
//  {1, "dmagmeas",    dmagmeas,     "foo"},
//  {1, "dmags",       dmags,        "plot differential magnitudes between filters"},
  {1, "dmt",         dmt,          "plot mag scatter"},
  {1, "elixir",      elixir,       "talk to elixir"},
//  {1, "fitcolors",   fitcolors,    "fit chip-to-chip color terms"},
  {1, "fitsed",      fitsed,       "fit stellar SEDs to objects"},
  {1, "gcat",        gcat,         "get catalog at location"},
  {1, "catlist",     catlist,      "get list of catalogs for region / host"},
  {1, "gimages",     gimages,      "get images at location"},
  {1, "gstar",       gstar,        "get star statistics"},
  {1, "hosts",       hosts,        "remote host support functions"},
  {1, "images",      images,       "plot image boxes"},
  {1, "imbox",       imbox,        "plot expected image box"},
  {1, "imdata",      imdata,       "extract data for specific images"},
  {1, "imdense",     imdense,      "image density plot"},
  {1, "imextract",   imextract,    "extract vectors from catalogs"},
  {1, "imlist",      imlist,       "list image info"},
  {1, "imphot",      imphot,       "image photometry info"},
  {1, "imrough",     imrough,      "get info from imruf database"},
  {1, "imsearch",    imsearch,     "get info from imreg database"},
  {1, "lcat",        lcat,         "list catalogs in region"},
  {1, "lcurve",      lcurve,       "plot lightcurve for a star"},
  {1, "lightcurve",  lightcurve,   "extract lightcurve for a star"},
  {1, "mextract",    mextract,     "extract measure data values"},
  {1, "mmatch",      mmatch,       "extract measure data values matched to RA,DEC points"},
  {1, "mmextract",   mmextract,    "extract joined measurements"},
  {1, "objectcoverage", objectcoverage, "plot catalog boundaries"},
  {1, "pcat",        skycat,       "plot catalog boundaries"},
  {1, "photcodes",   photcodes,    "list photometry codes"},
  {1, "pmeasure",    pmeasure,     "plot individual measurements"},
  {1, "paverage",    paverage,     "plot average magnitude"},
  {1, "procks",      procks,       "plot rocks"},
  {1, "remote",      remote,       "generic remote dvo client operation"},
  {1, "showtile",    showtile,     "plot tile pattern"},
  {1, "skycat",      skycat,       "show sky catalog boundaries"},
  {1, "skycoverage", skycoverage,  "measure image union on sky"},
  {1, "skyregion",   skyregion,    "set sky region for db queries"},
  {1, "simage",      simage,       "plot stars in an image"},
  {1, "subpix",      subpix,       "get subpixel positions"},
  {1, "version",     version,      "show version information"},
//{1, "addxtra",     addxtra,      "add extra data to object"},
//{1, "getxtra",     getxtra,      "get extra data from object"},
}; 

/* move to astro */

void InitDVO () {
  
  int i;

  for (i = 0; i < sizeof (cmds) / sizeof (Command); i++) {
    AddCommand (&cmds[i]);
  }
}

void FreeDVO () {
}
