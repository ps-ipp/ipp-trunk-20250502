# include <ohana.h>
# define GROWTHTEST 0
# define VERY_VERBOSE 0
# define MAX_RANGE 1.0e16

// Gauss-Jordan elimination using full pivots based on Press et al's description.  Substantially
// reworked for Ohana: major modifications to conform to C indexing, use a boolean to track the
// completed pivot rows and catch the singular matrix early on.  Also, much cleaner control loops
// than their implementation.  (largely based on version by William Kahan)

int dgaussjordan (double **A, double **B, int N, int M) {

# define myMIN_PIVOT 1.0e-7
  int status = dgaussjordan_pivot (A, B, N, M, myMIN_PIVOT);
  return status;
}

// MAX_RANGE is used to test for ill-conditioned input matrices.  For an ill-conditioned
// matrix, one or more of the pivots trends towards zero.  Rather than allow this to go to the
// numerical precision, I am raising an error if |growth| > 1e8
int dgaussjordan_pivot (double **A, double **B, int N, int M, double MIN_PIVOT) {
  OHANA_UNUSED_PARAM(MIN_PIVOT);

  int *colIndex;
  int *rowIndex;
  int *pivot;
  
  int diag, col, row;

  ALLOCATE (colIndex, int, N);
  ALLOCATE (rowIndex, int, N);
  ALLOCATE (pivot, int, N);
  memset (pivot, 0, N*sizeof(int));

  double growth = 1.0;

  // determine underflow conditions
  // double underFlow = DBL_MIN;
# if (GROWTHTEST)
  double roundTest = 4.0;
  roundTest /= 3.0;
  roundTest -= 1.0;
  double epsilon = fabs(((roundTest+roundTest) - 1.0) + roundTest);
# endif

  // Following the algorithm laid out by Press et al., we loop along the matrix diagonal,
  // but we do not operate on the diagonal elements in order instead, we are looking for
  // the current max element and operating on that diagonal element.  this is effectively
  // column pivoting.  row pivoting is perfomed explicitly.  

  for (diag = 0; diag < N; diag++) {

    double maxval = 0.0;
    int maxrow = 0;
    int maxcol = 0;

    // search for the next pivot
    for (row = 0; row < N; row++) {
      if (!finite(A[row][diag])) { 
	push_error ("infinity"); 
	goto escape; 
      }

      // if we have already operated on this row (pivot[row] is true), skip it
      if (pivot[row]) continue;

      // if we have not yet operated on this row (pivot[row] is false), look for pivot for this row
      for (col = 0; col < N; col++) {
	if (pivot[col]) continue;
	if (fabs (A[row][col]) < maxval) continue;
	maxval = fabs (A[row][col]);
	maxrow = row;
	maxcol = col;
      }
    }

# if (GROWTHTEST)
    fprintf (stderr, "maxcol: %d\n", maxcol);
# if (VERY_VERBOSE)
    fprintf (stderr, "full A matrix:\n");
    for (row = 0; row < N; row++) {
      for (col = 0; col < N; col++) {
    	fprintf (stderr, "%10.3e ", A[row][col]);
      }
      fprintf (stderr, "\n");
    }
    fprintf (stderr, "\n");
# endif
# endif

    // if pivot[maxcol] is set, we have already done this row: this implies a singular matrix
    if (pivot[maxcol]) { 
      push_error ("singular"); 
      goto escape; 
    }
    pivot[maxcol] = TRUE;

    // if the selected pivot is off the diagonal, do a row swap
    if (maxrow != maxcol) {
      for (col = 0; col < N; col++) SWAP (A[maxrow][col], A[maxcol][col]);
      for (col = 0; col < M; col++) SWAP (B[maxrow][col], B[maxcol][col]);
    }
    rowIndex[diag] = maxrow;
    colIndex[diag] = maxcol;
    if (A[maxcol][maxcol] == 0.0) { 
      push_error ("zero pivot"); 
      goto escape;
    }

    // XXX Kahan replaces the 0.0 pivot with epsilon*(largest element in column) + underFlow

    /* rescale by pivot reciprocal */
    double tmpval = 1.0 / A[maxcol][maxcol];

# if (0)
    if (fabs(A[maxcol][maxcol]) < MIN_PIVOT) {
      // we are ill-conditioned.  set this row & col to 0.0, 1.0 on pivot 
      // fprintf (stderr, "WARNING: eliminating degenerate pivot %lf @ A[%d][%d]\n", A[maxcol][maxcol], maxcol, maxcol);
      for (col = 0; col < N; col++) A[maxcol][col] = 0.0;
      for (col = 0; col < M; col++) B[maxcol][col] = 0.0;
      for (row = 0; row < N; row++) A[row][maxcol] = 0.0;
      A[maxcol][maxcol] = 1.0;
      continue;
    }
# endif

    // XXX why is this here (don't I double count this element?) A[maxcol][maxcol] = 1.0;
    A[maxcol][maxcol] = 1.0;
    for (col = 0; col < N; col++) A[maxcol][col] *= tmpval;
    for (col = 0; col < M; col++) B[maxcol][col] *= tmpval;

    // check for ill-conditioned matrix
    growth *= tmpval;

    // report the pivot growth
#   if (GROWTHTEST)
    fprintf (stderr, "column: %d, maxval : %f, growth: %e, epsilon: %e\n", maxcol, tmpval, growth, epsilon);
# if (VERY_VERBOSE)
    fprintf (stderr, "A diagonal: ");
    for (col = 0; col < N; col++) fprintf (stderr, "%f ", A[col][col]);
    fprintf (stderr, "\n");
# endif
# endif

    if (fabs(growth) > MAX_RANGE) { 
      push_error ("max range"); 
# if (VERY_VERBOSE)
      fprintf (stderr, "full A matrix:\n");
      for (row = 0; row < N; row++) {
	for (col = 0; col < N; col++) {
	  fprintf (stderr, "%10.3e ", A[row][col]);
	}
	fprintf (stderr, "\n");
      }
      fprintf (stderr, "\n");
# endif
      goto escape;
    }

    /* adjust the elements above the pivot */
    for (row = 0; row < N; row++) {
      if (row == maxcol) continue;
      tmpval = A[row][maxcol];
      A[row][maxcol] = 0.0;
      for (col = 0; col < N; col++) A[row][col] -= A[maxcol][col]*tmpval;
      for (col = 0; col < M; col++) B[row][col] -= B[maxcol][col]*tmpval;
    }
  }

# if (GROWTHTEST && VERY_VERBOSE)
  fprintf (stderr, "final A matrix:\n");
  for (row = 0; row < N; row++) {
    for (col = 0; col < N; col++) {
      fprintf (stderr, "%10.3e ", A[row][col]);
    }
    fprintf (stderr, "\n");
  }
  fprintf (stderr, "\n");
# endif

  // swap back the inverse matrix based on the row swaps above
  for (col = N - 1; col >= 0; col--) {
    if (rowIndex[col] != colIndex[col]) {
      for (row = 0; row < N; row++) SWAP (A[row][rowIndex[col]], A[row][colIndex[col]]);
    }
  }

  free (pivot);
  free (rowIndex);
  free (colIndex);
  return (TRUE);

escape:
  free (pivot);
  free (rowIndex);
  free (colIndex);
  return (FALSE);
}

