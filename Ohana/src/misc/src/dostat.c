# include <ohana.h>

main() {

  FILE *f, *fopen();
  char file[50];
  int N,n1,n2,n3,n4,n5,n6,n7,n8,nperf,type,status;
  double Fx,Fy,alpha, FWHMx,FWHMy,angle, ap, F, S, sky;
  double dtype, A, A2, Ap, Ap2, apmifit, df, S2;
  char line[512];

  while (fscanf (stdin, "%s", file) != EOF) {
    f = fopen (file, "r");
    if (f == NULL) {
      fprintf (stderr, "file %s not found\n", file);
      continue;
    }
    n1 = n2 = n3 = n4 = n5 = n6 = n7 = n8 = nperf = 0;
    S2 = S = Fx = Fy = alpha = A = A2 = 0;
    while (scan_line (f, line) != EOF) {
      status = TRUE;
      status &= dparse (&dtype, 2, line);
      status &= dparse (&df,   6, line);
      status &= dparse (&sky,   7, line);
      status &= dparse (&FWHMx, 8, line);
      status &= dparse (&FWHMy, 9, line);
      status &= dparse (&angle, 10, line);
      status &= dparse (&ap,      12, line);
      status &= dparse (&apmifit, 15, line);
      if (!status) {
	fprintf (stderr, "error in file %s, line %d\n", file, n1+n2+n3+n4+n5+n6+n7+n8);
	continue;
      }
      type = dtype;
      switch (type) {
      case 1:
	n1 ++;
	S += sky;
	if (ap < 99) {
	  nperf ++;
	  A += apmifit/(df*df);
	  A2 += apmifit*apmifit/(df*df);
	  S2 += 1.0/(df*df);
	}
	Fx = FWHMx;
	Fy = FWHMy;
	alpha = angle;
	break;
      case 2:
	n2 ++;
	break;
      case 3:
	n3 ++;
	break;
      case 4:
	n4 ++;
	break;
      case 5:
	n5 ++;
	break;
      case 6:
	n6 ++;
	break;
      case 7:
	n7 ++;
	break;
      case 8:
	n8 ++;
	break;
      }
    }
    S= S / (1.0*n1);
    F = nperf / (1.0 * n1);
    N = n1+n2+n3+n4+n5+n6+n7+n8;
    Ap = A / S2;
    Ap2 = sqrt(A2 / (S2) - Ap*Ap);
    fprintf (stdout, "%s: %5d %5d %5d %5d %5d %5d %5d %5d",
	     file,n1,n2,n3,n4,n5,n6,n7,n8);
    fprintf (stdout, "  %5d %5d  %5.3f %7.2f %7.2f %7.1f %7.1f %7.3f %7.3f\n",
	     N, nperf, F, Fx, Fy, alpha, S, Ap, Ap2);
    fclose(f);
  }
}

