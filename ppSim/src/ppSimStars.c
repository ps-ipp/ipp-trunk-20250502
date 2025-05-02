# include "ppSim.h"

void ppSimStarFree(ppSimStar *star)
{
    return;
}

ppSimStar *ppSimStarAlloc(void) {

    ppSimStar *star = (ppSimStar *) psAlloc(sizeof(ppSimStar));

    star->ra = NAN;
    star->dec = NAN;
    star->mag = NAN;
    star->x = NAN;
    star->y = NAN;
    star->flux = NAN;
    star->peak = NAN;
    star->external = FALSE; // star was supplied (not randomly generated) for this analysis

    psMemSetDeallocator(star, (psFreeFunc) ppSimStarFree);
    return star;
}

void ppSimGalaxyFree(ppSimGalaxy *galaxy)
{
    return;
}

ppSimGalaxy *ppSimGalaxyAlloc(void) {

    ppSimGalaxy *galaxy = (ppSimGalaxy *) psAlloc(sizeof(ppSimGalaxy));
    psMemSetDeallocator(galaxy, (psFreeFunc) ppSimGalaxyFree);

    return galaxy;
}

float ppSimStarSkyNoise (float skySigma, float seeingSigma) {

    float skyNoise = skySigma * sqrt(4*M_PI*PS_SQR(seeingSigma));
    return skyNoise;
}

float ppSimStarPeakToFlux (float peak, float seeingSigma) {

    float psfArea = 2.0*M_PI*PS_SQR(seeingSigma);
    float flux = peak * psfArea;
    return flux;
}

float ppSimStarFluxToPeak (float flux, float seeingSigma) {

    float psfArea = 2.0*M_PI*PS_SQR(seeingSigma);
    float peak = flux / psfArea;
    return peak;
}

float ppSimFluxToMag (float flux, float zp) {

    float mag = -2.5*log10(flux) + zp;
    return mag;
}

float ppSimMagToFlux (float mag, float zp) {

    float flux = powf (10.0, -0.4*(mag - zp));
    return flux;
}
