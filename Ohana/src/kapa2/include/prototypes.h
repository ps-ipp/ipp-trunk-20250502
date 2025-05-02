
/* top-level program */
int	      args		  PROTO((int *argc, char **argv));
void          SetUpGraphic        PROTO((int *argc, char **argv));
void          FreeGraphic         PROTO((void));
void          DefineLayout        PROTO((int, char **));
int	      EventLoop		  PROTO((void));
void	      CloseDisplay	  PROTO((void));
int           MemoryDump          PROTO((int sock));
int           MemoryDumpLines     PROTO((int sock));
int           MemoryDumpOnExit    PROTO((int sock));
int           MemoryDumpSetOnExit PROTO((int state));
int           MemoryDumpAndExit   PROTO((void));

/* SetUpGraphic */
char         *CheckDisplayName    PROTO((int *argc, char **argv));
Display      *OpenDisplay         PROTO((char *name, int *screen));
void          CheckColors         PROTO((Graphic *graphic, int *argc, char **argv));

/* SetUpWindow */
void          CheckGeometry       PROTO((Graphic *graphic, int *argc, char **argv));
void          TopWindow           PROTO((Graphic *graphic, Icon *icon));
void          CreateWindow        PROTO((Graphic *graphic, Window parent, int border, long events));
void	      MakeGC	    	  PROTO((Graphic *graphic));
void          MakeCursor          PROTO((Graphic *graphic, unsigned int cursor));
void          LoadFont            PROTO((Graphic *graphic, int *argc, char **argv, char *default_name));
void          SetNormalHints      PROTO((Graphic *graphic));
void          SetWMHints          PROTO((Graphic *graphic, Icon *icon));
void          NameWindow          PROTO((Graphic *graphic, char *name));
void          MapWindow           PROTO((Graphic *graphic));
void          CheckVisual         PROTO((Graphic *graphic, int *argc, char **argv));

/* X drawing utilities */
int	      DrawFrame		  PROTO((KapaGraphWidget *graph));
int           DrawObjects         PROTO((KapaGraphWidget *graph));
void          DrawLabels          PROTO((KapaGraphWidget *graph));
void          EraseLabels         PROTO((KapaGraphWidget *graph));
void          DrawLabelsRaw       PROTO((Graphic *graphic, KapaGraphWidget *graph, int color));
void	      DrawTextlines	  PROTO((KapaGraphWidget *graph));
void          DrawConnect         PROTO((Graphic *graphic, KapaGraphWidget *graph, Gobjects *objects));
void          DrawPolygon         PROTO((Graphic *graphic, KapaGraphWidget *graph, Gobjects *objects));
void          DrawPolyfill        PROTO((Graphic *graphic, KapaGraphWidget *graph, Gobjects *objects));
void          DrawHistogram       PROTO((Graphic *graphic, KapaGraphWidget *graph, Gobjects *objects));
int           DrawObjectN         PROTO((Graphic *graphic, KapaGraphWidget *graph, Gobjects *objects));
void          DrawPoints          PROTO((Graphic *graphic, KapaGraphWidget *graph, Gobjects *objects));
void          DrawBars            PROTO((Graphic *graphic, KapaGraphWidget *graph, Gobjects *object, int mode));
void          ClipLine            PROTO((Graphic *graphic, double x0, double y0, double x1, double y1, double X0, double Y0, double X1, double Y1));
void          DrawXErrors         PROTO((Graphic *graphic, KapaGraphWidget *graph, Gobjects *objects));
void          DrawYErrors         PROTO((Graphic *graphic, KapaGraphWidget *graph, Gobjects *objects));
void	      DrawTick		  PROTO((Graphic *graphic, Axis *axis, int P, TickMarkData *tick, int naxis));

/* TickMark functions */
void	      AxisTickScale	  PROTO((Axis *axis, double *range, double *major, double *minor, int *nsignif));
TickMarkData *CreateAxisTicks     PROTO((Axis *axis, int *nticks));
int           PrintTick           PROTO((char *string, TickMarkData *tick, double min, double max));

/* EventLoop */
int           PScommand           PROTO((int sock));
int           PNGcommand          PROTO((int sock));
int           CheckPipe           PROTO((void));
int           Reconfig            PROTO((XEvent *event));
void          Refresh             PROTO((void));
void          DrawSectionBG       PROTO((Graphic *graphic, Section *section));

