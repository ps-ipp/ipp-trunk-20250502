# include "checkastro.h"

void initialize (int argc, char **argv) {

  ConfigInit (&argc, argv);
  args (argc, argv);

  if (PHOTCODE_KEEP_LIST)  fprintf (stderr, "PHOTCODE_KEEP_LIST:  %s\n", PHOTCODE_KEEP_LIST);
  if (PHOTCODE_SKIP_LIST)  fprintf (stderr, "PHOTCODE_SKIP_LIST:  %s\n", PHOTCODE_SKIP_LIST);
  if (PHOTCODE_RESET_LIST) fprintf (stderr, "PHOTCODE_RESET_LIST: %s\n", PHOTCODE_RESET_LIST);

  photcodesKeep   = ParsePhotcodeList (PHOTCODE_KEEP_LIST,  &NphotcodesKeep,   FALSE);
  photcodesSkip   = ParsePhotcodeList (PHOTCODE_SKIP_LIST,  &NphotcodesSkip,   FALSE);
  photcodesReset  = ParsePhotcodeList (PHOTCODE_RESET_LIST, &NphotcodesReset,  FALSE);
}

void initialize_client (int argc, char **argv) {

  ConfigInit (&argc, argv);
  args_client (argc, argv);

  if (PHOTCODE_KEEP_LIST)  fprintf (stderr, "PHOTCODE_KEEP_LIST:  %s\n", PHOTCODE_KEEP_LIST);
  if (PHOTCODE_SKIP_LIST)  fprintf (stderr, "PHOTCODE_SKIP_LIST:  %s\n", PHOTCODE_SKIP_LIST);
  if (PHOTCODE_RESET_LIST) fprintf (stderr, "PHOTCODE_RESET_LIST: %s\n", PHOTCODE_RESET_LIST);

  photcodesKeep   = ParsePhotcodeList (PHOTCODE_KEEP_LIST,  &NphotcodesKeep,   FALSE);
  photcodesSkip   = ParsePhotcodeList (PHOTCODE_SKIP_LIST,  &NphotcodesSkip,   FALSE);
  photcodesReset  = ParsePhotcodeList (PHOTCODE_RESET_LIST, &NphotcodesReset,  FALSE);
}
