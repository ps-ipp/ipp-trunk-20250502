# include "basic.h"

int exec_next (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);
  OHANA_UNUSED_PARAM(argv);

  loop_next = TRUE;
  return (TRUE);
}

int exec_last (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argc);
  OHANA_UNUSED_PARAM(argv);

  loop_last = TRUE;
  return (TRUE);
}

