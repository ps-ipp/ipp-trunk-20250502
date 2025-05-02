# include "data.h"

# undef SWAP_BYTE
# undef SWAP_WORD
# undef SWAP_DBLE

# ifdef BYTE_SWAP							
# define SWAP_BYTE(X) {							\
    char tmp; char *tmp_Ptr = (char *) &X;				\
    tmp = tmp_Ptr[0]; tmp_Ptr[0] = tmp_Ptr[1]; tmp_Ptr[1] = tmp; }		
# define SWAP_WORD(X) {							\
    char tmp; char *tmp_Ptr = (char *) &X;				\
    tmp = tmp_Ptr[0]; tmp_Ptr[0] = tmp_Ptr[3]; tmp_Ptr[3] = tmp;	\
    tmp = tmp_Ptr[1]; tmp_Ptr[1] = tmp_Ptr[2]; tmp_Ptr[2] = tmp; } 
# define SWAP_DBLE(X) {							\
    char tmp; char *tmp_Ptr = (char *) &X;				\
    tmp = tmp_Ptr[0]; tmp_Ptr[0] = tmp_Ptr[7]; tmp_Ptr[7] = tmp;	\
    tmp = tmp_Ptr[1]; tmp_Ptr[1] = tmp_Ptr[6]; tmp_Ptr[6] = tmp;	\
    tmp = tmp_Ptr[2]; tmp_Ptr[2] = tmp_Ptr[5]; tmp_Ptr[5] = tmp;	\
    tmp = tmp_Ptr[3]; tmp_Ptr[3] = tmp_Ptr[4]; tmp_Ptr[4] = tmp; }
# else
# define SWAP_BYTE(X)
# define SWAP_WORD(X)
# define SWAP_DBLE(X)
# endif

typedef enum { IDX_NONE, IDX_UBYTE, IDX_SBYTE, IDX_SHORT, IDX_INT, IDX_FLOAT, IDX_DOUBLE} idxType;

