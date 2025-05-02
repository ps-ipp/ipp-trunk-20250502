# include "dimm.h"
# include <termios.h>

# define CR   0x0D
# define LF   0x0A
# define BEEP 0x07
# define OPENERR    -1   /* Port could not be opened */
# define PORTERR    -2   /* Opened port is not a serial (tty) port */
# define BADCMDERR  -3   /* Command sent to camera is not understood */
# define TIMEOUTERR -4   /* No response */
 
# define SER_VERBOSE 0
# define SER_DEBUG   0

# ifndef SER_VERBOSE
# define SER_VERBOSE 1           /* Be verbose? */
# endif
# ifndef SER_DEBUG 
# define SER_DEBUG 1
# endif

/* Defines for Serial Port  */
typedef struct {
  int f;
  int rate;
  int parity;
  int bits;
  int stpbit;
  char port[64];
} Serial;

static Serial serial = {0, 0, 0, 0, 0};
static int SER_ECHO = 0;

int SerialOpen (char *);

int SerialVerbose (int mode) {

  SER_ECHO = mode;
  return (TRUE);

}

int SerialInit (char *port) {
  
  strcpy (serial.port, port);
  serial.rate = 2400;
  serial.rate = 9600;
  serial.parity = 0;
  serial.bits = 8;
  serial.stpbit = 1;
  
  serial.f = SerialOpen (serial.port);
  if (serial.f <= 0) {
    gprint (GP_ERR, "Error opening serial port %s - error %d.\n", serial.port, serial.f);
    return (FALSE);
  }
  if (SerialBaudRate (serial.f, serial.rate))   return (FALSE);
  if (SerialParity   (serial.f, serial.parity)) return (FALSE);
  if (SerialDataBits (serial.f, serial.bits))   return (FALSE);
  if (SerialStopBit  (serial.f, serial.stpbit)) return (FALSE);
  return (TRUE);
}

/********************************** Open *********************************/
int SerialOpen (char *port) {
  
  int err = 0;
  struct termios term;
  char prefix[100];
  int fdesc = -1;
  int locked;
  struct flock lock;
   
  /* open serial line */
   if (SER_VERBOSE) printf ("Opening the serial port %s for read/write.\n", port);
   fdesc = open (port, O_RDWR);
   if (fdesc == -1) {
     if (SER_VERBOSE) gprint (GP_ERR, "Cannot open %s for read/write.\n", port);
     return OPENERR;
   }
   /* lock serial line */
   lock.l_type = F_WRLCK;
   lock.l_len = 0;
   lock.l_start = 0;
   lock.l_whence = 0;
   locked = fcntl (fdesc, F_SETLK, &lock);
   if (locked == -1) {
     gprint (GP_ERR, "can't lock serial line\n");
     close (fdesc);
     return OPENERR;
   }

   /* Get the serial port's attributes... */
   if (tcgetattr (fdesc, &term) == -1) {
      if (SER_VERBOSE) gprint (GP_ERR, "Port is not a tty\n");
      return PORTERR;
   }

   /* cfmakeraw (&term); */
   term.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
   term.c_oflag &= ~OPOST;
   term.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
   term.c_cflag &= ~(CSIZE|PARENB);
   term.c_cflag |= CS8;
   term.c_cc[VMIN] = 0;     /* MIN setting...if 0, wait only for timeout */
   term.c_cc[VTIME] = 2;    /* TIME setting...wait at most 1 sec for response 
		              (ignored if c_cc[VMIN]>0) */
   tcsetattr (fdesc, TCSAFLUSH, &term);

# if (0)   
   term.c_lflag &= ~ICANON; /* Turn OFF canonical input 
 	                      (so it does it character by character) */
   term.c_cc[VMIN] = 0;     /* MIN setting...if 0, wait only for timeout */
   term.c_cc[VTIME] = 10;    /* TIME setting...wait at most 1 sec for response 
		              (ignored if c_cc[VMIN]>0) */
   term.c_lflag &= ~ECHO;   /* Turn OFF echoing... */
   term.c_iflag &= ~ICRNL;   /* Don't map CR to NL on input */

   /* Set port (terminal) to reflect the change...(flush first) */
   tcsetattr (fdesc, TCSAFLUSH, &term);
# endif

   return fdesc;
}

