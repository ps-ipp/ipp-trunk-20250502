# include <kapa_internal.h>

// buffer->pixels carries the plot image
// buffer->mask is 0 if the pixel is untouched, 1 if the pixel has data

void bDrawCircleSingle (bDrawBuffer *buffer, double xc, double yc, double radius);

int bDrawMerge (bDrawBuffer *base, bDrawBuffer *layer) {

  // the two bDrawBuffers must match in size and depth

  if (base->Nx != layer->Nx) return FALSE;
  if (base->Ny != layer->Ny) return FALSE;
  if (base->Nbyte != layer->Nbyte) return FALSE;

  for (int j = 0; j < base->Ny; j++) {
    for (int i = 0; i < base->Nx; i++) {

      if (!layer->mask[j][i]) continue;
      
      // completely opaque top layer:
      if (base->Nbyte == 1) {
	base->pixels[j][i] = layer->pixels[j][i];
      } else {
	base->pixels[j][3*i+0] = layer->pixels[j][3*i+0];
	base->pixels[j][3*i+1] = layer->pixels[j][3*i+1];
	base->pixels[j][3*i+2] = layer->pixels[j][3*i+2];
      }
    }
  }
  return TRUE;
}

// create a drawing buffer with either 1 or 3 byte colors
bDrawBuffer *bDrawBufferCreate (int Nx, int Ny, int Nbyte, png_color *palette, int Npalette) {

  int i, j;
  bDrawColor white, white_R, white_G, white_B;
  bDrawBuffer *buffer;

  myAssert((Nbyte == 1) || (Nbyte == 3), "invalid depth");
  myAssert(palette, "missing palette");

  white = KapaColorByName ("white");
  white_R = palette[white].red;
  white_G = palette[white].green;
  white_B = palette[white].blue;

  ALLOCATE (buffer, bDrawBuffer, 1);
  buffer[0].Nx = Nx;
  buffer[0].Ny = Ny;
  buffer[0].Nbyte = Nbyte;
  buffer[0].palette = palette;
  buffer[0].Npalette = Npalette;

  ALLOCATE (buffer[0].pixels, bDrawColor *, Ny);
  ALLOCATE (buffer[0].mask, char *, Ny);
  for (i = 0; i < Ny; i++) {
    ALLOCATE (buffer[0].pixels[i], bDrawColor, Nbyte*Nx);
    ALLOCATE (buffer[0].mask[i], char, Nx);
    for (j = 0; j < Nx; j++) {
      if (Nbyte == 1) {
	buffer[0].pixels[i][j] = white;
      } else {
	buffer[0].pixels[i][3*j+0] = white_R;
	buffer[0].pixels[i][3*j+1] = white_G;
	buffer[0].pixels[i][3*j+2] = white_B;
      }
      buffer[0].mask[i][j] = 0; // for now: 0 = no data, 1 = data
    }
  }

  buffer[0].bWeight = 0;
  buffer[0].bType = 0;
  buffer[0].bColor = white;
  buffer[0].bColor_R = white_R;
  buffer[0].bColor_G = white_G;
  buffer[0].bColor_B = white_B;
  return (buffer);
}

void bDrawBufferFree (bDrawBuffer *buffer) {

  int i;

  for (i = 0; i < buffer[0].Ny; i++) {
    free (buffer[0].pixels[i]);
    free (buffer[0].mask[i]);
  }
  free (buffer[0].pixels);
  free (buffer[0].mask);
  // free (buffer[0].palette);
  free (buffer);
  return;
}

// void bDrawSetBuffer (bDrawBuffer *buffer) {
//   myAssert(buffer[0].Nbyte == 1, "invalid depth");
//   bBuffer = buffer;
//   return;
// }

// "bDrawColor color" is one of the hardwired colors in KapaColors.c
void bDrawSetColor (bDrawBuffer *buffer, bDrawColor color) {
  buffer->bColor = color;
  buffer->bColor_R = buffer->palette[color].red;
  buffer->bColor_G = buffer->palette[color].green;
  buffer->bColor_B = buffer->palette[color].blue;

  return;
}

void bDrawSetStyle (bDrawBuffer *buffer, bDrawColor color, int lw, int lt, float alpha) {
  bDrawSetColor (buffer, color);

  buffer->bWeight = lw;
  buffer->bType = lt;
  buffer->alpha = alpha;
  return;
}

