# include "opihi.h"

/* the result of a matrix operation must go into a temp variable,
   labeled by "m". if one of the input matrices is already a temp variable, we 
   can continue using it this round 
*/

int SSS_trinary (StackVar *OUT, StackVar *V1, StackVar *V2, StackVar *V3, char *op) {

  char line[512]; // this is only used to report an error 
  
  // set up the possible operations : int OP int -> int, all else yield float
  // OP is the operation performed on *M1 and *M2
# define SSS_FUNC(OP) {							\
    if ((V1->type == ST_SCALAR_FLT) && (V2->type == ST_SCALAR_FLT) && (V3->type == ST_SCALAR_FLT)) { \
      opihi_flt M1  =  V1[0].FltValue;					\
      opihi_flt M2  =  V2[0].FltValue;					\
      opihi_flt M3  =  V3[0].FltValue;					\
      OUT[0].type = ST_SCALAR_FLT;					\
      OUT[0].FltValue = OP;						\
      break;								\
    }									\
    if ((V1->type == ST_SCALAR_INT) && (V2->type == ST_SCALAR_INT) && (V3->type == ST_SCALAR_INT)) { \
      opihi_int M1  =  V1[0].IntValue;					\
      opihi_int M2  =  V2[0].IntValue;					\
      opihi_int M3  =  V3[0].IntValue;					\
      OUT[0].type = ST_SCALAR_INT;					\
      OUT[0].IntValue = OP;						\
      break;								\
    }									\
  }

  switch (op[0]) {
    case '?': SSS_FUNC(M1 ? M2: M3);
    default:
      snprintf (line, 512, "error: op %c not defined as scalar trinary op!", op[0]);
      push_error (line);
      return (FALSE);
  }
# undef SSS_FUNC

  clear_stack (V1);
  clear_stack (V2);
  clear_stack (V3);
  return (TRUE);

}

int VVV_trinary (StackVar *OUT, StackVar *V1, StackVar *V2, StackVar *V3, char *op) {

  int i, Nx;
  char line[512]; // this is only used to report an error 
  
  // the vectors have to match in length
  if (V1[0].vector[0].Nelements != V2[0].vector[0].Nelements) {
    return (FALSE);
  }
  if (V1[0].vector[0].Nelements != V3[0].vector[0].Nelements) {
    return (FALSE);
  }

  Nx = V1[0].vector[0].Nelements;

  // create the output vector guaranteed to be temporary until the very end
  OUT[0].vector = InitVector ();
  OUT[0].type = ST_VECTOR_TMP;

  // set up the possible operations : int OP int -> int, all else yield float
  // OP is the operation performed on *M1 and *M2
# define VVV_FUNC(OP) {							\
    if ((V1->vector->type == OPIHI_FLT) && (V2->vector->type == OPIHI_FLT) && (V3->vector->type == OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V1[0].vector);				\
      opihi_flt *M1  =  V1[0].vector[0].elements.Flt;			\
      opihi_flt *M2  =  V2[0].vector[0].elements.Flt;			\
      opihi_flt *M3  =  V3[0].vector[0].elements.Flt;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++, M2++, M3++) {		\
  	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V1->vector->type == OPIHI_FLT) && (V2->vector->type == OPIHI_FLT) && (V3->vector->type == OPIHI_INT)) { \
      CopyVector (OUT[0].vector, V1[0].vector);				\
      opihi_flt *M1  =  V1[0].vector[0].elements.Flt;			\
      opihi_flt *M2  =  V2[0].vector[0].elements.Flt;			\
      opihi_int *M3  =  V3[0].vector[0].elements.Int;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++, M2++, M3++) {		\
  	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V1->vector->type == OPIHI_FLT) && (V2->vector->type == OPIHI_INT) && (V3->vector->type == OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V1[0].vector);				\
      opihi_flt *M1  =  V1[0].vector[0].elements.Flt;			\
      opihi_int *M2  =  V2[0].vector[0].elements.Int;			\
      opihi_flt *M3  =  V3[0].vector[0].elements.Flt;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++, M2++, M3++) {		\
  	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V1->vector->type == OPIHI_FLT) && (V2->vector->type == OPIHI_INT) && (V3->vector->type == OPIHI_INT)) { \
      CopyVector (OUT[0].vector, V2[0].vector);				\
      opihi_flt *M1  =  V1[0].vector[0].elements.Flt;			\
      opihi_int *M2  =  V2[0].vector[0].elements.Int;			\
      opihi_int *M3  =  V3[0].vector[0].elements.Int;			\
      opihi_int *out = OUT[0].vector[0].elements.Int;			\
      for (i = 0; i < Nx; i++, out++, M1++, M2++, M3++) {		\
  	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V1->vector->type == OPIHI_INT) && (V2->vector->type == OPIHI_FLT) && (V3->vector->type == OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V2[0].vector);				\
      opihi_int *M1  =  V1[0].vector[0].elements.Int;			\
      opihi_flt *M2  =  V2[0].vector[0].elements.Flt;			\
      opihi_flt *M3  =  V3[0].vector[0].elements.Flt;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++, M2++, M3++) {		\
  	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V1->vector->type == OPIHI_INT) && (V2->vector->type == OPIHI_FLT) && (V3->vector->type == OPIHI_INT)) { \
      CopyVector (OUT[0].vector, V2[0].vector);				\
      opihi_int *M1  =  V1[0].vector[0].elements.Int;			\
      opihi_flt *M2  =  V2[0].vector[0].elements.Flt;			\
      opihi_int *M3  =  V3[0].vector[0].elements.Int;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++, M2++, M3++) {		\
  	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V1->vector->type == OPIHI_INT) && (V2->vector->type == OPIHI_INT) && (V3->vector->type == OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V3[0].vector);				\
      opihi_int *M1  =  V1[0].vector[0].elements.Int;			\
      opihi_int *M2  =  V2[0].vector[0].elements.Int;			\
      opihi_flt *M3  =  V3[0].vector[0].elements.Flt;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++, M2++, M3++) {		\
  	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V1->vector->type == OPIHI_INT) && (V2->vector->type == OPIHI_INT) && (V3->vector->type == OPIHI_INT)) { \
      CopyVector (OUT[0].vector, V1[0].vector);				\
      opihi_int *M1  =  V1[0].vector[0].elements.Int;			\
      opihi_int *M2  =  V2[0].vector[0].elements.Int;			\
      opihi_int *M3  =  V3[0].vector[0].elements.Int;			\
      opihi_int *out = OUT[0].vector[0].elements.Int;			\
      for (i = 0; i < Nx; i++, out++, M1++, M2++, M3++) {		\
  	*out = OP;							\
      }									\
      break;								\
    }									\
  }

  switch (op[0]) {
    case '?': VVV_FUNC(*M1 ? *M2: *M3);
    default:
      snprintf (line, 512, "error: op %c not defined as vector trinary op!", op[0]);
      push_error (line);
      return (FALSE);
  }
# undef VVV_FUNC

  /** free up any temporary buffers: **/

  if (V1[0].type == ST_VECTOR_TMP) {
    free (V1[0].vector[0].elements.Ptr);
    free (V1[0].vector);
  }
  if (V2[0].type == ST_VECTOR_TMP) {
    free (V2[0].vector[0].elements.Ptr);
    free (V2[0].vector);
  }
  if (V3[0].type == ST_VECTOR_TMP) {
    free (V3[0].vector[0].elements.Ptr);
    free (V3[0].vector);
  }
  /* at the end, V1 and V2 are deleted only if they were temporary */

  clear_stack (V1);
  clear_stack (V2);
  clear_stack (V3);
  return (TRUE);

}

int MMM_trinary (StackVar *OUT, StackVar *V1, StackVar *V2, StackVar *V3, char *op) {

  int i;
  float *out, *M1, *M2, *M3;
  char line[512]; // this is only used to report an error 
  
  int Npix = gfits_npix_matrix (&V1[0].buffer[0].matrix);
  
  if (V1[0].type == ST_MATRIX_TMP) {  /** use V1 as temp buffer **/
    OUT[0].buffer = V1[0].buffer;
    V1[0].type = ST_MATRIX; /* prevent it from being freed below */
  } else {
    if (V2[0].type == ST_MATRIX_TMP) { /** use V2 as temp buffer, but header of V1 **/
      OUT[0].buffer = V2[0].buffer;
      V2[0].type = ST_MATRIX; /* prevent it from being freed below */
    } else {  /* no spare temp buffer */
      OUT[0].buffer = InitBuffer ();
      CopyBuffer (OUT[0].buffer, V1[0].buffer);
    }
  }
  OUT[0].type = ST_MATRIX_TMP; /*** <<--- says this is a temporary matrix ***/

  M1  = (float *)V1[0].buffer[0].matrix.buffer;
  M2  = (float *)V2[0].buffer[0].matrix.buffer;
  M3  = (float *)V3[0].buffer[0].matrix.buffer;
  out = (float *)OUT[0].buffer[0].matrix.buffer;

# define MMM_FUNC(OP)					\
  for (i = 0; i < Npix; i++, out++, M1++, M2++, M3++) {	\
    *out = OP;						\
  }							\
  break; 

  switch (op[0]) {
    case '?': MMM_FUNC(*M1 ? *M2: *M3);
    default:
      snprintf (line, 512, "error: op %c not defined as matrix trinary op!", op[0]);
      push_error (line);
      return (FALSE);
  }
# undef MMM_FUNC

  /** free up any temporary buffers: **/

  if (V1[0].type == ST_MATRIX_TMP) {
    free (V1[0].buffer[0].header.buffer);
    free (V1[0].buffer[0].matrix.buffer);
    free (V1[0].buffer);
  }
  if (V2[0].type == ST_MATRIX_TMP) {
    free (V2[0].buffer[0].header.buffer);
    free (V2[0].buffer[0].matrix.buffer);
    free (V2[0].buffer);
  }
  if (V3[0].type == ST_MATRIX_TMP) {
    free (V3[0].buffer[0].header.buffer);
    free (V3[0].buffer[0].matrix.buffer);
    free (V3[0].buffer);
  }

  /* at the end, V1 and V2 are deleted only if they were temporary */

  clear_stack (V1);
  clear_stack (V2);
  clear_stack (V3);
  return (TRUE);

}

// XXX we temporarily drop the concept of using one of the temporary input vectors for 
// the output vector (thus saving an ALLOC): we have to juggle the size of the input vectors 
// as well as their temporary state

