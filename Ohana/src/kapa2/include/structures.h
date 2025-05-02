
/*** 3C color cube histogram tree thingy ***/
typedef struct CCNode {
  // for the moment, this structure is specific to the color-histogram analysis.  if
  // this were a void *pointer and we defined some additional apis, we could use this
  // structure to track any 3D space
  int count;    
  int pixel;
  char bottom;
  float min[3]; // min value for each of the 3 dimensions for this node
  float mid[3]; // mid-point for each of the 3 dimensions for this node
  float max[3]; // max value for each of the 3 dimensions for this node
  struct CCNode *sub[2][2][2];
} CCNode;

/**************** Graphic carries X info around ****************/
typedef struct {
  Display       *display;     // X display pointer
  int            screen;      // X screen number
  int            depth;
  Window         window;
  Visual        *visual;
  int            dynamicColors; // is visual dynamic?
  int            Nbits;	      // pixel depth in bits (8, 16, 24, 32)
  GC             gc;
  XFontStruct   *font;
  Cursor         cursor;
  int            x,  y;	      // corner coord in X world coords
  unsigned int   dx, dy;      // size of window in X coords
  int            xwin, ywin;  // corner coord of display subregion (eg, png, ps plot)
  int            dxwin, dywin; // corner coord of display subregion (eg, png, ps plot)
  CCNode        *cube;
  XColor        *cmap;
  Colormap       colormap;
  unsigned long *color;      // graph plotting colors
  int            Ncolors;
  char          *colormapName;

  unsigned long *pixels;      // image pixel colors
  int Npixels;		      // number of pixels

  int            ColorScaleMode; // single colormap for all images??
  int nRed;
  int nBlue;
  int nGreen;

  unsigned long  fore;	      // basic foreground color 
  unsigned long  back;	      // basic background color

  float smooth_sigma; // anti-aliasing smoothing scale

  unsigned long  overlay_color[NOVERLAYS]; // image plotting colors 
} Graphic;

/**************** X related "widget" structures ****************/
typedef struct {
  int      x, y, dx, dy;      /* position and size */
  int      width, height;     /* size of the bitmap */
  unsigned char *bitmap;            /* picture on button */
  int    (*function_1) ();    /* mouse_button 1 function */
  int    (*function_2) ();    /* mouse_button 2 function */
  int    (*function_3) ();    /* mouse_button 3 function */
} Button;

typedef struct {
  int      x, y, dx, dy;   /* position and size */
  char    *label;          /* label on TextLine */
  char     text[LONG_LINE_LENGTH];     /* words of TextLine */
  char     old_text[LONG_LINE_LENGTH]; /* words of TextLine */
  int      outline;        /* draw an outline?  */ 
  int      cursor;         /* location of cursor (if selected) */
  int    (*function) ();   /* textline function */
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
  unsigned char *bits;
} Icon;

typedef struct {
  int      x, y;	      // location of picture in graphic
  int      dx, dy;	      // size of picture
  int      DX, DY;	      // size of displayed picture (must be updated with new images...)
  int      expand;	      // zoomscale
  double   Xc,  Yc;	      // center of image in picture
  char     flipx, flipy;      // parity (0 = +; 1 = -)
  XImage  *pix;
  char    *data;
} Picture;

// objects associated with a KiiImage
// XXX rename thi
typedef struct {
  char type[10];
  double x, y;
  double dx, dy;
  double angle;
  char  *text;
} Object;

typedef struct {
  int x, y;
  int dx, dy; // unused?
  double angle;
  int  size;
  int justify;
  int color;
  char font[64]; 
  char text[LABEL_MAXLEN];
} Label;

typedef struct {
  int active;
  int Nobjects;
  unsigned long color;
  KiiOverlay *objects;
} Overlay;
  
