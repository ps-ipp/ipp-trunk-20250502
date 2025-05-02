We need two FITS tables to describe the burn corrections.  Each
correction area is basically a rectangle within a cell and there are a
bunch of parameters which describe the size and other properties of
the rectangle.

Then, for each column in each rectangle there are three parameters
which describe the burn fit: the start x,y and one fit parameter (the
amplitude; all fits share the same shape).  The rows in this table
need to have a pointer back to the first table of generic rectangle info.

There is also data calculated for each cell: bias, sky, and
background, which are probably not worth keeping.

The persist_read() routine reads the tabulated quantities and stuffs
them into an array of OBJBOX structures, and these are then copied to
an array of CELL structures, one for each of the 64 cells, using the
OBJBOX pointer and count.  The persist_write() routine writes this
information to a text file.  (The structures are in burntool.h; the
read/write routines are in persistio.c.)

Here are the entries for the first table, listed as entries in an
OBJBOX structure:

  int cell;      /* what cell is this one in? */
  int time;      /* PON time when it was created */
  int cx;        /* center x (position of max) */
  int cy;        /* center y (position of max) */
  int max;       /* max data value (above sky) */
  int y0;        /* y origin for the fit (stamp sy for stars) */
  int sx;        /* left corner */
  int sy;        /* bottom corner */
  int ex;        /* right corner */
  int ey;        /* top corner */
  int y0m;       /* min y value at sx */
  int y0p;       /* max y value at sx */
  int y1m;       /* min y value at ex */
  int y1p;       /* max y value at ex */
  int x0m;       /* min x value at sy */
  int x0p;       /* max x value at sy */
  int x1m;       /* min x value at ey */
  int x1p;       /* max x value at ey */
  int func;      /* what are we going to do about it? */
  int up;        /* does it trail up or down? */
  int nfit;      /* how many columns were corrected? */
  int sxfit;     /* starting column for fits */
  int exfit;     /* ending column for fits */
  double slope;  /* slope of fit */

Here are the entries in the second table, listed as entries in three
arrays which are part of the OBJBOX structure.  In addition each entry
in the second pointer requires a pointer back to its burn rectangle in
the first table, which can be an integer coding the (cell,cx,cy)
parameters because they will be unique.

  int objptr;    /* Cell,cx,cy coded as ccxxxyyy */
  int xfit;	 /* x of each value of the start of correction */
  int yfit;	 /* y value of the start of correction */
  double zero;	 /* zero of fit for each of the columns */

The CELL structure only needs to have the following two entries filled
in for each of the 64 structures in the array describing the entire OTA:

  int npersist;	    /* Number of old persistence streaks */
  OBJBOX *persist;  /* Persistent streaks */
