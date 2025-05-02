# include <stdio.h>
# include <math.h>
# include <stdlib.h>
# include <string.h>
# define TRUE 1
# define FALSE 0

# define ALLOCATE(X,T,S)  \
  X=(T *)malloc((unsigned) ((S)*sizeof(T)));\
  if(X==NULL) \
    { \
      fprintf(stderr,"failed to malloc X\n");\
        exit(0);\
    } 
# define REALLOCATE(X,T,S) \
  X=(T *)realloc(X,(unsigned) ((S)*sizeof(T))); \
  if(X==NULL) \
    { \
       fprintf(stderr,"failed to realloc X\n"); \
       exit(0); \
    }

#define blank_width 5
#define blank_height 1
static unsigned char blank_bits[] = {0x00};

typedef struct {
  int dx;
  int dy;
  float dXps;
  int ascent;
  int Nb;
  unsigned char *bits;
  char name[64];
} RotFont;

char flip_bits (char a);
int scan_line (FILE *f, char *line);

int main (int argc, char **argv) {

  int BitMap, Nvalue, code;
  unsigned int bits;
  int i, j, dx, dy, ddx, ddy;
  char buffer[1000], name[1000], glyph[1000];
  unsigned char value[1000], bitchar[3];
  RotFont font[256];
  
  FILE *f;

  if (argc != 4) {
    fprintf (stderr, "USAGE: fixfont (file.bdf) (file.psx) (name)\n");
    exit (0);
  }

  char *bdfname = argv[1];
  char *psxname = argv[2];
  char *fontname = argv[3];

  // read in the PS sizes first
  f = fopen (psxname, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "failed to open file %s\n", psxname);
    exit (0);
  }

  while (scan_line (f, buffer) != EOF) {
    int N;
    float dXps;
    int Nread = sscanf (buffer, "%d %f", &N, &dXps);
    if (Nread != 2) continue;
    if (N < 1) continue;
    if (N > 255) continue;
    font[N].dXps = dXps;
  }
  fclose (f);

  f = fopen (bdfname, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "failed to open file %s\n", bdfname);
    exit (0);
  }

  for (i = 0; i < 256; i++) {
    font[i].bits = 0;
  }

  BitMap = FALSE;
  Nvalue = 0;
  while (scan_line (f, buffer) != EOF) {
    sscanf (buffer, "%s", name);
    if (!strcmp (name, "ENDCHAR")) {
      BitMap = FALSE;
      if ((code >= 0) && (code < 256)) {
	font[code].dx = dx;
	font[code].dy = dy;
	font[code].ascent = dy + ddy;
	font[code].Nb = Nvalue;
	ALLOCATE (font[code].bits, unsigned char, Nvalue);
	strcpy (font[code].name, glyph);
	for (i = 0; i < Nvalue; i++) { 
	  font[code].bits[i] = value[i];
	}
      }
      fprintf (stderr, "found %s\n", glyph);
    }
    if (BitMap) {
      bitchar[2] = 0;
      for (j = 0; buffer[j] != 0; j+=2) {
	bitchar[0] = buffer[j];
	bitchar[1] = buffer[j+1];
	sscanf (bitchar, "%x", &bits);
	value[Nvalue] = flip_bits (bits);
	Nvalue ++;
      }
      continue;
    } 
    if (!strcmp (name, "STARTCHAR")) {
      sscanf (buffer, "%*s %s", glyph);
    }
    if (!strcmp (name, "ENCODING")) {
      sscanf (buffer, "%*s %d", &code);
    }
    if (!strcmp (name, "BBX")) {
      sscanf (buffer, "%*s %d %d %d %d", &dx, &dy, &ddx, &ddy);
    }
    if (!strcmp (name, "BITMAP")) {
      BitMap = TRUE;
      Nvalue = 0;
    }
  }    

  // spaces are being set to zero width in the bdf files
  // set to width of 'i'
  font[32].dx   = font[105].dx;
  font[32].dy   = font[105].dy;
  font[32].dXps = font[105].dXps;

  for (i = 0; i < 256; i++) {
    if (font[i].bits  == 0) {
      font[i].bits = blank_bits;
      font[i].dx = blank_width;
      font[i].dy = blank_height;
      font[i].Nb = 1;
      font[i].ascent = blank_height;
      strcpy (font[i].name, "blank");
    }
    fprintf (stdout, "static unsigned char %s_%d_bits[] = {", fontname, i);
    for (j = 0; j < font[i].Nb; j++) {
      if (!(j % 12)) fprintf (stdout, "\n");
      if (j == font[i].Nb - 1) fprintf (stdout, "0x%02x", font[i].bits[j]);
      else fprintf (stdout, "0x%02x, ", font[i].bits[j]);
    }
    fprintf (stdout, "};\n");
  }
  
  fprintf (stdout, "static RotFont %sfont[] = {\n", fontname);
  for (i = 0; i < 255; i++) {
    fprintf (stdout, "{%3d, %3d, %5.2f, %3d, %s_%d_bits},\n", 
	     font[i].dx, font[i].dy, font[i].dXps, font[i].ascent, fontname, i);
  }
  fprintf (stdout, "{%3d, %3d, %5.2f, %3d, %s_%d_bits}};\n", 
	   font[i].dx, font[i].dy, font[i].dXps, font[i].ascent, fontname, i);

      
  exit (0);
}

int scan_line (FILE *f, char *line) {

  int i, status;
  char c;
  
  status = EOF + 1;
  
  for (i = 0, c = 0; (c != '\n') && (status != EOF); i++) {
    status = fscanf (f, "%c", &c);
    line[i] = c;
  }
  line[i - 1] = 0;  /* this could make things crash! */

  if (i > 1) {
    status = EOF + 1;
  }

  return (status);

}

char flip_bits (char a) {

  char b, c;
  
  c = 0;
  b = (a & 0x01) << 7;
  c = c | b;
  b = (a & 0x02) << 5;
  c = c | b;
  b = (a & 0x04) << 3;
  c = c | b;
  b = (a & 0x08) << 1;
  c = c | b;
  b = (a & 0x10) >> 1;
  c = c | b;
  b = (a & 0x20) >> 3;
  c = c | b;
  b = (a & 0x40) >> 5;
  c = c | b;
  b = (a & 0x80) >> 7;
  c = c | b;

  return (c);

}
