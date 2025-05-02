# include "opihi.h"
# ifndef MAX_INT
# define MAX_INT 2147483647
# endif

int check_stack (StackVar *stack, int Nstack, int validsize) {

  int i, Nx, Ny, Nz, Nv, size;
  char *c1, *c2;

  Nv = Nx = Ny = Nz = -1;

  for (i = 0; i < Nstack; i++) {
    if (stack[i].type == ST_VALUE) {

      /** if this is a number, put it on the list of scalars and move on.  assume value is
       * an int unless proven otherwise. 

       If we have built libdvo with opihi_int defined as a 32bit signed int, then we have
       an overflow for values > MAX_INT (2^31).  If the float value is larger than this,
       we should treat the value as a float.  (NOTE: this means we cannot use 32bit flags,
       only 31bit flags.

      **/

      stack[i].FltValue = strtod (stack[i].name, &c1);
      stack[i].IntValue = strtol (stack[i].name, &c2, 0);
      if ((fabs(stack[i].FltValue) > MAX_INT) && (c1 == stack[i].name + strlen (stack[i].name))) {
	stack[i].type  = ST_SCALAR_FLT; // (float)
	continue;
      } 
      if (c2 == stack[i].name + strlen (stack[i].name)) {
	stack[i].type  = ST_SCALAR_INT; // (int)
	continue;
      } 
      if (c1 == stack[i].name + strlen (stack[i].name)) {
	stack[i].type  = ST_SCALAR_FLT; // (float)
	continue;
      } 

      /** if this is a matrix, find the dimensions and check with existing values **/
      if (IsBuffer (stack[i].name)) {
	stack[i].buffer = SelectBuffer (stack[i].name, OLDBUFFER, TRUE);
	stack[i].type   = ST_MATRIX;
	if (Nx == -1) {
	  Nx = stack[i].buffer[0].matrix.Naxis[0];
	  Ny = stack[i].buffer[0].matrix.Naxis[1];
	  Nz = stack[i].buffer[0].matrix.Naxis[2];
	} 
	if ((Nx != stack[i].buffer[0].matrix.Naxis[0]) ||
	    (Ny != stack[i].buffer[0].matrix.Naxis[1]) |
	    (Nz != stack[i].buffer[0].matrix.Naxis[2])) {
	  push_error ("dimensions don't match");
	  return (-1);
	}	
	if (Nv != -1) {
	  if ((Nv != Nx) && (Nv != Ny)) {
	    push_error ("dimensions don't match");
	    return (-1);
	  }
	}	
	continue;
      }

      /** if this is a vector, find the dimensions and check with existing values **/
      if (IsVector (stack[i].name)) {
	stack[i].vector = SelectVector (stack[i].name, OLDVECTOR, FALSE);
	stack[i].type   = ST_VECTOR;

	if (Nv == -1) Nv = stack[i].vector[0].Nelements;
	if (Nv != stack[i].vector[0].Nelements) {
	  push_error ("dimensions don't match");
	  return (-1);
	}
	if (Nx != -1) {
	  if ((Nx != Nv) && (Ny != Nv)) {
	    push_error ("dimensions don't match");
	    return (-1);
	  }
	}	
	continue;
      }

      /* this is not a scalar, vector, or matrix.  must be string */
      stack[i].type  = ST_STRING;
      
      /* I could strip the quotes from the name here, 
	 but this might change the behavior for string tests
      */

      // I cannot require quotes around a string here because variables are expanded upstream
    }
  }

  /* return object dimensions */
  size = 0;
  if (Nv != -1) size = 1;
  if (Nx != -1) size = 2;
  if (validsize == -1)   return (size);
  if (validsize != size) return (-1);
  return (size);
}

/* check stack identifies the data elements as scalar, vector, matrix, or word.
   operators have already been identified.  
   check stack returns the total stack dimensionality (0,1,2)
   on error, check stack returns -1
   possible errors:
     - mismatch with requested dimensionality
     - mismatch in data dimensions
*/
