/** @file psastroMosaicDemoPlot.c
 *
 *  @brief 
 *
 *  @ingroup libpsastro
 *
 *  @author IfA
 *  @version $Revision: 1.12 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

# if (HAVE_KAPA)
# include <kapa.h>

bool pmKapaPlotVectorTriple_AutoLimitsZscale_OpenGraph (int kapa, Graphdata *graphdata, psVector *xVec, psVector *yVec, psVector *zVec, bool increasing);

bool psastroPlotRawstars (psArray *rawstars, pmFPA *fpa, pmChip *chip, psMetadata *recipe)
{
    Graphdata graphdata;
    KapaSection section;

    int kapa = pmKapaOpen (true);
    if (kapa == -1) {
        psError(PS_ERR_UNKNOWN, true, "failure to open kapa");
        return false;
    }

    bool status = false;
    float iMagMin = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.INST.MAG.MIN");
    float iMagMax = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.INST.MAG.MAX");

    KapaResize (kapa, 1000, 1000);
    KapaInitGraph (&graphdata);
    KapaClearPlots (kapa);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 7;
    graphdata.size = 0.5;
    graphdata.style = 2;

    section.dx = 0.5;
    section.dy = 0.5;

    psVector *xVec = psVectorAlloc (rawstars->n, PS_TYPE_F32);
    psVector *yVec = psVectorAlloc (rawstars->n, PS_TYPE_F32);
    psVector *zVec = psVectorAlloc (rawstars->n, PS_TYPE_F32);

    section.x = 0.0;
    section.y = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "a0");
    KapaSetSection (kapa, &section);
    psFree (section.name);

    int n = 0;
    for (int i = 0; i < rawstars->n; i++) {
        pmAstromObj *raw = rawstars->data[i];
        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;

        xVec->data.F32[n] = raw->chip->x;
        yVec->data.F32[n] = raw->chip->y;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;
    pmKapaPlotVectorTriple_AutoLimits_OpenGraph (kapa, &graphdata, xVec, yVec, zVec, false);

    section.x = 0.5;
    section.y = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "a1");
    KapaSetSection (kapa, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < rawstars->n; i++) {
        pmAstromObj *raw = rawstars->data[i];
        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;

        xVec->data.F32[n] = raw->FP->x;
        yVec->data.F32[n] = raw->FP->y;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;
    pmKapaPlotVectorTriple_AutoLimits_OpenGraph (kapa, &graphdata, xVec, yVec, zVec, false);

    section.x = 0.0;
    section.y = 0.0;
    section.name = NULL;
    psStringAppend (&section.name, "a2");
    KapaSetSection (kapa, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < rawstars->n; i++) {
        pmAstromObj *raw = rawstars->data[i];
        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;

        xVec->data.F32[n] = raw->TP->x;
        yVec->data.F32[n] = raw->TP->y;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;

    pmKapaPlotVectorTriple_AutoLimits_OpenGraph (kapa, &graphdata, xVec, yVec, zVec, false);

    section.x = 0.5;
    section.y = 0.0;
    section.name = NULL;
    psStringAppend (&section.name, "a3");
    KapaSetSection (kapa, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < rawstars->n; i++) {
        pmAstromObj *raw = rawstars->data[i];
        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;

        xVec->data.F32[n] = DEG_RAD*raw->sky->r;
        yVec->data.F32[n] = DEG_RAD*raw->sky->d;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;
    pmKapaPlotVectorTriple_AutoLimits_OpenGraph (kapa, &graphdata, xVec, yVec, zVec, false);

    // flip x (East increase to left)
    SWAP (graphdata.xmin, graphdata.xmax);
    KapaSetLimits (kapa, &graphdata);

    // pause and wait for user input:
    // continue, save (provide name), ??
    char key[10], name[80];
    fprintf (stdout, "(s)ave plot or [c]ontinue? ");
    if (!fgets(key, 8, stdin)) {
        psWarning("Unable to read option");
    } else if (key[0] == 's') {
        fprintf (stdout, "enter plot name [rawstars.png]: ");
        if (fscanf(stdin, "%s", name) != 1) {
            psWarning("Unable to read plot name");
        } else if (!strcmp (name, "")) {
            strcpy (name, "rawstars.png");
        }
        KapaPNG (kapa, name);
    }

    psFree (xVec);
    psFree (yVec);
    psFree (zVec);
    return true;
}

bool psastroPlotRefstars (psArray *refstars, psMetadata *recipe)
{
    Graphdata graphdata;

    int kapa = pmKapaOpen (true);
    if (kapa == -1) {
        psError(PS_ERR_UNKNOWN, true, "failure to open kapa");
        return false;
    }

    bool status = false;
    float rMagMin = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.REF.MAG.MIN");
    float rMagMax = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.REF.MAG.MAX");

    KapaResize (kapa, 1000, 1000);
    KapaInitGraph (&graphdata);
    KapaClearSections (kapa);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 7;
    graphdata.size = 0.5;
    graphdata.style = 2;

    psVector *xVec = psVectorAlloc (refstars->n, PS_TYPE_F32);
    psVector *yVec = psVectorAlloc (refstars->n, PS_TYPE_F32);
    psVector *zVec = psVectorAlloc (refstars->n, PS_TYPE_F32);

    int n = 0;
    for (int i = 0; i < refstars->n; i++) {
        pmAstromObj *ref = refstars->data[i];
        if (!isfinite(ref->Mag)) continue;
        if (ref->Mag > rMagMax) continue;
        if (ref->Mag < rMagMin) continue;

        xVec->data.F32[n] = DEG_RAD*ref->sky->r;
        yVec->data.F32[n] = DEG_RAD*ref->sky->d;
        zVec->data.F32[n] = ref->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;
    pmKapaPlotVectorTriple_AutoLimits_OpenGraph (kapa, &graphdata, xVec, yVec, zVec, false);

    // flip x (East increase to left)
    SWAP (graphdata.xmin, graphdata.xmax);
    KapaSetLimits (kapa, &graphdata);

    // pause and wait for user input:
    // continue, save (provide name), ??
    char key[10], name[80];
    fprintf (stdout, "(s)ave plot or [c]ontinue? ");
    if (!fgets(key, 8, stdin)) {
        psWarning("Couldn't read anything.");
    } else if (key[0] == 's') {
        fprintf (stdout, "enter plot name [refstars.png]: ");
        if (fscanf (stdin, "%s", name) != 1) {
            psWarning("Unable to read name");
        } else if (!strcmp (name, "")) {
            strcpy (name, "refstars.png");
        }
        KapaPNG (kapa, name);
    }

    psFree (xVec);
    psFree (yVec);
    psFree (zVec);
    return true;
}

bool psastroPlotOneChipFit (psArray *rawstars, psArray *refstars, psArray *match, psMetadata *recipe) {

    Graphdata graphdata;
    KapaSection section;

    int kapa = pmKapaOpen (true);
    if (kapa == -1) {
        psError(PS_ERR_UNKNOWN, true, "failure to open kapa");
        return false;
    }

    bool status = false;
    float iMagMin = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.INST.MAG.MIN");
    float iMagMax = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.INST.MAG.MAX");
    float rMagMin = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.REF.MAG.MIN");
    float rMagMax = psMetadataLookupF32 (&status, recipe, "PSASTRO.PLOT.REF.MAG.MAX");

    KapaResize (kapa, 1000, 1000);
    KapaInitGraph (&graphdata);
    KapaClearPlots (kapa);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 7;
    graphdata.size = 0.5;
    graphdata.style = 2;

    section.dx = 0.5;
    section.dy = 0.5;

    psVector *xVec = psVectorAlloc (match->n, PS_TYPE_F32);
    psVector *yVec = psVectorAlloc (match->n, PS_TYPE_F32);
    psVector *zVec = psVectorAlloc (match->n, PS_TYPE_F32);

    // X vs dX
    section.x = 0.0;
    section.y = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "a0");
    KapaSetSection (kapa, &section);
    psFree (section.name);

    int n = 0;
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];

        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = raw->chip->x;
        yVec->data.F32[n] = raw->chip->x - ref->chip->x;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;
    pmKapaPlotVectorTriple_AutoLimits_OpenGraph (kapa, &graphdata, xVec, yVec, zVec, false);

    // X vs dY
    section.x = 0.5;
    section.y = 0.5;
    section.name = NULL;
    psStringAppend (&section.name, "a1");
    KapaSetSection (kapa, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];

        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = raw->chip->x;
        yVec->data.F32[n] = raw->chip->y - ref->chip->y;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;
    pmKapaPlotVectorTriple_AutoLimits_OpenGraph (kapa, &graphdata, xVec, yVec, zVec, false);

    // Y vs dX
    section.x = 0.0;
    section.y = 0.0;
    section.name = NULL;
    psStringAppend (&section.name, "a2");
    KapaSetSection (kapa, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];

        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = raw->chip->y;
        yVec->data.F32[n] = raw->chip->x - ref->chip->x;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;
    pmKapaPlotVectorTriple_AutoLimits_OpenGraph (kapa, &graphdata, xVec, yVec, zVec, false);

    // Y vs dY
    section.x = 0.5;
    section.y = 0.0;
    section.name = NULL;
    psStringAppend (&section.name, "a3");
    KapaSetSection (kapa, &section);
    psFree (section.name);

    n = 0;
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *raw = rawstars->data[pair->raw];
        pmAstromObj *ref = refstars->data[pair->ref];

        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = raw->chip->y;
        yVec->data.F32[n] = raw->chip->y - ref->chip->y;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;
    pmKapaPlotVectorTriple_AutoLimits_OpenGraph (kapa, &graphdata, xVec, yVec, zVec, false);

    // *** X vs Y plot (different window)
    int kapa2 = KapaOpenNamedSocket ("kapa", "XvsY");
    if (kapa2 == -1) {
        psError(PS_ERR_UNKNOWN, true, "failure to open kapa");
        return false;
    }

    KapaResize (kapa2, 1000, 1000);
    KapaInitGraph (&graphdata);
    KapaClearPlots (kapa2);

    graphdata.color = KapaColorByName ("black");
    graphdata.ptype = 2;
    graphdata.style = 2;

    psFree (xVec);
    psFree (yVec);
    psFree (zVec);

    xVec = psVectorAlloc (rawstars->n, PS_TYPE_F32);
    yVec = psVectorAlloc (rawstars->n, PS_TYPE_F32);
    zVec = psVectorAlloc (rawstars->n, PS_TYPE_F32);

    // X vs Y by mag (raw)
    n = 0;
    for (int i = 0; i < rawstars->n; i++) {
        pmAstromObj *raw = rawstars->data[i];
        if (!isfinite(raw->Mag)) continue;
        if (raw->Mag < iMagMin) continue;
        if (raw->Mag > iMagMax) continue;

        xVec->data.F32[n] = raw->chip->x;
        yVec->data.F32[n] = raw->chip->y;
        zVec->data.F32[n] = raw->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;
    pmKapaPlotVectorTriple_AutoLimits_OpenGraph (kapa2, &graphdata, xVec, yVec, zVec, false);

    graphdata.color = KapaColorByName ("red");
    graphdata.ptype = 7;
    graphdata.style = 2;

    psFree (xVec);
    psFree (yVec);
    psFree (zVec);

    xVec = psVectorAlloc (refstars->n, PS_TYPE_F32);
    yVec = psVectorAlloc (refstars->n, PS_TYPE_F32);
    zVec = psVectorAlloc (refstars->n, PS_TYPE_F32);

    // X vs Y by mag (raw)
    n = 0;
    for (int i = 0; i < refstars->n; i++) {
        pmAstromObj *ref = refstars->data[i];
        if (!isfinite(ref->Mag)) continue;
        if (ref->Mag < rMagMin) continue;
        if (ref->Mag > rMagMax) continue;

        xVec->data.F32[n] = ref->chip->x;
        yVec->data.F32[n] = ref->chip->y;
        zVec->data.F32[n] = ref->Mag;
        n++;
    }
    xVec->n = yVec->n = zVec->n = n;
    pmKapaPlotVectorTriple_AutoLimitsZscale_OpenGraph (kapa2, &graphdata, xVec, yVec, zVec, false);

    // pause and wait for user input:
    // continue, save (provide name), ??
    char key[10], name[80];
    fprintf (stdout, "(s)ave plot or [c]ontinue? ");
    if (!fgets (key, 8, stdin)) {
        psWarning("Couldn't read anything");
    } else if (key[0] == 's') {
        fprintf (stdout, "enter plot name [chipfit.png]: ");
        if (fscanf (stdin, "%s", name) != 1) {
            psWarning("Couldn't read name");
        } else if (!strcmp (name, "")) {
            strcpy (name, "chipfit.png");
        }
        KapaPNG (kapa, name);
    }

    close (kapa2);
    psFree (xVec);
    psFree (yVec);
    psFree (zVec);
    return true;
}

bool pmKapaPlotVectorTriple_AutoLimitsZscale_OpenGraph (int kapa, Graphdata *graphdata, psVector *xVec, psVector *yVec, psVector *zVec, bool increasing)
{

    // set limits based on data values
    float zmin = +FLT_MAX;
    float zmax = -FLT_MAX;
    for (int i = 0; i < xVec->n; i++) {
        zmin = PS_MIN (zmin, zVec->data.F32[i]);
        zmax = PS_MAX (zmax, zVec->data.F32[i]);
    }

    // set the scale vector
    psVector *zScale = psVectorAlloc (zVec->n, PS_DATA_F32);

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

    KapaSetFont (kapa, "helvetica", 14);
    KapaBox (kapa, graphdata);

    // the point size will be scaled from the z vector
    graphdata->size = -1;
    KapaPrepPlot (kapa, xVec->n, graphdata);
    KapaPlotVector (kapa, xVec->n, xVec->data.F32, "x");
    KapaPlotVector (kapa, yVec->n, yVec->data.F32, "y");
    KapaPlotVector (kapa, zVec->n, zScale->data.F32, "z");
    psFree (zScale);
    return true;
}

# else

bool psastroPlotRawstars (psArray *rawstars, pmFPA *fpa, pmChip *chip, psMetadata *recipe)
{
    return false;
}

bool psastroPlotRefstars (psArray *refstars, psMetadata *recipe)
{
    return false;
}

bool psastroPlotOneChipFit (psArray *rawstars, psArray *refstars, psArray *match,
    psMetadata *recipe)
{
    return false;
}

# endif
