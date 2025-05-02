# include "psphotInternal.h"

# if (HAVE_KAPA)

static int kapa_fd = -1;

int psphotKapaOpen ()
{
    char kapa[64];

    strcpy (kapa, "kapa");

    if (kapa_fd == -1) {
        kapa_fd = KapaOpenNamedSocket (kapa, "psphot");
    }
    return kapa_fd;
}

bool psphotKapaClose ()
{

    if (kapa_fd == -1)
        return true;
    KapaClose (kapa_fd);
    kapa_fd = -1;
    return true;
}

bool psphotImageBackgroundCellHistogram (psVector *values, float mean, float sigma, int ix, int iy)
{

    KapaSection section;
    Graphdata graphdata;

    int kapa = pmKapaOpen (true);
    if (kapa == -1) {
        psError(PS_ERR_UNKNOWN, true, "failure to open kapa");
        return false;
    }

    psStats *stats = psStatsAlloc (PS_STAT_MAX | PS_STAT_MIN);
    if (!psVectorStats (stats, values, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats for histogram");
        return false;
    }

    psHistogram *histogram = psHistogramAlloc (stats->min, stats->max, 1000);
    psVectorHistogram (histogram, values, NULL, NULL, 0);

    KapaInitGraph (&graphdata);
    KapaClearPlots (kapa);

    // plot 1 is the full range
    section.x  = 0.0;
    section.y  = 0.0;
    section.dx = 1.0;
    section.dy = 0.5;
    section.name = strcreate ("bottom");
    KapaSetSection (kapa, &section);
    free (section.name);

    // set limits based on data values
    graphdata.xmin = stats->min;
    graphdata.xmax = stats->max;

    psStatsInit (stats);
    stats->options = PS_STAT_MAX | PS_STAT_MIN;
    if (!psVectorStats (stats, histogram->nums, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats for histogram");
	psFree(histogram);
        return false;
    }

    // scale the plot to hold the histogram
    graphdata.ymin = stats->min - 0.05*(stats->max - stats->min);
    graphdata.ymax = stats->max + 0.05*(stats->max - stats->min);

    KapaSetLimits (kapa, &graphdata);
    KapaSetFont (kapa, "helvetica", 14);
    KapaBox (kapa, &graphdata);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 1;
    KapaPrepPlot (kapa, histogram->nums->n, &graphdata);
    KapaPlotVector (kapa, histogram->nums->n, histogram->bounds->data.F32, "x");
    KapaPlotVector (kapa, histogram->nums->n, histogram->nums->data.F32, "y");
    psFree (histogram);

    // plot 2 is the +/- 10 sigma
    section.x  = 0.0;
    section.y  = 0.5;
    section.dx = 1.0;
    section.dy = 0.5;
    section.name = strcreate ("top");
    KapaSetSection (kapa, &section);
    free (section.name);

    // +/- 10 sigma
    graphdata.xmin = mean - 10.0*sigma;
    graphdata.xmax = mean + 10.0*sigma;

    histogram = psHistogramAlloc (graphdata.xmin, graphdata.xmax, 100);
    psVectorHistogram (histogram, values, NULL, NULL, 0);

    psStatsInit (stats);
    stats->options = PS_STAT_MAX | PS_STAT_MIN;
    if (!psVectorStats (stats, histogram->nums, NULL, NULL, 0)) {
	psError(PS_ERR_UNKNOWN, false, "failure to measure stats for histogram");
	psFree(histogram);
        return false;
    }

    // scale the plot to hold the histogram
    graphdata.ymin = stats->min - 0.05*(stats->max - stats->min);
    graphdata.ymax = stats->max + 0.05*(stats->max - stats->min);

    KapaSetLimits (kapa, &graphdata);
    KapaSetFont (kapa, "helvetica", 14);
    KapaBox (kapa, &graphdata);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 0;
    graphdata.size = 0.5;
    graphdata.style = 1;
    KapaPrepPlot (kapa, histogram->nums->n, &graphdata);
    KapaPlotVector (kapa, histogram->nums->n, histogram->bounds->data.F32, "x");
    KapaPlotVector (kapa, histogram->nums->n, histogram->nums->data.F32, "y");

    char line[128];
    sprintf (line, "sky: %f +/- %f, cell %d,%d", mean, sigma, ix, iy);
    KapaSendLabel (kapa, line, KAPA_LABEL_XP);

    // pause until user types 'return'
    fprintf(stdout, "press return");
    if (!fgets(line, 64, stdin)) {
        // This is just to avoid a compiler warning on some systems; it's not really necessary
        psWarning("Couldn't read anything.");
    }

    psFree (stats);
    psFree (histogram);

    return true;
}

bool psphotDiagnosticPlots (const pmConfig *config, const char *name, ...) {

    va_list argPtr;
    bool status;

    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    assert (recipe);

    psMetadata *plots = psMetadataLookupPtr (&status, recipe, "DIAGNOSTIC.PLOTS");
    assert (plots);

    // do we want the requested plot?
    if (!psMetadataLookupBool (&status, plots, name)) return false;

    // Get the variable list parameters to pass to allocation function
    va_start(argPtr, name);

    if (!strcmp(name, "IMAGE.BACKGROUND.CELL.HISTOGRAM")) {

        int ix = va_arg(argPtr, psS32);
        int iy = va_arg(argPtr, psS32);
        float mean  = va_arg(argPtr, double);
        float sigma = va_arg(argPtr, double);
        psVector *values = va_arg(argPtr, psPtr);

        int xPlot = psMetadataLookupS32 (&status, plots, "IMAGE.BACKGROUND.CELL.HISTOGRAM.X");
        assert (status);
        int yPlot = psMetadataLookupS32 (&status, plots, "IMAGE.BACKGROUND.CELL.HISTOGRAM.Y");
        assert (status);

        bool gotX = (xPlot < 0) || (xPlot == ix);
        bool gotY = (yPlot < 0) || (yPlot == iy);

        if (gotX && gotY) {
            psphotImageBackgroundCellHistogram (values, mean, sigma, ix, iy);
            goto done;
        }
    }

    // Clean up stack after variable arguement has been used
done:
    va_end(argPtr);
    return true;
}

# else

int psphotKapaOpen () { return -1; }
bool psphotKapaClose () { return true; }
bool psphotImageBackgroundCellHistogram (psVector *values, float mean, float sigma, int ix, int iy)
{
    return true;
}
bool psphotDiagnosticPlots (pmConfig *config, char *name, ...) { return true; }

# endif