/* CheckPipe */
int           PNGit               PROTO((char *filename));
int           PPMit               PROTO((int sock));
int           LoadFrame           PROTO((int sock));
int           LoadObject          PROTO((int sock));
int           LoadLabels          PROTO((int sock));
int           LoadTextlines       PROTO((int sock));
int           Resize              PROTO((int sock));
int           Relocate            PROTO((int sock));
int           GetLimits           PROTO((int sock));
int           SetLimits           PROTO((int sock));
int           SetSection          PROTO((int sock));
int           ListSection         PROTO((int sock));
int           SetSectionBG        PROTO((int sock));
int           MoveSection         PROTO((int sock));
int           DefineSection       PROTO((int sock));
int           DefineSectionByImage PROTO((int sock));
int           SetFont             PROTO((int sock));
int           EraseCurrentPlot    PROTO((void));
int           ErasePlots          PROTO((void));
int           EraseSections       PROTO((void));
int           EraseImage          PROTO((void));
int           SetGraphData        PROTO((int sock));
int           GetGraphData        PROTO((int sock));
int           SetImageData        PROTO((int sock));
int           GetImageData        PROTO((int sock));
int           SetImageCoords      PROTO((int sock));
int           GetImageCoords      PROTO((int sock));
int           GetImageRange       PROTO((int sock));
int           SetChannel          PROTO((int sock));
int           SetColormapFromPipe PROTO((int sock));
int           SetNanColorFromPipe PROTO((int sock));
int           SetSmoothSigma      PROTO((int sock));

int           LoadVectorData      PROTO((int sock, KapaGraphWidget *graph, int N, char *type));

/* Section Utilities */
Section      *InitSection	  PROTO((void));
void          FreeSection	  PROTO((Section *section));
void          FreeSections	  PROTO((void));
Section      *AddSection	  PROTO((char *name, float x, float y, float dx, float dy, int bg));
int           DelSection	  PROTO((char *name));
int           GetSectionByName	  PROTO((char *name));
int           GetNumberOfSections PROTO((void));
Section      *GetSectionByNumber  PROTO((int N));
Section      *GetActiveSection	  PROTO((void));
int           SetActiveSectionByNumber PROTO((int N));
int           ListSection         PROTO((int sock));
void          SetSectionSizes     PROTO((Section *section));
int           SectionMinBoundary  PROTO((Graphic *graphic));

KapaGraphWidget *InitGraph        PROTO((void));
void          DrawGraph           PROTO((KapaGraphWidget *graph));
void          SetGraphSize        PROTO((Section *section));
void          FreeGraph           PROTO((KapaGraphWidget *graph));

void          InitLayout          PROTO((int argc, char **argv));
void          FreeLayout          PROTO((void));

/* PDF drawing primitives */
double        PDF_SetLineWeight  PROTO((IOBuffer *buffer, double lweightIn));
void          PDF_SetKapaColor   PROTO((IOBuffer *buffer, bDrawColor color));
void          PDF_SetScaledColor PROTO((IOBuffer *buffer, float *pixel1, float *pixel2, float *pixel3, float value, int Npixels));
void          PDF_DrawCircle     PROTO((IOBuffer *buffer, float Xc, float Yc, float R, int isFill));

/* PDF print utils */
PDF_FILE     *PDF_Open           PROTO((char *filename));
int           PDF_Print          PROTO((PDF_FILE *obj, int newObject, char *format, ...));
int           PDF_Close          PROTO((PDF_FILE *obj));
int           PDF_CreateStream   PROTO((IOBuffer *buffer, float scale, int Xoff, int Yoff));
int           PDF_WriteStream    PROTO((PDF_FILE *obj, IOBuffer *buffer));
int           PDF_WriteImage     PROTO((PDF_FILE *obj, IOBuffer *buffer, int dX, int dY));
void          PDF_AlphaDump      PROTO((PDF_FILE *obj));
void          PDF_AlphaSet       PROTO((Gobjects *object, IOBuffer *buffer));
void          PDF_AlphaInit      PROTO(());

