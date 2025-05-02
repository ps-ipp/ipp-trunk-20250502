# include "astro.h"

int altaz                   PROTO((int, char **));
int biassub                 PROTO((int, char **));
int cgrid                   PROTO((int, char **));
int coords                  PROTO((int, char **));
int cdot                    PROTO((int, char **));
int cline                   PROTO((int, char **));
int cneedles                PROTO((int, char **));
int cplot                   PROTO((int, char **));
int crotation               PROTO((int, char **));
int csystem                 PROTO((int, char **));
int ctimes                  PROTO((int, char **));
int cval                    PROTO((int, char **));
int czplot                  PROTO((int, char **));
int czcplot                 PROTO((int, char **));
int cdensify                PROTO((int, char **));
int cdhistogram             PROTO((int, char **));
int drizzle                 PROTO((int, char **));
int flux                    PROTO((int, char **));
int fitplx                  PROTO((int, char **));
int fitpm                   PROTO((int, char **));
int fitpm_irls              PROTO((int, char **));
int fitplx_irls             PROTO((int, char **));
int fixwrap                 PROTO((int, char **));
int fiximage                PROTO((int, char **));
int fixcols                 PROTO((int, char **));
int fixrows                 PROTO((int, char **));
int forcedphot              PROTO((int, char **));
int gauss                   PROTO((int, char **));
int gaussfit                PROTO((int, char **));
int getvel                  PROTO((int, char **));
int getlst                  PROTO((int, char **));
int imfit                   PROTO((int, char **));
int imsub                   PROTO((int, char **));
int jdtolst                 PROTO((int, char **));
int mjdtolst                PROTO((int, char **));
int medianmap               PROTO((int, char **));
int galsectors              PROTO((int, char **));
int galprofiles             PROTO((int, char **));
int galradius               PROTO((int, char **));
int galradbins              PROTO((int, char **));
int getcoords               PROTO((int, char **));
int elliprofile             PROTO((int, char **));
int ringflux                PROTO((int, char **));
int petrosian               PROTO((int, char **));
int kronflux                PROTO((int, char **));
int mkgauss                 PROTO((int, char **));
int mksersic                PROTO((int, char **));
int multifit                PROTO((int, char **));
int objload                 PROTO((int, char **));
int outline                 PROTO((int, char **));
int parallax_factor         PROTO((int, char **));
int parallax_factor_3d      PROTO((int, char **));
int polar                   PROTO((int, char **));
int precess                 PROTO((int, char **));
int profile                 PROTO((int, char **));
int radec                   PROTO((int, char **));
int region                  PROTO((int, char **));
int rotcurve                PROTO((int, char **));
int scale                   PROTO((int, char **));
int sexigesimal             PROTO((int, char **));
int sersic                  PROTO((int, char **));
int spec                    PROTO((int, char **));
int specpairfit             PROTO((int, char **));
int spexseq                 PROTO((int, char **));
int spex1dgas               PROTO((int, char **));
int spex2dgas               PROTO((int, char **));
int mkclusters              PROTO((int, char **));
int star                    PROTO((int, char **));
int times                   PROTO((int, char **));
int transform               PROTO((int, char **));
int vshimage                PROTO((int, char **));
int shimage                 PROTO((int, char **));
int wcs                     PROTO((int, char **));

