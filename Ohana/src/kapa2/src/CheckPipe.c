# include "Ximage.h"
# include <errno.h>
# define STRCONST(A) ((int)(0x1000000*A[0] + 0x10000*A[1] + 0x100*A[2] + 0x1*A[3]))

static KapaSockAddress Address;
static int InitSocket = -1;
static int sock = -1;

// we can supply a port here, with only small changes
void InitPipe (char *namedSocket) {

  if (namedSocket == NULL) {
    InitSocket = KapaServerInit (&Address);
  } else {
    sock = KapaWaitNamedSocket (namedSocket);
  }
  return;
}

int GetActiveSocket () {
  return (sock);
}

// after we have processed the command, we unblock the socket
# define FINISHED(A) { fcntl (sock, F_SETFL, O_NONBLOCK); return (A); }

int CheckPipe () {

  int status;
  char word[5];

  // check if we have a valid connection. if not, see if we can get one
  if (sock == -1) {
    sock = KapaServerWait (InitSocket, &Address);
    if (sock == -1) return (TRUE);
    close (InitSocket); /* stop listening for new connections */
    fcntl (sock, F_SETFL, O_NONBLOCK);  
  }

  /***** read (4 byte) message word from socket ****/
  status = read (sock, word, 4);
  word[4] = 0;
  switch (status) {
  case -1:
    if (errno == EAGAIN) {
      /* no input from pipe: continue */
      return (TRUE);
    } 
    perror ("exiting due to problem with socket connection in CheckPipe");
    return (FALSE);
    break;

  case 0:
    fprintf (stderr, "pipe has died!\n");
    return (FALSE);
    break;

  case 4:
    break;

  default:
    fprintf (stderr, "weird signal: too many or few bytes!  %d\n", status);
    return (TRUE);
    break;
  }
  
  /* once we get a command, we block to ensure we get complete messages */
  fcntl (sock, F_SETFL, !O_NONBLOCK);  

  /***** handle different messages ****/
  if (ACTIVE_CURSOR) {
    if (strcmp (word, "NCUR")) {
      fprintf (stderr, "wrong end message %s\n", word);
      KiiSendCommand (sock, 4, "DONE");
      FINISHED (TRUE);
    }
    ACTIVE_CURSOR = FALSE;
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  } 

  if (!strcmp (word, "QUIT")) FINISHED (FALSE);
  
  if (!strcmp (word, "CURS")) {
    ACTIVE_CURSOR = TRUE;
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "PSIT")) {
    status = PScommand (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }
  
  if (!strcmp (word, "PDFT")) {
    status = PDFcommand (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }
  
  if (!strcmp (word, "PNGF")) {
    status = PNGcommand (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }
  
  if (!strcmp (word, "JPEG")) {
    status = JPEGcommand (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  if (!strcmp (word, "PPMF")) {
    status = PPMit (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }
  
  if (!strcmp (word, "DBOX")) {
    status = LoadFrame (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }
  
  if (!strcmp (word, "PLOT")) {
    status = LoadObject (sock);
    // LoadObject sends its own handshake
    // KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }
  
  if (!strcmp (word, "LABL")) {
    status = LoadLabels (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "PTXT")) {
    status = LoadTextlines (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "RSIZ")) {
    status = Resize (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  if (!strcmp (word, "ISIZ")) {
    status = ResizeByImage (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "MOVE")) {
    status = Relocate (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  if (!strcmp (word, "GLIM")) {
    GetLimits (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "SLIM")) {
    status = SetLimits (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "GSTY")) {
    GetGraphData (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "SSTY")) {
    status = SetGraphData (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "SIMC")) {
    SetImageCoords (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "GIMC")) {
    GetImageCoords (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "GIMR")) {
    GetImageRange (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "GIMD")) {
    GetImageData (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "SIMD")) {
    status = SetImageData (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "TOOL")) {
    SetToolbox (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "SSEC")) {
    status = SetSection (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "LSEC")) {
    status = ListSection (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "DSEC")) {
    status = DefineSection (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "ISEC")) {
    status = DefineSectionByImage (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "MSEC")) {
    status = MoveSection (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "BSEC")) {
    status = SetSectionBG (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  if (!strcmp (word, "FONT")) {
    status = SetFont (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }
  
  /* Erase Section */
  if (!strcmp (word, "ERSC")) {
    status = EraseCurrentPlot ();
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }
  
  /* Erase Plots */
  if (!strcmp (word, "ERSP")) {
    status = ErasePlots ();
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }
  
  /* Erase Sections */
  if (!strcmp (word, "ERSS")) {
    status = EraseSections ();
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  /* Erase Image */
  if (!strcmp (word, "ERSI")) {
    status = EraseImage ();
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  /* Erase Overlay for this section */
  if (!strcmp (word, "ERSO")) {
    status = EraseOverlay (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  if (!strcmp (word, "READ")) {
    status = LoadPicture (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  if (!strcmp (word, "LOAD")) {
    status = LoadOverlay (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  if (!strcmp (word, "CHAN")) {
    status = SetChannel (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  if (!strcmp (word, "CMAP")) {
    status = SetColormapFromPipe (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  if (!strcmp (word, "CNAN")) {
    status = SetNanColorFromPipe (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  if (!strcmp (word, "SAVE")) {
    status = SaveOverlay (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  if (!strcmp (word, "CSVE")) {
    status = CSaveOverlay (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  if (!strcmp (word, "CENT")) {
    status = Center (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  if (!strcmp (word, "PARI")) {
    status = Parity (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (status);
  }

  if (!strcmp (word, "NPIX")) {
    GetPixelCount (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }

  if (!strcmp (word, "SIGM")) {
    SetSmoothSigma (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }

  if (!strcmp (word, "MEMD")) {
    MemoryDump (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }

  if (!strcmp (word, "MEML")) {
    MemoryDumpLines (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }

  if (!strcmp (word, "MEMX")) {
    MemoryDumpOnExit (sock);
    KiiSendCommand (sock, 4, "DONE");
    FINISHED (TRUE);
  }

  fprintf (stderr, "unknown signal %s\n", word);
  KiiSendCommand (sock, 4, "DONE");
  FINISHED (TRUE);
}