// if values were from 0.0 = white to 1.0 = black, the math would be:
// out = (1 - alpha) * in + alpha * new
// but values go from 0.0 = black to 1.0 = white
// rin = 1 - in, rnew = 1 - new
// rout = (1 - alpha) * (1 - in) + alpha (1 - new)
// out = 1 - ((1 - alpha) * (1 - in) + alpha (1 - new))
// out = 1 - ((1 - in - alpha + in * alpha) + alpha - alpha * new)
// out = 1 - ((1 - in + in*alpha - alpha*new))
// out = in - in*alpha + alpha*new

// draw a point in the current color 
// adding in an alpha-channel: out = (1 - alpha) * in + alpha * new
// bcolor_R,G,B are values in the range 0x00 - 0xff
void bDrawPoint (bDrawBuffer *buffer, int x, int y) {

  float alpha = buffer->alpha;
  float beta = (1.0 - alpha);

  // myAssert(buffer[0].Nbyte == 1, "invalid depth");
  if (x < 0) return;
  if (y < 0) return;
  if (x >= buffer[0].Nx) return;
  if (y >= buffer[0].Ny) return;
  if (buffer[0].Nbyte == 1) {
    buffer[0].pixels[y][x] = buffer->bColor;
  } else {
    buffer[0].pixels[y][3*x+0] = MAX (0x00, MIN(0xff, beta * buffer[0].pixels[y][3*x+0] + alpha * buffer->bColor_R)); // was just buffer->bColor_R, etc
    buffer[0].pixels[y][3*x+1] = MAX (0x00, MIN(0xff, beta * buffer[0].pixels[y][3*x+1] + alpha * buffer->bColor_G));
    buffer[0].pixels[y][3*x+2] = MAX (0x00, MIN(0xff, beta * buffer[0].pixels[y][3*x+2] + alpha * buffer->bColor_B));
  }
  buffer[0].mask[y][x] = 1;
  return;
}

// draw a point in the current color 
void bDrawPointf (bDrawBuffer *buffer, float x, float y) {

  bDrawPoint (buffer, ROUND(x), ROUND(y));
  return;
}

void bDrawTriOpen (bDrawBuffer *buffer, double x1, double y1, double x2, double y2, double x3, double y3) {

  bDrawLine (buffer, x1, y1, x2, y2);
  bDrawLine (buffer, x2, y2, x3, y3);
  bDrawLine (buffer, x3, y3, x1, y1);

  return;
}

// x1,y1 is lower-left corner, x2,y2 is upper-right corner
void bDrawRectOpen (bDrawBuffer *buffer, double x1, double y1, double x2, double y2) {

  if (x1 > x2) SWAP (x1, x2);
  if (y1 > y2) SWAP (y1, y2);

  int X1 = MIN (MAX (ROUND (x1), 0), buffer[0].Nx - 1);
  int X2 = MIN (MAX (ROUND (x2), 1), buffer[0].Nx - 1);

  int Y1 = MIN (MAX (ROUND (y1), 0), buffer[0].Ny - 1);
  int Y2 = MIN (MAX (ROUND (y2), 1), buffer[0].Ny - 1);

  int dNs = -0.5*(buffer->bWeight - 1); 
  /* 0, 0, 0, -1, -1, -2, -2 */

  int dNe = +0.5*buffer->bWeight + 1; 
  /* 1, 1, 2, 2, 2, 3, 3 */

  for (int dN = dNs; dN < dNe; dN ++) {
    // line on the bottom needs to run longer for the negative dN values
    bDrawLineHorizontal (buffer, X1 + dN, X2 + 1 - dN, Y1 + dN);

    // line on the top needs to run longer for the positive dN values
    bDrawLineHorizontal (buffer, X1 + dN, X2 + 1 - dN, Y2 - dN);

    // line on the left needs to run longer for the negative dN values
    bDrawLineVertical   (buffer, X1 + dN, Y1 + dN, Y2 - dN);

    // line on the right needs to run longer for the positive dN values
    bDrawLineVertical   (buffer, X2 - dN, Y1 + dN, Y2 - dN);
  }
  return;
}

void bDrawRectFill (bDrawBuffer *buffer, double x1, double y1, double x2, double y2) {

  int i;
  int X1, Y1, X2, Y2;

  if (x1 > x2) SWAP (x1, x2);
  if (y1 > y2) SWAP (y1, y2);

  X1 = MIN (MAX (ROUND (x1), 0), buffer[0].Nx - 1);
  X2 = MIN (MAX (ROUND (x2), 1), buffer[0].Nx - 1);

  Y1 = MIN (MAX (ROUND (y1), 0), buffer[0].Ny - 1);
  Y2 = MIN (MAX (ROUND (y2), 0), buffer[0].Ny - 1);

  for (i = Y1; i < Y2; i++) {
    // this should be a line of width 1 or we duplicate pixels
    bDrawLineHorizontal (buffer, X1, X2, i);
  } 
  return;
}