int fgaussjordan (float **A, float **B, int N, int M) {

  int *colIndex;
  int *rowIndex;
  int *pivot;
  
  int diag, col, row;

  ALLOCATE (colIndex, int, N);
  ALLOCATE (rowIndex, int, N);
  ALLOCATE (pivot, int, N);
  memset (pivot, 0, N*sizeof(int));

  float growth = 1.0;

  // determine underflow conditions
  // float underFlow = FLT_MIN;
# if (GROWTHTEST)
  float roundTest = 4.0;
  roundTest /= 3.0;
  roundTest -= 1.0;
  float epsilon = fabs(((roundTest+roundTest) - 1.0) + roundTest);
# endif

  // we loop along the matrix diagonal, but we do not operate on the diagonal elements in
  // order instead, we are looking for the current max element and operating on that
  // diagonal element.  this is effectively column pivoting.  row pivoting is perfomed
  // explicitly 

  for (diag = 0; diag < N; diag++) {

    float maxval = 0.0;
    int maxrow = 0;
    int maxcol = 0;

    // search for the next pivot
    for (row = 0; row < N; row++) {
      if (!finite(A[row][diag])) goto escape;

      // if we have already operated on this row (pivot[row] is true), skip it
      if (pivot[row]) continue;

      // if we have not yet operated on this row (pivot[row] is false), look for pivot for this row
      for (col = 0; col < N; col++) {
	if (pivot[col]) continue;
	if (fabs (A[row][col]) < maxval) continue;
	maxval = fabs (A[row][col]);
	maxrow = row;
	maxcol = col;
      }
    }

    // if pivot[maxcol] is set, we have already done this row: this implies a singular matrix
    if (pivot[maxcol]) goto escape;
    pivot[maxcol] = TRUE;

    // if the selected pivot is off the diagonal, do a row swap
    if (maxrow != maxcol) {
      for (col = 0; col < N; col++) SWAP (A[maxrow][col], A[maxcol][col]);
      for (col = 0; col < M; col++) SWAP (B[maxrow][col], B[maxcol][col]);
    }
    rowIndex[diag] = maxrow;
    colIndex[diag] = maxcol;
    if (A[maxcol][maxcol] == 0.0) goto escape;
    // XXX Kahan replaces the 0.0 pivot with epsilon*(largest element in column) + underFlow

    /* rescale by pivot reciprocal */
    float tmpval = 1.0 / A[maxcol][maxcol];
    A[maxcol][maxcol] = 1.0;
    for (col = 0; col < N; col++) A[maxcol][col] *= tmpval;
    for (col = 0; col < M; col++) B[maxcol][col] *= tmpval;

    // check for ill-conditioned matrix
    growth *= tmpval;

    // report the pivot growth
#   if (GROWTHTEST)
    fprintf (stderr, "column: %d, maxval : %f, growth: %e, epsilon: %e\n", maxcol, tmpval, growth, epsilon);
    fprintf (stderr, "A diagonal: ");
    for (col = 0; col < N; col++) fprintf (stderr, "%f ", A[col][col]);
    fprintf (stderr, "\n");
# endif

    if (fabs(growth) > MAX_RANGE) goto escape;

    /* adjust the elements above the pivot */
    for (row = 0; row < N; row++) {
      if (row == maxcol) continue;
      tmpval = A[row][maxcol];
      A[row][maxcol] = 0.0;
      for (col = 0; col < N; col++) A[row][col] -= A[maxcol][col]*tmpval;
      for (col = 0; col < M; col++) B[row][col] -= B[maxcol][col]*tmpval;
    }
  }

  // swap back the inverse matrix based on the row swaps above
  for (col = N - 1; col >= 0; col--) {
    if (rowIndex[col] != colIndex[col]) {
      for (row = 0; row < N; row++) SWAP (A[row][rowIndex[col]], A[row][colIndex[col]]);
    }
  }

  free (pivot);
  free (rowIndex);
  free (colIndex);
  return (TRUE);

escape:
  free (pivot);
  free (rowIndex);
  free (colIndex);
  return (FALSE);
}


