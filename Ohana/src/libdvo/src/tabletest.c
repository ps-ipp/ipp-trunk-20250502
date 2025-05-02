# include "dvo.h"

int main (int argc, char **argv) {

  SkyTable *table;

  SkyTableFromGSC (argv[1], 2, TRUE);

  exit (0);
}
