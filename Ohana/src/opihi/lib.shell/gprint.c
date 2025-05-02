# include "shell.h"

/* we need to control the output destinations a bit carefully
   in the pantasks_server mode.  The server needs to be able to 
   send the output to a specific output device (eg, logging file)
   of to save it within an internal buffer */

/* further notes:
   gprintf (int destination, char *format, ...);

   the destination may be GP_LOG or GP_ERR.  by default, these go to stdout and
   stderr.  either one may be redirected to another file or to a buffer.  as a
   stand-along program, the outfile command redirects the GP_LOG output to an
   alternate output file.  in the server mode, we redirect LOG to a standard log
   file and ERR to a standard error file for server messages.  Commands executed
   by the client have both streams returned to the client in turn, and are in
   turn sent to stderr or the current stdout destination.

   each thread has an independently set output destination.

   option 1: NULL for invalid element:

     a stream is either set to a FILE or an IOBuffer.  when it is set to a FILE,
     the IOBuffer is freed and set to NULL.  when it is an IOBuffer, the file
     pointer is closed and set to NULL.  setting a new FILE results in the 
     IObuffer being freed and an already opened FILE to be flushed and closed.


  gprint & mutex:

     gprintInit : init_stream_mutex
     gprintGetStream : init_stream_mutex
     gprintCloseFile : init_stream_mutex

     gprintSetFileAllThreads : set_file_mutex
     (gprintSetFile -> gprintCloseFile)

     gprintSetFileThisThread : set_file_mutex
     (gprintGetStream)
     (gprintSetFile -> gprintCloseFile)

     we are safe from dead locks: init_stream_mutex can be wrapped by set_file_mutex, but
     not vice-versa

*/

static gpStream **streams = NULL;
static int Nstreams = 0;

static pthread_mutex_t init_stream_mutex = PTHREAD_MUTEX_INITIALIZER;

void gprintInit () {

  int N;
  pthread_t id;

  /* need to use a mutex to prevent two threads from initing simultaneously */
  pthread_mutex_lock (&init_stream_mutex);

  // streams is an array of pointers so we can add more streams without changing pointers 
  if (streams == NULL) {
    Nstreams = 2;
    ALLOCATE (streams, gpStream *, Nstreams);
  } else {
    Nstreams += 2;
    REALLOCATE (streams, gpStream *, Nstreams);
  }

  /* create two output streams for this thread: LOG and ERR */
  id = pthread_self();

  N = Nstreams - 2;
  ALLOCATE (streams[N], gpStream, 1);
  streams[N][0].dest = GP_LOG;
  streams[N][0].file = stdout;
  streams[N][0].name = strcreate ("stdout");

  ALLOCATE (streams[N][0].buffer, IOBuffer, 1);
  InitIOBuffer (streams[N][0].buffer, 64);
  streams[N][0].mode = GP_FILE;
  streams[N][0].thread = id;

  N = Nstreams - 1;
  ALLOCATE (streams[N], gpStream, 1);
  streams[N][0].dest = GP_ERR;
  streams[N][0].file = stderr;
  streams[N][0].name = strcreate ("stderr");

  ALLOCATE (streams[N][0].buffer, IOBuffer, 1);
  InitIOBuffer (streams[N][0].buffer, 64);
  streams[N][0].mode = GP_FILE;
  streams[N][0].thread = id;

  pthread_mutex_unlock (&init_stream_mutex);
}

gpStream *gprintGetStream (gpDest dest) {

  int i;
  pthread_t id;
  gpStream *stream;

  // need to wait for initialization to be finished before getting stream or the array
  // (streams[i]) may move
  pthread_mutex_lock (&init_stream_mutex);

  id = pthread_self();

  /* find the existing output stream which matches */
  for (i = 0; i < Nstreams; i++) {
    if (streams[i][0].dest != dest) continue;
    if (!pthread_equal (streams[i][0].thread, id)) continue;
    stream = streams[i];
    pthread_mutex_unlock (&init_stream_mutex);
    return stream;
  }
  pthread_mutex_unlock (&init_stream_mutex);
  fprintf (stderr, "programming error: gprintInit not called for thread\n");
  abort ();
}

// close if necessary, set to file (may be NULL)
void gprintCloseFile (gpStream *stream, FILE *file) {

  int i, Nmatch;

  // do not close the file if the new file is the same
  if (stream[0].file == file) return;

  // do not close the file if the old one is NULL
  if (stream[0].file == NULL) {
    stream[0].file = file;    
    return;
  }

  // check the special cases (do not close old file in these cases)
  if (stream[0].file == stdout) {
    stream[0].file = file;
    return;
  }
  if (stream[0].file == stderr) {
    stream[0].file = file;
    return;
  }

  // we cannot do the operation below while another thread is initing
  pthread_mutex_lock (&init_stream_mutex);

  // must we close the existing file? if still being used, then no
  Nmatch = 0;
  for (i = 0; i < Nstreams; i++) {
    if (stream == streams[i]) continue;
    if (streams[i][0].file == stream[0].file) Nmatch ++;
  }
  if (Nmatch == 0) {
    // fprintf (stderr, "closed %x, opened %x (%s) -- thread %d\n", stream[0].file, file, stream[0].name, stream[0].thread);
    fflush (stream[0].file);
    fclose (stream[0].file);
  }
  stream[0].file = file;
  pthread_mutex_unlock (&init_stream_mutex);

  return;
}

// this thread only operates on its own stream
void gprintSetBuffer (gpDest dest) {

  gpStream *stream;

  stream = gprintGetStream (dest);

  // close the existing file (if needed), set to NULL
  gprintCloseFile (stream, NULL);

  assert (stream[0].buffer);
  FlushIOBuffer (stream[0].buffer);
  
  stream[0].mode = GP_BUFF;
}