// identify the quadrant and draw the correct line
void bDrawLine (bDrawBuffer *buffer, double x1, double y1, double x2, double y2) {

  int FlipDirect, FlipCoords;
  int X1, Y1, X2, Y2, dX, dY;

  /* rather than draw the line from float positions, we find the closest
     integer end-points and draw the line between those pixels */ 

  X1 = ROUND(x1);
  Y1 = ROUND(y1);
  X2 = ROUND(x2);
  Y2 = ROUND(y2);

  dX = X2 - X1;
  dY = Y2 - Y1;

  FlipCoords = (abs(dX) < abs(dY));
  FlipDirect = FlipCoords ? (y1 > y2) : (x1 > x2);

  if (!FlipDirect && !FlipCoords) bDrawLineWeight (buffer, X1, Y1, X2, Y2, FALSE);
  if ( FlipDirect && !FlipCoords) bDrawLineWeight (buffer, X2, Y2, X1, Y1, FALSE);
  if (!FlipDirect &&  FlipCoords) bDrawLineWeight (buffer, Y1, X1, Y2, X2, TRUE);
  if ( FlipDirect &&  FlipCoords) bDrawLineWeight (buffer, Y2, X2, Y1, X1, TRUE);

  return;
}

// draw a series of lines to give the line weight
void bDrawLineWeight (bDrawBuffer *buffer, int X1, int Y1, int X2, int Y2, int swapcoords) {

  int dN, dNs, dNe;

  dNs = -0.5*(buffer->bWeight - 1); 
  /* 0, 0, 0, -1, -1, -2, -2 */

  dNe = +0.5*buffer->bWeight + 1; 
  /* 1, 1, 2, 2, 2, 3, 3 */

  for (dN = dNs; dN < dNe; dN++) {
    bDrawLineBresen (buffer, X1, Y1 + dN, X2, Y2 + dN, swapcoords);
  }
  return;
}

// use the Bresenham line drawing technique
// integer-only Bresenham line-draw version which is fast
// bresenham assumes x1 < x2 and (y2 - y1) < (x2 - x1)
void bDrawLineBresen (bDrawBuffer *buffer, int X1, int Y1, int X2, int Y2, int swapcoords) {

  int X, Y, dX, dY;
  int e, e2;
  int N, DashOn;

  dX = X2 - X1;
  dY = Y2 - Y1;

  DashOn = TRUE;

  Y = Y1;
  e = 0;
  for (X = X1, N = 0; X <= X2; X++, N++) {
    if (buffer->bType == KAPA_LINE_DOT) {
      DashOn = (N % 5) == 0 || (N % 5) == 1;
    }
    if (buffer->bType == KAPA_LINE_DASH_SHORT) {
      DashOn = (N % 8) < 4;
    }
    if (buffer->bType == KAPA_LINE_DASH_LONG) {
      DashOn = (N % 16) < 8;
    }
    if (buffer->bType == KAPA_LINE_DOT_DASH) {
      DashOn = ((N % 12) < 2) || ((N % 12 >= 6) && (N % 12 < 10)) ;
    }
    if (swapcoords) {
      if (DashOn) bDrawPoint (buffer, Y,X);
    } else {
      if (DashOn) bDrawPoint (buffer, X,Y);
    }
    e += dY;
    e2 = 2 * e;
    if (e2 > dX) {
      Y++;
      e -= dX;
    } 
    if (e2 < -dX) {
      Y--;
      e += dX;
    }
  }
  return;
}

void bDrawLineHorizontal (bDrawBuffer *buffer, int X1, int X2, int Y) {
  
  int i;

  if (Y <  0) return;
  if (Y >= buffer[0].Ny) return;

  float alpha = buffer->alpha;
  float beta = (1.0 - alpha);

  for (i = X1; i < X2; i++) {
    if (i <  0) continue;
    if (i >= buffer[0].Nx) continue;
    if (buffer[0].Nbyte == 1) {
      buffer[0].pixels[Y][i] = buffer->bColor;
    } else {
      buffer[0].pixels[Y][3*i+0] = MAX (0x00, MIN(0xff, beta * buffer[0].pixels[Y][3*i+0] + alpha * buffer->bColor_R));
      buffer[0].pixels[Y][3*i+1] = MAX (0x00, MIN(0xff, beta * buffer[0].pixels[Y][3*i+1] + alpha * buffer->bColor_G));
      buffer[0].pixels[Y][3*i+2] = MAX (0x00, MIN(0xff, beta * buffer[0].pixels[Y][3*i+2] + alpha * buffer->bColor_B));
    }
    buffer[0].mask[Y][i] = 1;
  }
  return;
}