int VV_binary (StackVar *OUT, StackVar *V1, StackVar *V2, char *op) {

  int i, Nx;
  char line[512]; // this is only used to report an error 
  
  // the vectors have to match in length
  if (V1[0].vector[0].Nelements != V2[0].vector[0].Nelements) {
    return (FALSE);
  }

  if ((V1->vector->type == OPIHI_STR) && (V2->vector->type == OPIHI_STR)) {
    int status = LL_binary (OUT, V1, V2, op);
    return status;
  }

  Nx = V1[0].vector[0].Nelements;

  // create the output vector guaranteed to be temporary until the very end
  OUT[0].vector = InitVector ();
  OUT[0].type = ST_VECTOR_TMP;

  // set up the possible operations : int OP int -> int, all else yield float
  // OP is the operation performed on *M1 and *M2
# define VV_FUNC(FTYPE,OP) {						\
    if ((V1->vector->type == OPIHI_FLT) && (V2->vector->type == OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V1[0].vector);				\
      opihi_flt *M1  =  V1[0].vector[0].elements.Flt;			\
      opihi_flt *M2  =  V2[0].vector[0].elements.Flt;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++, M2++) {			\
  	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V1->vector->type == OPIHI_FLT) && (V2->vector->type != OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V1[0].vector);				\
      opihi_flt *M1  =  V1[0].vector[0].elements.Flt;			\
      opihi_int *M2  =  V2[0].vector[0].elements.Int;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++, M2++) {			\
  	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V1->vector->type != OPIHI_FLT) && (V2->vector->type == OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V2[0].vector);				\
      opihi_int *M1  =  V1[0].vector[0].elements.Int;			\
      opihi_flt *M2  =  V2[0].vector[0].elements.Flt;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++, M2++) {			\
  	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((FTYPE == ST_SCALAR_FLT) && (V1->vector->type != OPIHI_FLT) && (V2->vector->type != OPIHI_FLT)) { \
      MatchVector (OUT[0].vector, V1[0].vector, OPIHI_FLT);		\
      opihi_int *M1  =  V1[0].vector[0].elements.Int;			\
      opihi_int *M2  =  V2[0].vector[0].elements.Int;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++, M2++) {			\
	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V1->vector->type != OPIHI_FLT) && (V2->vector->type != OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V1[0].vector);				\
      opihi_int *M1  =  V1[0].vector[0].elements.Int;			\
      opihi_int *M2  =  V2[0].vector[0].elements.Int;			\
      opihi_int *out = OUT[0].vector[0].elements.Int;			\
      for (i = 0; i < Nx; i++, out++, M1++, M2++) {			\
	*out = OP;							\
      }									\
      break;								\
    }									\
  }

  switch (op[0]) {
    case '+': VV_FUNC(ST_SCALAR_INT, *M1 + *M2);
    case '-': VV_FUNC(ST_SCALAR_INT, *M1 - *M2);
    case '*': VV_FUNC(ST_SCALAR_INT, *M1 * *M2);
    case '/': VV_FUNC(ST_SCALAR_FLT, *M1 / (opihi_flt) *M2);
    case '%': VV_FUNC(ST_SCALAR_INT, (long long)*M1 % (long long)*M2);
    case '^': VV_FUNC(ST_SCALAR_FLT, pow (*M1, *M2));
    case '@': VV_FUNC(ST_SCALAR_FLT, DEG_RAD*atan2 (*M1, *M2));
    case 'd': VV_FUNC(ST_SCALAR_FLT, DEG_RAD*atan2 (*M1, *M2));
    case 'a': VV_FUNC(ST_SCALAR_FLT,         atan2 (*M1, *M2));
    case 'D': VV_FUNC(ST_SCALAR_INT, MIN (*M1, *M2));
    case 'U': VV_FUNC(ST_SCALAR_INT, MAX (*M1, *M2));
    case '<': VV_FUNC(ST_SCALAR_INT, (*M1 < *M2) ? 1 : 0);
    case '>': VV_FUNC(ST_SCALAR_INT, (*M1 > *M2) ? 1 : 0);
    case '&': VV_FUNC(ST_SCALAR_INT, ((long long)*M1 & (long long)*M2));
    case '|': VV_FUNC(ST_SCALAR_INT, ((long long)*M1 | (long long)*M2));
    case 'E': VV_FUNC(ST_SCALAR_INT, (*M1 == *M2) ? 1 : 0);
    case 'N': VV_FUNC(ST_SCALAR_INT, (*M1 != *M2) ? 1 : 0);
    case 'L': VV_FUNC(ST_SCALAR_INT, (*M1 <= *M2) ? 1 : 0);
    case 'G': VV_FUNC(ST_SCALAR_INT, (*M1 >= *M2) ? 1 : 0);
    case 'A': VV_FUNC(ST_SCALAR_INT, (*M1 && *M2) ? 1 : 0);
    case 'O': VV_FUNC(ST_SCALAR_INT, (*M1 || *M2) ? 1 : 0);

    // for the bitshift operators, we have to treat the INT and FLT values differently
    // this makes the operator incompatible with the macros used above
    case 'l': {
      CopyVector (OUT[0].vector, V1[0].vector);
      if ((V1->vector->type == OPIHI_FLT) || (V2->vector->type == OPIHI_FLT)) {
	// bitshift is not valid with float valuess
	for (i = 0; i < Nx; i++) {
	  OUT[0].vector[0].elements.Flt[i] = NAN;
	}
	break;
      }
      // I could just do this for all types and bitshift regardless...
      opihi_int *M1  =  V1[0].vector[0].elements.Int;
      opihi_int *M2  =  V2[0].vector[0].elements.Int;
      opihi_int *out = OUT[0].vector[0].elements.Int;
      for (i = 0; i < Nx; i++, out++, M1++, M2++) {
  	*out = *M1 << *M2;
      }
      break;
    }

    case 'r': {
      CopyVector (OUT[0].vector, V1[0].vector);
      if ((V1->vector->type == OPIHI_FLT) || (V2->vector->type == OPIHI_FLT)) {
	// bitshift is not valid with float valuess
	CopyVector (OUT[0].vector, V1[0].vector);
	for (i = 0; i < Nx; i++) {
	  OUT[0].vector[0].elements.Flt[i] = NAN;
	}
	break;
      }
      // I could just do this for all types and bitshift regardless...
      opihi_int *M1  =  V1[0].vector[0].elements.Int;
      opihi_int *M2  =  V2[0].vector[0].elements.Int;
      opihi_int *out = OUT[0].vector[0].elements.Int;
      for (i = 0; i < Nx; i++, out++, M1++, M2++) {
  	*out = *M1 >> *M2;
      }
      break;
    }

    default:
      snprintf (line, 512, "error: op %c not defined for (vector OP vector)!", op[0]);
      push_error (line);
      return (FALSE);
  }
# undef VV_FUNC

  /** free up any temporary buffers: **/

  if (V1[0].type == ST_VECTOR_TMP) {
    free (V1[0].vector[0].elements.Ptr);
    free (V1[0].vector);
  }
  if (V2[0].type == ST_VECTOR_TMP) {
    free (V2[0].vector[0].elements.Ptr);
    free (V2[0].vector);
  }
  /* at the end, V1 and V2 are deleted only if they were temporary */

  clear_stack (V1);
  clear_stack (V2);
  return (TRUE);

}

int SV_binary (StackVar *OUT, StackVar *V1, StackVar *V2, char *op) {

  int i, Nx;
  char line[512]; // this is only used to report an error 
  
  Nx = V2[0].vector[0].Nelements;

  OUT[0].vector = InitVector ();
  OUT[0].type = ST_VECTOR_TMP;   /*** <<--- says this is a temporary matrix ***/

  // set up the possible operations : int OP int -> int, all else yield float
  // OP is the operation performed on *M1 and *M2
# define SV_FUNC(FTYPE,OP) {						\
    if ((V1->type == ST_SCALAR_FLT) && (V2->vector->type == OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V2[0].vector);				\
      opihi_flt  M1  =  V1[0].FltValue;					\
      opihi_flt *M2  =  V2[0].vector[0].elements.Flt;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M2++) {				\
	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V1->type == ST_SCALAR_FLT) && (V2->vector->type != OPIHI_FLT)) { \
      MatchVector (OUT[0].vector, V2[0].vector, OPIHI_FLT);		\
      opihi_flt  M1  =  V1[0].FltValue;					\
      opihi_int *M2  =  V2[0].vector[0].elements.Int;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M2++) {				\
	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V1->type == ST_SCALAR_INT) && (V2->vector->type == OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V2[0].vector);				\
      opihi_int  M1  =  V1[0].IntValue;					\
      opihi_flt *M2  =  V2[0].vector[0].elements.Flt;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M2++) {				\
	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((FTYPE == ST_SCALAR_FLT) && (V1->type == ST_SCALAR_INT) && (V2->vector->type != OPIHI_FLT)) { \
      MatchVector (OUT[0].vector, V2[0].vector, OPIHI_FLT);		\
      opihi_int  M1  =  V1[0].IntValue;					\
      opihi_int *M2  =  V2[0].vector[0].elements.Int;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M2++) {				\
	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V1->type == ST_SCALAR_INT) && (V2->vector->type != OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V2[0].vector);				\
      opihi_int  M1  =  V1[0].IntValue;					\
      opihi_int *M2  =  V2[0].vector[0].elements.Int;			\
      opihi_int *out = OUT[0].vector[0].elements.Int;			\
      for (i = 0; i < Nx; i++, out++, M2++) {				\
	*out = OP;							\
      }									\
      break;								\
    }									\
  }

  switch (op[0]) { 
    case '+': SV_FUNC(ST_SCALAR_INT, M1 + *M2);
    case '-': SV_FUNC(ST_SCALAR_INT, M1 - *M2);
    case '*': SV_FUNC(ST_SCALAR_INT, M1 * *M2);
    case '/': SV_FUNC(ST_SCALAR_FLT, M1 / (opihi_flt) *M2);
    case '%': SV_FUNC(ST_SCALAR_INT, (long long) M1 % (long long) *M2);
    case '^': SV_FUNC(ST_SCALAR_FLT, pow (M1, *M2));
    case '@': SV_FUNC(ST_SCALAR_FLT, DEG_RAD*atan2 (M1, *M2));
    case 'd': SV_FUNC(ST_SCALAR_FLT, DEG_RAD*atan2 (M1, *M2));
    case 'a': SV_FUNC(ST_SCALAR_FLT,         atan2 (M1, *M2));
    case 'D': SV_FUNC(ST_SCALAR_INT, MIN (M1, *M2));
    case 'U': SV_FUNC(ST_SCALAR_INT, MAX (M1, *M2));
    case '<': SV_FUNC(ST_SCALAR_INT, (M1 < *M2) ? 1 : 0);
    case '>': SV_FUNC(ST_SCALAR_INT, (M1 > *M2) ? 1 : 0);
    case '&': SV_FUNC(ST_SCALAR_INT, ((long long)M1 & (long long)*M2));
    case '|': SV_FUNC(ST_SCALAR_INT, ((long long)M1 | (long long)*M2));
    case 'E': SV_FUNC(ST_SCALAR_INT, (M1 == *M2) ? 1 : 0);
    case 'N': SV_FUNC(ST_SCALAR_INT, (M1 != *M2) ? 1 : 0);
    case 'L': SV_FUNC(ST_SCALAR_INT, (M1 <= *M2) ? 1 : 0);
    case 'G': SV_FUNC(ST_SCALAR_INT, (M1 >= *M2) ? 1 : 0);
    case 'A': SV_FUNC(ST_SCALAR_INT, (M1 && *M2) ? 1 : 0);
    case 'O': SV_FUNC(ST_SCALAR_INT, (M1 || *M2) ? 1 : 0);

    // for the bitshift operators, we have to treat the INT and FLT values differently
    // this makes the operator incompatible with the macros used above
    case 'l': {
      CopyVector (OUT[0].vector, V2[0].vector);
      if ((V1->type == ST_SCALAR_FLT) || (V2->vector->type == OPIHI_FLT)) {
	// bitshift is not valid with float valuess
	for (i = 0; i < Nx; i++) {
	  OUT[0].vector[0].elements.Flt[i] = NAN;
	}
	break;
      }
      opihi_int  M1  =  V1[0].IntValue;					\
      opihi_int *M2  =  V2[0].vector[0].elements.Int;
      opihi_int *out = OUT[0].vector[0].elements.Int;
      for (i = 0; i < Nx; i++, out++, M2++) {
  	*out = M1 << *M2;
      }
      break;
    }

    case 'r': {
      CopyVector (OUT[0].vector, V2[0].vector);
      if ((V1->type == ST_SCALAR_FLT) || (V2->vector->type == OPIHI_FLT)) {
	// bitshift is not valid with float valuess
	for (i = 0; i < Nx; i++) {
	  OUT[0].vector[0].elements.Flt[i] = NAN;
	}
	break;
      }
      opihi_int  M1  =  V1[0].IntValue;					\
      opihi_int *M2  =  V2[0].vector[0].elements.Int;
      opihi_int *out = OUT[0].vector[0].elements.Int;
      for (i = 0; i < Nx; i++, out++, M2++) {
  	*out = M1 >> *M2;
      }
      break;
    }

    default:
      snprintf (line, 512, "error: op %c not defined for (scalar OP vector)!", op[0]);
      push_error (line);
      return (FALSE);
  }
