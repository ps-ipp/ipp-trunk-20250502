# include "mosastro.h"

/* measure local gradient vs field position : dP, dQ = f(L,M) 
   gradient is measured in small boxes across the field */
Gradients *GetGradients () {

  int i, j, nx, ny, Nval, Nx, Ny, Npts, DX, DY;
  double **a, **b;
  double L, M, dP, dQ;
  double Xmin, Xmax, Ymin, Ymax;

  Gradients *grad;

  ALLOCATE (grad, Gradients, 1);

  /** this should not be hard-wired **/
  Nx = 2;
  Ny = 2;

  ALLOCATE (grad[0].dPdL, double, Nchip*Nx*Ny);
  ALLOCATE (grad[0].dPdM, double, Nchip*Nx*Ny);
  ALLOCATE (grad[0].dQdL, double, Nchip*Nx*Ny);
  ALLOCATE (grad[0].dQdM, double, Nchip*Nx*Ny);

  ALLOCATE (grad[0].Lo, double, Nchip*Nx*Ny);
  ALLOCATE (grad[0].Mo, double, Nchip*Nx*Ny);
  
  ALLOCATE (a, double *, 3);
  ALLOCATE (b, double *, 3);
  for (i = 0; i < 3; i++) {
    ALLOCATE (a[i], double, 3);
    ALLOCATE (b[i], double, 2);
  }

  Nval = 0;
  for (i = 0; i < Nchip; i++) {
    DX = chip[i].NX / Nx;
    DY = chip[i].NY / Ny;
    for (nx = 0; nx < Nx; nx++) {
      for (ny = 0; ny < Ny; ny++) {
	Xmin = nx*DX;
	Xmax = Xmin + DX;
	Ymin = ny*DY;
	Ymax = Ymin + DY;

	for (j = 0; j < 3; j++) {
	  bzero (a[j], 3*sizeof(double));
	  bzero (b[j], 2*sizeof(double));
	}
  
	/* find local gradient in limited box */
	for (j = 0; j < chip[i].Nmatch; j++) {
	  if (chip[i].raw[j].X < Xmin) continue;
	  if (chip[i].raw[j].X > Xmax) continue;
	  if (chip[i].raw[j].Y < Ymin) continue;
	  if (chip[i].raw[j].Y > Ymax) continue;
	  
	  L   = chip[i].ref[j].L;
	  M   = chip[i].ref[j].M;
	  dP  = chip[i].ref[j].P - chip[i].raw[j].P;
	  dQ  = chip[i].ref[j].Q - chip[i].raw[j].Q;
	  
	  a[0][0] += 1;
	  a[1][0] += L;
	  a[2][0] += M;
	  a[1][1] += L*L;
	  a[2][1] += L*M;
	  a[2][2] += M*M;

	  b[0][0] += dP;
	  b[1][0] += dP*L;
	  b[2][0] += dP*M;

	  b[0][1] += dQ;
	  b[1][1] += dQ*L;
	  b[2][1] += dQ*M;

	}
	/* fitting dP = a[0][0] + a[1][0]*L + a[2][0]*M
	           dQ = a[0][1] + a[1][1]*L + a[2][1]*M 
	*/
	a[0][1] = a[1][0];
	a[0][2] = a[2][0];
	a[1][2] = a[2][1];

	Npts = a[0][0];
	grad[0].Lo[Nval] = a[1][0] / a[0][0];
	grad[0].Mo[Nval] = a[2][0] / a[0][0];
	/* point-weighted average coordinate */

	if (Npts < 5) continue;
	if (!dgaussjordan (a, b, 3, 2)) continue;

	/* we only care about the slopes, not the offsets */
	grad[0].dPdL[Nval] = b[1][0];
	grad[0].dPdM[Nval] = b[2][0];
	grad[0].dQdL[Nval] = b[1][1];
	grad[0].dQdM[Nval] = b[2][1];

	Nval ++;

      }
    }
  }
  grad[0].Npts = Nval;
  if ((DUMP != NULL) && !strcmp (DUMP, "grads")) dump_grads (grad, "grads.dat");
  return (grad);
}