void bDrawLineVertical (bDrawBuffer *buffer, int X, int Y1, int Y2) {
  
  int i;

  if (X <  0) return;
  if (X >= buffer[0].Nx) return;

  float alpha = buffer->alpha;
  float beta = (1.0 - alpha);

  for (i = Y1; i < Y2; i++) {
    if (i <  0) continue;
    if (i >= buffer[0].Ny) continue;
    if (buffer[0].Nbyte == 1) {
      buffer[0].pixels[i][X] = buffer->bColor;
    } else {
      buffer[0].pixels[i][3*X+0] = MAX (0x00, MIN(0xff, beta * buffer[0].pixels[i][3*X+0] + alpha * buffer->bColor_R));
      buffer[0].pixels[i][3*X+1] = MAX (0x00, MIN(0xff, beta * buffer[0].pixels[i][3*X+1] + alpha * buffer->bColor_G));
      buffer[0].pixels[i][3*X+2] = MAX (0x00, MIN(0xff, beta * buffer[0].pixels[i][3*X+2] + alpha * buffer->bColor_B));
    }
    buffer[0].mask[i][X] = 1;
  }
  return;
}

// upright triangle with base of width 2 dX + 1, height of dY, center of base at x1, y1
void bDrawTriFill (bDrawBuffer *buffer, double x1, double y1, double dx, double dy) {

  int xs, xe, x, y;
  float e, m;

  double dX = fabs(dx);
  
  xs = x1 - dX;
  xe = x1 + dX + 1;
  e = 0;
  m = dX / dy;

  if (dy > 0.0) {
    for (y = y1; y < y1 + dy + 1; y++) {
      for (x = xs; x < xe; x++) {
	bDrawPoint (buffer, x, y);
      }
      e += m;
      if (e > 0.5) {
	xs ++;
	xe --;
	e -= 1.0;
      }
      if (e < -0.5) {
	xs ++;
	xe --;
	e += 1.0;
      }
    }    
  } else {
    for (y = y1; y > y1 + dy - 1; y--) {
      for (x = xs; x < xe; x++) {
	bDrawPoint (buffer, x, y);
      }
      e += m;
      if (e > 0.5) {
	xs ++;
	xe --;
	e -= 1.0;
      }
      if (e < -0.5) {
	xs ++;
	xe --;
	e += 1.0;
      }
    }    
  }
 
  return;
}

# define BOT_LEFT 0
# define BOT_RGHT 1
# define TOP_LEFT 2
# define TOP_RGHT 3

// I should probably look up an appropriate recipe for this 
// source code for XDrawPolyFill?

int GetNextSeqNumber (int seq, int Npoint, int Clockwise, int LeftSide);
int bDrawFillBetweenSegments (bDrawBuffer *buffer, int *x, int *y, int ystart);

void bDrawPolyFill (bDrawBuffer *buffer, double *x, double *y, int Npoints) {

  // The coord list which is passed in (x,y) must be in order around the contour
  for (int i = 0; i < Npoints; i++) {
    fprintf (stderr, "%d : %f,%f\n", i, x[i], y[i]);
  }

  // First, find the lowest point and start at that sequence number
  int iMin = 0;
  double yMin = y[iMin];
  for (int i = 1; i < Npoints; i++) {
    if (y[i] < yMin) { iMin = i; yMin = y[iMin]; }
  }

  // we generate two line segments and will fill between them
  int xval[4];
  int yval[4];

  // the starting point is a vertex
  xval[BOT_LEFT] = x[iMin];  yval[BOT_LEFT] = y[iMin];
  xval[BOT_RGHT] = x[iMin];  yval[BOT_RGHT] = y[iMin];

  // get iNext, iPrev assuming ClockWise, then test
  int isCW = TRUE;
  int iNextLeft = GetNextSeqNumber (iMin, Npoints, isCW, TRUE); // TRUE => left-side
  int iNextRght = GetNextSeqNumber (iMin, Npoints, isCW, FALSE); 

  // if this is true, the points are not clockwise
  if (x[iNextLeft] > x[iNextRght]) {
    isCW = FALSE;
    int tmp = iNextLeft;
    iNextLeft = iNextRght;
    iNextRght = tmp;
  }

  int iTL = iNextLeft; // sequence number of the top,left point
  int iTR = iNextRght; // sequence number of the top,right point
  xval[TOP_LEFT] = x[iTL]; yval[TOP_LEFT] = y[iTL];
  xval[TOP_RGHT] = x[iTR]; yval[TOP_RGHT] = y[iTR];

  // we need to track the starting row, start at the bottom
  int ystart = yval[BOT_LEFT];

  while (TRUE) {
    int isLeft = bDrawFillBetweenSegments (buffer, xval, yval, ystart);

    // the last pair of segments in the sequence end at the same top point:
    if (iTL == iTR) break;

    // after one pass, if isLeft is true, then cycle the points on the left
    // segment, otherwise cycle the right segment:

    if (isLeft) {
      iTL = GetNextSeqNumber (iTL, Npoints, isCW, TRUE);
      xval[BOT_LEFT] = xval[TOP_LEFT]; yval[BOT_LEFT] = yval[TOP_LEFT];
      xval[TOP_LEFT] = x[iTL];         yval[TOP_LEFT] = y[iTL];
      ystart = yval[BOT_LEFT];
    } else {
      iTR = GetNextSeqNumber (iTR, Npoints, isCW, FALSE);
      xval[BOT_RGHT] = xval[TOP_RGHT]; yval[BOT_RGHT] = yval[TOP_RGHT];
      xval[TOP_RGHT] = x[iTR];         yval[TOP_RGHT] = y[iTR];
      ystart = yval[BOT_RGHT];
    }
  }
  return;
}

