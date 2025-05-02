/** The following are a collection of core procedures to aid the creation of visual diagnostics
 *  @author Chris Beaumont, IfA
 *  @date January 23, 2008
 **/

/* Include Files  */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pslib.h>

bool pmSubtractionVisualClose(void);
bool pmAstromVisualClose(void);
bool pmSubtractionVisualClose(void);
bool pmStackVisualClose(void);
bool pmSourceVisualClose(void);

#include "pmSourceSatstar.h"

# if (HAVE_KAPA)
# include <kapa.h>
#include "pmVisual.h"
#include "pmKapaPlots.h"

# define KAPAX 700
# define KAPAY 700

//#define TESTING

static bool isVisual = false;


bool pmVisualSetVisual(bool value) {
    isVisual = value;
    return true;
}


bool pmVisualIsVisual(void) {return isVisual;}

bool pmVisualClose(void) {
    pmAstromVisualClose();
    pmSubtractionVisualClose();
    pmStackVisualClose();
    pmSourceVisualClose();
    return true;
}

bool pmVisualInitWindow (int *kapid, char *name) {

    if (*kapid == -1) {
        *kapid = KapaOpenNamedSocket("kapa", name);
        if (*kapid == -1) {
            fprintf (stderr, "Failure to open kapa; visual mode disabled.\n");
	    pmVisualSetVisual(false);
            return false;
        }
        KapaResize (*kapid, KAPAX, KAPAY);
    }
    return true;
}


bool pmVisualInitGraph (int kapa, KapaSection *section, Graphdata *graphdata)
{
    KapaSetSection (kapa, section);
    KapaSetFont (kapa, "helvetica", 14);
    KapaSetLimits (kapa, graphdata);
    KapaBox (kapa, graphdata);
    return true;
}


// ask the user to continue or not.  give up after 2 seconds.
// XXX add option to turn on/off timeouts?
bool pmVisualAskUser(bool *plotFlag)
{
    struct timeval timeout;
    fd_set fdSet;
    int status;

    char key[10];
    if (plotFlag) {
	fprintf (stderr, "[p]ause? [c]ontinue? [s]kip the rest of these plots? [a]bort all visual plots? (c) ");
    } else {
	fprintf (stderr, "[p]ause? [c]ontinue? [a]bort all visual plots? (c) ");
    }

    /* Wait up to 1.0 second for a response, then continue */
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    FD_ZERO (&fdSet);
    FD_SET (STDIN_FILENO, &fdSet);

    status = select (1, &fdSet, NULL, NULL, &timeout);
    if (status <= 0) {
	fprintf (stderr, "\n");
	return true; // if no data, give up
    }

    while (true) {
	if (!fgets(key, 8, stdin)) {
	    psWarning("Unable to read option");
	}
	switch (key[0]) {
	  case 's':
	    if (plotFlag) *plotFlag = false;
	    return true;
	  case 'a':
	    isVisual = false;
	    return true;
	  case 'c':
	  case '\n':
	    return true;
	  default:
	    break;
	}
	
	if (plotFlag) {
	    fprintf (stderr, "[c]ontinue? [s]kip the rest of these plots? [a]bort all visual plots? (c) ");
	} else {
	    fprintf (stderr, "[c]ontinue? [a]bort all visual plots? (c) ");
	}
    }
    return true;
}


