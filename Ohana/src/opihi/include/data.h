# include "external.h"
# include "shell.h"
# include "dvomath.h"
# include "convert.h"
# include "display.h"
# include "get_graphdata.h"

# ifndef DATA_H
# define DATA_H

/*** typedef structs used by math functions ***/
typedef struct {
  int   NLINES;
  int   Nlines;
  char **lines;
  char  *name;
} Queue;

typedef struct {
  char *name;
  int NWORDS;
  int Nwords;
  char **words;
  char **value;
} Page;

typedef struct {
  char *name;
  int NPAGES;
  int Npages;
  Page **pages;
  // int *index; (why did I define this?  is it not used?)
  char **pageIDs;
} Book;

// the interpolating spline has valu
typedef struct {
  int Nknots;
  opihi_flt *xk;
  opihi_flt *yk;
  opihi_flt *y2;
  char *name;
} Spline;

typedef struct {
  char *name;
  int Ninput;
  int Nx;
  int Ny;
  float **flx;
  float **var;
} MedImageType;

/*** typedef structs used by the Neural Network functions (nnet_*) ***/
typedef struct {
  char *name;
  int Nlayer; // Nlayers = input layer + output layer + hidden layers
  int *Nnodes; // number of nodes per layer
  float **weight; // a matrix between each layer
  float **biases; // a vector for each layer

  float **zvalue; // a vector of z values for each layer (= w*input + b)
  float **svalue; // a vector of s values for each layer (= sigmoid(z))
  float **sprime; // 
  float **delta;  // 

  float ** Nabla_b; // a vector of Nabla_b values for each layer
  float **dNabla_b; // a vector of Nabla_b values for each layer

  float ** Nabla_w; // a matrix of Nabla_w values for each layer
  float **dNabla_w; // a matrix of Nabla_w values for each layer
} Nnet;

// a single chebyshev associates a name with the scaling
// relationship and maybe other stuff
typedef struct {
  char *name;
  opihi_flt scale[2];
  opihi_flt zero[2];
  int   order;
  int   dimen;
  opihi_flt *A;
} ChebyshevType;

void InitData (void);
void FreeData (void);

/* in book.c */
void InitBooks (void);
void InitBook (Book *book, char *name);
void FreeBook (Book *book);
Book *FindBook (char *name);
Book *GetBook (int where);
Book *CreateBook (char *name);
int DeleteBook (Book *book);
void ListBooks (void);

/* in page.c */
void InitPage (Page *page, char *name);
void FreePage (Page *page);
Page *FindPage (Book *book, char *name);
Page *GetPage (Book *book, int where);
Page *GetPageRestricted (Book *book, int where, char *keyName, char *keyValue);
Page *CreatePage (Book *book, char *name);
int ShufflePages (Book *book);
int DeletePage (Book *book, Page *page);
void ListPages (Book *book);
void ListWords (Page *page);
int BookSetWord (Page *page, char *word, char *value);
char *BookGetWord (Page *page, char *word);

/* in queues.c */
void InitQueues (void);
void ListQueues (void);
Queue *FindQueue (char *name);
Queue *CreateQueue (char *name);
void PushQueue (Queue *queue, char *line);
void PushNamedQueue (char *name, char *line);
char *PopQueue (Queue *queue);
char *PopQueueMatch (Queue *queue, char *Key, char *value);
void PushQueueUnique (Queue *queue, char *line, char *Key);
void PushQueueReplace (Queue *queue, char *line, char *Key);
int InitQueue (Queue *queue);
int DeleteQueue (Queue *queue);
int PrintQueue (Queue *queue);
int SaveQueue (Queue *queue, char *filename);

/* in fft.c */
void fft1D (float *dataRe, float *dataIm, int N, int Nbit, int forward);
int fftND (float *dataRe, float *dataIm, int Ndim, off_t *Nsize, int forward);
void dfft1D (double *dataRe, double *dataIm, int N, int Nbit, int forward);
int dfftND (double *dataRe, double *dataIm, int Ndim, int *Nsize, int forward);
int IsBinary (int N, int *Nbit);

/* in spline.c */
void spline_construct_flt (float *x, float *y, int N, float *y2, float dyLower, float dyUpper);
float spline_apply_flt (float *x, float *y, float *y2, int N, float X);
void spline_construct_dbl (opihi_flt *x, opihi_flt *y, int N, opihi_flt *y2, opihi_flt dyLower, opihi_flt dyUpper);
opihi_flt spline_apply_dbl (opihi_flt *x, opihi_flt *y, opihi_flt *y2, int N, opihi_flt X);

