# include "psphot.h"

bool psphotSetThreads () {

    psThreadTask *task = NULL;

    pmPSFThreads ();

    task = psThreadTaskAlloc("PSPHOT_MODEL_BACKGROUND", 15);
    task->function = &psphotModelBackground_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_GUESS_MODEL", 5);
    task->function = &psphotGuessModel_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_ADD_NOISE", 6);
    task->function = &psphotAddOrSubNoise_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_MAGNITUDES", 9);
    task->function = &psphotMagnitudes_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_PSF_WEIGHTS", 3);
    task->function = &psphotPSFWeights_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_APRESID_MAGS", 7);
    task->function = &psphotApResidMags_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_SOURCE_STATS", 11);
    task->function = &psphotSourceStats_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_KRON_ITERATE", 11);
    task->function = &psphotKronIterate_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_KRON_FLUX", 4);
    task->function = &psphotKronFlux_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_PETRO_FLUX", 4);
    task->function = &psphotPetroFlux_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_GALAXY_SHAPES", 7);
    task->function = &psphotGalaxyShape_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_BLEND_FIT", 10);
    task->function = &psphotBlendFit_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_EXTENDED_FIT", 16);
    task->function = &psphotExtendedSourceFits_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_EXTENDED_ANALYSIS", 8);
    task->function = &psphotExtendedSourceAnalysis_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_RADIAL_APERTURES", 6);
    task->function = &psphotRadialApertures_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_RADIAL_PROFILE_WINGS", 3);
    task->function = &psphotRadialProfileWings_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_CHIP_PARAMS", 2);
    task->function = &psphotChipParams_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSPHOT_GALAXY_PARAMS", 11);
    task->function = &psphotGalaxyParams_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    return true;
}