# undef SV_FUNC

  /** free up any temporary buffers: **/
  if (V2[0].type == ST_VECTOR_TMP) {
    free (V2[0].vector[0].elements.Ptr);
    free (V2[0].vector);
  }

  clear_stack (V1);
  clear_stack (V2);

  /* at the end, V1 and V2 are deleted only if they were temporary */
  return (TRUE);

}

int VS_binary (StackVar *OUT, StackVar *V1, StackVar *V2, char *op) {

  int i, Nx;
  char line[512]; // this is only used to report an error 
  
  Nx = V1[0].vector[0].Nelements;

  OUT[0].vector = InitVector ();
  OUT[0].type = ST_VECTOR_TMP;   /*** <<--- says this is a temporary matrix ***/

  // set up the possible operations : int OP int -> int, all else yield float
  // OP is the operation performed on *M1 and *M2
# define VS_FUNC(FTYPE,OP) {						\
    if ((V2->type == ST_SCALAR_FLT) && (V1->vector->type == OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V1[0].vector);				\
      opihi_flt *M1  =  V1[0].vector[0].elements.Flt;			\
      opihi_flt  M2  =  V2[0].FltValue;					\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++) {				\
	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V2->type == ST_SCALAR_FLT) && (V1->vector->type != OPIHI_FLT)) { \
      MatchVector (OUT[0].vector, V1[0].vector, OPIHI_FLT);		\
      opihi_int *M1  =  V1[0].vector[0].elements.Int;			\
      opihi_flt  M2  =  V2[0].FltValue;					\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++) {				\
	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V2->type == ST_SCALAR_INT) && (V1->vector->type == OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V1[0].vector);				\
      opihi_flt *M1  =  V1[0].vector[0].elements.Flt;			\
      opihi_int  M2  =  V2[0].IntValue;					\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++) {				\
	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((FTYPE == ST_SCALAR_FLT) && (V2->type == ST_SCALAR_INT) && (V1->vector->type != OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V1[0].vector);				\
      opihi_int *M1  =  V1[0].vector[0].elements.Int;			\
      opihi_int  M2  =  V2[0].IntValue;					\
      opihi_int *out = OUT[0].vector[0].elements.Int;			\
      for (i = 0; i < Nx; i++, out++, M1++) {				\
	*out = OP;							\
      }									\
      break;								\
    }									\
    if ((V2->type == ST_SCALAR_INT) && (V1->vector->type != OPIHI_FLT)) { \
      CopyVector (OUT[0].vector, V1[0].vector);				\
      opihi_int *M1  =  V1[0].vector[0].elements.Int;			\
      opihi_int  M2  =  V2[0].IntValue;					\
      opihi_int *out = OUT[0].vector[0].elements.Int;			\
      for (i = 0; i < Nx; i++, out++, M1++) {				\
	*out = OP;							\
      }									\
      break;								\
    }									\
  }

  switch (op[0]) { 
    case '+': VS_FUNC(ST_SCALAR_INT, *M1 + M2);
    case '-': VS_FUNC(ST_SCALAR_INT, *M1 - M2);
    case '*': VS_FUNC(ST_SCALAR_INT, *M1 * M2);
    case '/': VS_FUNC(ST_SCALAR_FLT, *M1 / (opihi_flt) M2);
    case '%': VS_FUNC(ST_SCALAR_INT, (long long) *M1 % (long long) M2);
    case '^': VS_FUNC(ST_SCALAR_FLT, pow (*M1, M2));
    case '@': VS_FUNC(ST_SCALAR_FLT, DEG_RAD*atan2 (*M1, M2));
    case 'd': VS_FUNC(ST_SCALAR_FLT, DEG_RAD*atan2 (*M1, M2));
    case 'a': VS_FUNC(ST_SCALAR_FLT,         atan2 (*M1, M2));
    case 'D': VS_FUNC(ST_SCALAR_INT, MIN (*M1, M2));
    case 'U': VS_FUNC(ST_SCALAR_INT, MAX (*M1, M2));
    case '<': VS_FUNC(ST_SCALAR_INT, (*M1 < M2) ? 1 : 0);
    case '>': VS_FUNC(ST_SCALAR_INT, (*M1 > M2) ? 1 : 0);
    case '&': VS_FUNC(ST_SCALAR_INT, ((long long)*M1 & (long long)M2));
    case '|': VS_FUNC(ST_SCALAR_INT, ((long long)*M1 | (long long)M2));
    case 'E': VS_FUNC(ST_SCALAR_INT, (*M1 == M2) ? 1 : 0);
    case 'N': VS_FUNC(ST_SCALAR_INT, (*M1 != M2) ? 1 : 0);
    case 'L': VS_FUNC(ST_SCALAR_INT, (*M1 <= M2) ? 1 : 0);
    case 'G': VS_FUNC(ST_SCALAR_INT, (*M1 >= M2) ? 1 : 0);
    case 'A': VS_FUNC(ST_SCALAR_INT, (*M1 && M2) ? 1 : 0);
    case 'O': VS_FUNC(ST_SCALAR_INT, (*M1 || M2) ? 1 : 0);

    case 'l': {
      CopyVector (OUT[0].vector, V1[0].vector);
      if ((V1->vector->type == OPIHI_FLT) || (V2->type == ST_SCALAR_FLT)) {
	// bitshift is not valid with float valuess
	for (i = 0; i < Nx; i++) {
	  OUT[0].vector[0].elements.Flt[i] = NAN;
	}
	break;
      }
      opihi_int *M1  =  V1[0].vector[0].elements.Int;
      opihi_int  M2  =  V2[0].IntValue;					\
      opihi_int *out = OUT[0].vector[0].elements.Int;
      for (i = 0; i < Nx; i++, out++, M1++) {
  	*out = *M1 << M2;
      }
      break;
    }

    case 'r': {
      CopyVector (OUT[0].vector, V1[0].vector);
      if ((V1->vector->type == OPIHI_FLT) || (V2->type == ST_SCALAR_FLT)) {
	// bitshift is not valid with float valuess
	CopyVector (OUT[0].vector, V1[0].vector);
	for (i = 0; i < Nx; i++) {
	  OUT[0].vector[0].elements.Flt[i] = NAN;
	}
	break;
      }
      opihi_int *M1  =  V1[0].vector[0].elements.Int;
      opihi_int  M2  =  V2[0].IntValue;					\
      opihi_int *out = OUT[0].vector[0].elements.Int;
      for (i = 0; i < Nx; i++, out++, M1++) {
  	*out = *M1 >> M2;
      }
      break;
    }

    default:
      snprintf (line, 512, "error: op %c not defined for (vector OP scalar)!", op[0]);
      push_error (line);
      return (FALSE);
  }
# undef VS_FUNC

  /** free up any temporary buffers: **/

  if (V1[0].type == ST_VECTOR_TMP) {
    free (V1[0].vector[0].elements.Ptr);
    free (V1[0].vector);
  }

  clear_stack (V1);
  clear_stack (V2);

  /* at the end, V1 and V2 are deleted only if they were temporary */
  return (TRUE);

}

