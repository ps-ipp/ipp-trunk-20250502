# ifndef KAPA_H
# define KAPA_H

/* linux is happy with this, not solaris */
//# include <netinet/in_systm.h> 
//# include <netinet/ip.h>

# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <arpa/inet.h>

# include <X11/Xlib.h>
# include <png.h>
# include <dvo.h>

/* if we are not correctly including the ohana headers, this will fail */
# ifndef BYTE_SWAP
# ifndef NOT_BYTE_SWAP
# error "neither BYTE_SWAP not NOT_BYTE_SWAP is set"
# endif
# endif

typedef struct sockaddr_in KapaSockAddress;

// retain historical numerical definitions:
typedef enum {
  KAPA_LINE_INVALID_MIN = -1,
  KAPA_LINE_SOLID       = 0,
  KAPA_LINE_DOT         = 1,
  KAPA_LINE_DASH_SHORT  = 2,
  KAPA_LINE_DASH_LONG   = 3,
  KAPA_LINE_DOT_DASH    = 4,
  KAPA_LINE_INVALID_MAX = 5,
} KapaLineType;

// retain historical numerical definitions:
typedef enum {
  KAPA_PLOT_INVALID_MIN  = -1,
  KAPA_PLOT_CONNECT      =  0,
  KAPA_PLOT_HISTOGRAM    =  1,
  KAPA_PLOT_POINTS       =  2, // CONNECT, HISTOGRAM, POINTS need to be 0, 1, 2 for backwards compatibility
  KAPA_PLOT_BARS_SOLID   =  3,
  KAPA_PLOT_BARS_OUTLINE =  4,
  KAPA_PLOT_BARS_OUTFILL =  5,
  KAPA_PLOT_POLYGON      =  6,
  KAPA_PLOT_POLYFILL     =  7,
  KAPA_PLOT_INVALID_MAX  =  8,
} KapaPlotStyle;

typedef enum {
  KAPA_POINT_INVALID_MIN         = -1,
  KAPA_POINT_BOX_SOLID           =  0,
  KAPA_POINT_BOX_OPEN            =  1,
  KAPA_POINT_CROSS               =  2, // OR PLUS
  KAPA_POINT_X                   =  3, 
  KAPA_POINT_Y                   =  4, 
  KAPA_POINT_TRIANGLE_SOLID      =  5, 
  KAPA_POINT_TRIANGLE_OPEN       =  6, 
  KAPA_POINT_CIRCLE_OPEN         =  7, 
  KAPA_POINT_PENTAGON            =  8, 
  KAPA_POINT_HEXAGON             =  9, 
  KAPA_POINT_CIRCLE_SOLID        = 10, 
  KAPA_POINT_TRIANGLE_SOLID_DOWN = 11, 
  KAPA_POINT_TRIANGLE_OPEN_DOWN  = 12, 
  KAPA_POINT_Y_DOWN              = 13, 
  KAPA_POINT_INVALID_MAX         = 14,
  KAPA_POINT_PAIR_CONNECT        = 100, // change to a plot style?
} KapaPointStyle;
// note that PAIR_CONNECT was historically 100


typedef enum {
  KII_OVERLAY_NONE, 
  KII_OVERLAY_TEXT, 
  KII_OVERLAY_BOX, 
  KII_OVERLAY_LINE,
  KII_OVERLAY_CIRCLE
} KiiOverlayType;

typedef enum {
  KAPA_LABEL_XM,
  KAPA_LABEL_YM,
  KAPA_LABEL_XP,
  KAPA_LABEL_YP,
  KAPA_LABEL_UL,
  KAPA_LABEL_UR,
  KAPA_LABEL_LL,
  KAPA_LABEL_LR,
} KapaLabelType;

typedef enum {
  KAPA_PS_NEWPLOT,
  KAPA_PS_NEWPAGE,
  KAPA_PS_RAWPAGE
} KapaPSmode;

typedef struct {
  float *data1d;
  float **data2d;
  int Nx;
  int Ny;
} KiiImage;

