# include "addstar.h"

typedef struct {
  double Amp;
  double Phase;
  double dM;
} SubPixFix;

static int Nsubpix;
static SubPixFix *Subpix;

void load_subpix () {

  int i;
  FILE *f;

  Nsubpix = 40;
  ALLOCATE (Subpix, SubPixFix, Nsubpix);

  f = fopen (SubpixDatafile, "r");
  if (f == NULL) Shutdown ("can't load subpix datafile %s", SubpixDatafile);

  for (i = 0; i < Nsubpix; i++) {
      if (fscanf (f, "%*s %*s %lf %lf %lf %*s\n",
                  &Subpix[i].Amp, &Subpix[i].Phase, &Subpix[i].dM) != 3) {
          Shutdown("can't read subpix datafile %s", SubpixDatafile);
      }
  }
  fclose (f);

}

double get_subpix (double x, double y) {

  int bin;
  double dy, dM;

  dy = y - (int)(y);
  bin = 5 * (int)(x/100) + (int)(y/100);
  dM = Subpix[bin].Amp*sin(2*3.14159*dy + Subpix[bin].Phase);
  return (dM);
}

double scat_subpix (double x, double y) {

  int bin;
  double dM;

  // dy = y - (int)(y);
  bin = 5 * (int)(x/100) + (int)(y/100);
  dM = Subpix[bin].dM;
  return (dM);
}
