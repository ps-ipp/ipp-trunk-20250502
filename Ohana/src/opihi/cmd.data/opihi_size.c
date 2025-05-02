# include "data.h"

int opihi_size (int argc, char **argv) {
  
  int N;

  char *resultName = NULL;
  Vector *resultVec = NULL;
  if ((N = get_argument (argc, argv, "-result"))) {
    remove_argument (N, &argc, argv);
    resultName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    if ((resultVec = SelectVector (resultName, ANYVECTOR, FALSE)) == NULL)   {
      gprint (GP_ERR, "invalid vector name %s for return result\n", resultName);
      FREE (resultName);
      return FALSE;
    }
  }

  if (argc != 2) goto usage;

  Vector *vec = NULL;
  Buffer *buf = NULL;

  // is it a Vector?
  if ((vec = SelectVector (argv[1], OLDVECTOR, FALSE)) != NULL) {
    if (resultVec) {
      ResetVector (resultVec, OPIHI_INT, 1);
      resultVec->elements.Int[0] = vec->Nelements;
    } else {
      gprint (GP_LOG, "%s vector size %d\n", argv[1], vec->Nelements);
    }
    goto got_result;
  }

  // is it a Matrix (Buffer)?
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, FALSE)) != NULL) {
    if (resultVec) {
      ResetVector (resultVec, OPIHI_INT, buf->header.Naxes);
      for (int i = 0; i < buf->header.Naxes; i++) {
	resultVec->elements.Int[i] = buf->header.Naxis[i];
      }
    } else {
      gprint (GP_LOG, "%s matrix sizes ", argv[1]);
      for (int i = 0; i < buf->header.Naxes; i++) {
	gprint (GP_LOG, " %d", (int) buf->header.Naxis[i]);
      }
      gprint (GP_LOG, "\n");
    }
    goto got_result;
  }
  
  if (resultVec) {
    ResetVector (resultVec, OPIHI_INT, 0);
  } else {
    gprint (GP_LOG, "%s not defined\n", argv[1]);
  }

got_result:
  FREE (resultName);
  return TRUE;
  
usage:
  gprint (GP_ERR, "SYNTAX: size (vector/buffer) [-result vector]\n");
  gprint (GP_ERR, "  returns type (vector/matrix) and sizes\n");
  gprint (GP_ERR, "  returns sizes in vector if -result is supplied\n");
  return (FALSE);
}
