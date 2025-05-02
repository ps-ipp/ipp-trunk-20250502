/** Diagnostic plots for pmStack
 * @author Chris Beaumont, IfA
 */

/* Include Files   */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pslib.h>

#include "pmKapaPlots.h"
#include "pmVisual.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmAstrometryObjects.h"

# if (HAVE_KAPA)
# include <kapa.h>

//variables to determine when things are plotted
static bool plotTestImage        = true;

// variables to store plotting window indices
static int kapa  = -1;
static int kapa2 = -1;

/** destroy windows at the end of a run*/
bool pmStackVisualClose()
{
    if(kapa != -1)
        KapaClose(kapa);
    if(kapa2 != -1)
        KapaClose(kapa2);
    return true;
}

/** Display a test image
 * @param image to plot
 * @param name for plot. No spaces allowed.
 */
bool pmStackVisualPlotTestImage(psImage *image, char *name) {
    if (!pmVisualIsVisual() || !plotTestImage) return true;
    if (!pmVisualInitWindow(&kapa, "pmStack:Images")) return false;

    if(!pmVisualScaleImage(kapa, image, (const char*)name, 0, true)) return false;
    pmVisualAskUser(&plotTestImage);
    return true;
}

#else
bool pmStackVisualClose() {return true;}
bool pmStackVisualPlotTestImage(psImage *image, char *name) {return true;}
#endif