// we have two line segments defined by x[],y[].  

// [0],[1] is the left-side segment
// [2],[3] is the right-side segment

// Fill the region between the two segments until 
// the lower of the two y coordinates y[1], y[3]

// return which of y[1] or y[3] was the ending row

int bDrawFillBetweenSegments (bDrawBuffer *buffer, int *x, int *y, int ystart) {

  int dY_L = y[TOP_LEFT] - y[BOT_LEFT];
  int dX_L = x[TOP_LEFT] - x[BOT_LEFT];

  int dY_R = y[TOP_RGHT] - y[BOT_RGHT];
  int dX_R = x[TOP_RGHT] - x[BOT_RGHT];
    
  double Slope_L = (dY_L == 0) ? NAN : dX_L / (double) dY_L;
  double Slope_R = (dY_R == 0) ? NAN : dX_R / (double) dY_R;

  // stop at whichever comes first
  // XXX not sure if I need to use < or <= here:
  // with <=, the value of Y after the loop will be one too high.
  int Y; 
  for (Y = ystart; (Y < y[TOP_LEFT]) && (Y < y[TOP_RGHT]); Y++) {

    // calculating X_L, X_R based on the slope and current y point
    int X_L = (dY_L == 0) ? x[BOT_LEFT] : (Y - y[BOT_LEFT]) * Slope_L + x[BOT_LEFT];
    int X_R = (dY_R == 0) ? x[BOT_RGHT] : (Y - y[BOT_RGHT]) * Slope_R + x[BOT_RGHT];
      
    // draw horizontal line from X_L to X_R at Y
    bDrawLineHorizontal (buffer, X_L, X_R, Y);
  }

  if (Y == y[TOP_LEFT]) {
    return TRUE; // TRUE is LEFT SIDE
  } else {
    return FALSE;
  }
}

int GetNextSeqNumber (int seq, int Npoints, int Clockwise, int LeftSide) {

  int iNext;

  if (Clockwise) {
    if (LeftSide) {
      // next point is next in sequence
      iNext = (seq < Npoints - 1) ? seq + 1 : 0;
    } else {
      // next point is prev in sequence
      iNext = (seq > 0) ? seq - 1 : Npoints - 1;
    }
  } else {
    if (LeftSide) {
      // next point is prev in sequence
      iNext = (seq > 0) ? seq - 1 : Npoints - 1;
    } else {
      // next point is next in sequence
      iNext = (seq < Npoints - 1) ? seq + 1 : 0;
    }
  }
  return iNext;
}

void bDrawArc (bDrawBuffer *buffer, double Xc, double Yc, double Xr, double Yr, double Ts, double Te) {

  float t, dt;
  int x, y;

  /* drawing a complete circle */
//  if ((fabs(Te - Ts) > 360.0) && (Xr == Yr)) {
//    bDrawCircle (Xc, Yc, Xr);
//    return;
//  }

  /* only draw a single loop */
  if (fabs(Te - Ts) > 360.0) {
    Te = 360.0;
    Ts = 0.0;
  }

  /* smallest angle is 1/Rmax */
  dt = MAX (fabs(Xr * sin(Ts*RAD_DEG)), fabs(Yr * cos(Ts*RAD_DEG)));
  dt = 1.0 / dt;

  for (t = Ts*RAD_DEG; t <= Te*RAD_DEG; t += dt) {
    x = Xr*cos(t) + Xc;
    y = Yr*sin(t) + Yc;

    /* we could use the value of MAX(dy/dt,dx/dt) to set dt */
    bDrawPoint (buffer, x,y);

    dt = MAX (fabs(Xr * sin(t)), fabs(Yr * cos(t)));
    dt = 1.0 / dt;
  }
  return;
}

