
int   ACTIVE_CURSOR;
int   HAVE_BACKING;
int   DEBUG;
int   USE_XWINDOW;
int   MAP_WINDOW;
int   FOREGROUND;
char *NAME_WINDOW;

int   NAN_RED;
int   NAN_GREEN;
int   NAN_BLUE;

int   NPIXELS_DYNAMIC;
int   NPIXELS_STATIC;

/* these should be absorbed into KapaImageWidget 
int OVERLAY[NOVERLAYS];
int MOVE_POINTER;
int DECIMAL_DEG;
*/

/* file descriptor for socket connection to mana */
// int sock; 

/* each layout / section defines one of the active portions of the graphics window
   there may be an arbitrary number of sections, and each section may fill an arbitrary
   rectangle in the X window.  The startup configuration assumes 1 section filling the 
   entire X window. */

// Layout *section;
// int    Nsection, TheSection;

/* graphic defines the basic details of the X window */
// Graphic graphic;