// the vector is applied to each ROW (currently only valid for 2D matrix), e.g.: M[20,10] * X[20] -> M'[20,10] where M'[2,1] = M[2,1] * X[2]
int MV_binary (StackVar *OUT, StackVar *V1, StackVar *V2, char *op) {

  int i, j, Nx, Ny;
  char line[512]; // this is only used to report an error 
 
  Nx = V1[0].buffer[0].matrix.Naxis[0];
  Ny = V1[0].buffer[0].matrix.Naxis[1];
  if (Nx != V2[0].vector[0].Nelements) {
    snprintf (line, 512, "error: matrix OP vector matrix dimensions do not match: (%d x %d) OP %d", Nx, Ny, V2[0].vector[0].Nelements);
    push_error (line);
    return (FALSE);
  }

  /* if possible, use V1 as temp buffer, otherwise create new one */
  if (V1[0].type == ST_MATRIX_TMP) {  
    OUT[0].buffer = V1[0].buffer;
    V1[0].type = ST_MATRIX; /* prevent it from being freed below */
  } else {  
    /* do buffer.matrix.buffer and buffer.header.buffer get correctly zeroed? */
    OUT[0].buffer = InitBuffer ();
    CopyBuffer (OUT[0].buffer, V1[0].buffer);
  }
  OUT[0].type = ST_MATRIX_TMP; /*** <<--- says this is a temporary matrix ***/

  float     *M1  = (float *) V1[0].buffer[0].matrix.buffer;
  float     *out = (float *)OUT[0].buffer[0].matrix.buffer;

# define MV_FUNC(OP) {					\
    if (V2->vector->type == OPIHI_FLT) {		\
      for (i = 0; i < Ny; i++) {			\
	opihi_flt *M2  =  V2[0].vector[0].elements.Flt;	\
	for (j = 0; j < Nx; j++, out++, M1++, M2++) {	\
	  *out = OP;					\
	}						\
      }							\
      break;						\
    }							\
    if (V2->vector->type != OPIHI_FLT) {		\
      for (i = 0; i < Ny; i++) {			\
	opihi_int *M2  =  V2[0].vector[0].elements.Int;	\
	for (j = 0; j < Nx; j++, out++, M1++, M2++) {	\
	  *out = OP;					\
	}						\
      }							\
      break;						\
    }							\
  }

  switch (op[0]) { 
    case '+': MV_FUNC(*M1 + *M2);
    case '-': MV_FUNC(*M1 - *M2);
    case '*': MV_FUNC(*M1 * *M2);
    case '/': MV_FUNC(*M1 / (opihi_flt) *M2);
    case '%': MV_FUNC((long long) *M1 % (long long) *M2);
    case '^': MV_FUNC(pow (*M1, *M2));
    case '@': MV_FUNC(DEG_RAD*atan2 (*M1, *M2));
    case 'd': MV_FUNC(DEG_RAD*atan2 (*M1, *M2));
    case 'a': MV_FUNC(        atan2 (*M1, *M2));
    case 'D': MV_FUNC(MIN (*M1, *M2));
    case 'U': MV_FUNC(MAX (*M1, *M2));
    case '<': MV_FUNC((*M1 < *M2) ? 1 : 0);
    case '>': MV_FUNC((*M1 > *M2) ? 1 : 0);
    case '&': MV_FUNC(((long long)*M1 & (long long)*M2));
    case '|': MV_FUNC(((long long)*M1 | (long long)*M2));
    case 'E': MV_FUNC((*M1 == *M2) ? 1 : 0);
    case 'N': MV_FUNC((*M1 != *M2) ? 1 : 0);
    case 'L': MV_FUNC((*M1 <= *M2) ? 1 : 0);
    case 'G': MV_FUNC((*M1 >= *M2) ? 1 : 0);
    case 'A': MV_FUNC((*M1 && *M2) ? 1 : 0);
    case 'O': MV_FUNC((*M1 || *M2) ? 1 : 0);

    default:
      snprintf (line, 512, "error: op %c not defined for (matrix OP vector)!", op[0]);
      push_error (line);
      return (FALSE);
  }
# undef MV_FUNC

  /** free up any temporary buffers: **/

  if (V1[0].type == ST_MATRIX_TMP) {
    free (V1[0].buffer[0].header.buffer);
    free (V1[0].buffer[0].matrix.buffer);
    free (V1[0].buffer);
  }
  if (V2[0].type == ST_VECTOR_TMP) {
    free (V2[0].vector[0].elements.Ptr);
    free (V2[0].vector);
  }

  clear_stack (V1);
  clear_stack (V2);

  /* at the end, V1 and V2 are deleted only if they were temporary */
  return (TRUE);
}

// the vector is applied to each COLUMN (currently only valid for 2D matrix)
// e.g.: X[10] * M[20,10] -> M'[20,10] where M'[2,1] = X[1] * M[2,1]
int VM_binary (StackVar *OUT, StackVar *V1, StackVar *V2, char *op) {

  int i, j, Nx, Ny;
  char line[512]; // this is only used to report an error 
  
  Nx = V2[0].buffer[0].matrix.Naxis[0];
  Ny = V2[0].buffer[0].matrix.Naxis[1];
  if (Ny != V1[0].vector[0].Nelements) {
    snprintf (line, 512, "error: vector OP matrix dimensions do not match: %d OP (%d x %d)", V1[0].vector[0].Nelements, Nx, Ny);
    push_error (line);
    return (FALSE);
  }

  /* if possible, use V2 as temp buffer, otherwise create new one */
  if (V2[0].type == ST_MATRIX_TMP) {
    OUT[0].buffer = V2[0].buffer;
    V2[0].type = ST_MATRIX; /* prevent it from being freed below */
  } else {  /* no spare temp buffer */
    OUT[0].buffer = InitBuffer ();
    CopyBuffer (OUT[0].buffer, V2[0].buffer);
  }
  OUT[0].type = ST_MATRIX_TMP; /*** <<--- says this is a temporary matrix ***/

  float     *M2  = (float *) V2[0].buffer[0].matrix.buffer;
  float     *out = (float *)OUT[0].buffer[0].matrix.buffer;

# define VM_FUNC(OP) {					\
    if (V1->vector->type == OPIHI_FLT) {		\
      opihi_flt *M1  = V1[0].vector[0].elements.Flt;	\
      for (i = 0; i < Ny; i++, M1++) {			\
	for (j = 0; j < Nx; j++, out++, M2++) {	\
	  *out = OP;					\
	}						\
      }							\
      break;						\
    }							\
    if (V1->vector->type != OPIHI_FLT) {		\
      opihi_int *M1  =  V1[0].vector[0].elements.Int;	\
      for (i = 0; i < Ny; i++, M1++) {			\
	for (j = 0; j < Nx; j++, out++, M2++) {	\
	  *out = OP;					\
	}						\
      }							\
      break;						\
    }							\
  }

  switch (op[0]) { 
    case '+': VM_FUNC(*M1 + *M2);
    case '-': VM_FUNC(*M1 - *M2);
    case '*': VM_FUNC(*M1 * *M2);
    case '/': VM_FUNC(*M1 / (opihi_flt) *M2);
    case '%': VM_FUNC((long long) *M1 % (long long) *M2);
    case '^': VM_FUNC(pow (*M1, *M2));
    case '@': VM_FUNC(DEG_RAD*atan2 (*M1, *M2));
    case 'd': VM_FUNC(DEG_RAD*atan2 (*M1, *M2));
    case 'a': VM_FUNC(        atan2 (*M1, *M2));
    case 'D': VM_FUNC(MIN (*M1, *M2));
    case 'U': VM_FUNC(MAX (*M1, *M2));
    case '<': VM_FUNC((*M1 < *M2) ? 1 : 0);
    case '>': VM_FUNC((*M1 > *M2) ? 1 : 0);
    case '&': VM_FUNC(((long long)*M1 & (long long)*M2));
    case '|': VM_FUNC(((long long)*M1 | (long long)*M2));
    case 'E': VM_FUNC((*M1 == *M2) ? 1 : 0);
    case 'N': VM_FUNC((*M1 != *M2) ? 1 : 0);
    case 'L': VM_FUNC((*M1 <= *M2) ? 1 : 0);
    case 'G': VM_FUNC((*M1 >= *M2) ? 1 : 0);
    case 'A': VM_FUNC((*M1 && *M2) ? 1 : 0);
    case 'O': VM_FUNC((*M1 || *M2) ? 1 : 0);
    default:
      snprintf (line, 512, "error: op %c not defined for (vector OP matrix)!", op[0]);
      push_error (line);
      return (FALSE);
  }
# undef VM_FUNC

  /** free up any temporary buffers: **/

  if (V1[0].type == ST_VECTOR_TMP) {
    free (V1[0].vector[0].elements.Ptr);
    free (V1[0].vector);
  }
  if (V2[0].type == ST_MATRIX_TMP) {
    free (V2[0].buffer[0].header.buffer);
    free (V2[0].buffer[0].matrix.buffer);
    free (V2[0].buffer);
  }

  clear_stack (V1);
  clear_stack (V2);

  /* at the end, V1 and V2 are deleted only if they were temporary */
  return (TRUE);

}

int MM_binary (StackVar *OUT, StackVar *V1, StackVar *V2, char *op) {

  int i;
  float *out, *M1, *M2;
  char line[512]; // this is only used to report an error 
  
  int Npix = gfits_npix_matrix (&V1[0].buffer[0].matrix);
  
  if (V1[0].type == ST_MATRIX_TMP) {  /** use V1 as temp buffer **/
    OUT[0].buffer = V1[0].buffer;
    V1[0].type = ST_MATRIX; /* prevent it from being freed below */
  } else {
    if (V2[0].type == ST_MATRIX_TMP) { /** use V2 as temp buffer, but header of V1 **/
      OUT[0].buffer = V2[0].buffer;
      V2[0].type = ST_MATRIX; /* prevent it from being freed below */
    } else {  /* no spare temp buffer */
      OUT[0].buffer = InitBuffer ();
      CopyBuffer (OUT[0].buffer, V1[0].buffer);
    }
  }
  OUT[0].type = ST_MATRIX_TMP; /*** <<--- says this is a temporary matrix ***/

  M1  = (float *)V1[0].buffer[0].matrix.buffer;
  M2  = (float *)V2[0].buffer[0].matrix.buffer;
  out = (float *)OUT[0].buffer[0].matrix.buffer;

# define MM_FUNC(OP)					\
  for (i = 0; i < Npix; i++, out++, M1++, M2++) {	\
    *out = OP;						\
  }							\
  break; 

  switch (op[0]) { 
    case '+': MM_FUNC(*M1 + *M2);
    case '-': MM_FUNC(*M1 - *M2);
    case '*': MM_FUNC(*M1 * *M2);
    case '/': MM_FUNC(*M1 / (float) *M2);
    case '%': MM_FUNC((long long) *M1 % (long long) *M2);
    case '^': MM_FUNC(pow (*M1, *M2));
    case '@': MM_FUNC(DEG_RAD*atan2 (*M1, *M2));
    case 'd': MM_FUNC(DEG_RAD*atan2 (*M1, *M2));
    case 'a': MM_FUNC(        atan2 (*M1, *M2));
    case 'D': MM_FUNC(MIN (*M1, *M2));
    case 'U': MM_FUNC(MAX (*M1, *M2));
    case '<': MM_FUNC((*M1 < *M2) ? 1 : 0);
    case '>': MM_FUNC((*M1 > *M2) ? 1 : 0);
    case '&': MM_FUNC(((long long)*M1 & (long long)*M2));
    case '|': MM_FUNC(((long long)*M1 | (long long)*M2));
    case 'E': MM_FUNC((*M1 == *M2) ? 1 : 0);
    case 'N': MM_FUNC((*M1 != *M2) ? 1 : 0);
    case 'L': MM_FUNC((*M1 <= *M2) ? 1 : 0);
    case 'G': MM_FUNC((*M1 >= *M2) ? 1 : 0);
    case 'A': MM_FUNC((*M1 && *M2) ? 1 : 0);
    case 'O': MM_FUNC((*M1 || *M2) ? 1 : 0);
    default:
      snprintf (line, 512, "error: op %c not defined for (matrix OP matrix)!", op[0]);
      push_error (line);
      return (FALSE);
  }
# undef MM_FUNC

  /** free up any temporary buffers: **/

  if (V1[0].type == ST_MATRIX_TMP) {
    free (V1[0].buffer[0].header.buffer);
    free (V1[0].buffer[0].matrix.buffer);
    free (V1[0].buffer);
  }
  if (V2[0].type == ST_MATRIX_TMP) {
    free (V2[0].buffer[0].header.buffer);
    free (V2[0].buffer[0].matrix.buffer);
    free (V2[0].buffer);
  }

  clear_stack (V1);
  clear_stack (V2);

  /* at the end, V1 and V2 are deleted only if they were temporary */
  return (TRUE);
}

