# include "opihi.h"

static Buffer **buffers;
static int     Nbuffers;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// this function is NOT thread protected : it is only used in startup and/or shutdown
void InitBuffers () {
  Nbuffers = 0;
  ALLOCATE (buffers, Buffer *, 1);
}

// this function is NOT thread protected : it is only used in startup and/or shutdown
void FreeBuffers () {

  int i;

  for (i = 0; i < Nbuffers; i++) {
    gfits_free_header (&buffers[i][0].header);
    gfits_free_matrix (&buffers[i][0].matrix);
    free (buffers[i]);
  }
  free (buffers);
}

Buffer *InitBuffer () {
  Buffer *buf;

  ALLOCATE (buf, Buffer, 1);
  bzero (buf[0].name, 1024);
  bzero (buf[0].file, 1024);
  ALLOCATE (buf[0].matrix.buffer, char, 1);
  ALLOCATE (buf[0].header.buffer, char, 1);
  return (buf);
}

int IsBuffer (char *name) {
 
  int i;

  if (name == NULL) return (FALSE);

  for (i = 0; (i < Nbuffers) && (strcmp(buffers[i][0].name, name)); i++);
  if (i == Nbuffers) return (FALSE);
  return (TRUE);
}

int IsBufferPtr (Buffer *buf) {
 
  int i;

  if (buf == NULL) return (FALSE);

  for (i = 0; (i < Nbuffers) && (buffers[i] != buf); i++);
  if (i == Nbuffers) return (FALSE);
  return (TRUE);
}

Buffer *SelectBuffer (char *name, int mode, int verbose) {

  int i;

  if (name == NULL) goto error;
  if (ISNUM(name[0])) goto error;
  if (IsVector(name)) goto error;

  for (i = 0; (i < Nbuffers) && (strcmp(buffers[i][0].name, name)); i++);
  /* is a new buffer */
  if (i == Nbuffers) { 
    if (mode == OLDBUFFER) goto error;
    // lock to protect static array
    pthread_mutex_lock (&mutex);
    Nbuffers ++;
    REALLOCATE (buffers, Buffer *, Nbuffers);
    buffers[i] = InitBuffer ();
    strcpy (buffers[i][0].name, name);
    pthread_mutex_unlock (&mutex);
    return (buffers[i]);
  } 
  /* is an old buffer */
  if (mode == NEWBUFFER) goto error;
  return (buffers[i]);

error:
  if (verbose) gprint (GP_ERR, "invalid matrix %s\n", name);
  return (NULL);
}

int ResetBuffer (Buffer *buf, int Nx, int Ny, int bitpix, float bzero, float bscale) {

  gfits_free_matrix (&buf[0].matrix);
  gfits_free_header (&buf[0].header);
  if (!CreateBuffer (buf, Nx, Ny, bitpix, bzero, bscale)) return FALSE;
  return TRUE;
}

int CreateBuffer (Buffer *buf, int Nx, int Ny, int bitpix, float bzero, float bscale) {

  if (Nx < 0) { gprint (GP_ERR, "invalid negative matrix size Nx : %d\n", Nx);  return FALSE; }
  if (Ny < 0) { gprint (GP_ERR, "invalid negative matrix size Ny : %d\n", Ny);  return FALSE; }

  /* store the default output values */
  gfits_init_header (&buf[0].header);

  /* assign the necessary internal values */
  buf[0].header.bitpix   = -32;
  buf[0].header.Naxes = 2;
  buf[0].header.Naxis[0] = Nx;
  buf[0].header.Naxis[1] = Ny;

  buf[0].bitpix = bitpix;
  buf[0].bzero  = bzero;
  buf[0].bscale = bscale;
  
  /* make some test of the validity of the values */

  /* create the appropriate header and matrix */
  gfits_create_header (&buf[0].header);
  gfits_create_matrix (&buf[0].header, &buf[0].matrix);

  return (TRUE);
}
  
