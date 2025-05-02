# include "relphot.h"

StatDataSet *StatDataSetAlloc (int Nsecfilt, int Nmax) {

  int i;

  StatDataSet *dataset = NULL;
  ALLOCATE (dataset, StatDataSet, Nsecfilt);
  for (i = 0; i < Nsecfilt; i++) {
    ALLOCATE (dataset[i].flxlist, double, Nmax);
    ALLOCATE (dataset[i].wgtlist, double, Nmax);
    ALLOCATE (dataset[i].errlist, double, Nmax);
    ALLOCATE (dataset[i].ranking,    int, Nmax);
    ALLOCATE (dataset[i].measSeq,    int, Nmax);
    ALLOCATE (dataset[i].msklist,    int, Nmax);

    ALLOCATE (dataset[i].values,   double, Nmax);
    ALLOCATE (dataset[i].wtvals,   double, Nmax);
    ALLOCATE (dataset[i].wtlist,   double, Nmax);
    ALLOCATE (dataset[i].ykeep,    double, Nmax);
    ALLOCATE (dataset[i].dykeep,   double, Nmax);
    ALLOCATE (dataset[i].wtkeep,   double, Nmax);
    ALLOCATE (dataset[i].ysample,  double, Nmax);
    ALLOCATE (dataset[i].dysample, double, Nmax);
    ALLOCATE (dataset[i].wtsample, double, Nmax);
    ALLOCATE (dataset[i].bvalue,   double, NBOOTSTRAP);
  }  

  return dataset;
}

void StatDataSetFree (StatDataSet *dataset, int Nsecfilt) {

  int i;

  for (i = 0; i < Nsecfilt; i++) {
    FREE (dataset[i].flxlist);
    FREE (dataset[i].wgtlist);
    FREE (dataset[i].errlist);
    FREE (dataset[i].ranking);
    FREE (dataset[i].measSeq);
    FREE (dataset[i].msklist);

    FREE (dataset[i].values);
    FREE (dataset[i].wtvals);
    FREE (dataset[i].wtlist);
    FREE (dataset[i].ykeep);
    FREE (dataset[i].dykeep);
    FREE (dataset[i].wtkeep);
    FREE (dataset[i].ysample);
    FREE (dataset[i].dysample);
    FREE (dataset[i].wtsample);
    FREE (dataset[i].bvalue);
  }  
  FREE (dataset);
}