int MS_binary (StackVar *OUT, StackVar *V1, StackVar *V2, char *op) {

  int i;
  char line[512]; // this is only used to report an error 
  
  int Npix = gfits_npix_matrix (&V1[0].buffer[0].matrix);

  /* if possible, use V1 as temp buffer, otherwise create new one */
  if (V1[0].type == ST_MATRIX_TMP) {
    OUT[0].buffer = V1[0].buffer;
    V1[0].type = ST_MATRIX; /* prevent it from being freed below */
  } else {
    OUT[0].buffer = InitBuffer ();
    CopyBuffer (OUT[0].buffer, V1[0].buffer);
  }
  OUT[0].type = ST_MATRIX_TMP;      /*** <<--- says this is a temporary matrix ***/

  float *M1    = (float *)V1[0].buffer[0].matrix.buffer;
  float *out   = (float *)OUT[0].buffer[0].matrix.buffer;

# define MS_FUNC(OP) {				\
    if (V2->type == ST_SCALAR_FLT)  {		\
      opihi_flt M2 = V2[0].FltValue;		\
      for (i = 0; i < Npix; i++, out++, M1++) {	\
	*out = OP;				\
      }						\
      break;					\
    }						\
    if (V2->type == ST_SCALAR_INT)  {		\
      opihi_int M2 = V2[0].IntValue;		\
      for (i = 0; i < Npix; i++, out++, M1++) {	\
	*out = OP;				\
      }						\
      break;					\
    }						\
  }

  switch (op[0]) { 
    case '+': MS_FUNC(*M1 + M2);
    case '-': MS_FUNC(*M1 - M2);
    case '*': MS_FUNC(*M1 * M2);
    case '/': MS_FUNC(*M1 / (float) M2);
    case '%': MS_FUNC((long long) *M1 % (long long) M2);
    case '^': MS_FUNC(pow (*M1, M2));
    case '@': MS_FUNC(DEG_RAD*atan2 (*M1, M2));
    case 'd': MS_FUNC(DEG_RAD*atan2 (*M1, M2));
    case 'a': MS_FUNC(        atan2 (*M1, M2));
    case 'D': MS_FUNC(MIN (*M1, M2));
    case 'U': MS_FUNC(MAX (*M1, M2));
    case '<': MS_FUNC((*M1 < M2) ? 1 : 0);
    case '>': MS_FUNC((*M1 > M2) ? 1 : 0);
    case '&': MS_FUNC(((long long)*M1 & (long long)M2));
    case '|': MS_FUNC(((long long)*M1 | (long long)M2));
    case 'E': MS_FUNC((*M1 == M2) ? 1 : 0);
    case 'N': MS_FUNC((*M1 != M2) ? 1 : 0);
    case 'L': MS_FUNC((*M1 <= M2) ? 1 : 0);
    case 'G': MS_FUNC((*M1 >= M2) ? 1 : 0);
    case 'A': MS_FUNC((*M1 && M2) ? 1 : 0);
    case 'O': MS_FUNC((*M1 || M2) ? 1 : 0);
    default:
      snprintf (line, 512, "error: op %c not defined for (matrix OP scalar)!", op[0]);
      push_error (line);
      return (FALSE);
  }
# undef MS_FUNC

  if (V1[0].type == ST_MATRIX_TMP) {
    free (V1[0].buffer[0].header.buffer);
    free (V1[0].buffer[0].matrix.buffer);
    free (V1[0].buffer);
  }
  clear_stack (V1);
  clear_stack (V2);

  return (TRUE);
}


int SM_binary (StackVar *OUT, StackVar *V1, StackVar *V2, char *op) {

  int i;
  char line[512]; // this is only used to report an error 
  
  int Npix = gfits_npix_matrix (&V2[0].buffer[0].matrix);
  
  if (V2[0].type == ST_MATRIX_TMP) {  /* V2[0] is NOT temporary, we can't use it for storage */
    OUT[0].buffer = V2[0].buffer;
    V2[0].type = ST_MATRIX; /* prevent it from being freed below */
  } else {
    OUT[0].buffer = InitBuffer ();
    CopyBuffer (OUT[0].buffer, V2[0].buffer);
  }
  OUT[0].type = ST_MATRIX_TMP; /*** <<--- says this is a temporary matrix ***/

  float *M2    = (float *)V2[0].buffer[0].matrix.buffer;
  float *out   = (float *)OUT[0].buffer[0].matrix.buffer;

# define SM_FUNC(OP) {				\
    if (V1->type == ST_SCALAR_FLT)  {		\
      opihi_flt M1 = V1[0].FltValue;		\
      for (i = 0; i < Npix; i++, out++, M2++) {	\
	*out = OP;				\
      }						\
      break;					\
    }						\
    if (V1->type == ST_SCALAR_INT)  {		\
      opihi_int M1 = V1[0].IntValue;		\
      for (i = 0; i < Npix; i++, out++, M2++) {	\
	*out = OP;				\
      }						\
      break;					\
    }						\
  }

  switch (op[0]) { 
    case '+': SM_FUNC(M1 + *M2);
    case '-': SM_FUNC(M1 - *M2);
    case '*': SM_FUNC(M1 * *M2);
    case '/': SM_FUNC(M1 / (float) *M2);
    case '%': SM_FUNC((long long) M1 % (long long) *M2);
    case '^': SM_FUNC(pow (M1, *M2));
    case '@': SM_FUNC(DEG_RAD*atan2 (M1, *M2));
    case 'd': SM_FUNC(DEG_RAD*atan2 (M1, *M2));
    case 'a': SM_FUNC(        atan2 (M1, *M2));
    case 'D': SM_FUNC(MIN (M1, *M2));
    case 'U': SM_FUNC(MAX (M1, *M2));
    case '<': SM_FUNC((M1 < *M2) ? 1 : 0);
    case '>': SM_FUNC((M1 > *M2) ? 1 : 0);
    case '&': SM_FUNC(((long long)M1 & (long long)*M2));
    case '|': SM_FUNC(((long long)M1 | (long long)*M2));
    case 'E': SM_FUNC((M1 == *M2) ? 1 : 0);
    case 'N': SM_FUNC((M1 != *M2) ? 1 : 0);
    case 'L': SM_FUNC((M1 <= *M2) ? 1 : 0);
    case 'G': SM_FUNC((M1 >= *M2) ? 1 : 0);
    case 'A': SM_FUNC((M1 && *M2) ? 1 : 0);
    case 'O': SM_FUNC((M1 || *M2) ? 1 : 0);
    default:
      snprintf (line, 512, "error: op %c not defined for (scalar OP matrix)!", op[0]);
      push_error (line);
      return (FALSE);
  }
# undef SM_FUNC

  if (V2[0].type == ST_MATRIX_TMP) {
    free (V2[0].buffer[0].header.buffer);
    free (V2[0].buffer[0].matrix.buffer);
    free (V2[0].buffer);
  }
  clear_stack (V1);
  clear_stack (V2);

  return (TRUE);

}

int SS_binary (StackVar *OUT, StackVar *V1, StackVar *V2, char *op) {

  char line[512]; // this is only used to report an error 

# define SS_FUNC(FTYPE,OP) {						\
    if ((V1->type == ST_SCALAR_FLT) && (V2->type == ST_SCALAR_FLT)) {	\
      opihi_flt M1 = V1[0].FltValue;					\
      opihi_flt M2 = V2[0].FltValue;					\
      OUT[0].type = ST_SCALAR_FLT;					\
      OUT[0].FltValue = OP;						\
      break;								\
    }									\
    if ((V1->type == ST_SCALAR_FLT) && (V2->type == ST_SCALAR_INT)) {	\
      opihi_flt M1 = V1[0].FltValue;					\
      opihi_int M2 = V2[0].IntValue;					\
      OUT[0].type = ST_SCALAR_FLT;					\
      OUT[0].FltValue = OP;						\
      break;								\
    }									\
    if ((V1->type == ST_SCALAR_INT) && (V2->type == ST_SCALAR_FLT)) {	\
      opihi_int M1 = V1[0].IntValue;					\
      opihi_flt M2 = V2[0].FltValue;					\
      OUT[0].type = ST_SCALAR_FLT;					\
      OUT[0].FltValue = OP;						\
      break;								\
    }									\
    if ((FTYPE == ST_SCALAR_FLT) && (V1->type == ST_SCALAR_INT) && (V2->type == ST_SCALAR_INT)) { \
      opihi_int M1 = V1[0].IntValue;					\
      opihi_int M2 = V2[0].IntValue;					\
      OUT[0].type = ST_SCALAR_FLT;					\
      OUT[0].FltValue = OP;						\
      break;								\
    }									\
    if ((V1->type == ST_SCALAR_INT) && (V2->type == ST_SCALAR_INT)) {	\
      opihi_int M1 = V1[0].IntValue;					\
      opihi_int M2 = V2[0].IntValue;					\
      OUT[0].type = ST_SCALAR_INT;					\
      OUT[0].IntValue = OP;						\
      break;								\
    }									\
  }

  switch (op[0]) { 
    case '+': SS_FUNC(ST_SCALAR_INT, M1 + M2);
    case '-': SS_FUNC(ST_SCALAR_INT, M1 - M2);
    case '*': SS_FUNC(ST_SCALAR_INT, M1 * M2);
    case '/': SS_FUNC(ST_SCALAR_FLT, M1 / (opihi_flt) M2);
    case '%': SS_FUNC(ST_SCALAR_INT, (long long) M1 % (long long) M2);
    case '^': SS_FUNC(ST_SCALAR_FLT, pow (M1, M2));
    case '@': SS_FUNC(ST_SCALAR_FLT, DEG_RAD*atan2 (M1, M2));
    case 'd': SS_FUNC(ST_SCALAR_FLT, DEG_RAD*atan2 (M1, M2));
    case 'a': SS_FUNC(ST_SCALAR_FLT,         atan2 (M1, M2));
    case 'D': SS_FUNC(ST_SCALAR_INT, MIN (M1, M2));
    case 'U': SS_FUNC(ST_SCALAR_INT, MAX (M1, M2));
    case '<': SS_FUNC(ST_SCALAR_INT, (M1 < M2) ? 1 : 0);
    case '>': SS_FUNC(ST_SCALAR_INT, (M1 > M2) ? 1 : 0);
    case '&': SS_FUNC(ST_SCALAR_INT, ((long long)M1 & (long long)M2));
    case '|': SS_FUNC(ST_SCALAR_INT, ((long long)M1 | (long long)M2));
    case 'E': SS_FUNC(ST_SCALAR_INT, (M1 == M2) ? 1 : 0);
    case 'N': SS_FUNC(ST_SCALAR_INT, (M1 != M2) ? 1 : 0);
    case 'L': SS_FUNC(ST_SCALAR_INT, (M1 <= M2) ? 1 : 0);
    case 'G': SS_FUNC(ST_SCALAR_INT, (M1 >= M2) ? 1 : 0);
    case 'A': SS_FUNC(ST_SCALAR_INT, (M1 && M2) ? 1 : 0);
    case 'O': SS_FUNC(ST_SCALAR_INT, (M1 || M2) ? 1 : 0);

    // for the bitshift operators, we have to treat the INT and FLT values differently
    // this makes the operator incompatible with the macros used above
    case 'l': {
      if ((V1->type == ST_SCALAR_FLT) || (V2->type == ST_SCALAR_FLT)) {
	// bitshift is not valid with float valuess
	OUT[0].type = ST_SCALAR_FLT;
	OUT[0].FltValue = NAN;
	break;
      }
      opihi_int M1 = V1[0].IntValue;
      opihi_int M2 = V2[0].IntValue;
      OUT[0].type = ST_SCALAR_INT;
      OUT[0].IntValue = M1 << M2;
      break;
    }

    case 'r': {
      if ((V1->type == ST_SCALAR_FLT) || (V2->type == ST_SCALAR_FLT)) {
	// bitshift is not valid with float valuess
	OUT[0].type = ST_SCALAR_FLT;
	OUT[0].FltValue = NAN;
	break;
      }
      opihi_int M1 = V1[0].IntValue;
      opihi_int M2 = V2[0].IntValue;
      OUT[0].type = ST_SCALAR_INT;
      OUT[0].IntValue = M1 >> M2;
      break;
    }

    default:
      snprintf (line, 512, "error: op %c not defined for (scalar OP scalar)!", op[0]);
      push_error (line);
      return (FALSE);
  }
