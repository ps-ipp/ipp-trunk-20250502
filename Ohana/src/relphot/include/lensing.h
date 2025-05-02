
// As a temporary hack (for UNIONS DR3), I am going to overload the following fields with
// radial aperture values from the convolved images:

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


// counter to track number of valid measurements for each radius
typedef struct {
  int N5_C0;  int N5_C1;  int N5_C2;
  int N6_C0;  int N6_C1;  int N6_C2;
  int N7_C0;  int N7_C1;  int N7_C2;
  int Nmeas;
} Lensctr;

Lensctr *dvo_lensctr_init (int Nsec);
int      dvo_lensctr_reset (Lensctr *lensctr, int Nsec);
int      dvo_lensing_accum (Lensobj *lensobj, Lensctr *lensctr, Lensing *lensing, float Fcal);
int      dvo_lensctr_has_values (Lensctr *lensctr);
int      dvo_lensobj_stat (float *mean, float *error, float *stdev, float *fill, int count);
int      dvo_lensobj_aves (Lensobj *lensobj, Lensctr *lensctr);