/* PDF drawing utilities */
int           PDFcommand          PROTO((int sock));
int           PDFit               PROTO((char *filename, char *pagename, int scaleMode, int pageMode));
int           PDF_Frame           PROTO((KapaGraphWidget *graph, IOBuffer *buffer));
int           PDF_Objects         PROTO((KapaGraphWidget *graph, IOBuffer *buffer));
void          PDF_Labels          PROTO((KapaGraphWidget *graph, IOBuffer *buffer));
void	      PDF_Textlines	  PROTO((KapaGraphWidget *graph, IOBuffer *buffer));
int           PDF_ObjectsN        PROTO((KapaGraphWidget *graph, Gobjects *objects, IOBuffer *buffer));
void          PDF_Connect         PROTO((KapaGraphWidget *graph, Gobjects *objects, IOBuffer *buffer));
void          PDF_Histogram       PROTO((KapaGraphWidget *graph, Gobjects *objects, IOBuffer *buffer));
void          PDF_Points          PROTO((KapaGraphWidget *graph, Gobjects *objects, IOBuffer *buffer));
void          PDF_XErrors         PROTO((KapaGraphWidget *graph, Gobjects *objects, IOBuffer *buffer));
void          PDF_YErrors         PROTO((KapaGraphWidget *graph, Gobjects *objects, IOBuffer *buffer));
void	      PDF_Tick		  PROTO((Graphic *graphic, Axis *axis, int P, TickMarkData *tick, int naxis, IOBuffer *buffer));
void          PDF_ClipLine        PROTO((double x0, double y0, double x1, double y1, double X0, double Y1, double X1, double Y0, IOBuffer *buffer));
int           PDF_Image    	  PROTO((PDF_FILE *obj, KapaImageWidget *image, IOBuffer *buffer));
void 	      PDF_Overlay  	  PROTO((KapaImageWidget *image, int N, IOBuffer *buffer, int extra));
void          PDF_Pixmap          PROTO((Graphic *graphic, KapaImageWidget *image, IOBuffer *buffer));

/* PS drawing utilities */
int           PSit                PROTO((char *filename, char *pagename, int scaleMode, int pageMode));
int           PSFrame             PROTO((KapaGraphWidget *graph, FILE *f));
int           PSObjects           PROTO((KapaGraphWidget *graph, FILE *f));
void          PSLabels            PROTO((KapaGraphWidget *graph, FILE *f));
void	      PSTextlines	  PROTO((KapaGraphWidget *graph, FILE *f));
int           PSObjectsN          PROTO((KapaGraphWidget *graph, Gobjects *objects, FILE *f));
void          PSConnect           PROTO((KapaGraphWidget *graph, Gobjects *objects, FILE *f));
void          PSHistogram         PROTO((KapaGraphWidget *graph, Gobjects *objects, FILE *f));
void          PSPoints            PROTO((KapaGraphWidget *graph, Gobjects *objects, FILE *f));
void          PSXErrors           PROTO((KapaGraphWidget *graph, Gobjects *objects, FILE *f));
void          PSYErrors           PROTO((KapaGraphWidget *graph, Gobjects *objects, FILE *f));
void	      PSTick		  PROTO((Graphic *graphic, Axis *axis, int P, TickMarkData *tick, int naxis, FILE *f));
void          ClipLinePS          PROTO((double x0, double y0, double x1, double y1, double X0, double Y1, double X1, double Y0, FILE *f));
int           PSimage    	  PROTO((KapaImageWidget *image, FILE *f));
void 	      PSOverlay  	  PROTO((KapaImageWidget *image, int N, FILE *f, int extra));
void 	      PSPixmap8  	  PROTO((Graphic *graphic, KapaImageWidget *image, FILE *f));
void 	      PSPixmap16 	  PROTO((Graphic *graphic, KapaImageWidget *image, FILE *f));
void 	      PSPixmap24 	  PROTO((Graphic *graphic, KapaImageWidget *image, FILE *f));
void 	      PSPixmap32 	  PROTO((Graphic *graphic, KapaImageWidget *image, FILE *f));
void          PSPixmap_3byte      PROTO((Graphic *graphic, KapaImageWidget *image, FILE *f));

