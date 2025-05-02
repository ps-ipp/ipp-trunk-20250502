
typedef struct {
  float chisq;
  opihi_flt *obj;
  opihi_flt *sky;
  opihi_flt *bck;
} DeimosResult;

float     *deimos_make_model (opihi_flt *obj, opihi_flt *sky, opihi_flt *bck, Vector *psf, Vector *profile, Spline *slit_trace_red, Spline *slit_trace_blu, float redlimit, Spline *psf_trace, int Nx, int Ny, int row);
void       deimos_make_kernel (float stilt, int Nx);
void       deimos_free_kernel ();
void       deimos_set_cross_ref (int value, int Nx);

// internal to make_model:
float     *deimos_make_straight_image (opihi_flt *obj, opihi_flt *sky, Vector *psf, Spline *psf_trace, int Nx, int Ny, int row);
float     *deimos_apply_tilt (float *input, int Nx, int Ny);
void       deimos_apply_profile (Vector *profile, float *out, int Nx, int Ny);
void       deimos_add_background (opihi_flt *backgnd, float *out, int Nx, int Ny, int row);
float     *deimos_apply_trace (Spline *slit_trace_red, Spline *slit_trace_blu, float redlimit, float *input, int Nx, int Ny, int row);