// buffer is 1D array referenced in the order:
// buffer[x + Nx*y + Nx*Ny*z + ...]
int CreateBuffer3D (Buffer *buf, int Nx, int Ny, int Nz, int bitpix, float bzero, float bscale) {

  if (Nx < 0) { gprint (GP_ERR, "invalid negative matrix size Nx : %d\n", Nx);  return FALSE; }
  if (Ny < 0) { gprint (GP_ERR, "invalid negative matrix size Ny : %d\n", Ny);  return FALSE; }
  if (Nz < 0) { gprint (GP_ERR, "invalid negative matrix size Nz : %d\n", Nz);  return FALSE; }

  /* store the default output values */
  gfits_init_header (&buf[0].header);

  /* assign the necessary internal values */
  buf[0].header.bitpix   = -32;
  buf[0].header.Naxes = 3;
  buf[0].header.Naxis[0] = Nx;
  buf[0].header.Naxis[1] = Ny;
  buf[0].header.Naxis[2] = Nz;

  buf[0].bitpix = bitpix;
  buf[0].bzero  = bzero;
  buf[0].bscale = bscale;
  
  /* make some test of the validity of the values */

  /* create the appropriate header and matrix */
  gfits_create_header (&buf[0].header);
  gfits_create_matrix (&buf[0].header, &buf[0].matrix);

  return (TRUE);
}
  
/* copy data from in to out - new memory space */
int CopyNamedBuffer (char *out, char *in) {
  Buffer *In, *Out;
  if ((In  = SelectBuffer (in,  OLDBUFFER, FALSE)) == NULL) return (FALSE);
  if ((Out = SelectBuffer (out, ANYBUFFER, FALSE)) == NULL) return (FALSE);
  CopyBuffer (Out, In);
  return (TRUE);
}

int CopyBuffer (Buffer *out, Buffer *in) {
  out[0].bitpix = in[0].bitpix;
  out[0].unsign = in[0].unsign;
  out[0].bscale = in[0].bscale;
  out[0].bzero  = in[0].bzero;
  strcpy (out[0].file, in[0].file);
  gfits_copy_matrix (&in[0].matrix, &out[0].matrix);
  gfits_copy_header (&in[0].header, &out[0].header);
  return (TRUE);
}

/* move data from in to out - use old memory space */
int MoveNamedBuffer (char *out, char *in) {
  Buffer *In, *Out;
  if ((In  = SelectBuffer (in,  OLDBUFFER, FALSE)) == NULL) return (FALSE);
  if ((Out = SelectBuffer (out, ANYBUFFER, FALSE)) == NULL) return (FALSE);
  MoveBuffer (Out, In);
  return (TRUE);
}
int MoveBuffer (Buffer *out, Buffer *in) {
  int i, j;

  free (out[0].matrix.buffer);
  free (out[0].header.buffer);
  out[0].bitpix = in[0].bitpix;
  out[0].unsign = in[0].unsign;
  out[0].bscale = in[0].bscale;
  out[0].bzero  = in[0].bzero;
  out[0].matrix = in[0].matrix;
  out[0].header = in[0].header;
  strcpy (out[0].file, in[0].file);

  /* delete buffer entry from buffer list, if it exists */
  for (i = 0; (i < Nbuffers) && (in != buffers[i]); i++);
  if (i == Nbuffers) {
    free (in);
    return (TRUE);
  }
  free (in);

  pthread_mutex_lock (&mutex);
  for (j = i; j < Nbuffers - 1; j++) buffers[j] = buffers[j + 1];
  Nbuffers --;
  REALLOCATE (buffers, Buffer *, MAX (Nbuffers, 1));
  pthread_mutex_unlock (&mutex);
  return (TRUE);
}

/* delete by ptr */
int DeleteBuffer (Buffer *buf) {

  int i, j;

  if (buf == NULL) return (FALSE);
  for (i = 0; (i < Nbuffers) && (buf != buffers[i]); i++);
  if (i == Nbuffers) return (FALSE);

  gfits_free_header (&buffers[i][0].header);
  gfits_free_matrix (&buffers[i][0].matrix);
  free (buffers[i]);

  pthread_mutex_lock (&mutex);
  for (j = i; j < Nbuffers - 1; j++) buffers[j] = buffers[j + 1];
  Nbuffers --;
  REALLOCATE (buffers, Buffer *, MAX (Nbuffers, 1));
  pthread_mutex_unlock (&mutex);
  return (TRUE);
}
  
