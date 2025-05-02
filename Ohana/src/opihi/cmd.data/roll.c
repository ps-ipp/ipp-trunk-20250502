# include "data.h"

int roll (int argc, char **argv) {
  
  Buffer *buf;

  // this function is probably not needed
  gprint (GP_ERR, "ERROR: use 'shift' instead of 'roll'\n");
  return (FALSE);

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: roll <buffer> dx dy\n");
    return (FALSE);
  }

  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  int dx = atoi (argv[2]);
  int dy = atoi (argv[3]);

  /* if (dx,dy < 0), we are moving the start position backwards,
     if (dx,dy > 0), we are moving the start position forward */
  
  int Nx = buf[0].matrix.Naxis[0];
  int Ny = buf[0].matrix.Naxis[1];

  if (dy == 0) {
    // moves in just dx can be done row-by-row

    int dX = abs(dx);
    int Nbytes = (Nx * Ny - dX) * sizeof (float);
    int Nextra = dX * sizeof (float);

    if (dx < 0) {
      memmove (buf[0].matrix.buffer, &buf[0].matrix.buffer[dX*sizeof(float)], Nbytes);
      memset (&buf[0].matrix.buffer[Nbytes], 0, Nextra);
    } else {
      memmove (&buf[0].matrix.buffer[dX*sizeof(float)], buf[0].matrix.buffer, Nbytes);
      memset (buf[0].matrix.buffer, 0, Nextra);
    }

    return TRUE;
  }

  if (dx == 0) {
    // moves in just dy can be done row-by-row

    ALLOCATE_PTR (output, float, Nx*Ny);

    float *input = (float *) buf[0].matrix.buffer;

    for (int iy = 0; iy < Ny; iy++) {
      if (iy + dy < 0) continue;
      if (iy + dy >= Ny) continue;
      memcpy (&output[iy*Nx], &input[(iy + dy)*Nx], Nx);
    }
    if (dy > 0) {
      memset (output, 0, dy*Nx*sizeof(float));
    } else {
      int Nbytes = Nx * (Ny - abs(dy)) * sizeof (float);
      int Nextra = abs(dy) * Nx * sizeof (float);
      memset (&output[Nbytes], 0, Nextra);
    }
    free (buf[0].matrix.buffer);
    buf[0].matrix.buffer = (char *) output;
    return TRUE;
  }

  ALLOCATE_PTR (output, float, Nx*Ny);
  float *input = (float *) buf[0].matrix.buffer;

  for (int iy = 0; iy < Ny; iy++) {
    if (iy + dy < 0) continue;
    if (iy + dy >= Ny) continue;
    for (int ix = 0; ix < Nx; ix++) {
      if (ix + dx < 0) continue;
      if (ix + dx >= Nx) continue;
      output[ix + iy*Nx] = input[(ix + dx) + (iy + dy)*Nx];
    }
  }
  free (buf[0].matrix.buffer);
  buf[0].matrix.buffer = (char *) output;
  return TRUE;
}


