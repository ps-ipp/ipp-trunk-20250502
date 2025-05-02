# include "Ximage.h"
# define DEBUG 0

/* list events being selected below, all other masks are ignored */ 
# define IgnoreMask (long) (~(StructureNotifyMask | SubstructureNotifyMask | ExposureMask | KeyPressMask | ButtonPressMask | PointerMotionMask))

int LastEvent (Display *display, int type, XEvent *event) {

  int found, Nfound;

  found = FALSE;
  Nfound = 0;
  while (XCheckTypedEvent (display, type, event)) {
    // If the link is slow, then I should wait a little while for some config events to
    // build up (typically, the window is being dragged around, so we get a whole series of 
    // config events.  the flush / sync time is slow on a slow link, so we should try to wait
    // for the config events to stop, but on a timescale deteremined by the flush/sync time
    // if (type == ConfigureNotify) fprintf (stderr, "config (%d)\n", Nfound);
    Nfound ++;
    found = TRUE;
  }
  if (DEBUG && found) {
    if (type == ConfigureNotify) fprintf (stderr, "config (%d)\n", Nfound);
    if (type == CirculateNotify) fprintf (stderr, "circul (%d)\n", Nfound);
    if (type == Expose) fprintf (stderr, "expose (%d)\n", Nfound);
  }

  // if I have a ConfigureNotify event, I should purge all Expose events as well:
  if (found && (type == ConfigureNotify)) {
    XEvent discard;
    while (XCheckTypedEvent (display, Expose, &discard));
  }
  return (found);
}

int EventLoop () {
  
  int      status;
  XEvent   event;
  Display *display;
  Graphic *graphic;

  graphic = GetGraphic();
  display = graphic->display;
  
  if (USE_XWINDOW) Refresh ();

  status = TRUE;
  while (status) {
    
    if (!CheckPipe ()) return (FALSE);
    
    if (!USE_XWINDOW) {
      usleep (50000);
      continue;
    }

    if (XEventsQueued (display, QueuedAfterFlush) < 1) {
      usleep (50000);
      continue;
    }

    // If I have a config event, I want to also purge all expose events

    /* grab the last entry for these events */
    if (LastEvent (display, ConfigureNotify, &event)) { Reconfig (&event); continue; }
    if (LastEvent (display, CirculateNotify, &event)) { Reconfig (&event); continue; }
    if (LastEvent (display, Expose,          &event)) { Refresh (); continue; }
    if (LastEvent (display, MappingNotify,   &event)) XRefreshKeyboardMapping ((XMappingEvent *) &event);
    if (LastEvent (display, MotionNotify,    &event)) UpdatePointer (graphic, (XMotionEvent *) &event);
    if (LastEvent (display, ButtonPress,     &event)) InterpretPresses (graphic, (XButtonEvent *) &event);
    if (LastEvent (display, KeyPress,        &event)) InterpretKeys (graphic, (XKeyEvent *) &event);

    /* drop and ignore the following StructureNotifyMask events */
    LastEvent (display, GravityNotify, &event);
    LastEvent (display, ReparentNotify, &event);
    LastEvent (display, MapNotify, &event);
    LastEvent (display, UnmapNotify, &event);

    /* remove those events we will ignore */
    while (XCheckMaskEvent (display, IgnoreMask, &event)) continue;

    /* events to remove which have no mask component */
    while (XCheckTypedEvent (display, MappingNotify, &event)) continue;
    while (XCheckTypedEvent (display, ClientMessage, &event)) continue;
    while (XCheckTypedEvent (display, SelectionClear, &event)) continue;
    while (XCheckTypedEvent (display, SelectionNotify, &event)) continue;
    while (XCheckTypedEvent (display, SelectionRequest, &event)) continue;
  }
  return (status);
}

# if (0)

/* all masks from X.h for reference: */

#define NoEventMask                     0L
#define KeyPressMask                    (1L<<0)
#define KeyReleaseMask                  (1L<<1)
#define ButtonPressMask                 (1L<<2)
#define ButtonReleaseMask               (1L<<3)
#define EnterWindowMask                 (1L<<4)
#define LeaveWindowMask                 (1L<<5)
#define PointerMotionMask               (1L<<6)
#define PointerMotionHintMask           (1L<<7)
#define Button1MotionMask               (1L<<8)
#define Button2MotionMask               (1L<<9)
#define Button3MotionMask               (1L<<10)
#define Button4MotionMask               (1L<<11)
#define Button5MotionMask               (1L<<12)
#define ButtonMotionMask                (1L<<13)
#define KeymapStateMask                 (1L<<14)
#define ExposureMask                    (1L<<15)
#define VisibilityChangeMask            (1L<<16)
#define StructureNotifyMask             (1L<<17)
#define ResizeRedirectMask              (1L<<18)
#define SubstructureNotifyMask          (1L<<19)
#define SubstructureRedirectMask        (1L<<20)
#define FocusChangeMask                 (1L<<21)
#define PropertyChangeMask              (1L<<22)
#define ColormapChangeMask              (1L<<23)
#define OwnerGrabButtonMask             (1L<<24)

/* all events from X.h for reference: */

#define KeyPress                2
#define KeyRelease              3
#define ButtonPress             4
#define ButtonRelease           5
#define MotionNotify            6
#define EnterNotify             7
#define LeaveNotify             8
#define FocusIn                 9
#define FocusOut                10
#define KeymapNotify            11
#define Expose                  12
#define GraphicsExpose          13
#define NoExpose                14
#define VisibilityNotify        15
#define CreateNotify            16
#define DestroyNotify           17
#define UnmapNotify             18
#define MapNotify               19
#define MapRequest              20
#define ReparentNotify          21
#define ConfigureNotify         22
#define ConfigureRequest        23
#define GravityNotify           24
#define ResizeRequest           25
#define CirculateNotify         26
#define CirculateRequest        27
#define PropertyNotify          28
#define SelectionClear          29
#define SelectionRequest        30
#define SelectionNotify         31
#define ColormapNotify          32
#define ClientMessage           33
#define MappingNotify           34
#define LASTEvent               35      /* must be bigger than any event # */

# endif