# undef SS_FUNC

  clear_stack (V1);
  clear_stack (V2);

  return (TRUE);

}

int WW_binary (StackVar *OUT, StackVar *V1, StackVar *V2, char *op) {

  int value;
  char line[512]; // this is only used to report an error 

  /* only 'N' and 'E' are allowed for WW_binary operations. anything else is either a
     syntax error or is a string which looks like a math expression. */

  if ((op[0] != 'N') && (op[0] != 'E')) {
    snprintf (line, 512, "error: op %c not defined for string operations!", op[0]);
    push_error (line);
    return (FALSE);
  }

  /* evaluate stack will only call WW_binary with one numerical value,
     and only in the case that the string did not parse to a number 
     thus: string == number -> false */

  if (V1[0].type == ST_SCALAR_INT) {
    value = (op[0] == 'N');
    goto escape;
  }
  if (V1[0].type == ST_SCALAR_FLT) {
    value = (op[0] == 'N');
    goto escape;
  }
  if (V2[0].type == ST_SCALAR_INT) {
    value = (op[0] == 'N');
    goto escape;
  }
  if (V2[0].type == ST_SCALAR_FLT) {
    value = (op[0] == 'N');
    goto escape;
  }

  switch (op[0]) { 
    case 'E': 
      value = strcmp (V1[0].name, V2[0].name) ? 0 : 1;
      break; 
    case 'N': 
      value = strcmp (V1[0].name, V2[0].name) ? 1 : 0;
      break; 
    default:
      snprintf (line, 512, "error: op %c not defined for string operations!", op[0]);
      push_error (line);
      return (FALSE);
  }

escape:
  OUT[0].FltValue = value;
  OUT[0].type = ST_SCALAR_FLT;

  clear_stack (V1);
  clear_stack (V2);
  return (TRUE);

}

// vector string OP scalar string (called if V1 is vector, V2 is string)
// V1 MUST be a string-valued vector
int LW_binary (StackVar *OUT, StackVar *V1, StackVar *V2, char *op) {

  char line[512]; // this is only used to report an error 

  if (V1->vector->type != OPIHI_STR) {
    snprintf (line, 512, "error: binary operation between numerical vector and string not defined");
    push_error (line);
    return (FALSE);
  }

  /* only 'N' and 'E' are allowed for WW_binary operations. anything else is either a
     syntax error or is a string which looks like a math expression. */

  if ((op[0] != 'N') && (op[0] != 'E')) {
    snprintf (line, 512, "error: op %c not defined for vector string operations!", op[0]);
    push_error (line);
    return (FALSE);
  }

  int Nx = V1[0].vector[0].Nelements;

  OUT[0].vector = InitVector ();
  OUT[0].type = ST_VECTOR_TMP;   /*** <<--- says this is a temporary matrix ***/

  /* evaluate stack will only call WW_binary with one numerical value,
     and only in the case that the string did not parse to a number 
     thus: string == number -> false */

  // output vector is an integer (result of strcmp test)
  MatchVector (OUT[0].vector, V1[0].vector, OPIHI_INT);
  opihi_int *out = OUT[0].vector[0].elements.Int;

  // remove the quotes around the name (string value) here
  char *M2 = clean_stack_name(V2[0].name);

  for (int i = 0; i < Nx; i++, out++) {
    char *M1 =  V1[0].vector[0].elements.Str[i];
    if (op[0] == 'E') {
      //     *out = (M1 == NULL) || strcmp(M1, M2) ? 0 : 1;
      *out = strcmp(M1, M2) ? 0 : 1;
    }
    if (op[0] == 'N') {
      *out = strcmp(M1, M2) ? 1 : 0;
    }
  }

  clear_stack (V1);
  clear_stack (V2);
  FREE (M2);

  /* at the end, V1 and V2 are deleted only if they were temporary */
  return (TRUE);
}

// vector string OP scalar string (called if V2 is vector, V1 is string)
// V2 MUST be a string-valued vector
int WL_binary (StackVar *OUT, StackVar *V1, StackVar *V2, char *op) {

  char line[512]; // this is only used to report an error 

  if (V2->vector->type != OPIHI_STR) {
    snprintf (line, 512, "error: binary operation between numerical vector and string not defined");
    push_error (line);
    return (FALSE);
  }

  /* only 'N' and 'E' are allowed for WW_binary operations. anything else is either a
     syntax error or is a string which looks like a math expression. */

  if ((op[0] != 'N') && (op[0] != 'E')) {
    snprintf (line, 512, "error: op %c not defined for vector string operations!", op[0]);
    push_error (line);
    return (FALSE);
  }

  int Nx = V2[0].vector[0].Nelements;

  OUT[0].vector = InitVector ();
  OUT[0].type = ST_VECTOR_TMP;   /*** <<--- says this is a temporary matrix ***/

  /* evaluate stack will only call WW_binary with one numerical value,
     and only in the case that the string did not parse to a number 
     thus: string == number -> false */

  // output vector is an integer (result of strcmp test)
  MatchVector (OUT[0].vector, V2[0].vector, OPIHI_INT);
  opihi_int *out = OUT[0].vector[0].elements.Int;

  // remove the quotes around the name (string value) here
  char *M1 = clean_stack_name(V1[0].name);

  for (int i = 0; i < Nx; i++, out++) {
    char *M2 =  V2[0].vector[0].elements.Str[i];
    if (op[0] == 'E') {
      *out = strcmp(M1, M2) ? 0 : 1;
    }
    if (op[0] == 'N') {
      *out = strcmp(M1, M2) ? 1 : 0;
    }
  }

  clear_stack (V1);
  clear_stack (V2);
  FREE (M1);

  /* at the end, V1 and V2 are deleted only if they were temporary */
  return (TRUE);
}

// vector string OP vector string (called by VV_binary above if both V1 & V2 are vector strings)
int LL_binary (StackVar *OUT, StackVar *V1, StackVar *V2, char *op) {

  char line[512]; // this is only used to report an error 

  if ((V1->vector->type != OPIHI_STR) || (V2->vector->type != OPIHI_STR)) {
    snprintf (line, 512, "error: binary operations between numerical vector and string vector not defined");
    push_error (line);
    return (FALSE);
  }

  /* only 'N' and 'E' are allowed for WW_binary operations. anything else is either a
     syntax error or is a string which looks like a math expression. */

  if ((op[0] != 'N') && (op[0] != 'E')) {
    snprintf (line, 512, "error: op %c not defined for vector string operations!", op[0]);
    push_error (line);
    return (FALSE);
  }

  int Nx = V1[0].vector[0].Nelements;

  OUT[0].vector = InitVector ();
  OUT[0].type = ST_VECTOR_TMP;   /*** <<--- says this is a temporary matrix ***/

  // output vector is an integer (result of strcmp test)
  MatchVector (OUT[0].vector, V1[0].vector, OPIHI_INT);
  opihi_int *out = OUT[0].vector[0].elements.Int;

  char **M1 =  V1[0].vector[0].elements.Str;
  char **M2 =  V2[0].vector[0].elements.Str;

  for (int i = 0; i < Nx; i++, out++, M1++, M2++) {
    if (op[0] == 'E') {
      *out = strcmp(*M1, *M2) ? 0 : 1;
    }
    if (op[0] == 'N') {
      *out = strcmp(*M1, *M2) ? 1 : 0;
    }
  }

  clear_stack (V1);
  clear_stack (V2);

  /* at the end, V1 and V2 are deleted only if they were temporary */
  return (TRUE);
}

