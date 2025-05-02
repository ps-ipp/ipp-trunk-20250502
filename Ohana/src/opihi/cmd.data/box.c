# include "data.h"

int box (int argc, char **argv) {
  
  int i, N, kapa, size;
  char *name, fontname[64];
  Graphdata graphmode;
  
  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetGraph (&graphmode, &kapa, name)) return (FALSE);
  FREE (name);

  if ((N = get_argument (argc, argv, "-fn"))) {
    remove_argument (N, &argc, argv);
    strcpy (fontname, argv[N]);
    remove_argument (N, &argc, argv);
    size = atof (argv[N]);
    remove_argument (N, &argc, argv);
    KapaSetFont (kapa, fontname, size);
  } 

  // graphmode.lweight = 1;
  if ((N = get_argument (argc, argv, "-lw"))) {
    remove_argument (N, &argc, argv);
    graphmode.lweight = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  // graphmode.color = KapaColorByName ("black");
  if ((N = get_argument (argc, argv, "-c"))) {
    remove_argument (N, &argc, argv);
    graphmode.color = KapaColorByName (argv[N]);
    if (graphmode.color == -1) return (FALSE);
    remove_argument (N, &argc, argv);
  }

  // XXX need to get the current values from kapa
  // strcpy (graphmode.ticks, "2222");
  if ((N = get_argument (argc, argv, "-ticks"))) {
    remove_argument (N, &argc, argv);
    strcpy (graphmode.ticks, argv[N]);
    remove_argument (N, &argc, argv);
    if (strlen (graphmode.ticks) != 4) { goto usage; }
    for (i = 0; i < strlen (graphmode.ticks); i++) {
      if ((graphmode.ticks[i] != '0') && (graphmode.ticks[i] != '1') && (graphmode.ticks[i] != '2') && (graphmode.ticks[i] != '3')) { goto usage; }
    }
  }
  
  // strcpy (graphmode.labels, "2222");
  if ((N = get_argument (argc, argv, "-labels"))) {
    remove_argument (N, &argc, argv);
    strcpy (graphmode.labels, argv[N]);
    remove_argument (N, &argc, argv);
    if (strlen (graphmode.labels) != 4) { goto usage; }
    for (i = 0; i < strlen (graphmode.labels); i++) {
      if ((graphmode.labels[i] != '0') && (graphmode.labels[i] != '1') && (graphmode.labels[i] != '2')) { goto usage; }
    }
  }

  // strcpy (graphmode.axis, "2222");
  if ((N = get_argument (argc, argv, "-axis"))) {
    remove_argument (N, &argc, argv);
    strcpy (graphmode.axis, argv[N]);
    remove_argument (N, &argc, argv);
    if (strlen (graphmode.axis) != 4) { goto usage; }
    for (i = 0; i < strlen (graphmode.axis); i++) {
      if ((graphmode.axis[i] != '0') && (graphmode.axis[i] != '1') && (graphmode.axis[i] != '2')) { goto usage; }
    }
  }

  if ((N = get_argument (argc, argv, "-tickpad"))) {
    remove_argument (N, &argc, argv);
    graphmode.ticktextPad = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-labelpadx"))) {
    remove_argument (N, &argc, argv);
    graphmode.labelPadXm = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-labelpady"))) {
    remove_argument (N, &argc, argv);
    graphmode.labelPadYm = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "+labelpadx"))) {
    remove_argument (N, &argc, argv);
    graphmode.labelPadXp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "+labelpady"))) {
    remove_argument (N, &argc, argv);
    graphmode.labelPadYp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-labelpad"))) {
    remove_argument (N, &argc, argv);
    graphmode.labelPadXm = atof(argv[N]);
    graphmode.labelPadXp = atof(argv[N]);
    graphmode.labelPadYm = atof(argv[N]);
    graphmode.labelPadYp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-pad"))) {
    remove_argument (N, &argc, argv);
    graphmode.padXm = atof(argv[N]);
    graphmode.padXp = atof(argv[N]);
    graphmode.padYm = atof(argv[N]);
    graphmode.padYp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-xpad"))) {
    remove_argument (N, &argc, argv);
    graphmode.padXm = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "+xpad"))) {
    remove_argument (N, &argc, argv);
    graphmode.padXp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-ypad"))) {
    remove_argument (N, &argc, argv);
    graphmode.padYm = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "+ypad"))) {
    remove_argument (N, &argc, argv);
    graphmode.padYp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-fminor"))) {
    remove_argument (N, &argc, argv);
    graphmode.fMinorXm = atof(argv[N]);
    graphmode.fMinorXp = atof(argv[N]);
    graphmode.fMinorYm = atof(argv[N]);
    graphmode.fMinorYp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-xfminor"))) {
    remove_argument (N, &argc, argv);
    graphmode.fMinorXm = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "+xfminor"))) {
    remove_argument (N, &argc, argv);
    graphmode.fMinorXp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-yfminor"))) {
    remove_argument (N, &argc, argv);
    graphmode.fMinorYm = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "+yfminor"))) {
    remove_argument (N, &argc, argv);
    graphmode.fMinorYp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-dmajor"))) {
    remove_argument (N, &argc, argv);
    graphmode.dMajorXm = atof(argv[N]);
    graphmode.dMajorXp = atof(argv[N]);
    graphmode.dMajorYm = atof(argv[N]);
    graphmode.dMajorYp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-xdmajor"))) {
    remove_argument (N, &argc, argv);
    graphmode.dMajorXm = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "+xdmajor"))) {
    remove_argument (N, &argc, argv);
    graphmode.dMajorXp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-ydmajor"))) {
    remove_argument (N, &argc, argv);
    graphmode.dMajorYm = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "+ydmajor"))) {
    remove_argument (N, &argc, argv);
    graphmode.dMajorYp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-flabel"))) {
    remove_argument (N, &argc, argv);
    graphmode.fLabelRangeXm = atof(argv[N]);
    graphmode.fLabelRangeXp = atof(argv[N]);
    graphmode.fLabelRangeYm = atof(argv[N]);
    graphmode.fLabelRangeYp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-xflabel"))) {
    remove_argument (N, &argc, argv);
    graphmode.fLabelRangeXm = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "+xflabel"))) {
    remove_argument (N, &argc, argv);
    graphmode.fLabelRangeXp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-yflabel"))) {
    remove_argument (N, &argc, argv);
    graphmode.fLabelRangeYm = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "+yflabel"))) {
    remove_argument (N, &argc, argv);
    graphmode.fLabelRangeYp = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-format"))) {
    remove_argument (N, &argc, argv);
    if (strlen (argv[N]) > 15) { goto usage; }
    strcpy (graphmode.formatXm, argv[N]);
    strcpy (graphmode.formatXp, argv[N]);
    strcpy (graphmode.formatYm, argv[N]);
    strcpy (graphmode.formatYp, argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-xformat"))) {
    remove_argument (N, &argc, argv);
    if (strlen (argv[N]) > 15) { goto usage; }
    strcpy (graphmode.formatXm, argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-yformat"))) {
    remove_argument (N, &argc, argv);
    if (strlen (argv[N]) > 15) { goto usage; }
    strcpy (graphmode.formatYm, argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "+xformat"))) {
    remove_argument (N, &argc, argv);
    if (strlen (argv[N]) > 15) { goto usage; }
    strcpy (graphmode.formatXp, argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "+yformat"))) {
    remove_argument (N, &argc, argv);
    if (strlen (argv[N]) > 15) { goto usage; }
    strcpy (graphmode.formatYm, argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) goto usage;

  KapaBox (kapa, &graphmode);
  return (TRUE);
      
 usage:
  gprint (GP_ERR, "USAGE: box [-ticks NNNN] [-axis NNNN] [-labels NNNN]\n");

  gprint (GP_ERR, "  additional options:\n");
  gprint (GP_ERR, "  -fn (font) (size) : set font used for box\n");
  gprint (GP_ERR, "  -lw (weight) : set box line weight\n");
  gprint (GP_ERR, "  -c (color) : set box color\n");
  gprint (GP_ERR, "  \n");
  gprint (GP_ERR, "  for the descriptions below:\n");
  gprint (GP_ERR, "    -x refers to the bottom x-axis, \n");
  gprint (GP_ERR, "    -y refers to the left y-axis, \n");
  gprint (GP_ERR, "    +x refers to the top x-axis, \n");
  gprint (GP_ERR, "    +y refers to the right y-axis, \n");
  gprint (GP_ERR, "  \n");
  gprint (GP_ERR, "  -ticks [NNNN]  : turn on (1), off (0), or use default (2) for ticks\n");
  gprint (GP_ERR, "  -labels [NNNN] : turn on (1), off (0), or use default (2) for label\n");
  gprint (GP_ERR, "  -axis [NNNN]   : turn on (1), off (0), or use default (2) for axis\n");
  gprint (GP_ERR, "    the order for the NNNN values in the above options is: -x, -y, +x, +y\n");
  gprint (GP_ERR, "    for ticks, 1, 2, 3 give fewer values (larger tick spacing)\n");
  gprint (GP_ERR, "  \n");
  gprint (GP_ERR, "  -tickpad : set the spacing between the ticks and the tick text \n");
  gprint (GP_ERR, "  \n");
  gprint (GP_ERR, "  the following set the spacing between the label and the given axis:\n");
  gprint (GP_ERR, "    -labelpadx, -labelpady, +labelpadx, +labelpady\n");
  gprint (GP_ERR, "  \n");
  gprint (GP_ERR, "  -pad : set the spacing between the plot section boundary and the axes\n");
  gprint (GP_ERR, "         alternatively, set each axis independently with:\n");
  gprint (GP_ERR, "        -xpad, -ypad, +xpad, +ypad\n");
  gprint (GP_ERR, "  \n");
  gprint (GP_ERR, "  -fminor : set the number of minor ticks per major (all axes)\n");
  gprint (GP_ERR, "         alternatively, set each axis independently with:\n");
  gprint (GP_ERR, "        -xfminor, -yfminor, +xfminor, +yfminor\n");
  gprint (GP_ERR, "  \n");
  gprint (GP_ERR, "  -dmajor : set the major tick spacing (all axes)\n");
  gprint (GP_ERR, "         alternatively, set each axis independently with:\n");
  gprint (GP_ERR, "        -xdmajor, -ydmajor, +xdmajor, +ydmajor\n");
  gprint (GP_ERR, "  \n");
  gprint (GP_ERR, "  -flabel : set the fraction of axis over which major ticks have labels (all axes)\n");
  gprint (GP_ERR, "         alternatively, set each axis independently with:\n");
  gprint (GP_ERR, "        -xflabel, -yflabel, +xflabel, +yflabel\n");
  gprint (GP_ERR, "  \n");
  gprint (GP_ERR, "  -format : set the format (C-style) for the tick labels (all axes)\n");
  gprint (GP_ERR, "         alternatively, set each axis independently with:\n");
  gprint (GP_ERR, "        -xformat, -yformat, +xformat, +yformat\n");

  return (FALSE);
}


/* box has:
   axis
   labels
   ticks

   assign like this:   
   -axis 0000
   -labels 0000
   -ticks 0000

   default:
   -axis 1111
   -labels 1100
   -ticks 1111

   messages to kapa:

   DBOX
   (xmin) (xmax) (ymin) (ymax)
   AAAA LLLL TTTT

   A = axis
   L = label
   T = ticks
   
   0 = off
   1 = on
   2 = default / maintain

*/
