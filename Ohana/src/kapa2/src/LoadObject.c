# include "Ximage.h"
# include <errno.h>
# define DEBUG 0

int LoadObject (int sock) {
  
  int N;
  Section *section;
  KapaGraphWidget *graph;

  section = GetActiveSection();
  if (section->graph == NULL) {
    section->graph = InitGraph ();
    SetSectionSizes (section);
  }
  graph = section->graph;
  graph[0].haveGraph = TRUE;
  
  N = graph[0].Nobjects;
  graph[0].Nobjects ++;
  REALLOCATE (graph[0].objects, Gobjects, graph[0].Nobjects);
  graph[0].objects[N].x = graph[0].objects[N].y = graph[0].objects[N].z = (float *) NULL;
  graph[0].objects[N].dxm = graph[0].objects[N].dxp = (float *) NULL;
  graph[0].objects[N].dym = graph[0].objects[N].dyp = (float *) NULL;
  
  KiiScanMessage (sock, "%d %d %d %d %d %d %d %lf %lf %lf",
		  &graph[0].objects[N].Npts, &graph[0].objects[N].style, 
		  &graph[0].objects[N].ptype, &graph[0].objects[N].ltype, 
		  &graph[0].objects[N].etype, &graph[0].objects[N].ebar, 
		  &graph[0].objects[N].color, &graph[0].objects[N].alpha, 
		  &graph[0].objects[N].lweight, &graph[0].objects[N].size);
  
  if (DEBUG) fprintf (stderr, "%d %d %d %d %d %d %d %lf %lf\n",
		      graph[0].objects[N].Npts, graph[0].objects[N].style, 
		      graph[0].objects[N].ptype, graph[0].objects[N].ltype, 
		      graph[0].objects[N].etype, graph[0].objects[N].ebar, 
		      graph[0].objects[N].color, 
		      graph[0].objects[N].lweight, graph[0].objects[N].size);
  
  // limit color ranges (negative is allowed for ptype == 2 --> color scaled by z
  if (graph[0].objects[N].color >= KapaColormapSize()) {
      graph[0].objects[N].color = KapaColormapSize() - 1;
  }

  // XXX watch out for this restriction in DrawObjects / bDrawObjects
  int canScaleColor = FALSE;
  canScaleColor |= (graph[0].objects[N].style == KAPA_PLOT_POINTS);
  canScaleColor |= (graph[0].objects[N].style == KAPA_PLOT_POLYFILL);

  if (!canScaleColor && (graph[0].objects[N].color < 0)) {
      graph[0].objects[N].color = 0;
  }

  KiiScanMessage (sock, "%lf %lf %lf %lf",
		  &graph[0].objects[N].x0, &graph[0].objects[N].x1, 
		  &graph[0].objects[N].y0, &graph[0].objects[N].y1);

  // acknowledge receipt of the metadata
  KiiSendCommand (sock, 4, "DONE"); 
  
  // XXX Currently, I require these in a special order.  The data includes a message defining the
  // object type.  This could be made more flexible by using the information (though we still need
  // to know how many items will be sent)

  if (!LoadVectorData (sock, graph, N, "x")) {
    FreeObjectData (&graph[0].objects[N]);
    graph[0].Nobjects --;
    REALLOCATE (graph[0].objects, Gobjects, MAX (1, graph[0].Nobjects));
  }
    
  if (!LoadVectorData (sock, graph, N, "y")) {
    FreeObjectData (&graph[0].objects[N]);
    graph[0].Nobjects --;
    REALLOCATE (graph[0].objects, Gobjects, MAX (1, graph[0].Nobjects));
  }
  if ((graph[0].objects[N].size < 0.0) || (graph[0].objects[N].color < 0.0)) {
    if (!LoadVectorData (sock, graph, N, "z")) {
      FreeObjectData (&graph[0].objects[N]);
      graph[0].Nobjects --;
      REALLOCATE (graph[0].objects, Gobjects, MAX (1, graph[0].Nobjects));
    }
  }
  if (graph[0].objects[N].etype & 0x01) {
    if (!LoadVectorData (sock, graph, N, "dym")) {
      FreeObjectData (&graph[0].objects[N]);
      graph[0].Nobjects --;
      REALLOCATE (graph[0].objects, Gobjects, MAX (1, graph[0].Nobjects));
    }
    if (!LoadVectorData (sock, graph, N, "dyp")) {
      FreeObjectData (&graph[0].objects[N]);
      graph[0].Nobjects --;
      REALLOCATE (graph[0].objects, Gobjects, MAX (1, graph[0].Nobjects));
    }
  }
  if (graph[0].objects[N].etype & 0x02) {
    if (!LoadVectorData (sock, graph, N, "dxm")) {
      FreeObjectData (&graph[0].objects[N]);
      graph[0].Nobjects --;
      REALLOCATE (graph[0].objects, Gobjects, MAX (1, graph[0].Nobjects));
    }
    if (!LoadVectorData (sock, graph, N, "dxp")) {
      FreeObjectData (&graph[0].objects[N]);
      graph[0].Nobjects --;
      REALLOCATE (graph[0].objects, Gobjects, MAX (1, graph[0].Nobjects));
    }
  }

  if (DEBUG) fprintf (stderr, "loaded %d objects, using object %d\n", graph[0].objects[N].Npts, N);

  if (USE_XWINDOW) {
    if (0) {
      // use this if we are not using buffered plotting
      Graphic *graphic = GetGraphic();
      DrawObjectN (graphic, graph, &graph[0].objects[graph[0].Nobjects-1]);
      FlushDisplay ();
    } else {
      Refresh ();
    }
  }

  return (TRUE);
  
}