int S_unary (StackVar *OUT, StackVar *V1, char *op) {

# define S_FUNC(OP,FTYPE) {						\
    if (V1->type == ST_SCALAR_FLT) {					\
      opihi_flt M1  = V1[0].FltValue;					\
      OUT[0].type = ST_SCALAR_FLT;					\
      OUT[0].FltValue = OP;						\
      clear_stack (V1);							\
      return (TRUE);							\
    }									\
    if ((FTYPE == ST_SCALAR_FLT) && (V1->type == ST_SCALAR_INT)) {	\
      opihi_int M1  = V1[0].IntValue;					\
      OUT[0].type = ST_SCALAR_FLT;					\
      OUT[0].FltValue = OP;						\
      clear_stack (V1);							\
      return (TRUE);							\
    }									\
    if ((FTYPE == ST_SCALAR_INT) && (V1->type == ST_SCALAR_INT)) {	\
      opihi_int M1  = V1[0].IntValue;					\
      OUT[0].type = ST_SCALAR_INT;					\
      OUT[0].IntValue = OP;						\
      clear_stack (V1);							\
      return (TRUE);							\
    }									\
  }

# define W_FUNC(OP,FTYPE) {						\
    if (V1->type == ST_SCALAR_FLT) {					\
      OUT[0].type = ST_SCALAR_FLT;					\
      OUT[0].FltValue = OP;						\
      clear_stack (V1);							\
      return (TRUE);							\
    }									\
    if ((FTYPE == ST_SCALAR_FLT) && (V1->type == ST_SCALAR_INT)) {	\
      OUT[0].type = ST_SCALAR_FLT;					\
      OUT[0].FltValue = OP;						\
      clear_stack (V1);							\
      return (TRUE);							\
    }									\
    if ((FTYPE == ST_SCALAR_INT) && (V1->type == ST_SCALAR_INT)) {	\
      OUT[0].type = ST_SCALAR_INT;					\
      OUT[0].IntValue = OP;						\
      clear_stack (V1);							\
      return (TRUE);							\
    }									\
  }

  if (!strcmp (op, "="))      S_FUNC(M1, ST_SCALAR_INT);
  if (!strcmp (op, "abs"))    S_FUNC(fabs(M1), ST_SCALAR_INT);
  if (!strcmp (op, "int"))    S_FUNC((long long)(M1), ST_SCALAR_INT);
  if (!strcmp (op, "floor"))  S_FUNC(floor (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "ceil"))   S_FUNC(ceil (M1), ST_SCALAR_FLT);
  // if (!strcmp (op, "rint"))   S_FUNC(nearbyint (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "exp"))    S_FUNC(exp (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "ten"))    S_FUNC(pow (10.0,M1), ST_SCALAR_FLT);
  if (!strcmp (op, "log"))    S_FUNC(log10 (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "ln"))     S_FUNC(log (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "sqrt"))   S_FUNC(sqrt (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "erf"))    S_FUNC(erf (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "sinh"))   S_FUNC(sinh (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "cosh"))   S_FUNC(cosh (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "asinh"))  S_FUNC(asinh (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "acosh"))  S_FUNC(acosh (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "sin"))    S_FUNC(sin (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "cos"))    S_FUNC(cos (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "tan"))    S_FUNC(tan (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "dsin"))   S_FUNC(sin (M1*RAD_DEG), ST_SCALAR_FLT);
  if (!strcmp (op, "dcos"))   S_FUNC(cos (M1*RAD_DEG), ST_SCALAR_FLT);
  if (!strcmp (op, "dtan"))   S_FUNC(tan (M1*RAD_DEG), ST_SCALAR_FLT);
  if (!strcmp (op, "asin"))   S_FUNC(asin (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "acos"))   S_FUNC(acos (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "atan"))   S_FUNC(atan (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "dasin"))  S_FUNC(asin (M1)*DEG_RAD, ST_SCALAR_FLT);
  if (!strcmp (op, "dacos"))  S_FUNC(acos (M1)*DEG_RAD, ST_SCALAR_FLT);
  if (!strcmp (op, "datan"))  S_FUNC(atan (M1)*DEG_RAD, ST_SCALAR_FLT);
  if (!strcmp (op, "lgamma")) S_FUNC(lgamma (M1), ST_SCALAR_FLT);
  if (!strcmp (op, "not"))    S_FUNC(!(M1), ST_SCALAR_INT);
  if (!strcmp (op, "--"))     S_FUNC(-1*M1, ST_SCALAR_INT); // NOTE: opihi_int is signed, 
  if (!strcmp (op, "isinf"))  S_FUNC(!finite(M1), ST_SCALAR_FLT); // XXX modify in future 
  if (!strcmp (op, "isnan"))  S_FUNC(isnan((opihi_flt)(M1)), ST_SCALAR_FLT); // XXX modify in future   

  // these ops do not use the V1 value (W_FUNC does define a temp variable M1)
  if (!strcmp (op, "rnd"))    W_FUNC(drand48(), ST_SCALAR_FLT);
  if (!strcmp (op, "drnd"))   W_FUNC(drand48(), ST_SCALAR_FLT);
  if (!strcmp (op, "lrnd"))   W_FUNC(lrand48(), ST_SCALAR_INT);
  if (!strcmp (op, "mrnd"))   W_FUNC(mrand48(), ST_SCALAR_INT);

  // these are also valid for string values
  if (!strcmp (op, "isword")) W_FUNC(FALSE, ST_SCALAR_INT);
  if (!strcmp (op, "isnum"))  W_FUNC(TRUE,  ST_SCALAR_INT);
  if (!strcmp (op, "isint"))  W_FUNC((V1->type == ST_SCALAR_INT), ST_SCALAR_INT);
  if (!strcmp (op, "isflt"))  W_FUNC((V1->type == ST_SCALAR_FLT), ST_SCALAR_INT);

# undef S_FUNC
# undef W_FUNC

  clear_stack (V1);

  char line[512]; // this is only used to report an error 
  snprintf (line, 512, "error: op %s not defined as unary scalar op for a numerical value!", op);
  push_error (line);
  return (FALSE);
}

int W_unary (StackVar *OUT, StackVar *V1, char *op) {

# define W_FUNC(VALUE) {						\
    OUT[0].type = ST_SCALAR_INT;					\
    OUT[0].IntValue = VALUE;						\
    clear_stack (V1);							\
    return (TRUE);							\
  }

  // string unary functions (all result in int output values)
  if (!strcmp (op, "isword")) W_FUNC(TRUE);
  if (!strcmp (op, "isnum"))  W_FUNC(FALSE);
  if (!strcmp (op, "isint"))  W_FUNC(FALSE);
  if (!strcmp (op, "isflt"))  W_FUNC(FALSE);
  if (!strcmp (op, "length")) W_FUNC(strlen(V1[0].name));

# undef W_FUNC

  clear_stack (V1);

  char line[512]; // this is only used to report an error 
  snprintf (line, 512, "error: op %s not defined as unary scalar op for a string value!", op);
  push_error (line);
  return (FALSE);
}

int V_unary (StackVar *OUT, StackVar *V1, char *op) {

  int i, Nx;
  
  Nx = V1[0].vector[0].Nelements;

  OUT[0].vector = InitVector ();
  OUT[0].type = ST_VECTOR_TMP; /*** <<--- says this is a temporary matrix ***/

  if (V1->vector->type == OPIHI_STR) {
    ResetVector (OUT->vector, V1->vector->type, V1->vector->Nelements);
    for (i = 0; i < V1->vector->Nelements; i++) {
      OUT->vector->elements.Str[i] = strcreate (V1->vector->elements.Str[i]);
    }
    goto escape;
  }

# define V_FUNC(OP,FTYPE) {						\
    if (V1->vector->type == OPIHI_FLT) {				\
      CopyVector (OUT[0].vector, V1[0].vector);				\
      opihi_flt *M1  = V1[0].vector[0].elements.Flt;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++) {				\
	*out = OP;							\
      }									\
      goto escape;							\
    }									\
    if ((V1->vector->type == OPIHI_INT) && (FTYPE == ST_SCALAR_FLT)) {	\
      MatchVector (OUT[0].vector, V1[0].vector, OPIHI_FLT);		\
      opihi_int *M1  = V1[0].vector[0].elements.Int;			\
      opihi_flt *out = OUT[0].vector[0].elements.Flt;			\
      for (i = 0; i < Nx; i++, out++, M1++) {				\
	*out = OP;							\
      }									\
      goto escape;							\
    }									\
    if ((V1->vector->type == OPIHI_INT) && (FTYPE == ST_SCALAR_INT)) {	\
      CopyVector (OUT[0].vector, V1[0].vector);				\
      opihi_int *M1  = V1[0].vector[0].elements.Int;			\
      opihi_int *out = OUT[0].vector[0].elements.Int;			\
      for (i = 0; i < Nx; i++, out++, M1++) {				\
	*out = OP;							\
      }									\
      goto escape;							\
    } }							

  if (!strcmp (op, "="))      V_FUNC(*M1, ST_SCALAR_INT);
  if (!strcmp (op, "abs"))    V_FUNC(fabs(*M1), ST_SCALAR_INT);
  if (!strcmp (op, "int"))    V_FUNC((long long)(*M1), ST_SCALAR_INT);
  if (!strcmp (op, "floor"))  V_FUNC(floor (*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "ceil"))   V_FUNC(ceil (*M1), ST_SCALAR_FLT);
  // if (!strcmp (op, "rint"))   V_FUNC(nearbyint (*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "exp"))    V_FUNC(exp(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "ten"))    V_FUNC(pow(10.0,*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "log"))    V_FUNC(log10(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "ln"))     V_FUNC(log(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "sqrt"))   V_FUNC(sqrt(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "erf"))    V_FUNC(erf(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "sinh"))   V_FUNC(sinh(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "cosh"))   V_FUNC(cosh(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "asinh"))  V_FUNC(asinh(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "acosh"))  V_FUNC(acosh(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "sin"))    V_FUNC(sin(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "cos"))    V_FUNC(cos(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "tan"))    V_FUNC(tan(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "dsin"))   V_FUNC(sin(*M1*RAD_DEG), ST_SCALAR_FLT);
  if (!strcmp (op, "dcos"))   V_FUNC(cos(*M1*RAD_DEG), ST_SCALAR_FLT);
  if (!strcmp (op, "dtan"))   V_FUNC(tan(*M1*RAD_DEG), ST_SCALAR_FLT);
  if (!strcmp (op, "asin"))   V_FUNC(asin(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "acos"))   V_FUNC(acos(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "atan"))   V_FUNC(atan(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "dasin"))  V_FUNC(asin(*M1)*DEG_RAD, ST_SCALAR_FLT);
  if (!strcmp (op, "dacos"))  V_FUNC(acos(*M1)*DEG_RAD, ST_SCALAR_FLT);
  if (!strcmp (op, "datan"))  V_FUNC(atan(*M1)*DEG_RAD, ST_SCALAR_FLT);
  if (!strcmp (op, "lgamma")) V_FUNC(lgamma(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "rnd"))    V_FUNC(drand48(), ST_SCALAR_FLT);
  if (!strcmp (op, "drnd"))   V_FUNC(drand48(), ST_SCALAR_FLT);
  if (!strcmp (op, "lrnd"))   V_FUNC(lrand48(), ST_SCALAR_INT);
  if (!strcmp (op, "mrnd"))   V_FUNC(mrand48(), ST_SCALAR_INT);
  if (!strcmp (op, "not"))    V_FUNC(!(*M1), ST_SCALAR_INT);
  if (!strcmp (op, "--"))     V_FUNC(-1*(*M1), ST_SCALAR_INT); // NOTE: opihi_int is signed
  if (!strcmp (op, "isinf"))  V_FUNC(!finite(*M1), ST_SCALAR_FLT);
  if (!strcmp (op, "isnan"))  V_FUNC(isnan((opihi_flt)(*M1)), ST_SCALAR_FLT);
  if (!strcmp (op, "ramp"))   V_FUNC(i, ST_SCALAR_INT);
  if (!strcmp (op, "xramp"))  V_FUNC(i, ST_SCALAR_INT);
  if (!strcmp (op, "yramp"))  V_FUNC(0, ST_SCALAR_INT);
  if (!strcmp (op, "zramp"))  V_FUNC(0, ST_SCALAR_INT);
  if (!strcmp (op, "zero"))   V_FUNC(0, ST_SCALAR_INT);
  /* xramp, yramp, zramp above only make sense for matrices. for vectors, xramp = ramp, yramp = zero */