typedef struct {
  float x;
  float y;
  float dx;
  float dy;
  float angle;
  int type;
} KiiOverlayBase;

typedef struct {
  float x;
  float y;
  float dx;
  float dy;
  float angle;
  int type;
  char *text;
} KiiOverlay;

typedef struct {
  char *name;
  float x;
  float y;
  float dx;
  float dy;
  int bg;
} KapaSection;

/* this was moved to libdvo/include/get_graphdata.h */
#ifdef NOT_MOVED_TO_DVO
typedef struct graphdata {
  double xmin, xmax, ymin, ymax;
  int style, ptype, ltype, etype, ebar, color;
  double lweight, size;
  double ticktextPad;
  double labelPadXm, labelPadYm, labelPadXp, labelPadYp;
  double padXm, padXp, padYm, padYp;
  double fLabelRangeXm, fLabelRangeXp, fLabelRangeYm, fLabelRangeYp;
  double fMinorXm, fMinorXp, fMinorYm, fMinorYp;
  Coords coords;
  int flipeast, flipnorth;
  char axis[8], labels[8], ticks[8];
} Graphdata;
#endif

typedef struct {
  int logflux;
  double zero, range;
  char name[1024];
  char file[1024];
} KapaImageData;

typedef struct {
  int dx;
  int dy;
  float dXps;
  int ascent;
  unsigned char *bits;
} RotFont;

typedef struct {
  RotFont *font;
  char name[64];
  int size;
} FontSet;

typedef png_byte bDrawColor;

typedef struct {
  int Nx, Ny, Nbyte;
  bDrawColor **pixels;
  char **mask;
  png_color *palette;
  int Npalette;

  // current drawing values:
  int bWeight;
  int bType;
  float alpha;
  bDrawColor bColor;
  bDrawColor bColor_R;
  bDrawColor bColor_G;
  bDrawColor bColor_B;
} bDrawBuffer;

/* IOfuncs.c */
int KiiSendMessage (int device, char *format, ...) OHANA_FORMAT(printf, 2, 3);
int KiiScanMessage (int device, char *format, ...) OHANA_FORMAT(scanf, 2, 3);
int KiiSendCommand (int device, int length, char *format, ...) OHANA_FORMAT(printf, 3, 4);
int KiiScanCommand (int device, int length, char *format, ...) OHANA_FORMAT(scanf, 3, 4);
int KiiSendCommandV (int device, int length, char *format, va_list argp);
int KiiSendData (int device, char *data, int Nbytes);
char *KiiRecvData (int device);
int KiiWaitAnswer (int device, char *expect);

/* KiiPicture.c */
int KiiSetChannel (int fd, int channel);
int KiiSetColormap (int fd, char *colormap);
int KiiSetNanColor (int fd, int red, int green, int blue);
int KiiNewPicture1D (int fd, KiiImage *image, KapaImageData *data, Coords *coords);
int KiiNewPicture2D (int fd, KiiImage *image, KapaImageData *data, Coords *coords);

int KapaSetImageCoords (int fd, Coords *coords);
int KapaGetImageCoords (int fd, Coords *coords);
int KapaGetImageRange (int fd, double *Xmin, double *Xmax, double *Ymin, double *Ymax, int *dX, int *dY);

/* KiiOverlay.c */
int KiiSelectOverlay (char *name, int *number);
int KiiLoadOverlay (int fd, KiiOverlay *overlay, int Noverlay, char *name);
int KiiEraseOverlay (int fd, char *name);
int KiiSaveOverlay (int fd, int celestial, char *name, char *file);
int KiiOverlayTypeByName (char *name);
char *KiiOverlayTypeByNumber (int n);

/* KiiConvert.c */
int KiiPS (int fd, const char *filename, int scaleMode, int pageMode, char *pagename);
int KiiJPEG (int fd, const char *filename);
int KapaPNG (int fd, const char *filename);
int KapaPPM (int fd, const char *filename);
int KapaPDF (int fd, const char *filename, int scaleMode, int pageMode, char *pagename);