int LoadVectorData (int sock, KapaGraphWidget *graph, int N, char *type) {
  
  int i, Npts, Ninpts, status, Ntry;
  int bytes_left;
  char *byte, *buffer, type_send[16], tmp;
  int Npts_send, Nbytes_send, swap_client, swap_host;

  buffer = NULL;
  Npts = graph[0].objects[N].Npts;

  KiiWaitAnswer (sock, "PLOB");
  KiiScanMessage (sock, "%s %d %d %d", type_send, &Npts_send, &Nbytes_send, &swap_client);
  if (strcmp (type, type_send)) {
    fprintf (stderr, "Kapa Communication error: unexpected data type %s vs %s\n", type_send, type);
  }
  if (Npts_send != Npts) {
    fprintf (stderr, "Kapa Communication error: unexpected number of points %d vs %d\n", Npts_send, Npts);
  }
  if (Nbytes_send != Npts_send*sizeof(float)) {
    fprintf (stderr, "Kapa Communication error: unexpected data size %d vs %ld\n", Nbytes_send, (long) Npts_send*sizeof(float));
  }

  status = 1;
  if (!strcmp (type, "x")) {
    ALLOCATE (graph[0].objects[N].x, float, MAX (1, Npts));
    buffer = (char *) graph[0].objects[N].x;
  }
  if (!strcmp (type, "y")) {
    ALLOCATE (graph[0].objects[N].y, float, MAX (1, Npts));
    buffer = (char *) graph[0].objects[N].y;
  }
  if (!strcmp (type, "z")) {
    ALLOCATE (graph[0].objects[N].z, float, MAX (1, Npts));
    buffer = (char *) graph[0].objects[N].z;
  }
  if (!strcmp (type, "dxm")) {
    ALLOCATE (graph[0].objects[N].dxm, float, MAX (1, Npts));
    buffer = (char *) graph[0].objects[N].dxm;
  }
  if (!strcmp (type, "dxp")) {
    ALLOCATE (graph[0].objects[N].dxp, float, MAX (1, Npts));
    buffer = (char *) graph[0].objects[N].dxp;
  }
  if (!strcmp (type, "dym")) {
    ALLOCATE (graph[0].objects[N].dym, float, MAX (1, Npts));
    buffer = (char *) graph[0].objects[N].dym;
  }
  if (!strcmp (type, "dyp")) {
    ALLOCATE (graph[0].objects[N].dyp, float, MAX (1, Npts));
    buffer = (char *) graph[0].objects[N].dyp;
  }

  bytes_left = Npts*sizeof (float);

  fcntl (sock, F_SETFL, O_NONBLOCK);  

  // read the vector data as raw binary in client machine byte order (floats)
  Ntry = 0;
  if (DEBUG) fprintf (stderr, "starting vector load\n");
  Ninpts = 0;
  byte = buffer;
  while (bytes_left > 0) {
    status = read (sock, byte, bytes_left);
    if (DEBUG) fprintf (stderr, "status: %d, %d\n", status, bytes_left);
    if (status == 0) {  /* No more pipe */
      fprintf (stderr, "error: pipe closed\n");
      return (FALSE);
    }
    if (status != -1) { /* pipe has data */
      Ninpts += status;
      bytes_left -= status;
      byte = (char *)(byte + status);
      Ntry = 0;
      continue;
    }
    if (errno == EAGAIN) {
      Ntry ++;
      if (Ntry > 100) {
	fprintf (stderr, "kapa communication error\n");
	return (FALSE);
      }
      usleep (10000);
      continue;
    }
    perror ("kapa load");
  }

  fcntl (sock, F_SETFL, !O_NONBLOCK);  
  KiiSendCommand (sock, 4, "DONE"); 

# ifdef BYTE_SWAP
  swap_host = 1;
# else 
  swap_host = 0;
# endif  

  // if host and client have opposite swap parities, word swap the incoming data
  // SWAP_WORD is strangely defined... it takes the number of the start byte in a
  // buffer called 'byte'
  if ((swap_host && !swap_client) || (!swap_host && swap_client)) {
    byte = buffer;
    for (i = 0; i < Nbytes_send; i+=4) {
      SWAP_WORD (i);
    }
  }

  if (Ninpts != Npts*sizeof(float)) {  
    fprintf (stderr, "error: expected %d bytes, but got only %d\n", Ninpts, (unsigned int)(Npts*sizeof(float)));
    return (FALSE);
  }
  if (DEBUG) fprintf (stderr, "done vector load\n");
  return (TRUE);

}

void FreeObjectData (Gobjects *object) {

  if (object[0].x != (float *) NULL) free (object[0].x);
  if (object[0].y != (float *) NULL) free (object[0].y);
  if (object[0].z != (float *) NULL) free (object[0].z);

  if (object[0].dxm != (float *) NULL) free (object[0].dxm);
  if (object[0].dxp != (float *) NULL) free (object[0].dxp);
  if (object[0].dym != (float *) NULL) free (object[0].dym);
  if (object[0].dyp != (float *) NULL) free (object[0].dyp);
}
