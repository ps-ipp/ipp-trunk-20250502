#ifndef GET_GRAPHDATA_H
#define GET_GRAPHDATA_H

// This was moved from kapa.h

typedef struct graphdata {
  double xmin, xmax, ymin, ymax;
  int style, ptype, ltype, etype, ebar, color;
  double lweight, size, alpha;
  double ticktextPad;
  double labelPadXm, labelPadYm, labelPadXp, labelPadYp;
  double padXm, padXp, padYm, padYp;
  double fLabelRangeXm, fLabelRangeXp, fLabelRangeYm, fLabelRangeYp;
  double fMinorXm, fMinorXp, fMinorYm, fMinorYp;
  double dMajorXm, dMajorXp, dMajorYm, dMajorYp;
  Coords coords;
  int flipeast, flipnorth;
  char axis[8], labels[8], ticks[8];
  char formatXm[16], formatXp[16], formatYm[16], formatYp[16];
} Graphdata;


int GetGraphdata PROTO((Graphdata *data, int *kapa, char *name));

#endif //GET_GRAPHDATA_H