int idxread (int argc, char **argv) {

  int magic, Nread, byte_pix;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: idxread (file) (basename)\n");
    gprint (GP_ERR, "  the file may encode vectors, images, or higher dimension arrays.\n");
    gprint (GP_ERR, "  if N(dim) <= 3, the basename will be used for the vector or (2D or 3D) image name.\n");
    gprint (GP_ERR, "  if N(dim) >= 4, the basename will be used to generate 3D image names of the form basename_n_m.\n");
    return FALSE;
  }

  FILE *f = fopen (argv[1], "r");
  if (!f) {
    gprint (GP_ERR, "failed to open file %s\n", argv[1]);
    return (FALSE);
  }
 
  // read the magic int
  Nread = fread (&magic, 4, 1, f); 
  if (!Nread) {
    gprint (GP_ERR, "failed to read magic bytes from file %s\n", argv[1]);
    fclose (f);
    return (FALSE);
  }

  SWAP_WORD (magic);

  char *myBytes = (char *) &magic;
  if (myBytes[3] || myBytes[2]) {
    gprint (GP_ERR, "invalid magic number 0x%08x\n", magic);
    fclose (f);
    return (FALSE);
  }

  idxType type = IDX_NONE;
  switch (myBytes[1]) {
    case 0x08: type = IDX_UBYTE;  byte_pix = 1; break; 
    case 0x09: type = IDX_SBYTE;  byte_pix = 1; break; 
    case 0x0b: type = IDX_SHORT;  byte_pix = 2; break; 
    case 0x0c: type = IDX_INT;    byte_pix = 4; break; 
    case 0x0d: type = IDX_FLOAT;  byte_pix = 4; break; 
    case 0x0e: type = IDX_DOUBLE; byte_pix = 8; break; 
    default:
      gprint (GP_ERR, "invalid magic number 0x%08x (bad type)\n", magic);
      fclose (f);
      return (FALSE);
  }
      
  int Ndim = myBytes[0];
  if ((Ndim < 1) || (Ndim > 9)) {
    gprint (GP_ERR, "invalid magic number 0x%08x (bad Ndim)\n", magic);
    fclose (f);
    return (FALSE);
  }

  int sizes[10];
  
  // read the axis sizes
  Nread = fread (&sizes, 4, Ndim, f); 
  if (Nread != Ndim) {
    gprint (GP_ERR, "failed to read array sizes from file %s\n", argv[1]);
    fclose (f);
    return (FALSE);
  }

  // read the full data array in one go?

  int Nbytes = byte_pix;
  for (int i = 0; i < Ndim; i++) {
    SWAP_WORD (sizes[i]);
    Nbytes *= MAX (1, sizes[i]);
  }
  ALLOCATE_PTR (buffer, char, Nbytes);

  Nread = fread (buffer, 1, Nbytes, f); 
  fclose (f);

  if (Nread != Nbytes) {
    gprint (GP_ERR, "failed to read array sizes from file %s\n", argv[1]);
    return (FALSE);
  }

  // generate a vector
  if (Ndim == 1) {
    Vector *vector;
    if ((vector = SelectVector (argv[2], ANYVECTOR, TRUE)) == NULL) {
      gprint (GP_ERR, "cannot use %s for a vector\n", argv[2]);
      return FALSE;
    }

    // this relies on the enum being ordered above
    if (type < IDX_FLOAT) { 
      ResetVector (vector, OPIHI_INT, sizes[0]); 
    } else {
      ResetVector (vector, OPIHI_FLT, sizes[0]);       
    }

    switch (type) {
      case IDX_UBYTE:  { unsigned char *ptr = (unsigned char *) buffer; for (int i = 0; i < sizes[0]; i++) { 			vector->elements.Int[i] = ptr[i]; } } break;
      case IDX_SBYTE:  {          char *ptr = (         char *) buffer; for (int i = 0; i < sizes[0]; i++) { 			vector->elements.Int[i] = ptr[i]; } } break;
      case IDX_SHORT:  {         short *ptr = (        short *) buffer; for (int i = 0; i < sizes[0]; i++) { SWAP_BYTE(ptr[i]); vector->elements.Int[i] = ptr[i]; } } break;
      case IDX_INT:    {           int *ptr = (          int *) buffer; for (int i = 0; i < sizes[0]; i++) { SWAP_WORD(ptr[i]); vector->elements.Int[i] = ptr[i]; } } break;
      case IDX_FLOAT:  {         float *ptr = (        float *) buffer; for (int i = 0; i < sizes[0]; i++) { SWAP_WORD(ptr[i]); vector->elements.Flt[i] = ptr[i]; } } break;
      case IDX_DOUBLE: {        double *ptr = (       double *) buffer; for (int i = 0; i < sizes[0]; i++) { SWAP_DBLE(ptr[i]); vector->elements.Flt[i] = ptr[i]; } } break;
      default:
	gprint (GP_ERR, "this should not happen\n");
	return FALSE;
    }
  }

  // generate a matrix
  if ((Ndim == 2) || (Ndim == 3)) {
    Buffer *matrix;
    if ((matrix = SelectBuffer (argv[2], ANYBUFFER, TRUE)) == NULL) {
      gprint (GP_ERR, "cannot use %s for a buffer\n", argv[2]);
      return FALSE;
    }

    gfits_free_matrix (&matrix[0].matrix);
    gfits_free_header (&matrix[0].header);
    
    if (Ndim == 3) {
      // dimension order is Nz, Ny, Nx
      if (!CreateBuffer3D (matrix, sizes[2], sizes[1], sizes[0], -32, 1.0, 0.0)) return FALSE;
    } else {
      if (!CreateBuffer (matrix, sizes[1], sizes[0], -32, 1.0, 0.0)) return FALSE;
    }

    float *out = (float *) matrix[0].matrix.buffer;
    int Nvalue = sizes[0]*sizes[1];
    if (Ndim == 3) Nvalue *= sizes[2];

    switch (type) {
      case IDX_UBYTE:  { unsigned char *ptr = (unsigned char *) buffer; for (int i = 0; i < Nvalue; i++) { out[i] = ptr[i]; } } break;
      case IDX_SBYTE:  {          char *ptr = (         char *) buffer; for (int i = 0; i < Nvalue; i++) { out[i] = ptr[i]; } } break;
      case IDX_SHORT:  {         short *ptr = (        short *) buffer; for (int i = 0; i < Nvalue; i++) { out[i] = ptr[i]; } } break;
      case IDX_INT:    {           int *ptr = (          int *) buffer; for (int i = 0; i < Nvalue; i++) { out[i] = ptr[i]; } } break;
      case IDX_FLOAT:  {         float *ptr = (        float *) buffer; for (int i = 0; i < Nvalue; i++) { out[i] = ptr[i]; } } break;
      case IDX_DOUBLE: {        double *ptr = (       double *) buffer; for (int i = 0; i < Nvalue; i++) { out[i] = ptr[i]; } } break;
      default:
	gprint (GP_ERR, "this should not happen\n");
	return FALSE;
    }
  }

  return TRUE;
}