// ask the user to continue or not.  give up after 2 seconds.
bool pmVisualAskUserOrDump(bool *plotFlag, bool *dumpData)
{
    struct timeval timeout;
    fd_set fdSet;
    int status;

    if (dumpData) *dumpData = false;

    char key[10];
    if (plotFlag && dumpData) {
	fprintf (stderr, "[p]ause? [c]ontinue? [s]kip the rest of these plots? [d]ump the data? [a]bort all visual plots? (c) ");
    } 
    if (plotFlag && !dumpData) {
	fprintf (stderr, "[p]ause? [c]ontinue? [s]kip the rest of these plots? [a]bort all visual plots? (c) ");
    } 
    if (!plotFlag && dumpData) {
	fprintf (stderr, "[p]ause? [c]ontinue? [d]ump the data? [a]bort all visual plots? (c) ");
    } 
    if (!plotFlag && !dumpData) {
	fprintf (stderr, "[p]ause? [c]ontinue? [a]bort all visual plots? (c) ");
    }

    /* Wait up to 1.0 second for a response, then continue */
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    FD_ZERO (&fdSet);
    FD_SET (STDIN_FILENO, &fdSet);

    status = select (1, &fdSet, NULL, NULL, &timeout);
    if (status <= 0) {
	fprintf (stderr, "\n");
	return true; // if no data, give up
    }

    while (true) {
	if (!fgets(key, 8, stdin)) {
	    psWarning("Unable to read option");
	}
	switch (key[0]) {
	  case 's':
	    if (plotFlag) *plotFlag = false;
	    return true;
	  case 'd':
	    if (dumpData) *dumpData = true;
	    return true;
	  case 'a':
	    isVisual = false;
	    return true;
	  case 'c':
	  case '\n':
	    return true;
	  default:
	    break;
	}
	
	if (plotFlag && dumpData) {
	  fprintf (stderr, "[p]ause? [c]ontinue? [s]kip the rest of these plots? [d]ump the data? [a]bort all visual plots? (c) ");
	} 
	if (plotFlag && !dumpData) {
	  fprintf (stderr, "[p]ause? [c]ontinue? [s]kip the rest of these plots? [a]bort all visual plots? (c) ");
	} 
	if (!plotFlag && dumpData) {
	  fprintf (stderr, "[p]ause? [c]ontinue? [d]ump the data? [a]bort all visual plots? (c) ");
	} 
	if (!plotFlag && !dumpData) {
	  fprintf (stderr, "[p]ause? [c]ontinue? [a]bort all visual plots? (c) ");
	}
    }
    return true;
}

bool pmVisualImStats(psImage *image, double *mean, double *stdev, double *min, double *max) {

    *min = +FLT_MAX;
    *max = -FLT_MAX;
    double ex = 0;  // <x>
    double ex2 = 0; // <x^2>
    int numPix = 0; // number of finite pixels
    bool isDouble = image->type.type == PS_TYPE_F64;

    if(!isDouble && (image->type.type != PS_TYPE_F32)) {
        fprintf(stderr, "Image must be PS_TYPE_F32 or PS_TYPE_F64\n");
        return false;
    }

    for(int i = 0; i < image->numRows; i++) {
        for(int j = 0; j < image->numCols; j++) {
            double entry;
            if(isDouble) {
                if (!isfinite(image->data.F64[i][j])) continue;
                entry = image->data.F64[i][j];
            } else {
                if (!isfinite(image->data.F32[i][j])) continue;
                entry = image->data.F32[i][j];
            }
            numPix++;
            ex += entry;
            ex2 += (entry * entry);
            *min = PS_MIN(*min, entry);
            *max = PS_MAX(*max, entry);
        }
    }

    if (numPix == 0) return false;
    ex /= numPix;
    ex2 /= numPix;
    *mean = ex;
    *stdev = sqrt(ex2  - ex * ex);

    return true;
}


bool pmVisualTriplePlot (int kapid, Graphdata *graphdata, psVector *xVec, psVector *yVec, psVector *zVec, bool increasing)
{
    pmVisualScaleGraphdata (graphdata, xVec, yVec, true);
    //printf("%f %f %f %f \n",graphdata->xmin, graphdata->xmax, graphdata->ymin, graphdata->ymax);
    // set the scale vector
    psVector *zScale = psVectorAlloc (zVec->n, PS_DATA_F32);
    pmVisualCreateScaleVec (zVec, zScale, increasing);

    KapaSetFont (kapid, "helvetica", 14);
    KapaSetLimits(kapid, graphdata);
    KapaBox (kapid, graphdata);

    // the point size will be scaled from the z vector
    graphdata->ptype = KAPA_POINT_CIRCLE_OPEN;
    graphdata->style = KAPA_PLOT_POINTS;
    graphdata->size = -1;
    KapaPrepPlot (kapid, xVec->n, graphdata);
    KapaPlotVector (kapid, xVec->n, xVec->data.F32, "x");
    KapaPlotVector (kapid, yVec->n, yVec->data.F32, "y");
    KapaPlotVector (kapid, zVec->n, zScale->data.F32, "z");
    psFree (zScale);
    return true;
}