/* KiiCursor.c */
int KiiCursorOn (int fd);
int KiiCursorOff (int fd);
int KiiCursorRead (int fd, double *x, double *y, double *z, double *r, double *d, char *key);

/* KapaWindow.c */
int KiiResize (int fd, int Nx, int Ny);
int KiiResizeByImage (int fd);
int KiiRelocate (int fd, int x, int y);
int KiiCenter (int fd, double x, double y, int zoom);
int KiiParity (int fd, int xflip, int yflip);
int KapaBox (int fd, Graphdata *graphdata);
int KapaClearCurrentPlot (int fd);
int KapaClearPlots (int fd);
int KapaClearSections (int fd);
int KapaClearImage (int fd);
int KapaInitGraph (Graphdata *graphdata);
int KapaPrepPlot (int fd, int Npts, Graphdata *graphmode);
int KapaPlotVector (int fd, int Npts, float *values, char *type);
int KapaSetFont (int fd, char *name, int size);
int KapaSendLabel (int fd, char *string, int mode);
int KapaSendTextline (int fd, char *string, float x, float y, float angle, int justify, int color);
int KapaSetLimits (int fd, Graphdata *graphmode);
int KapaGetLimits (int fd, float *dx, float *dy);
int KapaSetSection (int fd, KapaSection *section);
int KapaSetSectionByImage (int fd, KapaSection *section);
int KapaSelectSection (int fd, char *name);
int KapaGetSection (int fd, char *name);
int KapaSectionBG (int fd, char *name, int bg);
int KapaMoveSection (int fd, char *name, char *direction);
int KapaSetGraphData (int fd, Graphdata *graphmode);
int KapaGetGraphData (int fd, Graphdata *graphmode);
int KapaScanGraphData (int fd, Graphdata *graphmode);
int KapaSendGraphData (int fd, Graphdata *graphmode);
int KapaSetImageData (int fd, KapaImageData *graphmode);
int KapaGetImageData (int fd, KapaImageData *graphmode);
int KapaSetToolbox (int fd, int location);
int KapaSetSmoothSigma (int fd, float sigma);
int KapaMemoryDump (int fd);
int KapaMemoryDumpLines (int fd, int Nlines);
int KapaMemoryDumpOnExit (int fd, int state);

/* KapaColors */
int KapaColorByName (char *name);
int KapaColormapSize (void);
char *KapaColorRGBString (int N);
char *KapaColorName (int N);
png_color *KapaPNGPalette (int *Npalette);
unsigned long *KapaX11colors (Display *display, Colormap colormap, unsigned long default_color, int *Ncolors);

/* KapaStyles.c */
KapaLineType KapaLineTypeFromString (char *string);
KapaPlotStyle KapaPlotStyleFromString (char *string);
KapaPointStyle KapaPointStyleFromString (char *string);

/* RotFont.c */
void InitRotFonts PROTO((void));
void FreeRotFonts PROTO((void));
int SetRotFont PROTO((char *name, int size));
char *GetRotFont PROTO((int *size));
RotFont *GetRotFontData (double *scale);
int RotStrlen PROTO((char *c));

/* DrawRotString.c */
int DrawRotText PROTO((int x, int y, char *string, int pos, double angle));
int DrawRotBitmap PROTO((int x, int y, int dx, int dy, unsigned char *bitmap, int mode, double angle, double scale));
int DrawRotTextInit (Display *display, Window window, GC gc, unsigned long fore, unsigned long back);

/* PDFRotFont.c */
void PDFRotText (IOBuffer *buffer, int x, int y, char *string, int pos, double angle);

/* PSRotFont.c */
void PSRotText PROTO((FILE *f, int x, int y, char *string, int pos, double angle));
int PSRotStrlen PROTO((char *c));