/******************************* Baud Rate *******************************/
int SerialBaudRate (int fdesc, int rate) {
   int err = 0;
   struct termios term;

  /* Get the serial port's attributes... */
   if (SER_VERBOSE) printf("Setting the serial port's baud rate.\n");
   if (tcgetattr (fdesc, &term) == -1) {
      if (SER_VERBOSE) gprint (GP_ERR, "Port is not a tty\n");
      return PORTERR;
   }
 
   /* Set the input and output baud rates to 'rate'... */
   switch (rate) { 
   case 1200:         /* Set speed to 1200 */ 
      if (cfgetospeed(&term) != B1200) cfsetospeed(&term, B1200);
      if (cfgetispeed(&term) != B1200) cfsetispeed(&term, B1200);
      break;
   case 2400:         /* Set speed to 2400 */ 
      if (cfgetospeed(&term) != B2400) cfsetospeed(&term, B2400);
      if (cfgetispeed(&term) != B2400) cfsetispeed(&term, B2400);
      break;
   case 9600:         /* Set speed to 9600 */
      if (cfgetospeed(&term) != B9600) cfsetospeed(&term, B9600);
      if (cfgetispeed(&term) != B9600) cfsetispeed(&term, B9600);
      break;
   case 19200:        /* Set speed to 19200 */ 
      if (cfgetospeed(&term) != B19200) cfsetospeed(&term, B19200);
      if (cfgetispeed(&term) != B19200) cfsetispeed(&term, B19200);
      break;
   default:
      printf ("ERROR:  Unknown Baud Rate\n"); 
      break;
   }	

   /* Set port (terminal) to reflect the change...(flush first) */
   tcsetattr(fdesc, TCSAFLUSH, &term);
   return (FALSE);
}

/****************************** Parity ********************************/
int SerialParity (int fdesc, int parity) { 
   int err=0;
   struct termios term;
   char *prefix = "/dev/term/";

   /* Get the serial port's attributes... */
   if (SER_VERBOSE) printf("Setting the serial port's parity.\n");
   if (tcgetattr(fdesc, &term) == -1) {
      if (SER_VERBOSE) gprint (GP_ERR, "Port is not a tty\n");
      return PORTERR;
   }

   if (parity == 0)              /* Turn off parity generation... */
      term.c_cflag &= ~PARENB;     
   else if (parity == 1) {            
      term.c_cflag |= PARENB;     /* Turn on parity generation... */
      term.c_cflag |= PARODD;     /* Sets parity to odd */ 
   }	 
   else if (parity == 2)         /* Turn on parity generation... */
      term.c_cflag |= PARENB;     /* Defaults parity to even */ 
   else 
      printf ("ERROR:  Unknown parity specification\n");
 
   /* Set port (terminal) to reflect the change...(flush first) */
   tcsetattr(fdesc, TCSAFLUSH, &term);
   return (FALSE);
}

/****************************Data Bit Size ***************************/
int SerialDataBits(int fdesc, int bits) { 
   int err=0;
   struct termios term;

   /* Get the serial port's attributes... */
   if (SER_VERBOSE) printf("Setting the serial port's data bit size.\n");
   if (tcgetattr(fdesc, &term) == -1) {
      if (SER_VERBOSE) gprint (GP_ERR, "Port is not a tty\n");
      return PORTERR;
   }

   switch (bits) {
   case 5:  			/* Sets data bits to 5 */
      term.c_cflag &= ~CSIZE; 
      term.c_cflag |= CS5;
      break; 
   case 6:   			/* Sets data bits to 6 */ 
      term.c_cflag &= ~CSIZE; 
      term.c_cflag |= CS6;
      break; 
   case 7:     		/* Sets data bits to 7 */ 
      term.c_cflag &= ~CSIZE; 
      term.c_cflag |= CS7;
      break; 
   case 8:  			/* Sets data bits to 8 */
      term.c_cflag &= ~CSIZE; 
      term.c_cflag |= CS8;
      break; 
   default:
      printf ("ERROR:  Illegal data bit size\n");
      break; 
   }

   /* Set port (terminal) to reflect the change...(flush first) */
   tcsetattr(fdesc, TCSAFLUSH, &term);
   return (FALSE);
}

/****************************** Stop Bit ****************************/
int SerialStopBit(int fdesc, int stpbit) { 
   int err=0;
   struct termios term;

   /* Get the serial port's attributes... */
   if (SER_VERBOSE) printf("Setting the serial port's stop bit.\n");
   if (tcgetattr(fdesc, &term) == -1) {
      if (SER_VERBOSE) gprint (GP_ERR, "Port is not a tty\n");
      return PORTERR;
   }

   if (stpbit == 1) {     		/* Sets stop bit to 1 */  
      if (term.c_cflag & CSTOPB) term.c_cflag |= CSTOPB;
   } else { 				/* Else stop bit to 2 */ 
      term.c_cflag & CSTOPB;
   }

   /* Set port (terminal) to reflect the change...(flush first) */
   tcsetattr(fdesc, TCSAFLUSH, &term);
   return (FALSE);
}

