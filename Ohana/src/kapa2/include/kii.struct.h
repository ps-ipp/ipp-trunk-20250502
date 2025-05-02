
/**************** Graphic carries X info around ****************/
typedef struct {
  Display       *display;
  int            screen;
  int            depth;
  Window         window;
  Visual        *visual;
  int            visualclass;
  GC             gc;
  XFontStruct   *font;
  Cursor         cursor;
  int            x,  y;
  int            dx, dy;
  Colormap       colormap;
  unsigned long  Npixels, pixels[256];
  int            Nbits;
  /*
  unsigned long  fore;
  unsigned long  back;
  */
  unsigned long  black, white;
} Graphic;

/**************** X related "widget" structures ****************/
typedef struct {
  int      x, y, dx, dy;      /* position and size */
  int      text;              /* does this have a picture or text? */
  int      width, height;
  char    *bitmap;          /* picture on button */
  int    (*function_1) (void);  /* mouse_button 1 function */
  int    (*function_2) (void);  /* mouse_button 2 function */
  int    (*function_3) (void);  /* mouse_button 3 function */
} Button;

typedef struct {
  int      x, y, dx, dy;   /* position and size */
  char    *label;          /* label on TextLine */
  char     text[1024];     /* words of TextLine */
  char     old_text[1024]; /* words of TextLine */
  int      outline;        /* draw an outline?  */ 
  int      cursor;         /* location of cursor (if selected) */
  int    (*function) (void);   /* textline function */
} TextLine;

typedef char STRING[1024];

typedef struct {
  int      x, y, dx, dy;   /* position and size */
  STRING  *text;           /* words of TextLine */
  int      Nlines;
  int      outline;        /* draw an outline?  */ 
  int      cursor_line;    /* cursor line */
  int      cursor_x;       /* location of cursor (if selected) */
  int      cursor_y;       /* location of cursor (if selected) */
  int    (*function) (void);   /* textline function */
} TextBox;

/**************** general structures ****************/
typedef struct {
  Pixmap pixmap;
  int    width;
  int    height;
  char  *bits;
} Icon;

typedef struct {
  int      dx, dy, x, y;
  XImage  *pix;
  char *data;
} Picture;

typedef struct {
  char type[10];
  double x, y;
  double dx, dy;
  double angle;
  char  *text;
} Object;

typedef struct {
  int Nobjects;
  unsigned long color;
  Object *objects;
} Overlay;
  
/******** Here we define the Layout struct specific to this program  *******/
typedef struct {
  /* objects on the mana window */
  Picture  picture;
  Picture  cmapbar;
  Picture  zoom;
  Button   PS_button;
  Button   recenter_button;
  Button   hms_button; /* toggle HMS/DECIMAL mode (DECIMAL_DEG macro) */
  Button   grey_button;
  Button   rainbow_button;
  Button   puns_button;
  Button   overlay_button[NOVERLAYS];
  Overlay  overlay[NOVERLAYS];
  Overlay  tickmarks;
  int      text_x, text_y;

  /* file descriptor for socket connection to mana */
  int Ximage; 

  /* data mana needs */
  Matrix   matrix;         /* data for picture */
  double   X, Y;           /* image pixel at screen center */
  int      expand;         /* zoomscale */
  double   zero, range;    /* zero, range for picture to cmap */
  double   max, min;       /* zero, range for data to z-value */
  double   start, slope;   /* zero, range for cmap to pixels */
  double   x, y, z;        /* last pointer coords */
  Coords   coords;
  char     file[1024];     /* name of file */
  char     buffer_name[1024];  /* name of buffer */

  /* fundamental pieces */
  unsigned long  black;
  unsigned long  white;
  XColor   cmap[256];
  int      Npixels;

} Layout;

/* this routine is independent of the number of overlays */


