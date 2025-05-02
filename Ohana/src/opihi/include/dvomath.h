/*** dvomath.h ***/

# ifndef DVOMATH_H
# define DVOMATH_H

# define OPIHI_NAME_SIZE 1024

/* OPIHI_FLT, OPIHI_INT and related are defined in libdvo/include/dvodb.h */

# define NCHARS 256

# define REQUIRE_VECTOR_FLT(VECT,RVAL) { \
  if (VECT->type != OPIHI_FLT) { \
    gprint (GP_ERR, "function requires vector of type FLT\n"); \
    return (RVAL); \
  } }

# define REQUIRE_VECTOR_INT(VECT,RVAL) {	\
  if (VECT->type != OPIHI_INT) { \
    gprint (GP_ERR, "function requires vector of type INT\n"); \
    return (RVAL); \
  } }

enum {ANYVECTOR, NEWVECTOR, OLDVECTOR};
enum {ANYBUFFER, NEWBUFFER, OLDBUFFER};

typedef struct {			/* representation of a variable (0-D) */
  char     *name;
  char     *value;
} Variable;

typedef struct {			/* representation of a vector (1-D) */
  char name[OPIHI_NAME_SIZE];
  char type;
  union {
    void      *Ptr;
    char     **Str;
    opihi_flt *Flt;
    opihi_int *Int;
  } elements;
  int Nelements;
} Vector;

typedef struct {			/* representation of buffer (image) */
  char name[OPIHI_NAME_SIZE];
  char file[OPIHI_NAME_SIZE];
  Header header;
  Matrix matrix;
  int  bitpix, unsign;
  double bscale, bzero;
} Buffer;

typedef enum {
  ST_NONE,
  ST_LEFT,
  ST_RIGHT,
  ST_COMMA,
  ST_TRINARY,
  ST_OR,
  ST_AND,
  ST_LOGIC,
  ST_BITWISE,
  ST_ADD,
  ST_TIMES,
  ST_POWER,
  ST_UNARY,
  ST_BINARY,

  ST_VALUE,
  ST_SCALAR_INT,
  ST_SCALAR_FLT,
  ST_VECTOR,
  ST_VECTOR_TMP,
  ST_MATRIX,
  ST_MATRIX_TMP,

  ST_STRING,
  ST_STRING_TMP,
} StackVarType;

typedef struct {			/* math stack structure */
  char   *name;
  StackVarType type;
  Buffer *buffer;
  Vector *vector;
  opihi_flt FltValue;
  opihi_int IntValue;
} StackVar;

/* math functions */
char         *dvomath               PROTO((int argc, char **argv, int *size, int maxsize));
// MOVED to libohana
// char        **isolate_elements      PROTO((int argc, char **argv, int *nstack));
StackVar     *convert_to_RPN        PROTO((int argc, char **argv, int *nstack));
int           check_stack           PROTO((StackVar *stack, int Nstack, int validsize));
int           evaluate_stack        PROTO((StackVar *stack, int *Nstack));
void          init_stack            PROTO((StackVar *stack));
void          copy_stack	    PROTO((StackVar *stack1, StackVar *stack2));
void          move_stack	    PROTO((StackVar *stack1, StackVar *stack2));
void          clean_stack	    PROTO((StackVar *stack, int Nstack));
void          delete_stack	    PROTO((StackVar *stack, int Nstack));
void          clear_stack 	    PROTO((StackVar *stack));
void          assign_stack 	    PROTO((StackVar *stack, char *name, StackVarType type));
char         *clean_stack_name      PROTO((char *name));

int           SSS_trinary           PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, StackVar *V3, char *op));
int           VVV_trinary           PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, StackVar *V3, char *op));
int           MMM_trinary           PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, StackVar *V3, char *op));

int           VV_binary             PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, char *op));
int           SV_binary             PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, char *op));
int           VS_binary             PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, char *op));
int           MV_binary             PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, char *op));
int           VM_binary             PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, char *op));
int           MM_binary             PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, char *op));
int           MS_binary             PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, char *op));
int           SM_binary             PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, char *op));
int           SS_binary             PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, char *op));
int           LW_binary             PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, char *op));
int           WL_binary             PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, char *op));
int           LL_binary             PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, char *op));
int           WW_binary             PROTO((StackVar *OUT, StackVar *V1, StackVar *V2, char *op));
int           S_unary               PROTO((StackVar *OUT, StackVar *V1, char *op));
int           V_unary               PROTO((StackVar *OUT, StackVar *V1, char *op));
int           M_unary               PROTO((StackVar *OUT, StackVar *V1, char *op));
int           W_unary               PROTO((StackVar *OUT, StackVar *V1, char *op));
int           L_unary               PROTO((StackVar *OUT, StackVar *V1, char *op));