// draw a series of circles to give line weight
void bDrawCircle (bDrawBuffer *buffer, double xc, double yc, double radius) {

  int dN, dNs, dNe;

  dNs = -0.5*(buffer->bWeight - 1); 
  /* 0, 0, 0, -1, -1, -2, -2 */

  dNe = +0.5*buffer->bWeight + 1; 
  /* 1, 1, 2, 2, 2, 3, 3 */

  for (dN = dNs; dN < dNe; dN++) {
    bDrawCircleSingle (buffer, xc, yc, radius + dN);
  }
  return;
}

// draw points in a pure circle, using symmetry, but only draw a point once
void bDrawCirclePoints (bDrawBuffer *buffer, int Xc, int Yc, int x, int y) {

  if (x == 0) {
    bDrawPoint (buffer, Xc,     Yc + y);
    bDrawPoint (buffer, Xc,     Yc - y);
    bDrawPoint (buffer, Xc + y, Yc);
    bDrawPoint (buffer, Xc - y, Yc);
    return;
  }

  if (x == y) {
    bDrawPoint (buffer, Xc + x, Yc + y);
    bDrawPoint (buffer, Xc - x, Yc + y);
    bDrawPoint (buffer, Xc + x, Yc - y);
    bDrawPoint (buffer, Xc - x, Yc - y);
    return;
  }

  bDrawPoint (buffer, Xc + x, Yc + y);
  bDrawPoint (buffer, Xc + x, Yc - y);
  bDrawPoint (buffer, Xc - x, Yc + y);
  bDrawPoint (buffer, Xc - x, Yc - y);
  bDrawPoint (buffer, Xc + y, Yc + x);
  bDrawPoint (buffer, Xc + y, Yc - x);
  bDrawPoint (buffer, Xc - y, Yc + x);
  bDrawPoint (buffer, Xc - y, Yc - x);
}

# define VERSION_A 0
# define VERSION_B 0
# define VERSION_C 1

/* I am using version C, but I really should consider using the algorithm describe in
   Bresenham.pdf for an ellipse inside a rectangle (this allows 1/2 int steps for the
   radius)
 */

# if (VERSION_A) 
// draw a pure circle  
void bDrawCircleSingle (bDrawBuffer *buffer, double xc, double yc, double radius) {

  int Xc = ROUND(xc);
  int Yc = ROUND(yc);
  int Radius = ROUND(radius);

  int x = Radius - 1;
  int y = 0;
  int dx = 1;
  int dy = 1;

  int err = dx - (Radius << 1);

  while (x >= y) {
    bDrawCirclePoints (buffer, Xc, Yc, x, y);

    if (err <= 0) {
      y++;
      err += dy;
      dy += 2;
    }

    if (err > 0) {
      x--;
      dx += 2;
      err += dx - (Radius << 1);
    }
  }
  return;
}
# endif

# if (VERSION_B) 
// draw a pure circle  
void bDrawCircleSingle (bDrawBuffer *buffer, double xc, double yc, double radius) {

  int Xc = ROUND(xc);
  int Yc = ROUND(yc);
  int Radius = ROUND(radius);

  // fprintf (stderr, "radius: %d\n", Radius);

  int x = 0;
  int y = Radius;

  // d = 3 - 2*Radius;
  int d = (5 - 4*radius) / 4;

  bDrawCirclePoints (buffer, Xc, Yc, x, y);

  while (x < y) {
    x++;
    if (d < 0) {
      d += 2*x + 1;
    } else {
      y--;
      d += 2*(x-y) + 1;
    }
    bDrawCirclePoints (buffer, Xc, Yc, x, y);
  }
  return;
}
# endif