// a set of objects all have the same basic properties
typedef struct {
  float *x; // x-coordinates of the points
  float *y; // y-coordinates of the points
  float *z; // size/colorscale for points
  float *dxp; // lower-errorbar in x
  float *dxm; // upper-errorbar in x
  float *dyp; // lower-errorbar in y
  float *dym; // upper-errorbar in y
  int Npts;   // number of points in this set
  int style;  // how are the object draw: CONNECT, HISTOGRAM,
  int ptype;  // shape of object at each point
  // ptype is overloaded for NPOLYGON to be the number of points / polygon
  int ltype;  // style of line (solid, dot, dash, etc)
  int color;  // color for point (if not colorscaled)
  int etype;  // errorbars to draw (0x01 = y, 0x02 = x)
  int ebar;   // draw a cap on the error bar
  double lweight; // line thickness
  double size; // size of the object
  double x0, x1, y0, y1;  /* limits for this object */
  double alpha;
} Gobjects;

typedef struct {
  char format[16];
  double min, max;
  char isaxis, areticks, islabel, islog;
  double pad, labelPad, ticktextPad;
  double fx, dfx, fy, dfy;  /* axis location on graphic */
  double lweight;
  double fLabelRange;
  double fMinor;
  double dMajor;
  int color;
} Axis;

// structure to describe the ticks for an axis
typedef struct {
  char format[16];
  double value;
  int IsMajor;
  int IsLabel;
  int nsignif;
} TickMarkData;

// a single graph in the display window
typedef struct {
  Axis      axis[4];    /* coordinate axes */
  Label     label[8];   /* fixed axis labels */
  Graphdata data;       /* current graph data */

  Gobjects *objects;    /* graphic objects */    
  int      Nobjects;    

  Label    *textline;      /* placed text labels */
  int      Ntextline;      
  int      haveGraph;   // is there anything in the plot window?
} KapaGraphWidget;

typedef struct {
  // data associated with this image element
  Matrix   matrix;         /* data for picture */
  double   zero, range;    /* zero, range for picture to cmap */
  double   max, min;       /* zero, range for data to z-value */
  double   start, slope;   /* zero, range for cmap to pixels */
  Coords   coords;
  char     file[LONG_LINE_LENGTH];     /* name of file */
  char     name[LONG_LINE_LENGTH];     /* name of buffer */
} KapaImageChannel;

// a single image in the display window
typedef struct {
  // picture components of the image element
  Picture  picture;	      // the primary view of the image
  Picture  zoom;	      // the zoom window
  Picture  wide;	      // the wide-view window
  Picture  cmapbar;	      // the colormap bar

  // the control buttons
  Button   PS_button;
  Button   recenter_button;
  Button   hms_button; /* toggle HMS/DECIMAL mode (DECIMAL_DEG macro) */
  Button   hex_button; /* toggle HEX / FLOAT mode */
  Button   grey_button;
  Button   rainbow_button;
  Button   heat_button;
  Button   overlay_button[NOVERLAYS];
  Button   flipx_button;
  Button   flipy_button;

  // location of the status box
  int      text_x, text_y;
  int      text_dx, text_dy, text_dyo;
  int      location;	      // position of the zoom/status widgets (0 = none, 1-4 = bottom,left,top,right)
  int      MovePointer;
  int      DecimalDegrees;
  int      HexValue;

  // double   X, Y;           /* image pixel at screen center */
  // double   x, y, z;        /* last pointer coords */

  Overlay  overlay[NOVERLAYS];
  Overlay  tickmarks;

  unsigned short   *pixmap;   // lookup table for image pixel value to pixel index
  int      nPixels;
 
  KapaImageChannel *image;
  KapaImageChannel channel[NCHANNELS];
  int currentChannel;
} KapaImageWidget;

typedef struct {
  KapaGraphWidget *graph;
  KapaImageWidget *image;
  float  x,  y;
  float dx, dy;
  int bg;
  char *name;
} Section;

typedef struct {
  FILE *f;
  int Nsegment;
  int NSEGMENT;
  int *offset;
  int *objnum;

  int Nstream;
  int NSTREAM;
  int *streamObjnum;

  int Nimage;
  int NIMAGE;
  int *imageObjnum;
} PDF_FILE;