/**************************** Stop ***********************************/
void SerialStop (int fdesc) {
   if (SER_VERBOSE) printf("Closing the serial port.\n");
   close(fdesc); /* Close up shop... */
}

# define D_NREAD 1024

/* send a string to the serial port, wait for an answer */
/* answer is returned on the pointer provided */

/**************************** Command ***********************************/
int SerialCommand (char *in, char **out, int wait) {
  
  int i, j;
  char *line;
  int done, Nread, Nin, NREAD;
  
  if (SER_ECHO) gprint (GP_ERR, "command: %s\n", in); 

  if (serial.f <= 0) {
    gprint (GP_ERR, "serial line closed\n"); 
    if (out != (char **) NULL) {
      *out = strcreate ("SERIAL OFF");
      /* return (char *) NULL instead? */
    }
    return (FALSE);
  }

  /* flush out the line */
  tcflush (serial.f, TCIOFLUSH);

# if (0)
  for (i = 0; i < strlen(in); i++) {
    gprint (GP_ERR, "%d %x\n", i, in[i]);
  }
# endif

  /* send command to serial line */
  Nin = write (serial.f, in, strlen(in));
  if (Nin != strlen(in)) {
    gprint (GP_ERR, "Serial Command not sent\n");
    return (FALSE);
   }
  usleep (20000);
  /* LX200 GPS requires some lead time (10msec) to check ready state */
  
  /* create space to store answer */
  NREAD = D_NREAD;
  ALLOCATE (line, char, NREAD);
  bzero (line, NREAD);
  Nread = Nin = 0;

  /* read data back from serial line until no response (Nin == 0) 
     or timeout (Nin == -1) && (i == wait) */

  done = FALSE;
  for (i = 0; !done && (i < wait); i++) {
    Nin = read (serial.f, &line[Nread], 256);
# if (SER_DEBUG)
    gprint (GP_ERR, "%d ", Nin);
# endif
    if (Nin < 0) { /* error, check value */
      gprint (GP_ERR, "error?");
      continue;
    }
    if (Nin > 0) {
      Nread += Nin;
      line[Nread] = 0;
      i = 0;
    }
    if (Nread > D_NREAD - 257) {
      NREAD += D_NREAD;
      REALLOCATE (line, char, NREAD);
    }
    if ((i > 0) && (Nin == 0)) done = TRUE;
    usleep (2000);
  }

  if (SER_ECHO) gprint (GP_ERR, "answer: %s\n", line);

  if (out == (char **) NULL) {
    free (line);
  } else {
    *out = line;
  }
  return (TRUE);
}


/* raw:
   cfmakeraw sets the terminal attributes as follows:
   termios_p->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
   termios_p->c_oflag &= ~OPOST;
   termios_p->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
   termios_p->c_cflag &= ~(CSIZE|PARENB);
   termios_p->c_cflag |= CS8;

   there conditions are turned off:
   c_iflag:
       IGNBRK ignore BREAK condition on input
       BRKINT If IGNBRK is not set, generate SIGINT on BREAK condition, else read BREAK as
              character \0.
       PARMRK if  IGNPAR  is  not  set,  prefix a character with a parity error or framing
              error with \377 \0.  If neither IGNPAR nor PARMRK is set, read  a  character
              with a parity error or framing error as \0.
       (IGNPAR ignore framing errors and parity errors.)

       ISTRIP strip off eighth bit
       INLCR  translate NL to CR on input
       IGNCR  ignore carriage return on input
       ICRNL  translate carriage return to newline on input (unless IGNCR is set)
       IXON   enable XON/XOFF flow control on output

   c_oflag:
       OPOST  enable implementation-defined output processing

   c_lflag:
       ECHO   echo input characters.
       ECHONL if ICANON is also set, echo the NL character even if ECHO is not set.
       ICANON enable canonical mode.  This enables the special characters EOF, EOL,  EOL2,
              ERASE, KILL, REPRINT, STATUS, and WERASE, and buffers by lines.
       ISIG   when any of the characters INTR, QUIT, SUSP, or DSUSP are received, generate
              the corresponding signal.
       IEXTEN enable implementation-defined input processing.

   c_cflag:
       CSIZE  character size mask.  Values are CS5, CS6, CS7, or CS8. (CS8 set).
       PARENB enable parity generation on output and parity checking for input.

*/
