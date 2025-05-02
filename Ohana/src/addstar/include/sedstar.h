# include "addstar.h"
# include "kapa.h"

typedef enum {
  SED_FIT,
  SED_REQ,
  SED_MODEL,
  SED_SAMPLE,
} SEDtableModes;

typedef struct {
  float *mags;
  float color;
  float Temp;
  float Av;
} SEDtableRow;

typedef struct {
  float chisq;
  float Md;
  int row;
} SEDfit;

typedef struct {
  int Nfilter;
  float *wavecode;
  float *vegaToAB;
  int *mode;
  int *hashcode;
  int *code;
  int codeP;
  int codeM;
  SEDtableRow **row;
  int Nrow;
} SEDtable;

SEDtableRow **sort_SEDtable (SEDtableRow *raw, int N);
SEDfit SEDchisq (SEDtableRow *ref, SEDtableRow *data, SEDtableRow *error, int Nfilter);
SEDtable *SEDtableLoad (char *filename);
int SEDcolorBracket (SEDtable *table, float color, float delta);
int SEDfitInit (SEDtable *table);
int SEDfitPlot (SEDtable *table, double R, double D, SEDfit *minFit, SEDtableRow *sourceValue, SEDtableRow *sourceError);
int SEDfitClear (void);
int SEDfitCatalog (Catalog *outcat, Catalog *incat, SEDtable *table);
void SetLimitsRaw (float *xvec, float *yvec, int Nelements, Graphdata *graphmode);