// this thread only operates on its own stream
IOBuffer *gprintGetBuffer (gpDest dest) {

  gpStream *stream;
  stream = gprintGetStream (dest);
  return (stream[0].buffer);
}

static pthread_mutex_t set_file_mutex = PTHREAD_MUTEX_INITIALIZER;

void gprintSetFileAllThreads (gpDest dest, char *filename) {

  int i;

  // be sure we are not colliding with gprintSetFileThisThread
  pthread_mutex_lock (&set_file_mutex);

  for (i = 0; i < Nstreams; i++) {
    if (streams[i][0].dest != dest) continue;
    gprintSetFile (streams[i], dest, filename);
  }

  pthread_mutex_unlock (&set_file_mutex);
  return;
}

// this thread only operates on its own stream
void gprintSetFileThisThread (gpDest dest, char *filename) {

  gpStream *stream;

  // be sure we are not colliding with gprintSetFileAllThreads
  pthread_mutex_lock (&set_file_mutex);

  stream = gprintGetStream (dest);
  gprintSetFile (stream, dest, filename);

  pthread_mutex_unlock (&set_file_mutex);
  return;
}

void gprintSetFile (gpStream *stream, gpDest dest, char *filename) {

  FILE *file;

  assert (stream[0].buffer);

  /* if we have an open buffer, free it and null it */
  if (stream[0].buffer[0].Nbuffer) {
    // XXX we drop what was on the buffer, send it to the old or the new file?
    FlushIOBuffer (stream[0].buffer);
  }

  stream[0].mode = GP_FILE;

  // if NULL, reuse exising stream name
  if (filename != NULL) {
    free (stream[0].name);
    stream[0].name = strcreate (filename);
  }

  // we allow the user to set stdout to ERR and stderr to LOG if they want
  if (!strcmp (stream[0].name, "stdout")) {
    gprintCloseFile (stream, stdout);
    return;
  }
  if (!strcmp (stream[0].name, "stderr")) {
    gprintCloseFile (stream, stderr);
    return;
  }
  
  // open the file. only close old pointer if no one else is using it
  file = fopen (stream[0].name, "a");
  if (file == NULL) {
    // XXX this is a problem: we are leaving open the old file
    fprintf (stderr, "gprint cannot open file %s\n", stream[0].name);
    free (stream[0].name);
    file = (dest == GP_LOG) ? stdout : stderr;
    stream[0].name = (dest == GP_LOG) ? strcreate ("stdout") : strcreate("stderr");
    gprintCloseFile (stream, file);
    return;
  }

  // close the existing file (if needed), set to new file
  gprintCloseFile (stream, file);
  return;
}

// this thread only operates on its own stream
FILE *gprintGetFile (gpDest dest) {

  gpStream *stream;
  stream = gprintGetStream (dest);
  return (stream[0].file);
}

// this thread only operates on its own stream
char *gprintGetName (gpDest dest) {

  gpStream *stream;
  stream = gprintGetStream (dest);
  return (stream[0].name);
}

int gprint (gpDest dest, char *format, ...) {

  int status;
  va_list argp;  

  va_start (argp, format);
  status = gprintv (dest, format, argp);
  va_end (argp);
  return (status);
}

int gprintv (gpDest dest, char *format, va_list argp) {

  int status;
  gpStream *stream;

  // this thread only writes to its own stream
  stream = gprintGetStream (dest);

  if (stream[0].mode == GP_FILE) {
    // fprintf (stderr, "printing to %s, mode FILE, thread %d\n", stream[0].name, (int) stream[0].thread);
    status = vfprintf (stream[0].file, format, argp);
    fflush (stream[0].file);
    if (status < 0) {
      return (FALSE);
    }
  } else {
    // fprintf (stderr, "printing to %s, mode BUFF, thread %d\n", stream[0].name, (int) stream[0].thread);
    vPrintIOBuffer (stream[0].buffer, format, argp);
  }
  return (TRUE);
}

int gwrite (char *buffer, int size, int N, gpDest dest) {

  int Nbyte;
  IOBuffer *outbuff;
  gpStream *stream;

  // this thread only writes to its own stream
  stream = gprintGetStream (dest);

  if (stream[0].mode == GP_FILE) {
    fwrite (buffer, size, N, stream[0].file);
    fflush (stream[0].file);
  } else {
    // XXX can we not use exising IOBuffer APIs here?
    outbuff = stream[0].buffer;
    Nbyte = size * N;
    if (outbuff[0].Nbuffer + Nbyte >= outbuff[0].Nalloc) {
      outbuff[0].Nalloc = outbuff[0].Nbuffer + Nbyte + 64;
      REALLOCATE (outbuff[0].buffer, char, outbuff[0].Nalloc);
    }
    memcpy (&outbuff[0].buffer[outbuff[0].Nbuffer], buffer, Nbyte);
    outbuff[0].Nbuffer += Nbyte;
  }
  return (TRUE);
}

# define MAX_ERROR_LENGTH 256           // Maximum length string for error messages

// print an error (based on errno values) to gprint destination
int gprint_syserror (gpDest dest, int myError, char *format, ...) {

  char errorBuf[MAX_ERROR_LENGTH];
  char *errorMsg;
  va_list argp;  

  // there are two strerror_r implementations; choose the right one:
#if (((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE) || __APPLE__)
  strerror_r (myError, errorBuf, MAX_ERROR_LENGTH);
  errorMsg = errorBuf;
#else
  errorMsg = strerror_r (myError, errorBuf, MAX_ERROR_LENGTH);
#endif

  va_start (argp, format);
  gprintv (dest, format, argp);
  va_end (argp);

  gprint (dest, "%s\n", errorMsg);
  return TRUE;
}