# if (VERSION_C)
// draw a pure circle  
void bDrawCircleSingle (bDrawBuffer *buffer, double xc, double yc, double radius) {

  int Xc = ROUND(xc);
  int Yc = ROUND(yc);
  int Radius = ROUND(radius);

  if (Radius == 0) {
    bDrawCirclePoints (buffer, Xc, Yc, 0, 0);
  }

  // fprintf (stderr, "P radius: %f -> %d\n", radius, Radius);

  int x = 0;
  int y = Radius;

  // d = 3 - 2*Radius;
  int d = 5 - 4*radius;

  while (x <= y) {
    bDrawCirclePoints (buffer, Xc, Yc, x, y);

    if (d < 0) {
      // d = d + 4*x + 6;
      d = d + 8*x + 4;
    } else {
      // d = d + 4*(x-y) + 10;
      d = d + 8*(x-y) + 8;
      y--;
    }
    x++;
  }
}
# endif

// draw a pure circle  
void bDrawCircleLines (bDrawBuffer *buffer, int Xc, int Yc, int x, int ys, int decrementY) {

  int y = ys;
  if (decrementY) y ++;

  if (x == 0) {
    if (decrementY) {
      // this should must be a line or width 1 or we duplicate pixels
      bDrawLineHorizontal (buffer, Xc    , Xc + 1, Yc + y);
      bDrawLineHorizontal (buffer, Xc    , Xc + 1, Yc - y);
    }

    // center line 
    // this should must be a line or width 1 or we duplicate pixels
    bDrawLineHorizontal (buffer, Xc - y, Xc + y + 1, Yc    );
    return;
  }

  if (x == y) {
    // this should must be a line or width 1 or we duplicate pixels
    bDrawLineHorizontal (buffer, Xc - x, Xc + x + 1, Yc + y);
    bDrawLineHorizontal (buffer, Xc - x, Xc + x + 1, Yc - y);
    return;
  }

  // only draw these two lines if we decrement y
  if (decrementY) {
    // this must be a line or width 1 or we duplicate pixels
    bDrawLineHorizontal (buffer, Xc - x, Xc + x + 1, Yc + y);
    bDrawLineHorizontal (buffer, Xc - x, Xc + x + 1, Yc - y);
  }

  // always draw these two lines:
  // this must be a line or width 1 or we duplicate pixels
  bDrawLineHorizontal (buffer, Xc - y, Xc + y + 1, Yc + x);
  bDrawLineHorizontal (buffer, Xc - y, Xc + y + 1, Yc - x);
}

// draw a pure circle  
void bDrawCircleFill (bDrawBuffer *buffer, double xc, double yc, double radius) {

  int Xc, Yc, Radius;
  int x, y, d;

  Xc = ROUND(xc);
  Yc = ROUND(yc);
  Radius = ROUND(radius);

  x = 0;
  y = Radius;

  // d = 3 - 2*Radius;
  d = 5 - 4*radius;

  while (x <= y) {
    // bDrawLineHorizontal (buffer, Xc-x, Xc+x, Yc+y);
    // bDrawLineHorizontal (buffer, Xc-x, Xc+x, Yc-y);
    // bDrawLineHorizontal (buffer, Xc-y, Xc+y, Yc+x);
    // bDrawLineHorizontal (buffer, Xc-y, Xc+y, Yc-x);

    int decrementY = FALSE;
    if (d < 0) {
      // d = d + 4*x + 6;
      d = d + 8*x + 4;
    } else {
      // d = d + 4*(x-y) + 10;
      d = d + 8*(x-y) + 8;
      y--;
      decrementY = TRUE;
    }
    bDrawCircleLines (buffer, Xc, Yc, x, y, decrementY);
    x++;
  }
}