bool pmVisualTripleOverplot (int kapid, Graphdata *graphdata, psVector *xVec, psVector *yVec, psVector *zVec, bool increasing) {

    // set the scale vector
    psVector *zScale = psVectorAlloc (zVec->n, PS_DATA_F32);
    pmVisualCreateScaleVec (zVec, zScale, increasing);

    KapaSetFont (kapid, "helvetica", 14);

    // the point size will be scaled from the z vector
    graphdata->size = -1;
    KapaPrepPlot (kapid, xVec->n, graphdata);
    KapaPlotVector (kapid, xVec->n, xVec->data.F32, "x");
    KapaPlotVector (kapid, yVec->n, yVec->data.F32, "y");
    KapaPlotVector (kapid, zVec->n, zScale->data.F32, "z");
    psFree (zScale);
    return true;
}


bool pmVisualCreateScaleVec (psVector *zVec, psVector *zScale, bool increasing) {
    // set limits based on data values
    float zmin = +FLT_MAX;
    float zmax = -FLT_MAX;

    for (int i = 0; i < zVec->n; i++) {
        zmin = PS_MIN (zmin, zVec->data.F32[i]);
        zmax = PS_MAX (zmax, zVec->data.F32[i]);
    }

    if (increasing) {
	fprintf (stderr, "plotting points scaled from %f to %f\n", zmin, zmax);
    } else {
	fprintf (stderr, "plotting points scaled from %f to %f\n", zmax, zmin);
    }

    float range = zmax - zmin;
    if (range == 0.0) {
        psVectorInit (zScale, 1.0);
    } else {
        for (int i = 0; i < zVec->n; i++) {
            if (increasing) {
                zScale->data.F32[i] = PS_MIN (1.5, PS_MAX(0.05, 1.5*(zVec->data.F32[i] - zmin)/range));
            } else {
                zScale->data.F32[i] = PS_MIN (1.5, PS_MAX(0.05, 1.5*(zmax - zVec->data.F32[i])/range));
            }
        }
    }
    return true;
}


bool pmVisualScaleImage(int kapaFD, psImage *inImage, const char *name, int channel, bool clip) {
    KiiImage image;
    KapaImageData data;
    Coords coords;

    //make sure we have a compatible image
    if (inImage == NULL) {
        fprintf(stderr, "Image is NULL, and cannot be displayed\n");
        return false;
    }

    if(inImage->type.type != PS_TYPE_F32) {
        fprintf(stderr, "Cannot display this image (imcompatible data type)\n");
        return false;
    }

    strcpy (coords.ctype, "RA---TAN");


    double min, max, stdev, mean;
    if(!pmVisualImStats(inImage, &mean, &stdev, &min, &max)) return false;

    image.data2d = inImage->data.F32;
    image.Nx = inImage->numCols;
    image.Ny = inImage->numRows;
    strcpy (data.name, name);
    strcpy (data.file, name);

    data.zero = clip ? mean - 3 * stdev : min;
    data.range = clip ? 6 * stdev : max - min;
    data.logflux = 0;

    KiiSetChannel (kapaFD, channel);
    KiiNewPicture2D (kapaFD, &image, &data, &coords);

    return true;
}

bool pmVisualRangeImage (int kapaFD, psImage *inImage, const char *name, int channel, float min, float max) {

    KiiImage image;
    KapaImageData data;
    Coords coords;

    strcpy (coords.ctype, "RA---TAN");

    image.data2d = inImage->data.F32;
    image.Nx = inImage->numCols;
    image.Ny = inImage->numRows;

    strcpy (data.name, name);
    strcpy (data.file, name);
    data.zero = min;
    data.range = max - min;
    data.logflux = 0;

    KiiSetChannel (kapaFD, channel);
    KiiNewPicture2D (kapaFD, &image, &data, &coords);

    return true;
}

psImage* pmVisualImageToFloat(psImage *image) {
    psImage *result = psImageAlloc(image->numCols, image->numRows, PS_TYPE_F32);

    if (image->type.type == PS_TYPE_F32) {
        psImageOverlaySection(result, image, 0, 0, "=");
        return image;
    } else if (image->type.type == PS_TYPE_F64) {
        for (int i = 0; i < image->numRows; i++) {
            for (int j = 0; j < image->numCols; j++) {
                result->data.F32[i][j] = (float) image->data.F64[i][j];
            }
        }
    } else {
        fprintf(stderr, "Unsupported psImage data type (F32 or F64)");
        return NULL;
    }
    return result;
}


