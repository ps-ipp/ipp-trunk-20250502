# include "data.h"

int style (int argc, char **argv) {
  
  int kapa;
  Graphdata data;

  if (!style_args (&data, &argc, argv, &kapa)) return FALSE;

  if (argc > 1) {
    gprint (GP_ERR, "USAGE: style [-n Ngraph] [-x plot style] [-c color] [-pt point type] [-lt line type] [-lw line width] [-sz size]\n");
    return (FALSE);
  }
  KapaSetGraphData (kapa, &data);

  return (TRUE);
}
