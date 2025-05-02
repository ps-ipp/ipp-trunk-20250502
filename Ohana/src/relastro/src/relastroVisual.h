#ifndef RELASTRO_VISUAL_H
#define RELASTROPVISUAL_H

int relastroVisualPlotRawRef(StarData *raw, StarData *ref, double dRmax, int numObj);
int relastroVisualPlotScatter(double values[], double thresh, int npts);
int relastroVisualPlotOutliers(Catalog *catalog, int offset, int Nmeasure, 
			       StatType statsR, StatType statsD, double thresh);

#endif
