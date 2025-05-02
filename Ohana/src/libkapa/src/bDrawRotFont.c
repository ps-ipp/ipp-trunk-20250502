# include <kapa_internal.h>

# define NROTCHARS 256
# define XPROC(x,y) (scale*(cs*((x) - x0) - sn*((y) - y0)) + X0)
# define YPROC(x,y) (scale*(cs*((y) - y0) + sn*((x) - x0)) + Y0)
# define NEARINT(x) ((x < 0) ? ((int)(x - 0.5)) : ((int)(x + 0.5)))
  
// XXX need to pass the rot text color here
// static bDrawColor black;
// static bDrawColor white;

int bDrawRotText (bDrawBuffer *buffer, int x, int y, char *string, int pos, double angle) {

  unsigned char *bitmap;
  char *currentname, basename[64]; 
  int dy, dx, N, X, Y, code, protect;
  int dX, Xoff, dY, Yoff, YoffBase;
  int currentsize, basesize;
  double cs, sn, currentscale;
  RotFont *currentfont;

  // white = KapaColorByName ("white");
  // black = KapaColorByName ("black");

  currentname = GetRotFont (&currentsize);
  currentfont = GetRotFontData (&currentscale);
  strcpy (basename, currentname);
  basesize = currentsize;

  /* strip leading WHITESPACE */
  stripwhite (string);
  if (*string == 0) return (FALSE);
  
  /* compute string length */
  cs = cos(angle*RAD_DEG);
  sn = sin(angle*RAD_DEG);
  dX = RotStrlen (string);
  dY = currentfont[65].ascent;
  int dP = ceil(0.25 * dY);

  /* apply appropriate offset */
  Xoff = Yoff = 0;
  switch (pos) {
    case 0: Xoff =  -dX-dP; Yoff = dY+dP; break;
    case 1: Xoff = -0.5*dX; Yoff = dY+dP; break;
    case 2: Xoff =      dP; Yoff = dY+dP; break;
    case 3: Xoff =  -dX-dP; Yoff = 0.5*dY; break;
    case 4: Xoff = -0.5*dX; Yoff = 0.5*dY; break;
    case 5: Xoff =      dP; Yoff = 0.5*dY; break;
    case 6: Xoff =  -dX-dP; Yoff = -dP; break;
    case 7: Xoff = -0.5*dX; Yoff = -dP; break;
    case 8: Xoff =      dP; Yoff = -dP; break;
  }

  code = FALSE;
  protect = FALSE;

  YoffBase = Yoff;
  int Ydelta = 0;
  /* draw characters one-by-one */

  // successive subscripts / superscripts drop the line by
  // same amount each time, but size remains 0.8
  // sub followed by sup, or vice versa, should clear and do basic sup/sub

  unsigned int i;
  for (i = 0; i < strlen(string); i++) {
    N = (int)(string[i]);
    if ((N < 0) || (N >= NROTCHARS)) continue;

    if (N == 39) { // single-quote
      protect = protect ? FALSE : TRUE;
    } 

    /* check for special characters */
    if (!code && !protect) {
      if (N == 94) {
	SetRotFont (currentname, (int)(0.8*currentsize));
	currentfont = GetRotFontData (&currentscale);

	if (Ydelta > 0) { Ydelta = 0; Yoff = YoffBase; }
	Ydelta --;
	Yoff -= 0.75*currentscale*dY;
	continue;
      }
      if (N == 95) { 
	SetRotFont (currentname, (int)(0.8*currentsize));
	currentfont = GetRotFontData (&currentscale);

	if (Ydelta < 0) { Ydelta = 0; Yoff = YoffBase; }
	Ydelta ++;
	Yoff += 0.75*currentscale*dY;
	continue;
      }
      if (N == 124) {
	SetRotFont (currentname, basesize);
	currentfont = GetRotFontData (&currentscale);
	Yoff = YoffBase;
	Ydelta = 0;
	continue;
      }
      if (N == 92) {
	code = TRUE;
	continue;
      } 
      if (N == 38) {
	if (string[i+1] == 'h') {
	  SetRotFont ("helvetica", currentsize);
	  currentfont = GetRotFontData (&currentscale);
	}
	if (string[i+1] == 't') {
	  SetRotFont ("times", currentsize);
	  currentfont = GetRotFontData (&currentscale);
	}
	if (string[i+1] == 'c') {
	  SetRotFont ("courier", currentsize);
	  currentfont = GetRotFontData (&currentscale);
	}
	if (string[i+1] == 's') {
	  SetRotFont ("symbol", currentsize);
	  currentfont = GetRotFontData (&currentscale);
	}
	i++;
	continue;
      }
    }
    code = FALSE;

    bitmap = currentfont[N].bits;
    dx = currentfont[N].dx;
    dy = currentfont[N].dy;
    X = x + (int)(Xoff*cs - Yoff*sn) + (int)(currentscale*currentfont[N].ascent*sn);
    Y = y + (int)(Xoff*sn + Yoff*cs) - (int)(currentscale*currentfont[N].ascent*cs);
    bDrawRotBitmap (buffer, X, Y, dx, dy, bitmap, TRUE, angle, currentscale);
    Xoff += 1 + (int)(currentscale*dx + 0.5);
  }
  SetRotFont (basename, basesize);
  return (TRUE);
}

