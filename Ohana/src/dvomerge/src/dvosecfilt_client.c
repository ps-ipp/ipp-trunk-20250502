# include "dvomerge.h"

// modify the number of average photcodes : change only secfilt tables in place
int main (int argc, char **argv) {

  SetSignals ();
  dvosecfilt_client_help (argc, argv);
  ConfigInit (&argc, argv);
  dvosecfilt_client_args (&argc, argv);

  strcpy (CATDIR, argv[1]);
  int Nsecfilt = atoi(argv[2]);

  dvosecfilt_catalogs (Nsecfilt);

  exit (0);
}