static Command cmds[] = {  
  {1, "altaz",       altaz,        "convert alt/az to/from ra/dec"},
  {1, "biassub",     biassub,      "subtract medianed overscan row or column"},
  {1, "cgrid",       cgrid,        "plot sky coordinate grid"},
  {1, "coords",      coords,       "load coordinates for buffer from file"},
  {1, "cdot",        cdot,         "plot point in sky coordinates"},
  {1, "cline",       cline,        "plot line connecting two sky coordinates"},
  {1, "cneedles",    cneedles,     "plot vectors in sky coordinates"},
  {1, "cplot",       cplot,        "plot vectors in sky coordinates"},
  {1, "crotation",   crotation,    "rotate in 3D"},
  {1, "csystem",     csystem,      "convert between coordinate systems"},
  {1, "ctimes",      ctimes,       "convert between time formats"},
  {1, "cval",        cval,         "cosmic ray flux?"},
  {1, "czplot",      czplot,       "plot scaled vectors in sky coordinates"},
  {1, "czcplot",     czcplot,      "plot color-scaled vectors in sky coordinates"},
  {1, "cdensify",    cdensify,     "vectors to density history on projection"},
  {1, "cdhistogram", cdhistogram,  "vectors to 3D histogram on projection"},
  {1, "drizzle",     drizzle,      "transform image to image"},
  {1, "flux",        flux,         "flux in a convex contour"},
  {1, "fitplx",      fitplx,       "fit proper motion and parallax"},
  {1, "fitpm",       fitpm,        "fit proper motion only"},
  {1, "fitpm_irls",  fitpm_irls,   "fit proper motion only using irls method"},
  {1, "fitplx_irls", fitplx_irls,  "fit proper motion and parallax using irls method"},
  {1, "fixwrap",     fixwrap,      "fix megacam over-wrapped pixels"},
  {1, "fiximage",    fiximage,     "fix pixels in an image by interpolation"},
  {1, "fixcols",     fixcols,      "fix bad columns by comparing with others"},
  {1, "fixrows",     fixrows,      "fix bad rows by comparing with others"},
  {1, "forcedphot",  forcedphot,   "forced photometry on a star or set of stars, assuming gaussian profile"},
  {1, "gauss",       gauss,        "get statistics on a star, assuming gaussian profile"},
  {1, "getvel",      getvel,       "rotcurve to velocities"},
  {1, "getlst",      getlst,       "return LST given time and longitude"},
  {1, "imfit",       imfit,        "fit function"},
  {1, "imsub",       imsub,        "subtract function"},
  {1, "jdtolst",     jdtolst,      "JD to LST conversion"},
  {1, "mjdtolst",    mjdtolst,     "MJD to LST conversion"},
  {1, "medianmap",   medianmap,    "small median image"},
  {1, "mkgauss",     mkgauss,      "generate a 2-D gaussian centered in image"},
  {1, "mksersic",    mksersic,     "generate a 2-D sersic profile"},
  {1, "galsectors",  galsectors,   "generate radial vectors for sectors of width dtheta"},
  {1, "galprofiles", galprofiles,  "generate radial vectors with interpolation along paths"},
  {1, "galradius",   galradius,    "generate radial vectors with interpolation along paths"},
  {1, "galradbins",  galradbins,   "generate radial vectors with interpolation along paths"},
  {1, "getcoords",   getcoords,    "generate images containing the RA,DEC coord for each pixel"},
  {1, "elliprofile", elliprofile,  "generate radial vectors with interpolation along paths"},
  {1, "ringflux",    ringflux,     "mean flux in a ring"},
  {1, "petrosian",   petrosian,    "petrosian parameters given radial bins"},
  {1, "kronflux",    kronflux,     "measure kronflux stats"},
  {1, "multifit",    multifit,     "fit multi-order spectrum"},
  {1, "objload",     objload,      "plot obj data on Ximage "},
  {1, "outline",     outline,      "fit outline region"},
  {1, "parallax_factor", parallax_factor, "generate parallax_factors"},
  {1, "parallax_factor_3d", parallax_factor_3d, "generate parallax_factors (include in direction of star)"},
  {1, "polar",       polar,        "convert polar image to cartesian"},
  {1, "precess",     precess,      "precess coordinates"},
  {1, "profile",     profile,      "radial profile at X, Y"},
  {1, "radec",       radec,        "convert to/from radec in hms or dd"},
  {1, "region",      region,       "define sky region for plot"},
  {1, "rotcurve",    rotcurve,     "convert CO images to polar coords"},
  {1, "scale",       scale,        "get / set real bzero / bscale values"},
  {1, "sexigesimal", sexigesimal,  "convert to/from sexigesimal/decimal"},
  {1, "sersic",      sersic,       "generate sub-pixel resolved sersic model"},
  {1, "spec",        spec,         "extract a spectrum"},
  {1, "specpairfit", specpairfit,  "fit spectrum to another spectrum"},
  {1, "spexseq",     spexseq,      "generate the spectral sequence"},
  {1, "spex1dgas",   spex1dgas,    "minimize distances in 1D"},
  {1, "spex2dgas",   spex2dgas,    "minimize distances in 2D"},
  {1, "mkclusters",  mkclusters,   "group spectra by distance"},
  {1, "star",        star,         "star stats at rough coords"},
  {1, "transform",   transform,    "geometric transformation of image"},
  {1, "vshimage",    vshimage,     "generate images for vector spherical harmonic terms"},
  {1, "shimage",     shimage,      "generate images for spherical harmonic terms"},
  {1, "wcs",         wcs,          "set the wcs for the given image"},
}; 

/* not currently implemented 
  {"gaussfit",    gaussfit,     "fit a gaussian to pixels in a region"},
  {"testfit",     testfit, ""},
  {"times", , ""},
*/

void InitAstro () {
  
  int i;

  for (i = 0; i < sizeof (cmds) / sizeof (Command); i++) {
    AddCommand (&cmds[i]);
  }

}

void FreeAstro () {
}
