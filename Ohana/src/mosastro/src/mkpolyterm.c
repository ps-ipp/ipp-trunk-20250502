# include "mosastro.h"

int mkpolyterm (int n, int m) {
  
  int i, nt, N;
  
  N = 0;
  nt = n + m;
  for (i = 2; i < nt; i++) {
    N += i + 1;
  }
  N += m;
  return (N);
}
