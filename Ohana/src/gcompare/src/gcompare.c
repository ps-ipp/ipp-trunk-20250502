# include "gcompare.h"

int main (int argc, char **argv) {
  
  /* USAGE:  gcompare file1 X1 Y1 file2 X2 Y2 radius */
  
  int Nskip1, Nskip2, Nmatches, N;
  int X1, X2, Y1, Y2, deltas, match, nomatch1, nomatch2, greatcircle;
  data_type data1, data2;
  match_type *matches;
  double radius, DX, DY, noauto;

  DX = DY = Nskip1 = Nskip2 = 0;
  noauto = match = nomatch1 = nomatch2 = deltas = greatcircle = FALSE;

  if (remove_argument(get_argument (argc, argv, "-d"), &argc, argv))
    deltas = TRUE;
  if (remove_argument(get_argument (argc, argv, "-h"), &argc, argv))
    help();
  if (remove_argument(get_argument (argc, argv, "-help"), &argc, argv))
    help();
  if (remove_argument(get_argument (argc, argv, "-m"), &argc, argv))
    match = TRUE;
  if (remove_argument(get_argument (argc, argv, "-n1"), &argc, argv))
    nomatch1 = TRUE;
  if (remove_argument(get_argument (argc, argv, "-n2"), &argc, argv))
    nomatch2 = TRUE;
  if (remove_argument(get_argument (argc, argv, "-C"), &argc, argv))
    greatcircle = TRUE;
  if (!(match || deltas || nomatch1 || nomatch2)) {
    fprintf (stderr, "no output mode, use at least one of -d, -m, -n1, or -n2\n");
    exit (2);
  }
  if ((N = get_argument (argc, argv, "-na"))) {
    remove_argument(N, &argc, argv);
    noauto = atof(argv[N]);
    remove_argument(N, &argc, argv);
  }
    
  if ((N = get_argument (argc, argv, "-c")) && (N + 2 < argc)) {
    DX = atof(argv[N + 1]);
    DY = atof(argv[N + 2]);
    remove_argument(N, &argc, argv);
    remove_argument(N, &argc, argv);
    remove_argument(N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-s1"))) {
    Nskip1 = atof(argv[N + 1]);
    remove_argument(N, &argc, argv);
    remove_argument(N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-s2")) && (N + 1 < argc)) {
    Nskip2 = atof(argv[N + 1]);
    remove_argument(N, &argc, argv);
    remove_argument(N, &argc, argv);
  }
  
  if (argc != 8) {
    fprintf (stderr, "gcompare [mode] file1 X1 Y1 file2 X2 Y2 radius\n");
    exit (2);
  }

  X1 = atof (argv[2]);
  Y1 = atof (argv[3]);
  X2 = atof (argv[5]);
  Y2 = atof (argv[6]);
  radius = atof(argv[7]);
  fprintf (stderr, "radius = %f %s\n", radius, argv[7]);

  data1 = input (argv[1], X1, Y1, Nskip1);
  fprintf (stderr, "list 1: %d values %d %d\n", data1.Nvalues, X1, Y1);
  data2 = input (argv[4], X2, Y2, Nskip2);
  fprintf (stderr, "list 2: %d values %d %d\n", data2.Nvalues, X2, Y2);

  data_sort (data1); fprintf (stderr, "sorted 1\n");
  data_sort (data2); fprintf (stderr, "sorted 2\n");

  fprintf (stderr, "noauto: %e\n", noauto);
  if (greatcircle) {
    matches = gc_compare (data1, data2, &Nmatches, radius, DX, DY, noauto);
  }
  else {
    matches = compare (data1, data2, &Nmatches, radius, DX, DY, noauto);
  }


  output (data1, data2, matches, Nmatches, match, deltas, nomatch1, nomatch2);
  exit (0);
}



void
help() {

  fprintf (stderr, "gcompare [mode] file1 X1 Y1 file2 X2 Y2 radius\n");
  fprintf (stderr, " modes:\n");
  fprintf (stderr, "  -d      return delta coords\n");
  fprintf (stderr, "  -m      return matched lines\n");
  fprintf (stderr, "  -C      uses great circle calculation\n");
  fprintf (stderr, "  -n1     return unmatched lines, file 1\n");
  fprintf (stderr, "  -n2     return unmatched lines, file 2\n");
  fprintf (stderr, " additional options:\n");
  fprintf (stderr, "  -c X Y    set offset (file1 - file2)\n");
  fprintf (stderr, "  -s1 N     skip N lines from file 1\n");
  fprintf (stderr, "  -s2 N     skip N lines from file 2\n");
  fprintf (stderr, "  -na X     eliminate auto-matches (radius < X)\n");
  exit (2);
}