bool pmVisualScaleGraphdata(Graphdata *graphdata, psVector *xVec, psVector *yVec, bool clip) {

    graphdata->xmin = +FLT_MAX;
    graphdata->xmax = -FLT_MAX;
    graphdata->ymin = +FLT_MAX;
    graphdata->ymax = -FLT_MAX;

    //determine standard deviation of xVec and yVec
    psStats *statsX = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
    psStats *statsY = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
    if (!psVectorStats (statsX, xVec, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	return false;
    }
    if (!psVectorStats (statsY, yVec, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	return false;
    }

    float xhi  = statsX->sampleMedian + 3 *statsX->sampleStdev;
    float xlo = statsX->sampleMedian - 3 *statsX->sampleStdev;
    float yhi = statsY->sampleMedian + 3 *statsY->sampleStdev;
    float ylo = statsY->sampleMedian - 3 *statsY->sampleStdev;

    // don't sigma clip
    if (!clip) {
        xhi = +FLT_MAX;
        xlo = -FLT_MAX;
        yhi = +FLT_MAX;
        ylo = -FLT_MAX;
    }

    for(int i = 0; i < xVec->n; i++) {
        if (!isfinite(xVec->data.F32[i])) continue;
        if (xVec->data.F32[i] > xhi || xVec->data.F32[i] < xlo) continue;
        graphdata->xmin = PS_MIN (graphdata->xmin, xVec->data.F32[i]);
        graphdata->xmax = PS_MAX (graphdata->xmax, xVec->data.F32[i]);
    }

    for (int i = 0; i < yVec->n; i++) {
        if (!isfinite(xVec->data.F32[i])) continue;
        if (yVec->data.F32[i] > yhi || yVec->data.F32[i] < ylo) continue;
        graphdata->ymin = PS_MIN (graphdata->ymin, yVec->data.F32[i]);
        graphdata->ymax = PS_MAX (graphdata->ymax, yVec->data.F32[i]);
    }


    // abort if there is no good data
    if (!isfinite(xhi) || !isfinite(xlo) || !isfinite(yhi) || !isfinite(ylo)) {
        graphdata->xmin = -1;
        graphdata->ymin  = -1;
        graphdata->xmax = 1;
        graphdata->ymax = 1;
        psFree(statsX);
        psFree(statsY);
        return false;
    }

    // add a whitespace border
    float range = graphdata->xmax - graphdata->xmin;
    if (range == 0) range = 1;
    graphdata->xmin -= .05 * range;
    graphdata->xmax += .05 * range;

    range = graphdata->ymax - graphdata->ymin;
    if (range == 0) range = 1;
    graphdata->ymin -= .05 * range;
    graphdata->ymax += .05 * range;

    psFree (statsX);
    psFree (statsY);
    return true;
}

#else
bool pmVisualSetVisual(bool value) {return true;}
bool pmVisualIsVisual(void) {return false;}
bool pmVisualClose(void) {return true;}
bool pmVisualInitWindow(int *kapid, char *name){return true;}
bool pmVisualInitGraph (int kapa, void *section, void *graphdata){return true;}

bool pmVisualAskUser(bool *plotFlag){return true;}
bool pmVisualImStats(psImage *image, double *mean,
                     double *stdev, double *min, double *max){return true;}
bool pmVisualTriplePlot (int kapid, void *graphdata, psVector *xVec,
                         psVector *yVec, psVector *zVec, bool increasing){return true;}
bool pmVisualTripleOverplot (int kapid, void *graphdata, psVector *xVec,
                             psVector *yVec, psVector *zVec, bool increasing){return true;}
bool pmVisualCreateScaleVec (psVector *zVec, psVector *zScale, bool increasing){return true;}
bool pmVisualResidPlot (psArray *rawstars, psArray *refstars, psArray *match, psMetadata *recipe, char *title, int *kapa, int *kapa2){return true;}
bool pmVisualScaleImage(int kapaFD, psImage *inImage,
                        const char *name, int channel, bool clip){return true;}
bool pmVisualScaleGraphdata(void *graphdata, psVector *xVec,
                            psVector *yVec, bool clip){return true;}

psImage* pmVisualImageToFloat(psImage *image){return NULL;}


#endif