# undef V_FUNC

  // free the temp vector if needed
  if (V1[0].type == ST_VECTOR_TMP) {
    free (V1[0].vector[0].elements.Ptr);
    free (V1[0].vector);
    V1[0].vector = NULL;
  }  

  clear_stack (V1);

  char line[512]; // this is only used to report an error 
  snprintf (line, 512, "error: op %s not defined as unary vector op!", op);
  push_error (line);
  return (FALSE);

escape:

  if (V1[0].type == ST_VECTOR_TMP) {
    free (V1[0].vector[0].elements.Ptr);
    free (V1[0].vector);
    V1[0].vector = NULL;
  }  

  clear_stack (V1);
  return (TRUE);

}

// vector string operations
int L_unary (StackVar *OUT, StackVar *V1, char *op) {

  int Nx = V1[0].vector[0].Nelements;

  OUT[0].vector = InitVector ();
  OUT[0].type = ST_VECTOR_TMP; /*** <<--- says this is a temporary matrix ***/

  ResetVector (OUT[0].vector, OPIHI_STR, V1[0].vector[0].Nelements); 
  char **Iv =  V1[0].vector[0].elements.Str; 
  char **Ov = OUT[0].vector[0].elements.Str; 
  if (!strcmp (op, "=")) {
    for (int i = 0; i < Nx; i++) {			
      Ov[i] = strcreate (Iv[i]);
    }									
    goto escape;							
  }

  // free the temp vector if needed
  if (V1[0].type == ST_VECTOR_TMP) {
    // XXX this probably does not free the strings
    free (V1[0].vector[0].elements.Ptr);
    free (V1[0].vector);
    V1[0].vector = NULL;
  }  

  clear_stack (V1);

  char line[512]; // this is only used to report an error 
  snprintf (line, 512, "error: op %s not defined as unary string vector op!", op);
  push_error (line);
  return (FALSE);

escape:

  if (V1[0].type == ST_VECTOR_TMP) {
    free (V1[0].vector[0].elements.Ptr);
    free (V1[0].vector);
    V1[0].vector = NULL;
  }  

  clear_stack (V1);
  return (TRUE);

}

# define M_FUNC(OP) { for (i = 0; i < Npix; i++, out++, M1++) { *out = (OP); } goto escape; }

int M_unary (StackVar *OUT, StackVar *V1, char *op) {

  int i, j, k;
  float *out, *M1;
  
  int Npix = gfits_npix_matrix (&V1[0].buffer[0].matrix);
  
  if (V1[0].type == ST_MATRIX_TMP) {
    OUT[0].buffer = V1[0].buffer;
    V1[0].type = ST_MATRIX; /* prevent it from being freed below */
  } else {
    OUT[0].buffer = InitBuffer ();
    CopyBuffer (OUT[0].buffer, V1[0].buffer);
  }
  OUT[0].type = ST_MATRIX_TMP;      /*** <<--- says this is a temporary matrix ***/
  M1  = (float *) V1[0].buffer[0].matrix.buffer;
  out = (float *)OUT[0].buffer[0].matrix.buffer;
 
// if (!strcmp (op, "rint"))  { for (i = 0; i < Npix; i++, out++, M1++) { *out = nearbyint (*M1); }}
  
  if (!strcmp (op, "="))      { goto escape; }
  if (!strcmp (op, "abs"))    M_FUNC(fabs(*M1));
  if (!strcmp (op, "int"))    M_FUNC((opihi_flt)(long long)(*M1));
  if (!strcmp (op, "floor"))  M_FUNC(floor (*M1));
  if (!strcmp (op, "ceil"))   M_FUNC(ceil (*M1));
  if (!strcmp (op, "exp"))    M_FUNC(exp(*M1));
  if (!strcmp (op, "ten"))    M_FUNC(pow(10.0,*M1));
  if (!strcmp (op, "log"))    M_FUNC(log10(*M1));
  if (!strcmp (op, "ln"))     M_FUNC(log(*M1));
  if (!strcmp (op, "sqrt"))   M_FUNC(sqrt(*M1));
  if (!strcmp (op, "erf"))    M_FUNC(erf(*M1));
  if (!strcmp (op, "sinh"))   M_FUNC(sinh(*M1));
  if (!strcmp (op, "cosh"))   M_FUNC(cosh(*M1));
  if (!strcmp (op, "asinh"))  M_FUNC(asinh(*M1));
  if (!strcmp (op, "acosh"))  M_FUNC(acosh(*M1));
  if (!strcmp (op, "sin"))    M_FUNC(sin(*M1));
  if (!strcmp (op, "cos"))    M_FUNC(cos(*M1));
  if (!strcmp (op, "tan"))    M_FUNC(tan(*M1));
  if (!strcmp (op, "dsin"))   M_FUNC(sin(*M1*RAD_DEG));
  if (!strcmp (op, "dcos"))   M_FUNC(cos(*M1*RAD_DEG));
  if (!strcmp (op, "dtan"))   M_FUNC(tan(*M1*RAD_DEG));
  if (!strcmp (op, "asin"))   M_FUNC(asin(*M1));
  if (!strcmp (op, "acos"))   M_FUNC(acos(*M1));
  if (!strcmp (op, "atan"))   M_FUNC(atan(*M1));
  if (!strcmp (op, "dasin"))  M_FUNC(asin(*M1)*DEG_RAD);
  if (!strcmp (op, "dacos"))  M_FUNC(acos(*M1)*DEG_RAD);
  if (!strcmp (op, "datan"))  M_FUNC(atan(*M1)*DEG_RAD);
  if (!strcmp (op, "lgamma")) M_FUNC(lgamma(*M1));
  if (!strcmp (op, "rnd"))    M_FUNC(drand48());
  if (!strcmp (op, "drnd"))   M_FUNC(drand48());
  if (!strcmp (op, "lrnd"))   M_FUNC(lrand48());
  if (!strcmp (op, "mrnd"))   M_FUNC(mrand48());
  if (!strcmp (op, "not"))    M_FUNC(!(*M1));
  if (!strcmp (op, "--"))     M_FUNC(-(*M1));
  if (!strcmp (op, "ramp"))   M_FUNC(i);
  if (!strcmp (op, "isinf"))  M_FUNC(!finite(*M1));
  if (!strcmp (op, "isnan"))  M_FUNC(isnan(*M1));
  if (!strcmp (op, "zero"))   M_FUNC(0);

  /* xrm and yrm only make sense for 2D matrices. see special meaning for vectors */
  if (!strcmp (op, "xramp")) {
    int Nx = V1[0].buffer[0].matrix.Naxis[0];
    int Ny = V1[0].buffer[0].matrix.Naxis[1];
    int Nz = MAX (1, V1[0].buffer[0].matrix.Naxis[2]);
    for (k = 0; k < Nz; k++) {
      for (j = 0; j < Ny; j++) {
	for (i = 0; i < Nx; i++, out++, M1++) {
	  *out = i;
	}
      }
    }
    goto escape; 
  }
  if (!strcmp (op, "yramp")) {
    int Nx = V1[0].buffer[0].matrix.Naxis[0];
    int Ny = V1[0].buffer[0].matrix.Naxis[1];
    int Nz = MAX (1, V1[0].buffer[0].matrix.Naxis[2]);
    for (k = 0; k < Nz; k++) {
      for (j = 0; j < Ny; j++) {
	for (i = 0; i < Nx; i++, out++, M1++) {
	  *out = j;
	}
      }
    }
    goto escape; 
  }
  if (!strcmp (op, "zramp")) {
    int Nx = V1[0].buffer[0].matrix.Naxis[0];
    int Ny = V1[0].buffer[0].matrix.Naxis[1];
    int Nz = MAX (1, V1[0].buffer[0].matrix.Naxis[2]);
    for (k = 0; k < Nz; k++) {
      for (j = 0; j < Ny; j++) {
	for (i = 0; i < Nx; i++, out++, M1++) {
	  *out = k;
	}
      }
    }
    goto escape; 
  }
  
  if (V1[0].type == ST_MATRIX_TMP) {
    free (V1[0].buffer[0].header.buffer);
    free (V1[0].buffer[0].matrix.buffer);
    free (V1[0].buffer);
  }

  clear_stack (V1);

  char line[512]; // this is only used to report an error 
  snprintf (line, 512, "error: op %s not defined as unary matrix op!", op);
  push_error (line);
  return (FALSE);

 escape:

  if (V1[0].type == ST_MATRIX_TMP) {
    free (V1[0].buffer[0].header.buffer);
    free (V1[0].buffer[0].matrix.buffer);
    free (V1[0].buffer);
  }

  clear_stack (V1);
  return (TRUE);

}

/*********************** fits copy header ***********************************/
int gfits_copy_matrix_info (Matrix *matrix1, Matrix *matrix2) {

  int i;

  /* copy all but the matrix */

  matrix2[0].unsign   = matrix1[0].unsign;
  matrix2[0].bitpix   = matrix1[0].bitpix;
  matrix2[0].datasize = matrix1[0].datasize;
  matrix2[0].bzero    = matrix1[0].bzero;
  matrix2[0].bscale   = matrix1[0].bscale;
  matrix2[0].Naxes    = matrix1[0].Naxes;
  for (i = 0; i < FT_MAX_NAXES; i++) {
    matrix2[0].Naxis[i] = matrix1[0].Naxis[i];
  }

  return (TRUE);
}       
