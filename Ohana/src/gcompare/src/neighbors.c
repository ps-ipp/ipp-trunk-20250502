# include "gcompare.h"

void main (argc, argv)
int    argc;
char **argv;
{
  
  /* USAGE:  gcompare file1 X1 Y1 file2 X2 Y2 radius */
  
  int i, Nskip1, Nskip2, Nmatches, N;
  int X1, X2, Y1, Y2, deltas, match, nomatch1, nomatch2, noauto;
  data_type data1, data2;
  match_type *matches;
  double radius, DX, DY;

  DX = DY = Nskip1 = Nskip2 = 0;

  if (remove_argument(get_argument (argc, argv, "-h"), &argc, argv))
    help();
  if (remove_argument(get_argument (argc, argv, "-help"), &argc, argv))
    help();
    
  if ((N = get_argument (argc, argv, "-c")) && (N + 2 < argc)) {
    DX = atof(argv[N + 1]);
    DY = atof(argv[N + 2]);
    remove_argument(N, &argc, argv);
    remove_argument(N, &argc, argv);
    remove_argument(N, &argc, argv);
  }
  if (N = get_argument (argc, argv, "-s1")) {
    Nskip1 = atof(argv[N + 1]);
    remove_argument(N, &argc, argv);
    remove_argument(N, &argc, argv);
  }
  
  if (argc != 5) {
    fprintf (stderr, "neighbors [mode] file X Y radius\n");
    exit (0);
  }

  X1 = atof (argv[2]);
  Y1 = atof (argv[3]);
  radius = atof(argv[4]);

  data1 = input (argv[1], X1, Y1, Nskip1);
  fprintf (stderr, "list 1: %d values %d %d\n", data1.Nvalues, X1, Y1);

  data_sort (data1); fprintf (stderr, "sorted 1\n");

  count_neighbors (data1, radius, DX, DY);

}



void
help() {

  fprintf (stderr, "gcompare [mode] file1 X1 Y1 file2 X2 Y2 radius\n");
  fprintf (stderr, " modes:\n");
  fprintf (stderr, "  -d      return delta coords\n");
  fprintf (stderr, "  -m      return matched lines\n");
  fprintf (stderr, "  -n1     return unmatched lines, file 1\n");
  fprintf (stderr, "  -n2     return unmatched lines, file 2\n");
  fprintf (stderr, " additional options:\n");
  fprintf (stderr, "  -c X Y    set offset (file1 - file2)\n");
  fprintf (stderr, "  -s1 N     skip N lines from file 1\n");
  fprintf (stderr, "  -s2 N     skip N lines from file 2\n");
  fprintf (stderr, "  -na X     eliminate auto-matches (radius < X)\n");
  exit (0);
}