/* in svdcmp.c */
int svdcmp (float *a, opihi_flt *w, float *v, int Nx, int Ny);

/* in svdcmp_bond_raw.c */
int svdcmp_bond_raw(int m, int n, int withu, int withv, double eps, double tol, double **a, double *q, double **u, double **v);

/* in svdcmp_bond_new.c */
int svdcmp_bond_new(int m, int n, int withu, int withv, double eps, double tol, double **a, double *q, double **u, double **v);

/* mrqmin.c */
opihi_flt mrqcof (opihi_flt *x, opihi_flt *y, opihi_flt *dy, int Npts, 
	      opihi_flt *par, int Npar, opihi_flt **ta, opihi_flt **tb, 
	      opihi_flt (funcs)(opihi_flt, opihi_flt *, int, opihi_flt *));

opihi_flt mrqmin (opihi_flt *x, opihi_flt *y, opihi_flt *dy, int Npts, 
	      opihi_flt *par, int Npar, 
	      opihi_flt (funcs)(opihi_flt, opihi_flt *, int, opihi_flt *), int VERBOSE);

opihi_flt mrqinit (opihi_flt *x, opihi_flt *y, opihi_flt *dy, int Npts, 
	       opihi_flt *par, int Npar, 
	       opihi_flt (funcs)(opihi_flt, opihi_flt *, int, opihi_flt *), int VERBOSE);

opihi_flt **mrqcovar (int Npar);

void mrqfree (int Npar);

/* mrq2dmin.c */
opihi_flt mrq2dcof (opihi_flt *x, opihi_flt *t, opihi_flt *y, opihi_flt *dy, int Npts, 
		opihi_flt *par, int Npar, opihi_flt **ta, opihi_flt **tb, 
		opihi_flt (funcs)(opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *));

opihi_flt mrq2dmin (opihi_flt *x, opihi_flt *t, opihi_flt *y, opihi_flt *dy, int Npts, 
		opihi_flt *par, int Npar, 
		opihi_flt (funcs)(opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *), int VERBOSE);

opihi_flt mrq2dinit (opihi_flt *x, opihi_flt *t, opihi_flt *y, opihi_flt *dy, int Npts, 
		 opihi_flt *par, int Npar, 
		 opihi_flt (funcs)(opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *), int VERBOSE);

opihi_flt mrq2dchi (opihi_flt *x, opihi_flt *t, opihi_flt *y, opihi_flt *dy, int Npts, 
		opihi_flt *par, int Npar, 
		opihi_flt (funcs)(opihi_flt, opihi_flt, opihi_flt *, int, opihi_flt *));

int mrq2dlimits (opihi_flt *pmin, opihi_flt *pmax, int Npar);

opihi_flt **mrq2dcovar (int Npar);

void mrq2dfree (int Npar);

/* gaussian.c */
double gaussian (double x, double mean, double sigma);
void gauss_init (int Nbin);
double rnd_gauss (double mean, double sigma);

/* starfuncs.c */
double get_aperture_stats (Matrix *matrix, int X, int Y, int Npix, int Nborder, double max, int VERBOSE);
double get_box_stats (Matrix *matrix, int X, int Y, int dX, int dY, int Nborder, double max, int VERBOSE);

int set_rough_radii (double Ra, double Ri, double Ro);
int get_rough_star (float *data, int Nx, int Ny, int x, int y, opihi_flt *xc, opihi_flt *yc, opihi_flt *sx, opihi_flt *sy, opihi_flt *sxy, opihi_flt *zs, opihi_flt *zp, opihi_flt *sk);

/* precess.c */
double BtoJ (double in_epoch);
double get_epoch (char *in_epoch, char mode);

int SetGridScales (double *major, double *minor, double range);

/* graphtools.c */
void          SetLimits             PROTO((Vector *xvec, Vector *yvec, Graphdata *graphmode));
void          SetLimitsRaw          PROTO((float *xvec, float *yvec, int Npts, Graphdata *graphmode));
void          ApplyLimits           PROTO((int Xgraph, Graphdata *graphmode, int apply));
int           style_args            PROTO((Graphdata *graphmode, int *argc, char **argv, int *kapa));