/* Gauss-Jordan Inversion from William Kahan in Basic
500 ' Gauss-Jordan Matrix Inversion     X = A^(-1) in IBM PC BASIC
510 ' including checks for excessive    growth despite row-pivoting,
520 '          and adjustments for zero pivots to avoid .../0 .
530 ' DIM A(N,N), X(N,N), P(N) ...      are assumed.
540     DEFINT I-N ' ... integer variables; the rest are REAL.
550   '
560   ' First determine levels of roundoff and over/underflow.
570     UFL = 5.9E-39 ' ... = max{ under, 1/over}flow thresholds.
580        G=4 : G=G/3 : G=G-1      ' ... = 1/3 + roundoff in 4/3
590     EPS = ABS( ((G+G) - 1) + G ) ' ... = roundoff level.
600     G = 1 ' ... will record pivot-growth factor
610   '
620   ' Copy A to X and record each column's biggest element.
630     FOR J=1 TO N : P(J)=0
640          FOR I=1 TO N : T = A(I,J) : X(I,J) = T : T = ABS(T)
650                IF T > P(J) THEN P(J) = T
660                NEXT I : NEXT J
670   '
680     FOR K=1 TO N :' ... perform elimination upon column K .
690          Q=0 : J=K : ' ... search for Kth pivot ...
700          FOR I=K TO N
710                T=ABS(X(I,K)) : IF T>Q THEN Q=T : J=I
720                NEXT I
730          IF Q=0 THEN Q = EPS*P(K) + UFL : X(K,K)=Q
740          IF P(K)>0 THEN Q=Q/P(K) : IF Q>G THEN G=Q
750          IF G<=8*K THEN GOTO 790
760      PRINT "Growth factor g = ";G;" exceeds ";8*K;" ; try"
770      PRINT "moving A's column ";K;" to col. 1 to reduce g ."
780            STOP ' ... or go back to re-order A's columns.
790         P(K)=J ' ... record pivotal row exchange, if any.
800         IF J=K THEN GOTO 830 ' ... Don't bother to swap.
810             FOR L=1 TO N : Q=X(J,L) : X(J,L)=X(K,L)
820                              X(K,L)=Q : NEXT L
830         Q = X(K,K) : X(K,K) = 1
840         FOR J=1 TO N : X(K,J) = X(K,J)/Q : NEXT J
850         FOR I=1 TO N : IF I=K THEN GOTO 890
860              Q = X(I,K) : X(I,K) = 0
870              FOR J=1 TO N
880                   X(I,J) = X(I,J) - X(K,J)*Q : NEXT J
890              NEXT I : NEXT K
900 '
910 FOR K=N-1 TO 1 STEP -1 ' ... unswap columns of X
920         J=P(K) : IF J=K THEN GOTO 950
930         FOR I=1 TO N : Q=X(I,K) : X(I,K)=X(I,J)
940                          X(I,J)=Q : NEXT I
950         NEXT K
960 RETURN
*/
