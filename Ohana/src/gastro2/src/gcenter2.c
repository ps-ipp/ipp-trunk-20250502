# include "gastro2.h"

void gcenter (CmpCatalog *Target, RefCatalog *Ref) {

  int i, N, Imin;
  double angle, ChiMin;
  Answer *answer;
  RefCatalog Subset;

  gproject (Target, Ref, &Subset);
  if (PLOTSTUFF) plot_fullfield (Target, &Subset);

  fprintf (stderr, "Target: %d, Ref: %d\n", Target[0].N, Subset.N);
  dump_coords (Target);

  ALLOCATE (answer, Answer, 2*NROT + 1);

  N = 0;
  for (angle = ROT_ZERO-dROT*NROT; angle <= ROT_ZERO+dROT*NROT; angle += dROT, N++) {

    if (N == 2*NROT + 1) {
      fprintf (stderr, "ERROR in logic: Nanswer > 2*NROT+1 (%d, %d)\n", N, 2*NROT+1);
      exit (1);
    }
    answer[N].angle = angle;
    grid (Target, &Subset, &answer[N]);
  }

  Imin = 0;
  ChiMin = answer[0].Chi;
  for (i = 0; i < N; i++) {
    if (answer[i].Chi < ChiMin) {
      Imin = i;
      ChiMin = answer[i].Chi;
    }
  }
      
  fprintf (stderr, "best solution: angle: %6.1f, (%6.1f,%6.1f) - %10.8f for %d pairs\n", 
	   answer[Imin].angle, answer[Imin].Xoff, answer[Imin].Yoff, answer[Imin].Chi, answer[Imin].N);

  Target[0].answer = answer[Imin];

  /* adjust original coordinates for new center */
  { 

    double cs, sn;
    double pc11, pc12, pc21, pc22;
    double Xo, Yo;

    cs = cos(RAD_DEG*answer[Imin].angle);  sn = sin(RAD_DEG*answer[Imin].angle);
    
    pc11 = Target[0].coords.pc1_1;
    pc12 = Target[0].coords.pc1_2;
    pc21 = Target[0].coords.pc2_1;
    pc22 = Target[0].coords.pc2_2;

    Target[0].coords.pc1_1 =  pc11*cs - pc21*sn;
    Target[0].coords.pc1_2 =  pc11*sn + pc12*cs;
    Target[0].coords.pc2_1 =  pc21*cs - pc22*sn;
    Target[0].coords.pc2_2 =  pc21*sn + pc22*cs;
    
    Xo = Target[0].coords.crpix1;
    Yo = Target[0].coords.crpix2;
    Target[0].coords.crpix1 = answer[Imin].Xoff + cs*Xo - sn*Yo;
    Target[0].coords.crpix2 = answer[Imin].Yoff + sn*Xo + cs*Yo;
  }
}
