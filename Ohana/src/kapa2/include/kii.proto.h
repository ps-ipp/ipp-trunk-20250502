
/* top-level program */
int           args                PROTO((int *argc, char **argv));
void          SetUpDisplay        PROTO((Graphic *, int *, char **));
void          SetUpWindow         PROTO((Graphic *, int *, char **));
void          DefineLayout        PROTO((Layout *, Graphic *, int, char **));
int           EventLoop           PROTO((Graphic *, Layout *));
void          CloseDisplay        PROTO((Graphic *));

/* SetUpDisplay */
void          CheckDisplayName    PROTO((int *, char **, char *));
Display      *OpenDisplay         PROTO((char *, int *));
void	      CheckVisual	  PROTO((Graphic *graphic, int *argc, char **argv));
void          CheckColors         PROTO((Graphic *, int *, char **));

/* SetUpWindow */
void          CheckGeometry       PROTO((int *, char **, Graphic *));
void          TopWindow           PROTO((Graphic *, Icon *));
void           CreateWindow       PROTO((Graphic *, Window, int, long));
void           MakeGC             PROTO((Graphic *));
void           MakeCursor         PROTO((Graphic *, unsigned int));
void          LoadFont            PROTO((Graphic *, int *, char **, char *));
void           CheckFontName      PROTO((int *, char **, char *));
void          SetNormalHints      PROTO((Graphic *));
void          SetWMHints          PROTO((Graphic *, Icon *));
void          NameWindow          PROTO((Graphic *, char *));
void          MapWindow           PROTO((Graphic *));

/* DefineLayout - fns below may be called if layout is changed? */
void          PositionPictures    PROTO((Layout *, Graphic *));

void          MakeColormap        PROTO((Graphic *, Layout *, int, char **));
int	      SetColormap	  PROTO((Graphic *graphic, Layout *layout, char *name));
void          CreateColorbar      PROTO((Layout *, Graphic *));
void          CreatePicture       PROTO((Layout *, Graphic *));

/* EventLoop */
int           CheckPipe           PROTO((Graphic *, Layout *));
int           Reconfig            PROTO((Graphic *, Layout *, XEvent *));
void          Refresh             PROTO((Graphic *, Layout *, int));
int           UpdatePointer       PROTO((Graphic *, Layout *, XMotionEvent *));
int           InterpretKeys       PROTO((Graphic *, Layout *, XEvent *));
int           InterpretPresses    PROTO((Graphic *, Layout *, XButtonEvent *));

/* CheckPipe */
int           NewPicture          PROTO((Graphic *, Layout *));
int           EraseOverlay        PROTO((Graphic *, Layout *));
int           LoadOverlay         PROTO((Graphic *, Layout *));
int	      LoadTickmarks	  PROTO((Graphic *graphic, Layout *layout));
int           SaveOverlay         PROTO((Graphic *, Layout *));
int           CSaveOverlay        PROTO((Graphic *, Layout *));
int	      PSit		  PROTO((Graphic *graphic, Layout *layout, int Raw));
int	      JPEGit		  PROTO((Graphic *graphic, Layout *layout));
int	      JPEGit24		  PROTO((Graphic *graphic, Layout *layout));
int	      Resize		  PROTO((Graphic *graphic, Layout *layout));
int	      Center		  PROTO((Graphic *graphic, Layout *layout));

/* imagezoom functions */
void          CreateZoom          PROTO((Layout *, Graphic *, double, double));
void	      CreateZoom8	  PROTO((Layout *layout, Graphic *graphic, double x, double y));
void	      CreateZoom16	  PROTO((Layout *layout, Graphic *graphic, double x, double y));
void	      CreateZoom24	  PROTO((Layout *layout, Graphic *graphic, double x, double y));
void	      CreateZoom32	  PROTO((Layout *layout, Graphic *graphic, double x, double y));
void	      UpdateStatusBox	  PROTO((Graphic *graphic, Layout *layout, double x, double y, double z, int mode));
void          CrossHairs          PROTO((Graphic *, Layout *));

/* X image drawing functions */
void          Reorient            PROTO((Graphic *, Layout *, double, double, int));
void          ReorientOnButton    PROTO((Graphic *, Layout *, XButtonEvent *));
int	      Recenter		  PROTO((Graphic *graphic, Layout *layout));
void          PaintOverlay        PROTO((Graphic *, Layout *, int));
void	      PaintTickmarks	  PROTO((Graphic *graphic, Layout *layout));
void          Remap               PROTO((Graphic *, Layout *, Matrix  *));
void	      Remap8		  PROTO((Graphic *graphic, Layout *layout, Matrix *matrix));
void	      Remap16		  PROTO((Graphic *graphic, Layout *layout, Matrix *matrix));
void	      Remap24		  PROTO((Graphic *graphic, Layout *layout, Matrix *matrix));
void	      Remap32		  PROTO((Graphic *graphic, Layout *layout, Matrix *matrix));

/* PS image drawing functions */
void	      ConvertPixmap8	  PROTO((Layout *layout, FILE *f));
void	      ConvertPixmap16	  PROTO((Layout *layout, FILE *f));
void	      ConvertPixmap24	  PROTO((Layout *layout, FILE *f));
void	      ConvertPixmap32	  PROTO((Layout *layout, FILE *f));
void	      DrawOverlay	  PROTO((Graphic *graphic, Layout *layout, int N, FILE *f, int extra));

/* JPEG image drawing functions */
void	      bDrawOverlay	  PROTO((Layout *layout, int N));

/***** Prototypes for image ***************/
void          StatusBox           PROTO((Graphic *, Layout *));

/***** Prototypes for action ***************/
int           InPicture           PROTO((XButtonEvent *, Picture *));
int           Stop                PROTO((Graphic *, Layout *));
void          DragColorbar        PROTO((Graphic *, Layout *, XButtonEvent *));
void          ResetColorbar       PROTO((Graphic *, Layout *, double, double));

/***** Prototypes for button ***************/
Button       *CheckButtons        PROTO((XButtonEvent *, Layout *));
void          DrawButton          PROTO((Graphic *, Button *));
void          FlashButton         PROTO((Graphic *, Button *));
int           InButton            PROTO((XButtonEvent *, Button *));
void          InvertButton        PROTO((Graphic *, Button *));
int           greycolors          PROTO((Graphic *, Layout *));
int           rainbow             PROTO((Graphic *, Layout *));
int           puns                PROTO((Graphic *, Layout *));
int           Recenter            PROTO((Graphic *, Layout *));
int           RecenterRescale     PROTO((Graphic *, Layout *));
int           Rescale             PROTO((Graphic *, Layout *));
int           ToggleDEG           PROTO((Graphic *, Layout *));
int           Overlay0            PROTO((Graphic *, Layout *));
int           Overlay1            PROTO((Graphic *, Layout *));
int           Overlay2            PROTO((Graphic *, Layout *));
int           Overlay3            PROTO((Graphic *, Layout *));

/***** Prototypes for xtools ***************/
void	      Screen_to_Image     PROTO((double *x1, double *y1, double x2, double y2, Layout *layout));
void	      Image_to_Screen	  PROTO((double *x1, double *y1, double x2, double y2, Layout *layout));
unsigned long GetColor            PROTO((Display *, char *, Colormap, unsigned long));
void          QuitX               PROTO((Display *, char *, char *));
void          hh_hms              PROTO((char *, double, double, char));
void          DrawBitmap          PROTO((Graphic *, int, int, int, int, char *, int));
int	      cursor		  PROTO((Graphic *graphic, Layout *layout));
void	      FlushDisplay	  PROTO((Display *display));
