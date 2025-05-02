# include "basic.h"

int config (int argc, char **argv) {

  if (!ConfigInit (&argc, argv)) return (FALSE);

  return (TRUE);

}
