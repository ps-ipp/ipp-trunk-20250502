# include "data.h"
# include "deimos.h"

XXX: deprecated

// Use this to test the deimos_make_object function used by deimos_fitobj

int deimos_mkalt (int argc, char **argv) {

  // generate model observed flux for an object in a slit with background

  // input parameters:
  // * trace       : spline fit of slit central x pos vs y-coord
  // * profile     : slit window profile (vector)
  // * object      : vector of object flux vs y-coord 
  // * sky         : vector of local sky signal vs y-coord
  // * background  : vector of extra-slit background flux vs y-coord
  // * PSF         : point-spread function vector (flux normalized, x-dir)
  // * stilt       : slit tilt response : 2D kernel? 

  int N;

  // if any of these are not defined, they will have assumed identity values
  Vector *obj     = NULL;
  Vector *sky     = NULL;
  Vector *bck     = NULL;

  Spline *trace   = NULL;
  Vector *profile = NULL;
  Vector *psf     = NULL;
  float   stilt   = 0.0; // angle of the slit

  Buffer *output  = NULL;

  if ((N = get_argument (argc, argv, "-trace"))) {
    remove_argument (N, &argc, argv);
    if ((trace = FindSpline (argv[N])) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-profile"))) {
    remove_argument (N, &argc, argv);
    if ((profile = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-object"))) {
    remove_argument (N, &argc, argv);
    if ((obj = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-sky"))) {
    remove_argument (N, &argc, argv);
    if ((sky = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-backgnd"))) {
    remove_argument (N, &argc, argv);
    if ((bck = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-psf"))) {
    remove_argument (N, &argc, argv);
    if ((psf = SelectVector (argv[N], OLDVECTOR, TRUE)) == NULL) return (FALSE);
    remove_argument (N, &argc, argv);
  } else { goto usage; }
  if ((N = get_argument (argc, argv, "-stilt"))) {
    remove_argument (N, &argc, argv);
    stilt = atof (argv[N]);
    remove_argument (N, &argc, argv);
  } else { goto usage; }

  if (argc != 3) goto usage;

  int Nx = atoi (argv[2]);

  // all supplied vectors must be consistent 
  int Ny = obj->Nelements;
  if (Ny != sky->Nelements) {
    gprint (GP_ERR, "inconsistent wavelength scales (sky)\n");
    return FALSE;
  }
  if (Ny != bck->Nelements) {
    gprint (GP_ERR, "inconsistent wavelength scales (backgnd)\n");
    return FALSE;
  }

  if ((output = SelectBuffer (argv[1], ANYBUFFER, TRUE)) == NULL) return (FALSE);
  
  opihi_flt *objV = obj->elements.Flt;
  opihi_flt *skyV = sky->elements.Flt;
  opihi_flt *bckV = bck->elements.Flt;

  deimos_make_kernel (stilt, Nx);
  deimos_set_cross_ref (-1, Nx); 

  // generate the model based on the test values
  float *model = deimos_make_model (objV, skyV, bckV, psf, profile, trace, NULL, Nx, Ny, 0);

  ResetBuffer (output, Nx, Ny, -32, 0.0, 1.0);
  free (output[0].matrix.buffer);
  output[0].matrix.buffer = (char *) model;

  deimos_free_kernel();

  return TRUE;

usage:
  gprint (GP_ERR, "USAGE: deimos mkalt (buffer) (Nx) [-object vector] [-sky vector] [-backgnd vector] [-trace spline] [-profile vector] [-psf psf] [-stilt tilt]\n");
  return FALSE;
}