/* bDrawFuncs.c */
bDrawBuffer *bDrawBufferCreate (int Nx, int Ny, int Nbyte, png_color *palette, int Npalette);
void bDrawBufferFree (bDrawBuffer *buffer);
int bDrawMerge (bDrawBuffer *base, bDrawBuffer *layer);
void bDrawSetBuffer (bDrawBuffer *buffer);
void bDrawSetColor (bDrawBuffer *buffer, bDrawColor color);
void bDrawSetStyle (bDrawBuffer *buffer, bDrawColor color, int lw, int lt, float alpha);
void bDrawPoint (bDrawBuffer *buffer, int x, int y);
void bDrawPointf (bDrawBuffer *buffer, float x, float y);

void bDrawArc (bDrawBuffer *buffer, double Xc, double Yc, double Xr, double Yr, double Ts, double Te);
void bDrawCircle (bDrawBuffer *buffer, double Xc, double Yc, double radius);
void bDrawCircleFill (bDrawBuffer *buffer, double xc, double yc, double radius);

void bDrawLine (bDrawBuffer *buffer, double x1, double y1, double x2, double y2);
void bDrawLineWeight (bDrawBuffer *buffer, int X1, int Y1, int X2, int Y2, int swapcoords);
void bDrawLineBresen (bDrawBuffer *buffer, int X1, int Y1, int X2, int Y2, int swapcoords);
void bDrawLineHorizontal (bDrawBuffer *buffer, int X1, int X2, int Y);
void bDrawLineVertical (bDrawBuffer *buffer, int X, int Y1, int Y2);

void bDrawRectOpen (bDrawBuffer *buffer, double x1, double y1, double x2, double y2);
void bDrawRectFill (bDrawBuffer *buffer, double x1, double y1, double x2, double y2);
void bDrawTriOpen  (bDrawBuffer *buffer, double x1, double y1, double x2, double y2, double x3, double y3);
void bDrawTriFill  (bDrawBuffer *buffer, double x1, double y1, double dx, double dy);
void bDrawPolyFill (bDrawBuffer *buffer, double *x, double *y, int Npoints);

void bDrawSmooth (bDrawBuffer *buffer, float sigma);

/* bDrawRotFont.c */
int bDrawRotText (bDrawBuffer *buffer, int x, int y, char *string, int pos, double angle);
int bDrawRotBitmap (bDrawBuffer *buffer, int x, int y, int dx, int dy, unsigned char *bitmap, int mode, double angle, double scale);

/* Kapa Socket functions */
int KapaOpen (char *kapa_exec, char *kapa_name);
int KapaServerInit (KapaSockAddress *Address);
int KapaServerWait (int InitSocket, KapaSockAddress *Address);
int KapaDefineValidIP (char *ipstring);
int KapaClientSocket (char *hostname);
int KapaOpenNamedSocket (char *kapa_exec, char *name);
int KapaWaitNamedSocket (char *sockpath);
int KapaClose (int socket);

/* define Kapa names for shared functions */
// # define KapaOpen(p,n) KiiOpen(p,n)
# define KapaResize(fd,Nx,Ny) KiiResize(fd,Nx,Ny)
# define KapaCenter(fd,x,y,z) KiiResize(fd,x,y,z)
# define KapaCursorOn(fd) KiiCursorOn(fd)
# define KapaCursorOff(fd) KiiCursorOff(fd)
# define KapaCursorRead(fd,x,y,k) KiiCursorRead(fd,x,y,k)
# define KapaPS(fd,s,r,f) KiiPS(fd,s,r,f)

// deprecated
// # define KapaClose(socket) KiiClose(socket)
# define KapaWait(sockpath) KiiWait(sockpath)
# define KapaSendCommandV(device, length, format, argp) KiiSendCommandV(device, length, format, argp)
# define KapaSendData(device, data, Nbytes) KiiSendData(device, data, Nbytes)
# define KapaRecvData(device) KiiRecvData(device)

/* these use varargs: is this safe, or should we make a stub function? */
# define KapaSendMessage KiiSendMessage
# define KapaSendCommand KiiSendCommand
# define KapaScanMessage KiiScanMessage

# endif