/* kapa bDraw Functions */
int	      bDrawFrame	  PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph));
int	      bDrawObjects	  PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph));
void	      bDrawLabels	  PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph));
void	      bDrawTextlines	  PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph));
int	      bDrawObjectsN	  PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph, Gobjects *object));
void	      bDrawConnect	  PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph, Gobjects *object));
void	      bDrawHistogram	  PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph, Gobjects *object));
void          bDrawBars           PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph, Gobjects *object, int mode));
void	      bDrawPoints	  PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph, Gobjects *object));
void	      bDrawPolygons	  PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph, Gobjects *object));
void	      bDrawFillPolygons	  PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph, Gobjects *object));
void	      bDrawXErrors	  PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph, Gobjects *object));
void	      bDrawYErrors	  PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph, Gobjects *object));
void	      bDrawTick		  PROTO((bDrawBuffer *buffer, Axis *axis, int P, TickMarkData *tick, int naxis));
void	      bDrawClipLine	  PROTO((bDrawBuffer *buffer, double x0, double y0, double x1, double y1, double X0, double Y1, double X1, double Y0));
void          bDrawGraph          PROTO((bDrawBuffer *buffer, KapaGraphWidget *graph));
int           bDrawImage          PROTO((bDrawBuffer *buffer, KapaImageWidget *image, Graphic *graphic));
bDrawBuffer  *bDrawIt		  PROTO((png_color *palette, int Npalette, int Nbyte));

/* misc support */
int           LastEvent           PROTO((Display *display, int type, XEvent *event));
void	      FreeObjectData	  PROTO((Gobjects *object));
void	      FlushDisplay	  PROTO((void));
unsigned long GetColor            PROTO((Display *display, char *name, Colormap colormap, unsigned long default_color));
void          QuitX               PROTO((Display *display, char *message));
Graphic      *GetGraphic          PROTO((void));
int           GetPixelCount       PROTO((int sock));

int           Center              PROTO((int sock));
int           Parity              PROTO((int sock));
void          SetColorScale       PROTO((Graphic *graphic, KapaImageWidget *image));
void          SetColorScale1D     PROTO((Graphic *graphic, KapaImageWidget *image));
int           SetColorScale3D     PROTO((Graphic *graphic, KapaImageWidget *image));
void          Remap               PROTO((Graphic *graphic, KapaImageWidget *image));
void          Remap8              PROTO((Graphic *graphic, KapaImageWidget *image, Picture *picture, Matrix *matrix));
void          Remap16             PROTO((Graphic *graphic, KapaImageWidget *image, Picture *picture, Matrix *matrix));
void          Remap24             PROTO((Graphic *graphic, KapaImageWidget *image, Picture *picture, Matrix *matrix));
void          Remap32             PROTO((Graphic *graphic, KapaImageWidget *image, Picture *picture, Matrix *matrix));
int           LoadPicture         PROTO((int sock));

KapaImageWidget *InitImageWidget  PROTO((void));
int           InitImageChannel    PROTO((KapaImageChannel *channel));
void          FreeImage           PROTO((KapaImageWidget *image));
void          SetImageSize        PROTO((Section *section));

void          InitButtonSize      PROTO((Button *button, int width, int height, unsigned char *bitmap));
void          InitButtonFunc      PROTO((Button *button, int (*function)(Graphic *graphic, KapaImageWidget *image)));
void          DrawImage           PROTO((KapaImageWidget *image));
void          DrawImageTool       PROTO((KapaImageWidget *image));
void          DrawButton          PROTO((Graphic *graphic, Button *button));
void          DrawBitmap          PROTO((Graphic *graphic, int x, int y, int dx, int dy, unsigned char *bitmap, int mode));
void          CrossHairs          PROTO((Graphic *graphic, Picture *image));
void          hh_hms              PROTO((char *line, double ra, double dec, char sep, int Nchar));

int           SetColormap         PROTO((char *name));
void          MakeColormap        PROTO((int argc, char **argv));

void          PaintOverlay        PROTO((Graphic *graphic, KapaImageWidget *image, int N));
void          PaintTickmarks      PROTO((Graphic *graphic, KapaImageWidget *image));

void          Picture_to_Image    PROTO((double *x1, double *y1, double x2, double y2, Picture *picture));
void          Screen_to_Image     PROTO((double *x1, double *y1, double x2, double y2, Picture *picture));
void          Image_to_Picture    PROTO((double *x1, double *y1, double x2, double y2, Picture *picture));
void          Image_to_Screen     PROTO((double *x1, double *y1, double x2, double y2, Picture *picture));
void          Picture_Lower 	  PROTO((int *i_start, int *j_start, int *I_start, int *J_start, Matrix *matrix, Picture *picture));
void          Picture_Upper 	  PROTO((int *i_end, int *j_end, int i_start, int j_start, Matrix *matrix, Picture *picture));

