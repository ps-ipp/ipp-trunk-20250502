# include <kapa_internal.h>

# define N_KAPA_COLORS 42

static char KAPA_COLORS[N_KAPA_COLORS][3][16] = {
{"black",    	 "black",    	         "0.00 0.00 0.00"}, 
{"white",    	 "white",    	         "1.00 1.00 1.00"}, 
{"red",      	 "red",      	         "1.00 0.00 0.00"}, 
{"pink",     	 "pink",     	         "1.00 0.75 0.80"}, 
{"orange",   	 "orange",   	         "1.00 0.65 0.00"}, 
{"yellow",   	 "yellow",   	         "1.00 1.00 0.00"}, 
{"wheat",    	 "wheat",    	         "0.96 0.87 0.70"}, 
{"gold",     	 "gold",     	         "1.00 0.84 0.00"}, 
{"green",    	 "green",    	         "0.00 1.00 0.00"}, 
{"darkgreen",	 "darkgreen",	         "0.00 0.40 0.00"}, 
{"darkblue",     "darkblue", 	         "0.50 0.50 1.00"}, 
{"blue",     	 "blue",     	         "0.00 0.00 1.00"}, 
{"skyblue",  	 "skyblue",  	         "0.53 0.81 0.92"}, 
{"indigo",   	 "mediumpurple",         "0.57 0.44 0.86"}, 
{"violet",   	 "darkviolet", 	         "0.58 0.00 0.88"},
{"blue10",     	 "rgb:00/00/33",         "0.00 0.00 0.20"}, 
{"blue20",     	 "rgb:00/00/66",         "0.00 0.00 0.40"}, 
{"blue30",     	 "rgb:00/00/99",         "0.00 0.00 0.60"}, 
{"blue40",     	 "rgb:00/00/cc",         "0.00 0.00 0.80"}, 
{"blue50",     	 "rgb:00/00/ff",         "0.00 0.00 1.00"}, 
{"blue60",     	 "rgb:33/33/ff",         "0.20 0.20 1.00"}, 
{"blue70",     	 "rgb:66/66/ff",         "0.40 0.40 1.00"}, 
{"blue80",     	 "rgb:99/99/ff",         "0.60 0.60 1.00"}, 
{"blue90",     	 "rgb:cc/cc/ff",         "0.80 0.80 1.00"}, 
{"red10",     	 "rgb:33/00/00",     	 "0.20 0.00 0.00"}, 
{"red20",     	 "rgb:66/00/00",     	 "0.40 0.00 0.00"}, 
{"red30",     	 "rgb:99/00/00",     	 "0.60 0.00 0.00"}, 
{"red40",     	 "rgb:cc/00/00",     	 "0.80 0.00 0.00"}, 
{"red50",     	 "rgb:ff/00/00",     	 "1.00 0.00 0.00"}, 
{"red60",     	 "rgb:ff/33/33",     	 "1.00 0.20 0.20"}, 
{"red70",     	 "rgb:ff/66/66",     	 "1.00 0.40 0.40"}, 
{"red80",     	 "rgb:ff/99/99",     	 "1.00 0.60 0.60"}, 
{"red90",     	 "rgb:ff/cc/cc",     	 "1.00 0.80 0.80"}, 
{"grey10",   	 "grey10",   	         "0.10 0.10 0.10"},
{"grey20",   	 "grey20",   	         "0.20 0.20 0.20"},
{"grey30",   	 "grey30",   	         "0.30 0.30 0.30"},
{"grey40",   	 "grey40",   	         "0.40 0.40 0.40"},
{"grey50",   	 "grey50",   	         "0.50 0.50 0.50"},
{"grey60",   	 "grey60",   	         "0.60 0.60 0.60"},
{"grey70",   	 "grey70",   	         "0.70 0.70 0.70"},
{"grey80",   	 "grey80",   	         "0.80 0.80 0.80"},
{"grey90",   	 "grey90",   	         "0.90 0.90 0.90"}};

int KapaColorByName (char *name) {

  int i;
  
  for (i = 0; i < N_KAPA_COLORS; i++) {
    if (!strcmp (name, KAPA_COLORS[i][0])) {
      return (i);
    }	
  }
  if (!strcasecmp (name, "none")) return (-1);

  fprintf (stderr, "color may be one of:\n");
  for (i = 0; i < N_KAPA_COLORS; i++) {
    fprintf (stderr, "  %s\n", KAPA_COLORS[i][0]);
  }
  return (-1);
}	

int KapaColormapSize () {
  return (N_KAPA_COLORS);
}

char *KapaColorRGBString (int N) {
  int Nused = MAX (0, MIN (N_KAPA_COLORS, N));
  return (KAPA_COLORS[Nused][2]);
}

char *KapaColorName (int N) {
  int Nused = MAX (0, MIN (N_KAPA_COLORS, N));
  return (KAPA_COLORS[Nused][0]);
}

png_color *KapaPNGPalette (int *Npalette) {

  int i;
  float red, green, blue;
  png_color *palette;

  ALLOCATE (palette, png_color, N_KAPA_COLORS);

  /* define the palette */
  for (i = 0; i < N_KAPA_COLORS; i++) {
    sscanf (KAPA_COLORS[i][2], "%f %f %f", &red, &green, &blue);
    palette[i].red = (0xff * red);
    palette[i].green = (0xff * green);
    palette[i].blue = (0xff * blue);
  }
  *Npalette = N_KAPA_COLORS;
  return (palette);
}

unsigned long *KapaX11colors (Display *display, Colormap colormap, unsigned long default_color, int *Ncolors) {

  int i;
  int status;
  unsigned long *colors;
  XColor rgbcolor, hardwarecolor;

  *Ncolors = N_KAPA_COLORS;
  ALLOCATE (colors, unsigned long, N_KAPA_COLORS);

  for (i = 0; i < N_KAPA_COLORS; i++) {
    colors[i] = default_color;
    status = XLookupColor (display, colormap, KAPA_COLORS[i][1], &rgbcolor, &hardwarecolor);
    if (!status) continue;
    status = XAllocColor (display, colormap, &hardwarecolor);
    if (!status) continue;
    colors[i] = hardwarecolor.pixel;
  }
  return (colors);
}

/*

kapa objects, bDraw, and png all use the same pallete sequence, defined by
KAPA_COLORS above.  the color correspond to a color sequence.  

*/
