# include "data.h"

/*
  static float at,bt,ct;
  #define PYTHAG(a,b) ((at=fabs(a)) > (bt=fabs(b)) ? \
  (ct=bt/at,at*sqrt(1.0+ct*ct)) : (bt ? (ct=at/bt,bt*sqrt(1.0+ct*ct)): 0.0))
  pythag -> hypot (check on roundoff errors) 
*/

/* use simple max? */
#define FSIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

/* n == Nx, m == Ny */
int svdcmp (float *a, opihi_flt *w, float *v, int Nx, int Ny) {

  int flag, i, its, j, jj, k, l, nm;
  float c, f, h, s, x, y, z;
  float anorm=0.0, g = 0.0, scale = 0.0;
  float *rv1;

  if (Ny < Nx) return (0);

  l = nm = 0;
  ALLOCATE (rv1, float, Nx);

  for (i = 0; i < Nx; i++) {
    l = i + 1;
    rv1[i] = scale*g;
    g = s = scale = 0.0;
    if (i < Ny) {
      for (k = i; k < Ny; k++) scale += fabs(a[k*Nx + i]);
      if (scale > 0.0) {
	for (k = i; k < Ny; k++) {
	  a[k*Nx + i] /= scale;
	  s += a[k*Nx + i]*a[k*Nx + i];
	}
	f = a[i*Nx + i];
	g = sqrt(s);
	if (f > 0.0) { g = -g; }
	h = f*g - s;
	a[i*Nx + i] = f-g;
	if (i != Nx - 1) {
	  for (j = l; j < Nx; j++) {
	    s = 0.0;
	    for (k = i; k < Ny; k++) { s += a[k*Nx + i]*a[k*Nx + j]; }
	    f = s/h;
	    for (k = i; k < Ny; k++) { a[k*Nx + j] += f*a[k*Nx + i]; }
	  }
	}
	for (k = i; k < Ny; k++) { a[k*Nx + i] *= scale; }
      }
    }
    w[i] = scale*g;
    g = s = scale = 0.0;
    if ((i < Ny) && (i != (Nx - 1))) {
      for (k = l; k < Nx; k++) { scale += fabs(a[i*Nx + k]); }
      if (scale > 0.0) {
	for (k = l; k < Nx; k++) {
	  a[i*Nx + k] /= scale;
	  s += a[i*Nx + k]*a[i*Nx + k];
	}
	f = a[i*Nx + l];
	g = sqrt(s);
	if (f > 0.0) { g = -g; }
	h = f*g - s;
	a[i*Nx + l] = f-g;
	for (k = l; k < Nx; k++) { rv1[k] = a[i*Nx + k]/h; }
	if (i != Ny - 1) {
	  for (j = l; j < Ny; j++) {
	    s = 0.0;
	    for (k = l; k < Nx; k++) { s += a[j*Nx + k]*a[i*Nx + k];}
	    for (k = l; k < Nx; k++) { a[j*Nx + k] += s*rv1[k]; } 
	  }
	}
	for (k = l; k < Nx; k++) { a[i*Nx + k] *= scale; }
      }
    }
    anorm = MAX(anorm, (fabs(w[i]) + fabs(rv1[i])));
  }

  for (i = Nx - 1; i >= 0; i--) {
    if (i < Nx - 1) {
      if (g != 0.0) {
	/* isn't l == n to start?? */
	for (j = l; j < Nx; j++) v[j*Nx + i] = (a[i*Nx + j]/a[i*Nx + l])/g;
	for (j = l; j < Nx; j++) {
	  s = 0.0;
	  for (k = l; k < Nx; k++) s += a[i*Nx + k]*v[k*Nx + j];
	  for (k = l; k < Nx; k++) v[k*Nx + j] += s*v[k*Nx + i];
	}
      }
      for (j = l; j < Nx; j++) { v[i*Nx + j] = v[j*Nx + i] = 0.0; }
    }
    v[i*Nx + i] = 1.0;
    g = rv1[i];
    l = i;
  }
  for (i = Nx - 1; i >= 0; i--) {
    l = i + 1;
    g = w[i];
    if (i < Nx - 1) { for (j = l; j < Nx; j++) {a[i*Nx + j] = 0.0; }}
    if (g != 0.0) {
      g = 1.0/g;
      if (i != Nx - 1) {
	for (j = l; j < Nx; j++) {
	  s = 0.0;
	  for (k = l; k < Ny; k++) { s += a[k*Nx + i]*a[k*Nx + j];}
	  f = (s/a[i*Nx + i])*g;
	  for (k = i; k < Ny; k++) { a[k*Nx + j] += f*a[k*Nx + i]; }
	}
      }
      for (j = i; j < Ny; j++) { a[j*Nx + i] *= g; }
    } else {
      for (j = i; j < Ny; j++) { a[j*Nx + i] = 0.0; }
    }
    a[i*Nx + i] ++;
  }

  // int status = 1;
  for (k = Nx - 1; k >= 0; k--) {
    for (its = 0; its < 30; its++) {
      flag = 1;
      for (l = k; l >= 0; l--) {
	nm = l - 1;
	if (fabs(rv1[l])+anorm == anorm) {
	  flag = 0;
	  break;
	}
	if (fabs(w[nm])+anorm == anorm) break;
      }
      if (flag) {
	c = 0.0;
	s = 1.0;
	for (i = l; i <= k; i++) {
	  f = s*rv1[i];
	  if (fabs(f)+anorm != anorm) {
	    g = w[i];
	    h = hypot (f, g);
	    w[i] = h;
	    h = 1.0/h;
	    c = g*h;
	    s = (-f*h);
	    for (j = 0; j < Ny; j++) {
	      y = a[j*Nx + nm];
	      z = a[j*Nx + i];
	      a[j*Nx + nm] = y*c + z*s;
	      a[j*Nx + i] = z*c - y*s;
	    }
	  }
	}
      }
      z = w[k];
      if (l == k) {
	if (z < 0.0) {
	  w[k] = -z;
	  for (j = 0; j < Nx; j++) v[j*Nx + k] = (-v[j*Nx + k]);
	}
	break;
      }
      // if (its == 29) status = 0;
      x = w[l];
      nm = k-1;
      y = w[nm];
      g = rv1[nm];
      h = rv1[k];
      f = ((y-z)*(y+z) + (g-h)*(g+h))/(2.0*h*y);
      g = hypot (f, 1.0);
      
      float tmp = g;
      if (f < 0.0) { tmp = - tmp; }

      f = ((x-z)*(x+z) + h*((y/(f+tmp))-h))/x;

      c = s = 1.0;
      for (j = l; j <= nm; j++) {
	i = j+1;
	g = rv1[i];
	y = w[i];
	h = s*g;
	g = c*g;
	z = hypot (f, h);
	rv1[j] = z;
	c = f/z;
	s = h/z;
	f = x*c+g*s;
	g = g*c-x*s;
	h = y*s;
	y = y*c;
	for (jj = 0; jj < Nx; jj++) {
	  x = v[jj*Nx + j];
	  z = v[jj*Nx + i];
	  v[jj*Nx + j] = x*c+z*s;
	  v[jj*Nx + i] = z*c-x*s;
	}
	z = hypot(f, h);
	w[j] = z;
	if (z != 0.0) {
	  z = 1.0/z;
	  c = f*z;
	  s = h*z;
	}
	f = (c*g) + (s*y);
	x = (c*y) - (s*g);
	for (jj = 0; jj < Ny; jj++) {
	  y = a[jj*Nx + j];
	  z = a[jj*Nx + i];
	  a[jj*Nx + j] = y*c+z*s;
	  a[jj*Nx + i] = z*c-y*s;
	}
      }
      rv1[l] = 0.0;
      rv1[k] = f;
      w[k] = x;
    }
  }
  free(rv1);
  return (1);
}

#undef FSIGN
#undef MAX
#undef PYTHAG