// run a smoothing kernel on each channel for anti-aliasing
// XXX I need to consider the mask as well
void bDrawSmooth (bDrawBuffer *buffer, float sigma) {

  // generate a temp buffer for storage of the smoothed result
  int Nx = buffer[0].Nx;
  int Ny = buffer[0].Ny;
  ALLOCATE_PTR (temp, bDrawColor, Nx*Ny);

  //* build a 1D gaussian 
  int Nsigma = 2;
  int Ns = (int) (Nsigma*sigma + 0.5);
  int Ngauss = 2*Ns + 1;
  ALLOCATE_PTR (gaussnorm, float, Ngauss);
  float *gauss = &gaussnorm[Ns];
  for (int i = -Ns; i < Ns + 1; i++) {
    gauss[i] = exp ((i*i)/(-2*sigma*sigma));
  }

  // NOTE XXX: need to handle Nbyte = 1 or 3
  int Nchannel = 3;

  // we need to smooth the 3 channels independently, do this as three passes
  for (int ch = 0; ch < Nchannel; ch++) {

    // smooth in X direction 
    for (int j = 0; j < Ny; j++) {
      bDrawColor *vi = (bDrawColor *) &buffer[0].pixels[j][ch];
      bDrawColor *vo = &temp[j*Nx];
      for (int i = 0; i < Nx; i++, vo++, vi += Nchannel) {
	float g = 0.0, s = 0.0;
	for (int n = -Ns; n <= Ns; n++) {
	  if (i+n < 0) continue;
	  if (i+n >= Nx) continue;
	  s += gauss[n]*vi[Nchannel*n];
	  g += gauss[n];
	}
	*vo = s / g;
      }
    }

    // NOTE: output maps back into original image
    // careful on the counting

    /* smooth in Y direction */
    for (int i = 0; i < Nx; i++) {
      bDrawColor *vi = &temp[i];
      for (int j = 0; j < Ny; j++) {
	float g = 0.0, s = 0.0;
	for (int n = -Ns; n <= Ns; n++) {
	  if (j+n < 0) continue;
	  if (j+n >= Ny) continue;
	  s += gauss[n]*vi[(n+j)*Nx]; // vi is a single-index array, so this is stepping by rows
	  g += gauss[n];
	}
	buffer[0].pixels[j][Nchannel*i + ch] = s / g;
      }
    }
  }

  // smooth the mask
  ALLOCATE_PTR (temp_mask, float, Nx*Ny);

  // smooth in X direction 
  for (int j = 0; j < Ny; j++) {
    char *vi = (char *) &buffer[0].mask[j][0];
    float *vo = &temp_mask[j*Nx];
    for (int i = 0; i < Nx; i++, vo++, vi++) {
  	float g = 0.0, s = 0.0;
  	for (int n = -Ns; n <= Ns; n++) {
  	  if (i+n < 0) continue;
  	  if (i+n >= Nx) continue;
  	  s += gauss[n]*vi[n];
  	  g += gauss[n];
  	}
  	*vo = s / g;
    }
  }

  // NOTE: output maps back into original image
  // careful on the counting

  /* smooth in Y direction */
  for (int i = 0; i < Nx; i++) {
    float *vi = &temp_mask[i];
    for (int j = 0; j < Ny; j++) {
  	float g = 0.0, s = 0.0;
  	for (int n = -Ns; n <= Ns; n++) {
  	  if (j+n < 0) continue;
  	  if (j+n >= Ny) continue;
  	  s += gauss[n]*vi[(n+j)*Nx]; // vi is a single-index array, so this is stepping by rows
  	  g += gauss[n];
  	}
  	buffer[0].mask[j][i] = (s / g > 0.05) ? 1 : 0;
    }
  }

  free (temp);
  free (temp_mask);
  free (gaussnorm);

  return;
}

/* 
NOTES on Bresenham-style circles:
the discriminant of inside or outside the circle is:

f(x,y) = x^2 + y^2 - r^2

- negative: (x,y) inside circle
- positive: (x,y) outside circle

given d(0) = f(x,y); find d(1) = f(x+1,y):
d(1) = (x+1)^2 + y^2 - r^2
d(1) = d(0) + 2x + 1
(use d(1) if d(0) < 0, ie inside circle)

given d(0) = f(x,y); find d(1) = f(x+1,y-1):
d(1) = (x+1)^2 + (y-1)^2 - r^2
d(1) = d(0) + 2x + 1 - 2y + 1
d(1) = d(0) + 2(x-y) + 2

* init d to f(1,r-1/2) instead of r, keeping the effective boundary 
  between two pixels (also, an inside point)

f(1,r-1/2) = 1 + (r-1/2)^2 - r^2 = 5/4 - r
f(1,r-1/4) = 1 + (r-1/4)^2 - r^2 = 1 + r/2 + 1/16 = 17/16 + r/2

f(1,r-x)   = 1 + (r-x)^2 - r^2 = 1 - 2xr + x^2 = A (3 - 2r)

1+x^2 = 3A
-2x = -2A

A = x

1 - 3x + x^2 = 0

(3 +/- sqrt(9 - 4))/2 = (3 - sqrt(5))/2

multiply all d values by 4 to get integer tests:

d'(0) = 5 - 4r
d'(in) = d' + 8x + 4
d'(out) = d' + 8(x-y) + 8

*/

/* This is the Bresenham line-drawing algorithm for 1st and 4th quandrant
   vectors with positive and negative slopes < 1. this is the sequence if we use
   float errors and tests it is easy to understand, but slower than it could be

  { 
    float e, m;
    m = dY / dX;
    Y = Y1;
    e = 0;
    for (X = X1; X <= X2; X++) {
      plot (X,Y);
      e += m;
      if (e > 0.5) {
	Y++;
	e -= 1.0;
      }
      if (e < -0.5) {
	Y--;
	e += 1.0;
      }
    }
  }
*/