int bDrawRotBitmap (bDrawBuffer *buffer, int x, int y, int dx, int dy, unsigned char *bitmap, int mode, double angle, double scale) {
  OHANA_UNUSED_PARAM(mode);

  int ii, jj, byte_line, byte, bit, flag;
  double i, j, cs, sn, rscale, tmp;
  int X, Y, X0, X1, X2, Y0, Y1, Y2, x0, y0;

  /* this mode option is nort actually used... */
  // if (mode) {
  //   color = black;
  // } else {
  //   color = white;
  // } 
    
  byte_line = (int) ((dx + 7) / 8);

  cs = cos(angle*RAD_DEG);  sn = sin(angle*RAD_DEG);
  rscale = 1.0 / scale;

  X0 = 0;
  Y0 = 0;
  x0 = 0;
  y0 = 0;

  X2 = X1 = XPROC (0,0);
  Y2 = Y1 = YPROC (0,0);

  X = XPROC (dx,0);
  Y = YPROC (dx,0);
# ifdef DRAWBOXES
  bDrawLine (buffer, x+X, y+Y, x+X1, y+Y1);
  Xt = X;
  Yt = Y;
# endif
  X1 = MIN (X, X1);
  X2 = MAX (X, X2);
  Y1 = MIN (Y, Y1);
  Y2 = MAX (Y, Y2);

  X = XPROC (dx,dy);
  Y = YPROC (dx,dy);
# ifdef DRAWBOXES
  bDrawLine (buffer, x+X, y+Y, x+Xt, y+Yt);
  Xt = X;
  Yt = Y;
# endif
  Y1 = MIN (Y, Y1);
  Y2 = MAX (Y, Y2);
  X1 = MIN (X, X1);
  X2 = MAX (X, X2);

  X = XPROC (0,dy);
  Y = YPROC (0,dy);
# ifdef DRAWBOXES
  bDrawLine (buffer, x+X, y+Y, x+Xt, y+Yt);
  Xt = X;
  Yt = Y;
# endif
  Y1 = MIN (Y, Y1);
  Y2 = MAX (Y, Y2);
  X1 = MIN (X, X1);
  X2 = MAX (X, X2);

  if (scale > 1) {
    for (i = X1; i <= X2; i+=1) {
      for (j = Y1; j <= Y2; j+=1) {
	tmp = rscale*(cs*(i - X0) + sn*(j - Y0)) + x0;  ii = NEARINT (tmp);
	tmp = rscale*(cs*(j - Y0) - sn*(i - X0)) + y0;  jj = NEARINT (tmp);
	if ((ii < 0) || (ii >= dx) || (jj < 0) || (jj >= dy)) continue;
	byte = byte_line * jj + (ii / 8);
	bit = ii % 8;
	flag = 0x01 & (bitmap[byte] >> bit);
	if (flag) bDrawPointf (buffer, x + i, y + j);
      }
    }
  } else {
    for (i = X1; i <= X2; i+=scale) {
      for (j = Y1; j <= Y2; j+=scale) {
	tmp = rscale*(cs*(i - X0) + sn*(j - Y0)) + x0;  ii = NEARINT (tmp);
	tmp = rscale*(cs*(j - Y0) - sn*(i - X0)) + y0;  jj = NEARINT (tmp);
	if ((ii < 0) || (ii >= dx) || (jj < 0) || (jj >= dy)) continue;
	byte = byte_line * jj + (ii / 8);
	bit = ii % 8;
	flag = 0x01 & (bitmap[byte] >> bit);
	if (flag) bDrawPointf (buffer, x + i, y + j);
      }
    }
  }
  return (TRUE);
}