/* delete by name */
int DeleteNamedBuffer (char *name) {

  int i, j;

  if (name == NULL) return (FALSE);
  for (i = 0; (i < Nbuffers) && (strcmp(buffers[i][0].name, name)); i++);
  if (i == Nbuffers) return (FALSE);

  gfits_free_header (&buffers[i][0].header);
  gfits_free_matrix (&buffers[i][0].matrix);
  free (buffers[i]);

  pthread_mutex_lock (&mutex);
  for (j = i; j < Nbuffers - 1; j++) buffers[j] = buffers[j + 1];
  Nbuffers --;
  REALLOCATE (buffers, Buffer *, MAX (Nbuffers, 1));
  pthread_mutex_unlock (&mutex);
  return (TRUE);
}

void dump_buffers (int n) {

  int i;

  gprint (GP_ERR, "try %d\n", n);
  for (i = 0; i < Nbuffers; i++) {
    gprint (GP_ERR, "%d  %lx\n", i, (long) buffers[i]);
    gprint (GP_ERR, "%d  %lx  %s\n", i, (long) buffers[i][0].name, buffers[i][0].name);
    gprint (GP_ERR, "%d  %lx  %s\n", i, (long) buffers[i][0].file, buffers[i][0].file);
    gprint (GP_ERR, "%d  %lx  %lx\n", i, (long) &buffers[i][0].header, (long) buffers[i][0].header.buffer);
    gprint (GP_ERR, "%d  %lx  %lx\n", i, (long) &buffers[i][0].matrix, (long) buffers[i][0].matrix.buffer);
    gprint (GP_ERR, "%d  %d  %d  %f %f\n", i, buffers[i][0].bitpix, buffers[i][0].unsign, buffers[i][0].bscale, buffers[i][0].bzero);
  }
}

int PrintBuffers (int Long) {

  int i;

  if (Nbuffers == 0) {
    gprint (GP_ERR, "No allocated buffers\n");
    return (TRUE);
  }
  
  if (Long) {
    gprint (GP_LOG, "    N       name                      file ND     X     Y     Z      bytes  BP U      bzero     bscale\n");
    for (i = 0; i < Nbuffers; i++) {
      gprint (GP_LOG, "%5d %10s %25s %2d %5lld %5lld %5lld %10lld %3d %1d %10.4e %10.4e\n",
	      i, buffers[i][0].name, buffers[i][0].file, buffers[i][0].header.Naxes,
	      (long long) buffers[i][0].header.Naxis[0], (long long) buffers[i][0].header.Naxis[1], (long long) buffers[i][0].header.Naxis[2],
 	      (long long) buffers[i][0].header.datasize + buffers[i][0].matrix.datasize, buffers[i][0].bitpix, 
	      buffers[i][0].unsign, buffers[i][0].bzero, buffers[i][0].bscale);
    }
    return (TRUE);
  }

  gprint (GP_LOG, "    N       name                      file ND     X     Y     Z      bytes\n");
  for (i = 0; i < Nbuffers; i++) {
    gprint (GP_LOG, "%5d %10s %25s %2d %5lld %5lld %5lld %10lld\n",
	    i, buffers[i][0].name, buffers[i][0].file, buffers[i][0].header.Naxes,
	    (long long) buffers[i][0].header.Naxis[0], (long long) buffers[i][0].header.Naxis[1], (long long) buffers[i][0].header.Naxis[2],
	    (long long) buffers[i][0].header.datasize + buffers[i][0].matrix.datasize);
  }
  return (TRUE);
}

int ListBuffersToList (char *name) {

  int i;
  char line[1024];

  for (i = 0; i < Nbuffers; i++) {
    sprintf (line, "%s:%d", name, i);
    set_str_variable (line, buffers[i][0].name);
  }
  sprintf (line, "%s:n", name);
  set_int_variable (line, Nbuffers);
  return (Nbuffers);
}