/* variable handling */
char         *get_variable          PROTO((char *name));
char         *get_variable_ptr      PROTO((char *name));
char         *get_local_variable_ptr PROTO((char *name));
double        get_double_variable   PROTO((char *name, int *found));
int           get_int_variable      PROTO((char *name, int *found));
int           DeleteNamedScalar     PROTO((char *name));
int           IsScalar              PROTO((char *name));
int           set_variable          PROTO((char *name, double dvalue));
int           set_int_variable      PROTO((char *name, int ivalue));
int           set_str_variable      PROTO((char *name, char *value));
int           set_local_variable    PROTO((char *name, char *value));
void          InitVariables         PROTO((void));
void          ListVariables         PROTO((void));
float         get_variable_default  PROTO((char *name, float dvalue));
int           SelectScalar          PROTO((char *string, double *value));

/* vector handling */
void          InitVectors           PROTO((void));
void          FreeVectors           PROTO((void));
Vector       *InitVector            PROTO((void));
void          FreeVectorArray       PROTO((Vector **vec, int Nvec));
void          FreeVector            PROTO((Vector *vec));
int           CopyVector            PROTO((Vector *out, Vector *in));
int           ResetVector           PROTO((Vector *vec, char type, int Nelements));
int           SetVectorValues       PROTO((Vector *vec, void *data, char type, int Nelements));
int           SetVector             PROTO((Vector *vec, char type, int Nelements));
int           CastVector            PROTO((Vector *vec, char type));
int           MatchVector           PROTO((Vector *out, Vector *in, char type));
int           MoveVector            PROTO((Vector *out, Vector *in));
int           DeleteVector          PROTO((Vector *vec));
int           CopyNamedVector       PROTO((char *out, char *in));
int           MoveNamedVector       PROTO((char *out, char *in));
int           DeleteNamedVector     PROTO((char *name));
int           IsVector              PROTO((char *name));
int           IsVectorPtr           PROTO((Vector *vec));
int           ListVectors           PROTO((void));
int           ListVectorsToList     PROTO((char *name));
Vector       *SelectVector          PROTO((char *name, int mode, int verbose));
int           AssignVector          PROTO((Vector *vec, char *name, int mode, int verbose));
Vector      **MergeVectors          PROTO((Vector **vec, int *Nvec, Vector **invec, int Ninvec));
Vector      **MergeVectorsByIndex   PROTO((Vector **vec, int *Nvec, Vector **invec, int Ninvec, int Nelements));

/* vector IO functions */
int           WriteVectorTableFITS  PROTO((char *filename, char *extname, Header *extraheader, Vector **vec, int Nvec, int append, char *compress, char *format, int Ntile));
Vector      **ReadVectorTableFITS   PROTO((char *filename, char *extname, int *Nvec));

int           VectorAssignData          PROTO((Vector **vec, char *type, void *data, int Nrows, int Nval));
int           VectorAssignDataTranspose PROTO((Vector **vec, char *type, void *data, int Nrows, int Nval));

/* buffer handling */
Buffer       *InitBuffer            PROTO((void));
void          InitBuffers           PROTO((void));
int           CopyBuffer            PROTO((Buffer *out, Buffer *in));
int           MoveBuffer            PROTO((Buffer *out, Buffer *in));
int           DeleteBuffer          PROTO((Buffer *buf));
int           CopyNamedBuffer       PROTO((char *out, char *in));
int           MoveNamedBuffer       PROTO((char *out, char *in));
int           DeleteNamedBuffer     PROTO((char *name));
int           IsBuffer              PROTO((char *name));
int           IsBufferPtr           PROTO((Buffer *buf));
int           PrintBuffers          PROTO((int Long));
int           ListBuffersToList     PROTO((char *name));
int           CreateBuffer          PROTO((Buffer *buf, int Nx, int Ny, int bitpix, float bzero, float bscale));
int           CreateBuffer3D        PROTO((Buffer *buf, int Nx, int Ny, int Nz, int bitpix, float bzero, float bscale));
int           ResetBuffer           PROTO((Buffer *buf, int Nx, int Ny, int bitpix, float bzero, float bscale));
Buffer       *SelectBuffer          PROTO((char *name, int mode, int verbose));
void          dump_buffers          PROTO((int n));  /* deprecated? */
int           SelectOverlay         PROTO((char *name, int *number));

/* why are these in here? */
int           gfits_copy_matrix_info (Matrix *matrix1, Matrix *matrix2);
#ifndef MOVED_TO_LIBDVO
int           GetTimeFormat         PROTO((time_t *TimeReference, int *TimeFormat));
#endif

# endif
