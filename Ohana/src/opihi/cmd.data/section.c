# include "data.h"

enum {NONE, LIST, UP, DOWN, TOP, BOTTOM, TOOL, BG, IMAGE};

int section (int argc, char **argv) {
  
  int N, action, kapa, background;
  Graphdata graphmode;
  KapaSection section;
  char *name = NULL;
  char *location = NULL;

  action = NONE;
  if ((N = get_argument (argc, argv, "-list"))) {
    action = LIST;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-up"))) {
    action = UP;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-down"))) {
    action = DOWN;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-top"))) {
    action = TOP;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-bottom"))) {
    action = BOTTOM;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-imtool"))) {
    action = TOOL;
    remove_argument (N, &argc, argv);
    location = argv[N];
    remove_argument (N, &argc, argv);
  }

  background = -1; // invisible
  if ((N = get_argument (argc, argv, "-bg"))) {
    remove_argument (N, &argc, argv);
    if (!strcasecmp (argv[N], "NONE")) {
      background = -1;
    } else {
      background = KapaColorByName(argv[N]);
    }
    remove_argument (N, &argc, argv);
    action = BG;
  }

  if ((N = get_argument (argc, argv, "-image"))) {
    remove_argument (N, &argc, argv);
    action = IMAGE;
  }

  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetGraph (&graphmode, &kapa, name)) return (FALSE);
  FREE (name);

  /* list sections */
  if ((argc == 1) && (action == NONE)) {
    KapaGetSection (kapa, "*");
    gprint (GP_ERR, "USAGE: section name [x y dx dy] or [options]\n");
    gprint (GP_ERR, "OPTIONS: -list   : show properties of all sections\n");
    gprint (GP_ERR, "         -up     : move section up in display stack\n");
    gprint (GP_ERR, "         -down   : move section down in display stack\n");
    gprint (GP_ERR, "         -top    : move section to top of display stack\n");
    gprint (GP_ERR, "         -bottom : move section to bottom of display stack\n");
    gprint (GP_ERR, "         -imtool (position) : set location of image zoom / status box\n");
    gprint (GP_ERR, "                 (position may be: -x, +x, -y, +y, none)\n");
    gprint (GP_ERR, "         -image  : define section upper-right corner so image fills the section\n");
    gprint (GP_ERR, "         -bg (color) : set background color (see style -c help for color choices)\n");
    
    return (TRUE);
  } 
  
  if ((argc != 2) && (action != NONE)) {
      gprint (GP_ERR, "can only use dash options without numbers\n");
      return (FALSE);
  }

  if (argc == 2) {
    /* select / show section */
    switch (action) {
      case NONE:
	KapaSelectSection (kapa, argv[1]);
	break;

      case UP:
	KapaMoveSection (kapa, argv[1], "up");
	break;
      case DOWN:
	KapaMoveSection (kapa, argv[1], "down");
	break;
      case TOP:
	KapaMoveSection (kapa, argv[1], "top");
	break;
      case BOTTOM:
	KapaMoveSection (kapa, argv[1], "bottom");
	break;

      case BG:
	KapaSectionBG (kapa, argv[1], background);
	break;

      case TOOL:
	if (!strcmp(location, "-x")) {
	  KapaSetToolbox (kapa, 1);
	  break;
	}
	if (!strcmp(location, "+x")) {
	  KapaSetToolbox (kapa, 3);
	  break;
	}
	if (!strcmp(location, "-y")) {
	  KapaSetToolbox (kapa, 2);
	  break;
	}
	if (!strcmp(location, "+y")) {
	  KapaSetToolbox (kapa, 4);
	  break;
	}
	if (!strcmp(location, "none")) {
	  KapaSetToolbox (kapa, 0);
	  break;
	}
	gprint (GP_ERR, "unknown toolbox location %s\n", location);
	gprint (GP_ERR, "valid values: -x, +x, -y, +y, none\n");
	return (FALSE);

      case LIST:
	KapaGetSection (kapa, argv[1]);
	break;
    }
    return (TRUE);
  } 
  
  if (argc == 4) {
    /* set section */
    section.name = argv[1];
    section.x = atof (argv[2]);
    section.y = atof (argv[3]);
    section.bg = background;
    KapaSetSectionByImage (kapa, &section);
    return (TRUE);
  }

  if (argc == 6) {
    /* set section */
    section.name = argv[1];
    section.x = atof (argv[2]);
    section.y = atof (argv[3]);
    section.dx = atof (argv[4]);
    section.dy = atof (argv[5]);
    section.bg = background;
    KapaSetSection (kapa, &section);
    return (TRUE);
  }
  gprint (GP_ERR, "USAGE: section name [x y dx dy]\n");
  gprint (GP_ERR, "USAGE: section name [-image x y] : width based on current image\n");
  gprint (GP_ERR, "USAGE: section name [-list] [-up] [-down] [-top] [-bottom]\n");
  return (FALSE);
}

/* should do some range checking on x y dx dy
   should be between 0.0 and 1.0, precision of 0.001
   is sufficient
*/
