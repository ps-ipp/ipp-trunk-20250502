# include "imregister.h"
# include "imreg.h"
# include <errno.h>
# define MAXTIME 10

int SockScan (char *string, Fifo *fifo, int sock) {
  
  int i, status;
  char *done;

  done = string;
  status = FALSE;
  for (i = 0; (i < MAXTIME) && (done != NULL); i++) {
    ShiftFifo (fifo);
    status = ReadtoFifo (fifo, sock);
    switch (status) {
    case 0:
      break;
    case -1:
      usleep (1000);
      break;
    default:
      done = memstr (fifo[0].buffer, string, fifo[0].Nbuffer);
      break;
    }
  }
  return (status);
}
 
/* SockScan reads from the given socket looking for the given string
   SockScan stores data read from socket in the fifo buffer to ensure 
   it catches the given string.  a single invocation of SockScan will 
   wait for up to 10 msec for data to come down the pipe before giving up.
   SockScan can only search for one string in the output stream.
   */


/* need a way of signalling that the socket has closed...
   maybe change the sock entry to 0?
*/