void          DragColorbar        PROTO((Graphic *graphic, KapaImageWidget *image, XButtonEvent *mouse_event));
void          ResetColorbar       PROTO((Graphic *graphic, double start, double slope));
void          ReorientOnButton    PROTO((Graphic *graphic, KapaImageWidget *image, XButtonEvent *mouse_event));
void          Reorient            PROTO((Graphic *graphic, KapaImageWidget *image, double X, double Y, int mode));

Button       *CheckButtons        PROTO((XButtonEvent *event, KapaImageWidget *image));
int           InButton            PROTO((XButtonEvent *event, Button *button));
int           InPicture           PROTO((XButtonEvent *event, Picture *picture));

/* Button Functions */
int           greycolors	  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      heat		  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      rainbow		  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      Recenter		  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      Rescale		  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      RecenterRescale	  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      ToggleDEG		  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      ToggleHEX		  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      FlipImageX	  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      FlipImageY	  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      Overlay0		  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      Overlay1		  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      Overlay2		  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      Overlay3		  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      PSfunction	  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      PNGfunction	  PROTO((Graphic *graphic, KapaImageWidget *image));
int	      JPEGfunction	  PROTO((Graphic *graphic, KapaImageWidget *image));

/* misc functions */
void 	      SetToolbox   (int sock);
int  	      EraseOverlay (int sock);
int  	      LoadOverlay  (int sock);
int  	      SaveOverlay  (int sock);
int  	      CSaveOverlay (int sock);
int  	      JPEGit24     (char *filename);
int  	      JPEGcommand  (int sock);

int  	      UpdatePointer (Graphic *graphic, XMotionEvent *event);
int  	      InterpretPresses (Graphic *graphic, XButtonEvent *event);
int  	      InterpretKeys (Graphic *graphic, XKeyEvent *event);
void 	      InitPipe (char *namedSocket);
void 	      EraseGraph (KapaGraphWidget *graph);
void 	      StatusBox (Graphic *graphic, KapaImageWidget *image);

void 	      CreatePicture (KapaImageWidget *image, Graphic *graphic);
void 	      CreateColorbar (KapaImageWidget *image, Graphic *graphic);
void 	      CreateZoom (Graphic *graphic, KapaImageWidget *image);
void 	      CreateWide (Graphic *graphic, KapaImageWidget *image);
void 	      UpdateStatusBox (Graphic *graphic, KapaImageWidget *image, double x, double y, double z, int mode);

// void 	      CreateZoom8  (KapaImageWidget *image, Graphic *graphic, double x, double y);
// void 	      CreateZoom16 (KapaImageWidget *image, Graphic *graphic, double x, double y);
// void 	      CreateZoom24 (KapaImageWidget *image, Graphic *graphic, double x, double y);
// void 	      CreateZoom32 (KapaImageWidget *image, Graphic *graphic, double x, double y);

int  	      GetActiveSocket (void);
void 	      InvertButton (Graphic *graphic, Button *button);
void 	      bDrawOverlay (bDrawBuffer *buffer, KapaImageWidget *image, int N);

/* color cube tools */
CCNode *CCNodeAlloc (void);
CCNode *CCFindChild (CCNode *node, float x, float y, float z);
int CCSplitNode (CCNode *node);
int CCSplitNodeIterate (CCNode *node, int current, int max);
void CCNodeFree (CCNode *node);
CCNode *CCFindBottom (CCNode *top, float x, float y, float z);
int CCNodeExtractCounts (CCNode *node, float **values, int *nvalues, int *NVALUES);
int CCNodeDivideLimit (CCNode *node, float minValue);
int CCNodeInitCounts (CCNode *node, float value);

/* 3D color histogram */
void ColorHistogram (KapaImageWidget *image, CCNode *cube);
void CCNodeSetColorPixels (KapaImageWidget *image, CCNode *cube);
int CCNodeSetColorMap (CCNode *node, XColor *cmap, int Npixels, int *current);

int SetColorScale3D_CC (Graphic *graphic, KapaImageWidget *image);
int SetColorCubeHistogram (void);
int GetGraphBoundary (Section *section, double *x0, double *y0, double *x1, double *y1, int *dXm, int *dXp, int *dYm, int *dYp);
int ResizeByImage (int sock);

