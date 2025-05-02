/* hardwired values for some window parameters */

# define EVENT_MASK (long) \
(ButtonPressMask \
 | ClientMessage \
 | ButtonReleaseMask \
 | KeyPressMask \
 | ExposureMask \
 | StructureNotifyMask \
 | PointerMotionMask)

# define NCHANNELS 10
# define LONG_LINE_LENGTH 1024

# define PAD1  3
# define PAD2  5
# define TEXTPAD 25
# define COLORPAD 10
# define TEXT_Y 15
# define ZOOM_X 152
# define ZOOM_Y 152
# define NOVERLAYS 4
# define BUTTON_WIDTH 28
# define BUTTON_HEIGHT 28

# define DEFAULT_CURSOR XC_crosshair
# define BORDER_WIDTH 2
# define MIN_WIDTH 50
# define MIN_HEIGHT 50
# define LABEL_MAXLEN 512

typedef enum {
    KAPA_SCALE_1D,
    KAPA_SCALE_3D_RUFF,
    KAPA_SCALE_3D_FULL
} KapaColorScaleMode;

/* label names */
typedef enum {
  LABELX0,
  LABELY0,
  LABELX1,
  LABELY1,
  LABELUL,
  LABELUR,
  LABELLL,
  LABELLR
} KapaLabelMode;

typedef enum {
  KAPA_CHANNEL_RED,
  KAPA_CHANNEL_GREEN,
  KAPA_CHANNEL_BLUE,
} KapaChannels;

// use an enum to identify the 3 dimensions:
typedef enum {
  CC_X,
  CC_Y,
  CC_Z,
} CCDimen;

/* EVENT_MASK consists of:

ExposureMask        : Expose
StructureNotifyMask : CirculateNotify | 
                      ConfigureNotify | 
                      DestroyNotify   | 
		      GravityNotify   | 
		      MapNotify       |
		      ReparentNotify  |
		      UnmapNotify
ButtonPressMask     : ButtonPress
ButtonReleaseMask   : ButtonRelease
KeyPressMask        : KeyPress
PointerMotionMask   : MotionNotify
(always)            : ClientMessage 
(always)            : MappingNotify

*/

# define NOVERLAYS 4
/* number of overlays is defined here.
   the number is also crucial in the following files:
   PositionPictures.c
   MakeColormap.c
   prototypes.h
*/