int read_table_vectors (int argc, char **argv, char *extname);

void *db_getConnection (void);

int bracket (double *list, int Nlist, int mode, double value);
int ibracket (int *list, int Nlist, int mode, double value);

void FreeKapa (void);
void FreeQueues (void);
void FreeBooks (void);

/* in SplineOps.c */
void InitSplines ();
void FreeSplines ();
void InitSpline (Spline *spline, char *name, int Nknots);
void FreeSpline (Spline *spline);
Spline *GetSpline (int where);
Spline *FindSpline (char *name);
Spline *CreateSpline (char *name, int Nknots);
int DeleteSpline (Spline *spline);
void ListSplines ();
int SaveSpline (char *filename, char *name, int append);
int LoadSpline (char *filename, char *name);

/* hermitian functions */
double hermitian_polynomial (double x, int order);
double hermitian_00(double x);
double hermitian_01(double x);
double hermitian_02(double x);
double hermitian_03(double x);
double hermitian_04(double x);
double hermitian_05(double x);
double hermitian_06(double x);
double hermitian_07(double x);
double hermitian_08(double x);
double hermitian_09(double x);
double hermitian_10(double x);

/* in MedImageOps.c */
void InitMedImages ();
void FreeMedImages ();
void FreeMedImage (MedImageType *medimage);
MedImageType *FindMedImage (char *name);
MedImageType *CreateMedImage (char *name, int Nx, int Ny);
int DeleteMedImage (MedImageType *medimage);
void ListMedImages ();

// in tvchannel.c
int GetKapaChannelFromString (char *string);

// in sort_funcs.c:
void sort_opihi_flt_index (opihi_flt *X, int *IDX, int N);
void sort_int_index (opihi_int *X, int *IDX, int N);
void sort_float_index (float *X, int *IDX, int N);

/*** Neural Network functions (nnet_*) ***/

// in lib.data/nnet.c:
void InitNnets ();
void InitNnetData (Nnet *nnet, char *name, int Nlayer);

void FreeNnets ();
void FreeNnetData (Nnet *nnet);

Nnet *GetNnet (int where);
Nnet *FindNnet (char *name);
Nnet *CreateNnet (char *name, int Nlayer);
void CreateNnetData (Nnet *nnet, int LargeWeightInit);

void PrintNnet (Nnet *nnet);
int  DeleteNnet (Nnet *nnet);
void ListNnets ();

// in cmd.data/nnet.c:
int nnet_command (int argc, char **argv);

// in cmd.data/nnet_commands.c:
int nnet_init (int argc, char **argv);
int nnet_list (int argc, char **argv);
int nnet_delete (int argc, char **argv);
int nnet_create (int argc, char **argv);
int nnet_set (int argc, char **argv);
int nnet_get (int argc, char **argv);
int nnet_read (int argc, char **argv);
int nnet_write (int argc, char **argv);
int nnet_print (int argc, char **argv);

// in cmd.data/nnet_train.c:
int nnet_train (int argc, char **argv);
void nnet_feedforward (Nnet *nnet);

// in cmd.data/nnet_apply.c:
int nnet_apply (int argc, char **argv);

/*** Chebyshev functions (chebyshev.c, chebyshev_commands.c) ***/

void InitChebyshevs ();
void FreeChebyshevs ();
void FreeChebyshev (ChebyshevType *chebyshev);
ChebyshevType *FindChebyshev (char *name);
ChebyshevType *CreateChebyshev (char *name);
int DeleteChebyshev (ChebyshevType *chebyshev);
void ListChebyshevs ();
int ChebyshevSetScale (ChebyshevType *cheb, Vector *vec, int dir);
Vector *ChebyshevNormVector (ChebyshevType *cheb, Vector *vec, int dir);
Vector *ChebyshevPolyVector (Vector *Ti, Vector *vec, int order);
int ChebyshevPolyFit1D (ChebyshevType *cheb, Vector *vec, Vector **poly);
int ChebyshevPolyApplyFit1D (ChebyshevType *cheb, Vector *vec, Vector **poly);
int ChebyshevPolyFit2D (ChebyshevType *cheb, Vector *vec, Vector **xPoly, Vector **yPoly);
int ChebyshevPolyApplyFit2D (ChebyshevType *cheb, Vector *vec, Vector **xPoly, Vector **yPoly);

# endif
