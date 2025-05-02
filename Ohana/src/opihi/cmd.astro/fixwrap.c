# include "astro.h"

int fixwrap (int argc, char **argv) {
  
  int i, j, Nflip, n, Ny, Nx, flip, sat, rowfix;
  float *Vin, *outf, *outb, dO;
  Buffer *in;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: fixwrap <in> (rowfix)\n");
    return (FALSE);
  }

  if ((in = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);
  rowfix = atoi (argv[2]);

  Nx = in[0].matrix.Naxis[0];
  Ny = in[0].matrix.Naxis[1];

  ALLOCATE (outf, float, MAX(Nx, Ny));
  ALLOCATE (outb, float, MAX(Nx, Ny));

  for (j = 0; j < Ny; j++) {
    Vin  = (float *)(in[0].matrix.buffer)  + j*Nx;

    /* measure forward flips */ 
    sat = FALSE;
    for (i = 0; i < Nx; i++) {
      if ((i < 1056) || (i > 2079)) {
	outf[i] = Vin[i];
	continue;
      }

      dO = 2*outf[i-1] - outf[i-2] - Vin[i];
      flip = (fabs(dO - 0x8000) < fabs(dO));

      /* going onto saturation */
      if (!sat && (Vin[i] > 32766.5) && (Vin[i+1]  > 32766.5)) sat  = TRUE;
      if (!sat && (Vin[i] > 32766.5) && (Vin[i-Nx] > 65534.5)) sat  = TRUE;

      /* exiting saturation region */
      if ( sat && (Vin[i] < 32766.5) && (Vin[i-1] < 32766.5)) sat = FALSE;

      if (sat) flip = TRUE;

      outf[i] = flip ? (Vin[i] + 0x8000) : Vin[i];
    }

    /* measure backward flips */ 
    sat = FALSE;
    for (i = Nx - 1; i >= 0; i--) {
      if ((i < 1056) || (i > 2077)) {
	outb[i] = Vin[i];
	continue;
      }

      dO = 2*outb[i+1] - outb[i+2] - Vin[i];
      flip = (fabs(dO - 0x8000) < fabs(dO));

      /* going onto saturation */
      if (!sat && (Vin[i] > 32766.5) && (Vin[i-1] > 32766.5)) sat  = TRUE;

      /* exiting saturation region */
      if ( sat && (Vin[i] < 32766.5) && (Vin[i+1] < 32766.5)) sat = FALSE;

      if (sat) flip = TRUE;

      outb[i] = flip ? (Vin[i] + 0x8000) : Vin[i];
    }

    /* compare forward and backward flips: where they disagree, use column to predict */
    for (i = 0; (j > 1) && (i < Nx); i++) {
      if ((i < 1056) || (i > 2077)) continue;
      if (outf[i] != outb[i]) {
	/* use this column to predict, not the row */
	dO = 2*Vin[i - Nx] - Vin[i-2*Nx] - Vin[i];
	flip = (fabs(dO - 0x8000) < fabs(dO));
	outf[i] = flip ? Vin[i] + 0x8000 : Vin[i];
	outb[i] = outf[i];  /* save this for the row segments below */
      }
    }

    /* compare this row and previous (now fixed) row. if large segments are flipped, fix them */
    for (i = 0; rowfix && (j > 1) && (i < Nx); i++) {
      if ((i < 1056) || (i > 2077)) continue;

      if (fabs(outf[i] - Vin[i-Nx]) > 15000) {
	Nflip = 0;
	for (n = i - 8; n < i + 9; n++) {
	  if (fabs(outb[n] - Vin[n-Nx]) > 15000) {
	    Nflip ++;
	  }
	}
	if (Nflip > 5) {
	  if (outf[i] - Vin[i-Nx] > 15000) {
	    outf[i] -= 0x8000;
	  } else {
	    outf[i] += 0x8000;
	  }	    
	}
      }
    }
    for (i = 0; i < Nx; i++) {
      Vin[i] = outf[i];
    }
  }

  return (TRUE);
}

