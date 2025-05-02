# include "data.h"

int SetGridScales (double *major, double *minor, double range) {

  double lrange, factor, mantis, fmantis, power;

  lrange = log10(MAX(fabs(range), 1e-30));
  factor = (int) (lrange);
  if (lrange < 0) { factor -= 1; }
  mantis = lrange - factor;
  power = pow(10.0, factor);
  fmantis = pow(10.0, mantis);
  if ((fmantis >= 1.0) && (fmantis <=  2.0)) {
    *major = 0.5 * power;
    *minor = 0.1 * power;
  }
  if ((fmantis > 2.0) && (fmantis <=  4.0)) {
    *major = 1.0 * power;
    *minor = 0.2 * power;
  }
  if ((fmantis > 4.0) && (fmantis <=  6.0)) {
    *major = 1.0 * power;
    *minor = 0.5 * power;
  }
  if ((fmantis > 6.0) && (fmantis <=  10.0)) {
    *major = 2.0 * power;
    *minor = 0.5 * power;
  }
  return TRUE;
}

int grid (int argc, char **argv) {
  
  double range, major, minor, first, next;
  int j, kapa, N, MinorTick, MajorTick;
  Vector Xvec, Yvec;
  Graphdata graphmode;

  if (!style_args (&graphmode, &argc, argv, &kapa)) return (FALSE);

  MajorTick = TRUE;
  MinorTick = FALSE;
  if ((N = get_argument (argc, argv, "-all"))) {
    remove_argument (N, &argc, argv);
    MinorTick = TRUE;
  }

  if (argc > 1) {
    gprint (GP_ERR, "USAGE: grid [-n graph]\n");
    return (FALSE);
  }

  N = 0;
  SetVector (&Xvec, OPIHI_FLT, 200);
  SetVector (&Yvec, OPIHI_FLT, 200);

  major = minor = 1;
  range = graphmode.xmax - graphmode.xmin;
  SetGridScales (&major, &minor, range);
  if (graphmode.xmin > 0)
    first = minor + minor*((int)(graphmode.xmin/minor));
  else 
    first = -minor + minor*((int)(graphmode.xmin/minor));
  if (minor*((int)(graphmode.xmin/minor)) == graphmode.xmin) {
    first = graphmode.xmin;
  }
  
  for (j = 0, next = first; next <= graphmode.xmax; j++) {
    if ((fabs((int)(next/major) - (next/major)) < 0.5*(minor/major)) || (fabs ((int)((next + 0.5*minor)/major) - (next/major)) < 0.5*(minor/major))) {
      if (MajorTick) {
	/* major tick */
	Xvec.elements.Flt[N] = next;
	Yvec.elements.Flt[N] = graphmode.ymin;
	N++;
	if (N == Xvec.Nelements) {
	  Xvec.Nelements += 200;
	  Yvec.Nelements += 200;
	  REALLOCATE (Xvec.elements.Flt, opihi_flt, Xvec.Nelements);
	  REALLOCATE (Yvec.elements.Flt, opihi_flt, Yvec.Nelements);
	}
	Xvec.elements.Flt[N] = next;
	Yvec.elements.Flt[N] = graphmode.ymax;
	N++;
	if (N == Xvec.Nelements) {
	  Xvec.Nelements += 200;
	  Yvec.Nelements += 200;
	  REALLOCATE (Xvec.elements.Flt, opihi_flt, Xvec.Nelements);
	  REALLOCATE (Yvec.elements.Flt, opihi_flt, Yvec.Nelements);
	}
      }
    } else {
      if (MinorTick) {
	/* minor tick */
	Xvec.elements.Flt[N] = next;
	Yvec.elements.Flt[N] = graphmode.ymin;
	N++;
	if (N == Xvec.Nelements) {
	  Xvec.Nelements += 200;
	  Yvec.Nelements += 200;
	  REALLOCATE (Xvec.elements.Flt, opihi_flt, Xvec.Nelements);
	  REALLOCATE (Yvec.elements.Flt, opihi_flt, Yvec.Nelements);
	}
	Xvec.elements.Flt[N] = next;
	Yvec.elements.Flt[N] = graphmode.ymax;
	N++;
	if (N == Xvec.Nelements) {
	  Xvec.Nelements += 200;
	  Yvec.Nelements += 200;
	  REALLOCATE (Xvec.elements.Flt, opihi_flt, Xvec.Nelements);
	  REALLOCATE (Yvec.elements.Flt, opihi_flt, Yvec.Nelements);
	}
      }
    }
    next += minor;
  }

  range = graphmode.ymax - graphmode.ymin;
  SetGridScales (&major, &minor, range);
  if (graphmode.ymin > 0)
    first = minor + minor*((int)(graphmode.ymin/minor));
  else 
    first = -minor + minor*((int)(graphmode.ymin/minor));
  if (minor*((int)(graphmode.ymin/minor)) == graphmode.ymin) {
    first = graphmode.ymin;
  }
  
  for (j = 0, next = first; next <= graphmode.ymax; j++) {
    if ((fabs((int)(next/major) - (next/major)) < 0.5*(minor/major)) || (fabs ((int)((next + 0.5*minor)/major) - (next/major)) < 0.5*(minor/major))) {
      if (MajorTick) {
	/* major tick */
	Xvec.elements.Flt[N] = graphmode.xmin;
	Yvec.elements.Flt[N] = next;
	N++;
	if (N == Xvec.Nelements) {
	  Xvec.Nelements += 200;
	  Yvec.Nelements += 200;
	  REALLOCATE (Xvec.elements.Flt, opihi_flt, Xvec.Nelements);
	  REALLOCATE (Yvec.elements.Flt, opihi_flt, Yvec.Nelements);
	}
	Xvec.elements.Flt[N] = graphmode.xmax;
	Yvec.elements.Flt[N] = next;
	N++;
	if (N == Xvec.Nelements) {
	  Xvec.Nelements += 200;
	  Yvec.Nelements += 200;
	  REALLOCATE (Xvec.elements.Flt, opihi_flt, Xvec.Nelements);
	  REALLOCATE (Yvec.elements.Flt, opihi_flt, Yvec.Nelements);
	}
      }
    } else {
      if (MinorTick) {
	/* minor tick */
	Xvec.elements.Flt[N] = graphmode.xmin;
	Yvec.elements.Flt[N] = next;
	N++;
	if (N == Xvec.Nelements) {
	  Xvec.Nelements += 200;
	  Yvec.Nelements += 200;
	  REALLOCATE (Xvec.elements.Flt, opihi_flt, Xvec.Nelements);
	  REALLOCATE (Yvec.elements.Flt, opihi_flt, Yvec.Nelements);
	}
	Xvec.elements.Flt[N] = graphmode.xmax;
	Yvec.elements.Flt[N] = next;
	N++;
	if (N == Xvec.Nelements) {
	  Xvec.Nelements += 200;
	  Yvec.Nelements += 200;
	  REALLOCATE (Xvec.elements.Flt, opihi_flt, Xvec.Nelements);
	  REALLOCATE (Yvec.elements.Flt, opihi_flt, Yvec.Nelements);
	}
      }
    }
    next += minor;
  }

  Xvec.Nelements = Yvec.Nelements = N;
  graphmode.style = KAPA_PLOT_POINTS; /* points */
  graphmode.ptype = KAPA_POINT_PAIR_CONNECT; /* connect pairs of points */
  graphmode.etype = 0;
  PlotVectorPair (kapa, &Xvec, &Yvec, NULL, &graphmode);

  free (Xvec.elements.Flt);
  free (Yvec.elements.Flt);

  return (TRUE);

}

