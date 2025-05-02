/** @file  psEarthOrientation.c
 *
 *  @brief Function implementations for earth orientation calculations
 *
 *  @ingroup EarthOrientation
 *
 *  @author Dave Robbins, MHPCC
 *  @author Robert Daniel DeSonia, MHPCC
 *
 *  @version $Revision: 1.47 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 23:33:06 $
 *
 *  Copyright 2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <math.h>
#include <string.h>

#include "psEarthOrientation.h"
#include "psArray.h"
#include "psPolynomial.h"
#include "psVector.h"
#include "psMetadata.h"
#include "psMetadataConfig.h"
#include "psError.h"

#include "psMemory.h"
#include "psCoord.h"
#include "psAssert.h"

// Sun's Mass (src: Google's Calculator Service)
#define PS_M   1.98892e30 /* kilograms */
// Newton's Gravitational Constant (src:NIST)
#define PS_G   6.6742e-11 /* m^3/kg/s^2 */
// Speed of light in vacuum (src:NIST)
#define PS_C0  299792458.0 /* m/s */
// Average distance from earth to sun
#define PS_AU 149597890000.0 /* meters */
// Modified Julian Day 1/1/2000 00:00:00
#define MJD_2000  51544.5
// Days in Julian century
#define JULIAN_CENTURY 36525.0

//The arrays used for storage of x,y,&s-values extracted from the precession data tables.
// Created in p_psEOCInit from finalsTable and used in psPrecessionModel.
static psArray* xTable = NULL;
static psArray* yTable = NULL;
static psArray* sTable = NULL;

//used for storage of the IERS precession table.  Should contain MJD date followed by the
// corresponding dX & dY values from the finals2000A table.  (Used in psPrecessionCorr.)
static psArray* iersTable = NULL;
//used for precession data.  Should contain MJD date followed by the corresponding X, Y, &
// UT1-UTC values from the finals2000A table.  (Used in psPrecessionModel, psGetPolarMotion).
static psArray* finalsTable = NULL;

//1D Polynomials created from the coefficients given by the tab5.2?.txt files.  Used
// by psPrecessionModel to determine psEarthPole components.
static psPolynomial1D* xPoly = NULL;
static psPolynomial1D* yPoly = NULL;
static psPolynomial1D* sPoly = NULL;

//Boolean variable to tell whether the above EOC data has been initialized.
static bool eocInitialized = false;

//Internal function used for conversion from a 3x3 rotation matrix to a quaternion (psSphereRot).
static psSphereRot *rotMatrix_To_Quat(double A[3][3]);

static void earthPoleFree(psEarthPole *pole)
{
    // There are non dynamic allocated items
}

psEarthPole *psEarthPoleAlloc(void)
{
    psEarthPole* pole = psAlloc(sizeof(psEarthPole));
    psMemSetDeallocator(pole, (psFreeFunc) earthPoleFree);
    pole->x = 0.0;
    pole->y = 0.0;
    pole->s = 0.0;
    return pole;
}

bool p_psEOCInit()
{
    unsigned int nFail = 0;

    // Read config file
    psMetadata* eocMetadata = psMetadataConfigRead(NULL,
                              &nFail,
                              p_psTimeConfigFilename(NULL),
                              true);
    //Make sure reading of config file worked correctly
    if(eocMetadata == NULL) {
        return false;
    } else if(nFail != 0) {
        return false;
    }

    bool success = false;
    // Get table formats & error if lookups fail.
    char* tableFormat = psMetadataLookupStr(&success, eocMetadata,
                                            "psLib.eoc.precession.table.format");
    if(! success || tableFormat == NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.eoc.precession.table.format");
        return false;
    }

    char* xTableName = psMetadataLookupStr(&success, eocMetadata,
                                           "psLib.eoc.precession.table.file.x");
    if(! success || xTableName == NULL) {
        psFree(tableFormat);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.eoc.precession.x.file");
        return false;
    }

    char* yTableName = psMetadataLookupStr(&success, eocMetadata,
                                           "psLib.eoc.precession.table.file.y");
    if(! success || yTableName == NULL) {
        psFree(tableFormat);
        psFree(xTableName);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.eoc.precession.y.file");
        return false;
    }

    char* sTableName = psMetadataLookupStr(&success, eocMetadata,
                                           "psLib.eoc.precession.table.file.s");
    if(! success || sTableName == NULL) {
        psFree(tableFormat);
        psFree(xTableName);
        psFree(yTableName);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.eoc.precession.s.file");
        return false;
    }

    char* iersTableName = psMetadataLookupStr(&success, eocMetadata,
                          "psLib.eoc.precession.table.file.iers");
    if(! success || sTableName == NULL) {
        psFree(tableFormat);
        psFree(xTableName);
        psFree(yTableName);
        psFree(sTableName);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.eoc.precession.iers.file");
        return false;
    }

    char* finalsTableName = psMetadataLookupStr(&success, eocMetadata,
                            "psLib.eoc.precession.table.file.final");
    if(! success || sTableName == NULL) {
        psFree(tableFormat);
        psFree(xTableName);
        psFree(yTableName);
        psFree(sTableName);
        psFree(iersTableName);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.eoc.precession.finals.file");
        return false;
    }

    char* iersTableFormat = psMetadataLookupStr(&success, eocMetadata,
                            "psLib.eoc.precession.iers.table.format");
    if(! success || iersTableFormat == NULL) {
        psFree(tableFormat);
        psFree(xTableName);
        psFree(yTableName);
        psFree(sTableName);
        psFree(iersTableName);
        psFree(finalsTableName);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.eoc.precession.iers.table.format");
        return false;
    }

    char* finalsTableFormat = psMetadataLookupStr(&success, eocMetadata,
                              "psLib.eoc.precession.finals.table.format");
    if(! success || iersTableFormat == NULL) {
        psFree(tableFormat);
        psFree(xTableName);
        psFree(yTableName);
        psFree(sTableName);
        psFree(iersTableName);
        psFree(finalsTableName);
        psFree(iersTableFormat);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.eoc.precession.iers.table.format");
        return false;
    }

    //Extract the necessary data to setup the tables
    xTable = psVectorsReadFromFile(xTableName, tableFormat);
    yTable = psVectorsReadFromFile(yTableName, tableFormat);
    sTable = psVectorsReadFromFile(sTableName, tableFormat);
    iersTable = psVectorsReadFromFile(iersTableName, iersTableFormat);
    finalsTable = psVectorsReadFromFile(finalsTableName, finalsTableFormat);

    //Check that the data extraction was performed successfully.
    if(xTable == NULL || yTable == NULL || sTable == NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Failed to read the precession-nutation model tables.");
        return false;
    }
    if(iersTable == NULL || finalsTable == NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Failed to read the IERS/finals tables.");
        return false;
    }

    //Load the coefficient data to setup the polynomials.
    psVector* xCoeff = psMetadataLookupPtr(&success, eocMetadata, "psLib.eoc.precession.poly.x");
    if(! success || xCoeff == NULL || xCoeff->type.type != PS_TYPE_F64) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.eoc.precession.poly.x");
        return false;
    }
    psVector* yCoeff = psMetadataLookupPtr(&success, eocMetadata, "psLib.eoc.precession.poly.y");
    if(! success || yCoeff == NULL || yCoeff->type.type != PS_TYPE_F64) {
        psFree(xCoeff);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.eoc.precession.poly.y");
        return false;
    }
    psVector* sCoeff = psMetadataLookupPtr(&success, eocMetadata, "psLib.eoc.precession.poly.s");
    if(! success || sCoeff == NULL || sCoeff->type.type != PS_TYPE_F64) {
        psFree(xCoeff);
        psFree(yCoeff);
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, _("Failed find '%s' in time metadata."),
                "psLib.eoc.precession.poly.s");
        return false;
    }
    //Allocate the X,Y,&S polynomials to be used for earthpole calculations
    xPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, xCoeff->n);
    memcpy(xPoly->coeff, xCoeff->data.F64,PSELEMTYPE_SIZEOF(PS_TYPE_F64)*xCoeff->n);
    yPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, yCoeff->n);
    memcpy(yPoly->coeff, yCoeff->data.F64,PSELEMTYPE_SIZEOF(PS_TYPE_F64)*yCoeff->n);
    sPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, sCoeff->n);
    memcpy(sPoly->coeff, sCoeff->data.F64,PSELEMTYPE_SIZEOF(PS_TYPE_F64)*sCoeff->n);

    psFree(eocMetadata);

    return true;
}

bool p_psEOCFinalize(void)
{
    psFree(xTable);
    psFree(yTable);
    psFree(sTable);    // There are non dynamic allocated items
    psFree(iersTable);
    psFree(finalsTable);

    xTable = NULL;
    yTable = NULL;
    sTable = NULL;
    iersTable = NULL;
    finalsTable = NULL;

    psFree(xPoly);
    psFree(yPoly);
    psFree(sPoly);

    xPoly = NULL;
    yPoly = NULL;
    sPoly = NULL;

    eocInitialized = false;

    return true;
}

psSphere *psAberration(psSphere *apparent,
                       const psSphere *actual,
                       const psSphere *direction,
                       double speed)
{
    //Check for valid inputs
    PS_ASSERT_PTR_NON_NULL(actual, NULL);
    PS_ASSERT_PTR_NON_NULL(direction, NULL);
    if (fabs(speed) < DBL_EPSILON) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Aberration speed should not be equal to 0.\n");
        return NULL;
    }
    if (fabs(speed) > 1.0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Aberration speed should not greater than the speed of light!.\n");
        return NULL;
    }

    psCube *rp = psCubeAlloc();
    psCube *r_p = psCubeAlloc();
    double mu = 0.0;
    double mu_p = 0.0;
    double a = 0.0;

    //Convert the spherical input coords direction and actual to cubic coords for computations.
    psCube* directionVector = NULL;
    directionVector = psSphereToCube(direction);
    if (directionVector == NULL) {
        PS_ASSERT_PTR_NON_NULL(directionVector, NULL);
    }
    psCube* actualVector = NULL;
    actualVector = psSphereToCube(actual);
    if (actualVector == NULL) {
        PS_ASSERT_PTR_NON_NULL(actualVector, NULL);
    }

    //Calculate mu as the dot-product of direction and actual (from ADD)
    mu = (directionVector->x*actualVector->x +
          directionVector->y*actualVector->y +
          directionVector->z*actualVector->z);
    //Calculate r-perpendicular (as in sec 3.5.3.2 of ADD). actual = r-hat, direction = beta-hat.
    rp->x = actualVector->x - mu * directionVector->x;
    rp->y = actualVector->y - mu * directionVector->y;
    rp->z = actualVector->z - mu * directionVector->z;
    //Calculate mu-prime.  (Eqn. 129 of ADD).
    mu_p = mu - speed * ((mu * mu - 1.0) / (1.0 - speed * mu));
    //Calculate a-value.  (a = sqrt[ (1 - mu-prime^2) / modulus(r-perpendicular) ] )
    a = (1.0 - mu_p * mu_p) / (rp->x * rp->x + rp->y * rp->y + rp->z * rp->z);
    if (a < 0.0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Invalid parameter.  a-value cannot be negative.\n");
        psFree(rp);
        psFree(r_p);
        psFree(directionVector);
        psFree(actualVector);
        psFree(apparent);
        return NULL;
    } else {
        a = sqrt(a);
    }
    //Calculate r-prime values.  r-prime = mu-prime * beta-hat  +  a * r-perpendicular
    r_p->x = mu_p * directionVector->x + a * rp->x;
    r_p->y = mu_p * directionVector->y + a * rp->y;
    r_p->z = mu_p * directionVector->z + a * rp->z;
    //If apparent is non-NULL, deallocate it for use with psCubeToSphere
    if (apparent != NULL) {
        psFree(apparent);
    }
    //Convert r-prime to spherical coordinates for output.
    apparent = psCubeToSphere(r_p);
    if (apparent == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "psCubeToSphere returned a NULL sphere in psAberration.\n");
        return NULL;
    }
    psFree(rp);
    psFree(r_p);
    psFree(directionVector);
    psFree(actualVector);

    return apparent;
}

psSphere *psGravityDeflection(psSphere *apparent,
                              psSphere *actual,
                              psSphere *sun)
{
    //Check for valid inputs
    PS_ASSERT_PTR_NON_NULL(actual, NULL);
    PS_ASSERT_PTR_NON_NULL(sun, NULL);

    psSphere *temp = psSphereAlloc();
    psCube* sunVector = psSphereToCube(sun);
    psCube* actualVector = psSphereToCube(actual);

    // use dot product to calculate the angle of separation
    double dotProd = (sunVector->x*actualVector->x +
                      sunVector->y*actualVector->y + sunVector->z*actualVector->z);
    double theta, sunMag, actMag;
    sunMag = sqrt(sunVector->x*sunVector->x + sunVector->y*sunVector->y +
                  sunVector->z*sunVector->z);
    actMag = sqrt(actualVector->x*actualVector->x + actualVector->y*actualVector->y +
                  actualVector->z*actualVector->z);
    dotProd = dotProd / (sunMag * actMag);
    theta = acos(dotProd);

    printf(" Theta = %.13g\n", theta);
    double r0 = PS_AU * tan(theta);
    printf(" r0 = %.19e\n", r0);
    double deflection = 4.0*PS_G*PS_M/(PS_C0*PS_C0*r0);

    // make sure the deflection is not greater than 1.75 arcsec
    double limit = SEC_TO_RAD(1.75);
    if (deflection > limit) {
        //if deflection is greater than limit, the light rays will hit the sun
        psWarning("Invalid positions.  Light ray will not be seen on earth.\n");
        psFree(actualVector);
        psFree(sunVector);
        return apparent;
    }


    if (apparent != NULL) {
        psFree(apparent);
    }
    // bend the actual vector away from the sun vector by deflection angle.
    theta = 0.0;
    double phi = 0.0;
    //    deflection = SEC_TO_RAD(deflection) * 1e6;
    //    deflection *= M_PI * 1e-2;
    theta = atan(r0/PS_AU) * tan(deflection);
    //    phi = sqrt( deflection*deflection - theta*theta );
    //    phi = deflection * cos(asin(theta/deflection));

    //    phi = sqrt(theta*theta - deflection*deflection);
    //    phi = deflection * cos(asin(theta/deflection)) * 3e-2;
    //    phi = cos(asin(theta/deflection));
    //    phi = asin(theta/deflection);

    //    apparent->r = theta;
    //    apparent->d = phi;
    /*
        actualVector->x += actualVector->x*deflection;
        actualVector->y += actualVector->y*deflection;
        actualVector->z += actualVector->z*deflection;
        apparent = psCubeToSphere(actualVector);
    */
    theta = tan(sun->r - actual->r) * deflection;
    phi = tan(sun->d - actual->d) * deflection;

    printf(" Theta = %.13g\n", theta);
    printf(" phi = %.13g\n", phi);

    temp->r = theta;
    temp->d = phi;
    apparent = psSphereSetOffset(actual, temp, PS_SPHERICAL, PS_RADIAN);

    psFree(actualVector);
    psFree(sunVector);
    psFree(temp);

    return apparent;
}

psEarthPole *psEOC_PrecessionModel(const psTime *time)
{
    // Check for null parameter
    PS_ASSERT_PTR_NON_NULL(time, NULL);
    if (time->type == PS_TIME_UT1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Invalid time input.  Time cannot be of type UT1.\n");
        return NULL;
    }

    // Convert psTime to MJD
    double MJD = psTimeToMJD(time);
    if (isnan(MJD)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "Time conversion to MJD failed.  Invalid input time.\n");
        return NULL;
    }

    // Calculate number of Julian centuries since 2000
    double t = ( MJD - MJD_2000 ) / JULIAN_CENTURY;
    double t2 = t*t;
    double t3 = t*t*t;
    double t4 = t*t*t*t;

    // The following formulae are from the ADD (& s5.8 of IERS Technical Note 32):
    double F[14];
    // Mean Anomaly of the Moon (l)
    F[0] = DEG_TO_RAD(134.96340251) +
           SEC_TO_RAD(1717915923.2178)*t +
           SEC_TO_RAD(31.8792)*t2 +
           SEC_TO_RAD(0.051635)*t3 -
           SEC_TO_RAD(0.00024470)*t4;
    // Mean Anomaly of the Sun (l')
    F[1] = DEG_TO_RAD(357.52910918) +
           SEC_TO_RAD(129596581.0481)*t -
           SEC_TO_RAD(0.5532)*t2 +
           SEC_TO_RAD(0.000136)*t3 -
           SEC_TO_RAD(0.00001149)*t4;
    // L - Omega    (L = Mean Longitude of the Moon, Omega = F4)
    F[2] = DEG_TO_RAD(93.27209062) +
           SEC_TO_RAD(1739527262.8478)*t -
           SEC_TO_RAD(12.7512)*t2 -
           SEC_TO_RAD(0.001037)*t3 +
           SEC_TO_RAD(0.00000417)*t4;
    // Mean Elongation of the Moon from the Sun (D)
    F[3] = DEG_TO_RAD(297.85019547) +
           SEC_TO_RAD(1602961601.2090)*t -
           SEC_TO_RAD(6.3706)*t2 +
           SEC_TO_RAD(0.006593)*t3 -
           SEC_TO_RAD(0.00003169)*t4;
    // Mean Longitude of the Ascending Node of the Moon (Omega)
    F[4] = DEG_TO_RAD(125.04455501) -
           SEC_TO_RAD(6962890.5431)*t +
           SEC_TO_RAD(7.4722)*t2 +
           SEC_TO_RAD(0.007702)*t3 -
           SEC_TO_RAD(0.0000593)*t4;
    //F5-F13 are the mean longitudes of the planets
    F[5] = 4.402608842 + 2608.7903141574*t;
    F[6] = 3.176146697 + 1021.3285546211*t;
    F[7] = 1.753470314 + 628.3075849991*t;
    F[8] = 6.203480913 + 334.0612426700*t;
    F[9] = 0.599546497 + 52.9690962641*t;
    F[10] = 0.874016757 + 21.3299104960*t;
    F[11] = 5.481293872 + 7.4781598567*t;
    F[12] = 5.311886287 + 3.8133035638*t;
    F[13] = 0.024381750*t + 0.00000538691*t2;

    // Check if EOC data loaded
    if(! eocInitialized) {
        eocInitialized = p_psEOCInit();
        if(!eocInitialized) {
            psError(PS_ERR_UNKNOWN, false,
                    "Could not initialize EOC tables -- check data files.");
            return NULL;
        }
    }
    // calculate the polynomial portion first
    double X = psPolynomial1DEval(xPoly,t);
    double Y = psPolynomial1DEval(yPoly,t);
    double S = psPolynomial1DEval(sPoly,t);
    //Units from the table & coefficients are in micro-arcseconds so convert to radians.
    X = SEC_TO_RAD(X * 1e-6);
    Y = SEC_TO_RAD(Y * 1e-6);
    S = SEC_TO_RAD(S * 1e-6);

    // now calculate the non-poly portion from the tables
    psF64* cols[17];  //Used to store all of the table information from tab5.2?.dat
    for (int lcv = 0; lcv < 17; lcv++) {
        cols[lcv] = ((psVector*)(xTable->data[lcv]))->data.F64;
    }
    //Get the number of rows in the table and loop through the non-poly contributions.
    int numRows = ((psVector*)(xTable->data[0]))->n;
    for (int lcv = 0; lcv < numRows; lcv++) {
        double arg = 0.0;
        //Get the argument- from the table and corresponding F-value.  Convert to radians.
        for (int k = 0; k < 14; k++) {
            arg += cols[k+3][lcv]*F[k];
        }
        //The order of t for each part is specified by a column in the tab.dat files.
        double tj = pow(t,cols[0][lcv]);
        double as = cols[1][lcv];
        double ac = cols[2][lcv];
        as = SEC_TO_RAD(as) * 1e-6;
        ac = SEC_TO_RAD(ac) * 1e-6;
        X += (as*tj*sin(arg) + ac*cos(arg)) * tj;
    }
    //Do the same procedure from the previous 15 lines for Y.
    for (int lcv = 0; lcv < 17; lcv++) {
        cols[lcv] = ((psVector*)(yTable->data[lcv]))->data.F64;
    }
    numRows = ((psVector*)(yTable->data[0]))->n;
    for (int lcv = 0; lcv < numRows; lcv++) {
        double arg = 0.0;
        for (int k = 0; k < 14; k++) {
            arg += cols[k+3][lcv]*F[k];
        }
        double tj = pow(t,cols[0][lcv]);
        double as = cols[1][lcv];
        double ac = cols[2][lcv];
        as = SEC_TO_RAD(as) * 1e-6;
        ac = SEC_TO_RAD(ac) * 1e-6;
        Y += (as*tj*sin(arg) + ac*cos(arg)) * tj;
    }
    //Again for S.
    for (int lcv = 0; lcv < 17; lcv++) {
        cols[lcv] = ((psVector*)(sTable->data[lcv]))->data.F64;
    }
    numRows = ((psVector*)(sTable->data[0]))->n;
    for (int lcv = 0; lcv < numRows; lcv++) {
        double arg = 0.0;
        for (int k = 0; k < 14; k++) {
            arg += cols[k+3][lcv]*F[k];
        }
        double tj = pow(t,cols[0][lcv]);
        double as = cols[1][lcv];
        double ac = cols[2][lcv];
        as = SEC_TO_RAD(as) * 1e-6;
        ac = SEC_TO_RAD(ac) * 1e-6;
        S += (as*tj*sin(arg) + ac*cos(arg)) * tj;
    }

    //the tables for S actually gives S + XY/2, so let's get the real S now. (from ADD)
    S -= X*Y/2.0;

    //Create the output psEarthPole and set the corresponding component values.
    psEarthPole* pole = psEarthPoleAlloc();
    pole->x = X;
    pole->y = Y;
    pole->s = S;

    return pole;
}

psEarthPole *psEOC_PrecessionCorr(const psTime *time,
                                  psTimeBulletin bulletin)
{
    // Check for null parameter or invalid bulletin
    PS_ASSERT_PTR_NON_NULL(time,NULL);
    PS_ASSERT_INT_WITHIN_RANGE(bulletin, PS_IERS_A, PS_IERS_B, NULL);
    //Convert the input time to MJD.  If NAN is returned, return NULL for the function.
    double MJD = psTimeToMJD(time);
    if (isnan(MJD)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "Time conversion to MJD failed.  Invalid input time.\n");
        return NULL;
    }
    //Allocate space for the psEarthPole output.
    psEarthPole *out = psEarthPoleAlloc();

    // Check if EOC data loaded
    if(!eocInitialized) {
        eocInitialized = p_psEOCInit();
        if(!eocInitialized) {
            psError(PS_ERR_UNKNOWN, false,
                    "Could not initialize EOC tables -- check data files.");
            return NULL;
        }
    }

    //Load the data table and store in the corresponding, newly-allocated vectors.
    psF64* cols[5];
    for (int colNum = 0; colNum < 5; colNum++) {
        cols[colNum] = ((psVector*)(iersTable->data[colNum]))->data.F64;
    }
    int numRows = ((psVector*)(iersTable->data[0]))->n;
    psVector *X = psVectorAlloc(numRows, PS_TYPE_F64);
    psVector *Y = psVectorAlloc(numRows, PS_TYPE_F64);
    psVector *T = psVectorAlloc(numRows, PS_TYPE_F64);
    if (bulletin == PS_IERS_A) {
        for (int rowNum = 0; rowNum < numRows; rowNum++) {
            T->data.F64[rowNum] = cols[0][rowNum];
            X->data.F64[rowNum] = cols[1][rowNum];
            Y->data.F64[rowNum] = cols[2][rowNum];
        }
    } else {
        for (int rowNum = 0; rowNum < numRows; rowNum++) {
            T->data.F64[rowNum] = cols[0][rowNum];
            X->data.F64[rowNum] = cols[3][rowNum];
            Y->data.F64[rowNum] = cols[4][rowNum];
        }
    }

    //The following uses lagrange interpolation to calculate the corrections to X & Y.
    double xOut = 0.0;
    double yOut = 0.0;
    double xTerm = 0.0;
    double yTerm = 0.0;
    int k = 0;
    for (int i = 0; i < (numRows-1); i++) {
        if (MJD >= T->data.F64[i] && MJD < T->data.F64[i+1]) {
            k = i;
            if (k < 2) {
                k = 2;
            }
            if (k > (numRows-2)) {
                k = numRows-2;
            }
            for (int m = k-1; m <= k+2; m++) {
                xTerm = X->data.F64[m];
                yTerm = Y->data.F64[m];
                for (int j = k-1; j <= k+2; j++) {
                    if ( m != j) {
                        double term = (MJD - T->data.F64[j])/(T->data.F64[m] - T->data.F64[j]);
                        xTerm *= term;
                        yTerm *= term;
                    }
                }
                xOut += xTerm;
                yOut += yTerm;
            }
            i = numRows-1;
        }
    }
    //Convert the values from milli-arcsecond to radian.
    out->x = SEC_TO_RAD(xOut) * 1e-3;
    out->y = SEC_TO_RAD(yOut) * 1e-3;
    psFree(X);
    psFree(Y);
    psFree(T);

    return out;
}

static psSphereRot *rotMatrix_To_Quat(double A[3][3])
{
    int i;
    psSphereRot *new = psSphereRotAlloc(0.0, 0.0, 0.0);
    new->q3 = 0.0;

    //Convert rotation matrix to quaternions.  Formula directly from ADD.
    double diag_sum[4];
    int maxi;
    double recip;
    diag_sum[0] = 1.0 + A[0][0] - A[1][1] - A[2][2];
    diag_sum[1] = 1.0 - A[0][0] + A[1][1] - A[2][2];
    diag_sum[2] = 1.0 - A[0][0] - A[1][1] + A[2][2];
    diag_sum[3] = 1.0 + A[0][0] + A[1][1] + A[2][2];

    maxi = 0;
    for (i = 1; i < 4; ++i) {
        if (diag_sum[i] > diag_sum[maxi]) {
            maxi = i;
        }
    }

    double p = 0.5 * sqrt(diag_sum[maxi]);
    recip = 1.0 / (4.0 * p);

    if (maxi == 0) {
        new->q0 = p;
        new->q1 = recip * (A[0][1] + A[1][0]);
        new->q2 = recip * (A[2][0] + A[0][2]);
        new->q3 = recip * (A[1][2] - A[2][1]);
    } else if (maxi == 1) {
        new->q0 = recip * (A[0][1] + A[1][0]);
        new->q1 = p;
        new->q2 = recip * (A[1][2] + A[2][1]);
        new->q3 = recip * (A[2][0] - A[0][2]);
    } else if (maxi == 2) {
        new->q0 = recip * (A[2][0] + A[0][2]);
        new->q1 = recip * (A[1][2] + A[2][1]);
        new->q2 = p;
        new->q3 = recip * (A[0][1] - A[1][0]);
    } else if (maxi == 3) {
        new->q0 = recip * (A[1][2] - A[2][1]);
        new->q1 = recip * (A[2][0] - A[0][2]);
        new->q2 = recip * (A[0][1] - A[1][0]);
        new->q3 = p;
    }
    return new;
}

psSphereRot* psSphereRot_CEOtoGCRS(const psEarthPole *pole)
{
    // Check for null parameter
    PS_ASSERT_PTR_NON_NULL(pole,NULL);
    double A[3][3];
    psSphereRot *out = NULL;

    //Setup the rotation matrix and scalar value, a, as outlined by the ADD
    double a =  1.0 / (1.0 + sqrt(1.0 - (pole->x*pole->x + pole->y*pole->y) ) );
    A[0][0] = (1.0 - a*pole->x*pole->x)*cos(pole->s) - a*pole->x*pole->y*sin(pole->s);
    A[1][0] = -a*pole->x*pole->y*cos(pole->s) + (1.0 - a*pole->y*pole->y)*sin(pole->s);
    A[2][0] = pole->x*cos(pole->s) + pole->y*sin(pole->s);
    A[0][1] = -(1.0 - a*pole->x*pole->x)*sin(pole->s) - a*pole->x*pole->y*cos(pole->s);
    A[1][1] = a*pole->x*pole->y*sin(pole->s) + (1.0 - a*pole->y*pole->y)*cos(pole->s);
    A[2][1] = -pole->x*sin(pole->s) + pole->y*cos(pole->s);
    A[0][2] = -pole->x;
    A[1][2] = -pole->y;
    A[2][2] = 1.0 - a*(pole->x*pole->x + pole->y*pole->y);

    //Convert the rotation matrix to a psSphereRot (quaternions)
    out = rotMatrix_To_Quat(A);

    return out;
}

psSphereRot* psSphereRot_TEOtoCEO(const psTime *time,
                                  psEarthPole *tidalCorr)
{
    // Check for null parameter
    PS_ASSERT_PTR_NON_NULL(time,NULL);
    //Create a copy of the input time that can be manipulated/changed (input time is const).
    psTime *in = psTimeCopy(time);
    if (in->type != PS_TIME_UT1) {
        psTimeConvert(in, PS_TIME_UT1);
    }
    //Check if tidal corrections should be included.
    //If so, make sure values are positive and in the correct range.
    if (tidalCorr != NULL && fabs(tidalCorr->s) > FLT_EPSILON) {
        int nsec = in->nsec + (int)(tidalCorr->s * 1e9);
        if (nsec > 1e9) {
            nsec += -1e9;
            in->sec += 1;
        }
        if (nsec < 0) {
            in->sec += -1;
            in->nsec = (int)(1e9) + nsec;
        } else {
            in->nsec = nsec;
        }
    }
    //Calculate the Julian Date from the input time in UT1 format.
    double T = psTimeToJD(in);
    if (isnan(T)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "Time conversion to JD failed.  Invalid input time.\n");
        return NULL;
    }
    T += -2451545.0;
    //Formula for theta comes directly from the ADD.  Create output psSphereRot from theta.
    double theta = 2.0 * M_PI * (0.7790572732640 + 1.00273781191135448 * T);
    psSphereRot *out = psSphereRotAlloc(theta, 0.0, 0.0);

    psFree(in);
    return out;
}

psEarthPole* psEOC_GetPolarMotion(const psTime *time,
                                  psTimeBulletin bulletin)
{
    PS_ASSERT_PTR_NON_NULL(time, NULL);
    PS_ASSERT_INT_WITHIN_RANGE(bulletin, PS_IERS_A, PS_IERS_B, NULL);

    psEarthPole *out = psEarthPoleAlloc();
    out->x = 0.0;
    out->y = 0.0;
    out->s = 0.0;

    double MJD = psTimeToMJD(time);
    if ( isnan(MJD) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "Time conversion to MJD failed.  Invalid input time.\n");
        return NULL;
    }

    // Check if EOC data loaded
    if(! eocInitialized) {
        eocInitialized = p_psEOCInit();
        if(!eocInitialized) {
            // XXX: Move error message.
            psError(PS_ERR_UNKNOWN, false,
                    "Could not initialize EOC tables -- check data files.");
            return NULL;
        }
    }

    psF64* cols[7];
    for (int colNum = 0; colNum < 7; colNum++) {
        cols[colNum] = ((psVector*)(finalsTable->data[colNum]))->data.F64;
    }
    int numRows = ((psVector*)(finalsTable->data[0]))->n;
    psVector *X = psVectorAlloc(numRows, PS_TYPE_F64);
    psVector *Y = psVectorAlloc(numRows, PS_TYPE_F64);
    //    psVector *S = psVectorAlloc(numRows, PS_TYPE_F64);
    psVector *T = psVectorAlloc(numRows, PS_TYPE_F64);
    if (bulletin == PS_IERS_A) {
        for (int rowNum = 0; rowNum < numRows; rowNum++) {
            T->data.F64[rowNum] = cols[0][rowNum];
            X->data.F64[rowNum] = cols[1][rowNum];
            Y->data.F64[rowNum] = cols[2][rowNum];
            //            S->data.F64[rowNum] = cols[3][rowNum];
        }
    } else {
        for (int rowNum = 0; rowNum < numRows; rowNum++) {
            T->data.F64[rowNum] = cols[0][rowNum];
            X->data.F64[rowNum] = cols[4][rowNum];
            Y->data.F64[rowNum] = cols[5][rowNum];
            //            S->data.F64[rowNum] = cols[6][rowNum];
        }
    }

    double xOut = 0.0;
    double yOut = 0.0;
    //    double sOut = 0.0;
    double xTerm = 0.0;
    double yTerm = 0.0;
    //    double sTerm = 0.0;
    int k = 0;
    for (int i = 0; i < (numRows-1); i++) {
        if (MJD >= T->data.F64[i] && MJD < T->data.F64[i+1]) {
            k = i;
            if (k < 2) {
                k = 2;
            }
            //            if (k > (numRows-2)) {
            //                k = numRows-2;
            //            }
            for (int m = k-1; m <= k+2; m++) {
                xTerm = X->data.F64[m];
                yTerm = Y->data.F64[m];
                //                sTerm = S->data.F64[m];
                for (int j = k-1; j <= k+2; j++) {
                    if ( m != j) {
                        double term = (MJD - T->data.F64[j])/(T->data.F64[m] - T->data.F64[j]);
                        xTerm *= term;
                        yTerm *= term;
                        //                        sTerm *= term;
                    }
                }
                xOut += xTerm;
                yOut += yTerm;
                //                sOut += sTerm;
            }
            i = numRows-1;
        }
    }
    out->x = SEC_TO_RAD(xOut);
    out->y = SEC_TO_RAD(yOut);
    //    psEarthPole *polarTideCorr = psEOC_PolarTideCorr(time);
    //    out->x += polarTideCorr->x;
    //    out->y += polarTideCorr->y;
    //    psFree(polarTideCorr);

    //    out->s = SEC_TO_RAD(sOut);


    /*
        for (int rowNum = 0; rowNum < numRows; rowNum++) {
            if ( (MJD - cols[0][rowNum]) < 1.0 ) {
                if (bulletin == PS_IERS_A) {
                    out->x = cols[1][rowNum];
                    out->y = cols[2][rowNum];
                    out->s = cols[3][rowNum];
                    out->x = SEC_TO_RAD(out->x);
                    out->y = SEC_TO_RAD(out->y);
                    out->s = SEC_TO_RAD(out->s);
                    rowNum = numRows;
                } else {
                    out->x = cols[4][rowNum];
                    out->y = cols[5][rowNum];
                    out->s = cols[6][rowNum];
                    out->x = SEC_TO_RAD(out->x);
                    out->y = SEC_TO_RAD(out->y);
                    out->s = SEC_TO_RAD(out->s);
                    rowNum = numRows;
                }
            }
        }
    */
    psFree(X);
    psFree(Y);
    //    psFree(S);
    psFree(T);
    return out;
}

static double DMOD(double x, double y)
{
    //Internal function for calculating the remainder of a double quotient.
    //used often in the algorithm for psEOC_PolarTideCorr which is the Ray model of
    //Simon et. al. from its fortran implementation.
    double value = x - y * trunc(x/y);
    return value;
}

psEarthPole* psEOC_PolarTideCorr(const psTime *time)
{
    // Check for null parameter
    PS_ASSERT_PTR_NON_NULL(time, NULL);
    // Convert psTime to MJD
    double MJD = psTimeToMJD(time);
    if (isnan(MJD)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "Time conversion to MJD failed.  Invalid input time.\n");
        return NULL;
    }
    psEarthPole *out = psEarthPoleAlloc();

    //Formula comes from fortran reference of the Ray model of Simon et. al.
    double T, L, LPRIME, CAPF, CAPD, OMEGA, THETA, CORX, CORY, CORZ;
    double ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8;
    double T2, T3, T4;
    // Calculate number of Julian centuries since 2000
    T = (MJD - 51544.5) / 36525.0;
    T2 = T*T;
    T3 = T*T*T;
    T4 = T*T*T*T;
    L = -0.0002447 * T4 + 0.051635 * T3 + 31.8792 * T2 + 1717915923.2178 * T + 485868.249036;
    L = DMOD(L, 1296000.0);
    LPRIME = -0.00001149 * T4 - 0.000136 * T3 - 0.5532 * T2 + 129596581.0481 * T + 1287104.79305;
    LPRIME = DMOD(LPRIME, 1296000.0);
    CAPF = 0.00000417 * T4 - 0.001037 * T3 - 12.7512 * T2 + 1739527262.8478 * T + 335779.526232;
    CAPF = DMOD(CAPF, 1296000.0);
    CAPD = -0.00003169 * T4 + 0.006593 * T3 - 6.3706 * T2 + 1602961601.209 * T + 1072260.70369;
    CAPD = DMOD(CAPD, 1296000.0);
    OMEGA = -0.00005939 * T4 + 0.007702 * T3 + 7.4722 * T2 - 6962890.2665 * T + 450160.398036;
    OMEGA = DMOD(OMEGA, 1296000.0);
    THETA = (67310.54841 + (876600.0 * 3600.0 + 8640184.812866) * T + 0.093104 * T2 -
             6.2e-6 * T3) * 15.0 + 648000.0;
    ARG7 = DMOD((-L - 2.0 * CAPF - 2.0 * OMEGA + THETA) * M_PI / 648000.0, 2.0 * M_PI)
           - M_PI / 2.0;
    ARG1 = DMOD((-2.0 * CAPF - 2.0 * OMEGA + THETA) * M_PI / 648000.0, 2.0 * M_PI) - M_PI / 2.0;
    ARG2 = DMOD((-2.0 * CAPF + 2.0 * CAPD - 2.0 * OMEGA + THETA) * M_PI / 648000.0, 2.0 * M_PI)
           - M_PI / 2.0;
    ARG3 = DMOD(THETA * M_PI / 648000.0, 2.0 * M_PI) - M_PI / 2.0;
    ARG4 = DMOD((-L - 2.0 * CAPF - 2.0 * OMEGA + 2.0 * THETA) * M_PI / 648000.0, 2.0 * M_PI);
    ARG5 = DMOD((-2.0 * CAPF - 2.0 * OMEGA + 2.0 * THETA) * M_PI / 648000.0, 2.0 * M_PI);
    ARG6 = DMOD((-2.0 * CAPF + 2.0 * CAPD - 2.0 * OMEGA + 2.0 * THETA) * M_PI / 648000.0,
                2.0 * M_PI);
    ARG8 = DMOD((2.0 * THETA) * M_PI / 648000.0, 2.0 * M_PI);
    CORX = -0.026 * sin(ARG7) + 0.006 * cos(ARG7)
           -0.133 * sin(ARG1) + 0.049 * cos(ARG1)
           -0.050 * sin(ARG2) + 0.025 * cos(ARG2)
           -0.152 * sin(ARG3) + 0.078 * cos(ARG3)
           -0.057 * sin(ARG4) - 0.013 * cos(ARG4)
           -0.330 * sin(ARG5) - 0.028 * cos(ARG5)
           -0.145 * sin(ARG6) + 0.064 * cos(ARG6)
           -0.036 * sin(ARG8) + 0.017 * cos(ARG8);
    CORY = -0.006 * sin(ARG7) - 0.026 * cos(ARG7)
           -0.049 * sin(ARG1) - 0.133 * cos(ARG1)
           -0.025 * sin(ARG2) - 0.050 * cos(ARG2)
           -0.078 * sin(ARG3) - 0.152 * cos(ARG3)
           +0.011 * sin(ARG4) + 0.033 * cos(ARG4)
           +0.037 * sin(ARG5) + 0.196 * cos(ARG5)
           +0.059 * sin(ARG6) + 0.087 * cos(ARG6)
           +0.018 * sin(ARG8) + 0.022 * cos(ARG8);
    CORZ =  0.0245 * sin(ARG7) + 0.0503 * cos(ARG7)
            +0.1210 * sin(ARG1) + 0.1605 * cos(ARG1)
            +0.0286 * sin(ARG2) + 0.0516 * cos(ARG2)
            +0.0864 * sin(ARG3) + 0.1771 * cos(ARG3)
            -0.0380 * sin(ARG4) - 0.0154 * cos(ARG4)
            -0.1617 * sin(ARG5) - 0.0720 * cos(ARG5)
            -0.0759 * sin(ARG6) - 0.0004 * cos(ARG6)
            -0.0196 * sin(ARG8) - 0.0038 * cos(ARG8);
    CORX = CORX * 1.0e-3;
    CORY = CORY * 1.0e-3;
    CORZ = CORZ * 0.1e-3;

    CORX = SEC_TO_RAD(CORX);
    CORY = SEC_TO_RAD(CORY);

    out->x = CORX;
    out->y = CORY;
    out->s = CORZ;

    return out;
}

psEarthPole* psEOC_NutationCorr(psTime *time)
{
    // Check for null parameter
    PS_ASSERT_PTR_NON_NULL(time, NULL);
    // Convert psTime to MJD
    double MJD = psTimeToMJD(time);
    //    printf("\nMJD check = %.13g\n", MJD);
    if (isnan(MJD)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "Time conversion to MJD failed.  Invalid input time.\n");
        return NULL;
    }

    // Calculate number of Julian centuries since 2000
    double t = ( MJD - MJD_2000 ) / JULIAN_CENTURY;
    double t2 = t*t;
    double t3 = t*t*t;
    double t4 = t*t*t*t;

    // The following formulae are from the ADD (& s5.8 of IERS Technical Note 32):
    double F[5];
    // Mean Anomaly of the Moon
    F[0] = DEG_TO_RAD(134.96340251) +
           SEC_TO_RAD(1717915923.2178)*t +
           SEC_TO_RAD(31.8792)*t2 +
           SEC_TO_RAD(0.051635)*t3 -
           SEC_TO_RAD(0.00024470)*t4;

    // Mean Anomaly of the Sun
    F[1] = DEG_TO_RAD(357.52910918) +
           SEC_TO_RAD(129596581.0481)*t -
           SEC_TO_RAD(0.5532)*t2 +
           SEC_TO_RAD(0.000136)*t3 -
           SEC_TO_RAD(0.00001149)*t4;

    // L Å‚àí Omega    (L = Mean Longitude of the Moon, Omega = F4)
    F[2] = DEG_TO_RAD(93.27209062) +
           SEC_TO_RAD(1739527262.8478)*t -
           SEC_TO_RAD(12.7512)*t2 -
           SEC_TO_RAD(0.001037)*t3 +
           SEC_TO_RAD(0.00000417)*t4;

    // Mean Elongation of the Moon from the Sun
    F[3] = DEG_TO_RAD(297.85019547) +
           SEC_TO_RAD(1602961601.2090)*t -
           SEC_TO_RAD(6.3706)*t2 +
           SEC_TO_RAD(0.006593)*t3 -
           SEC_TO_RAD(0.00003169)*t4;

    // Mean Longitude of the Ascending Node of the Moon
    F[4] = DEG_TO_RAD(125.04455501) -
           SEC_TO_RAD(6962890.5431)*t +
           SEC_TO_RAD(7.4722)*t2 +
           SEC_TO_RAD(0.007702)*t3 -
           SEC_TO_RAD(0.00005939)*t4;

    //argument values taken from table 5.1 in IERS techical note No.32
    //http://maia.usno.navy.mil/conv2000/chapter5/tn32_c5.pdf, p38
    //Units are in micro-arcseconds here and must be converted to radians before using
    double w_l[10] = {SEC_TO_RAD(-1.0),
                      SEC_TO_RAD(-1.0),
                      SEC_TO_RAD(1.0),
                      0.0,
                      0.0,
                      SEC_TO_RAD(-1.0),
                      0.0,
                      0.0,
                      0.0,
                      SEC_TO_RAD(1.0)};
    double w_l_p[10] = {0.0,
                        0.0,
                        0.0,
                        0.0,
                        0.0,
                        0.0,
                        0.0,
                        0.0,
                        0.0,
                        0.0};
    double w_F[10] = {SEC_TO_RAD(-2.0),
                      SEC_TO_RAD(-2.0),
                      SEC_TO_RAD(-2.0),
                      SEC_TO_RAD(-2.0),
                      SEC_TO_RAD(-2.0),
                      0.0,
                      SEC_TO_RAD(-2.0),
                      0.0,
                      0.0,
                      0.0};
    double w_D[10] = {0.0,
                      0.0,
                      SEC_TO_RAD(-2.0),
                      0.0,
                      0.0,
                      0.0,
                      SEC_TO_RAD(2.0),
                      0.0,
                      0.0,
                      0.0};
    double w_Omega[10] = {SEC_TO_RAD(-1.0),
                          SEC_TO_RAD(-2.0),
                          SEC_TO_RAD(-2.0),
                          SEC_TO_RAD(-1.0),
                          SEC_TO_RAD(-2.0),
                          0.0,
                          SEC_TO_RAD(-2.0),
                          0.0,
                          SEC_TO_RAD(-1.0),
                          0.0};
    double xp_sin[10] = {SEC_TO_RAD(-0.44),
                         SEC_TO_RAD(-2.31),
                         SEC_TO_RAD(-0.44),
                         SEC_TO_RAD(-2.14),
                         SEC_TO_RAD(-11.36),
                         SEC_TO_RAD(0.84),
                         SEC_TO_RAD(-4.76),
                         SEC_TO_RAD(14.27),
                         SEC_TO_RAD(1.93),
                         SEC_TO_RAD(0.76)};
    double xp_cos[10] = {SEC_TO_RAD(0.25),
                         SEC_TO_RAD(1.32),
                         SEC_TO_RAD(0.25),
                         SEC_TO_RAD(1.23),
                         SEC_TO_RAD(6.52),
                         SEC_TO_RAD(-0.48),
                         SEC_TO_RAD(2.73),
                         SEC_TO_RAD(-8.19),
                         SEC_TO_RAD(-1.11),
                         SEC_TO_RAD(-0.43)};
    double yp_sin[10] = {SEC_TO_RAD(-0.25),
                         SEC_TO_RAD(-1.32),
                         SEC_TO_RAD(-0.25),
                         SEC_TO_RAD(-1.23),
                         SEC_TO_RAD(-6.52),
                         SEC_TO_RAD(0.48),
                         SEC_TO_RAD(-2.73),
                         SEC_TO_RAD(8.19),
                         SEC_TO_RAD(1.11),
                         SEC_TO_RAD(0.43)};
    double yp_cos[10] = {SEC_TO_RAD(-0.44),
                         SEC_TO_RAD(-2.31),
                         SEC_TO_RAD(-0.44),
                         SEC_TO_RAD(-2.14),
                         SEC_TO_RAD(-11.36),
                         SEC_TO_RAD(0.84),
                         SEC_TO_RAD(-4.76),
                         SEC_TO_RAD(14.27),
                         SEC_TO_RAD(1.93),
                         SEC_TO_RAD(0.76)};

    double X = 0.0;
    double Y = 0.0;

    //Implementation adapted from PM_GRAVI in interp.f from hpiers.obspm.fr/eop-pc/models/interp.f
    double arg[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    arg[0] = (67310.54841 +
              (876600.0*3600.0 + 8640184.812866)*t
              + 0.093104*t2 - 6.2e-6*t3) * 15.0 + 648000.0;
    arg[0] = DMOD(arg[1], 1296000.0);
    arg[0] = SEC_TO_RAD(arg[0]);
    arg[1] = RAD_TO_SEC(F[0]);
    arg[1] = DMOD(arg[1], 1296000.0);
    arg[1] = SEC_TO_RAD(arg[1]);
    arg[2] = RAD_TO_SEC(F[1]);
    arg[2] = DMOD(arg[2], 1296000.0);
    arg[2] = SEC_TO_RAD(arg[2]);
    arg[3] = RAD_TO_SEC(F[2]);
    arg[3] = DMOD(arg[3], 1296000.0);
    arg[3] = SEC_TO_RAD(arg[3]);
    arg[4] = RAD_TO_SEC(F[3]);
    arg[4] = DMOD(arg[4], 1296000.0);
    arg[4] = SEC_TO_RAD(arg[4]);
    arg[5] = RAD_TO_SEC(F[4]);
    arg[5] = DMOD(arg[5], 1296000.0);
    arg[5] = SEC_TO_RAD(arg[5]);

    for (int j = 0; j < 10; j++) {
        double ag = 0.0;
        ag = SEC_TO_RAD(1.0)*arg[0] + w_l[j]*arg[1] + w_l_p[j]*arg[2] + w_F[j]*arg[3]
             + w_D[j]*arg[4] + w_Omega[j]*arg[5];
        ag = RAD_TO_SEC(ag);
        ag = DMOD(ag, 2.0*M_PI);
        X += xp_sin[j] * SEC_TO_RAD(sin(ag)) + xp_cos[j] * SEC_TO_RAD(cos(ag));
        Y += yp_sin[j] * SEC_TO_RAD(sin(ag)) + yp_cos[j] * SEC_TO_RAD(cos(ag));
    }

    psEarthPole *pole = psEarthPoleAlloc();
    pole->x = X;
    pole->y = Y;
    //The value of s is simply: s = -4.7e-5 * t as specified by the ADD and IERS 32.
    pole->s = -SEC_TO_RAD(4.7e-5) * t;

    return pole;
}

psSphereRot* psSphereRot_ITRStoTEO(const psEarthPole* motion)
{
    // Check for null parameter
    PS_ASSERT_PTR_NON_NULL(motion,NULL);
    double A[3][3];
    psSphereRot *out = NULL;

    double x,y,s;
    x = motion->x;
    y = motion->y;
    s = motion->s;
    s = -s;

    //Setup Rotation matrix.  Rotation is constructed by rotation about the X-axis by y,
    // about the Y-axis by x, and about the Z-axis by s' (where s' = -s).
    A[0][0] = cos(x)*cos(s);
    A[1][0] = cos(x)*sin(s);
    A[2][0] = -sin(x);
    A[0][1] = sin(x)*sin(y)*cos(s) - cos(y)*sin(s);
    A[1][1] = sin(x)*sin(y)*sin(s) + cos(y)*cos(s);
    A[2][1] = cos(x)*sin(y);
    A[0][2] = sin(x)*cos(y)*cos(s) + sin(y)*sin(s);
    A[1][2] = sin(x)*cos(y)*sin(s) - sin(y)*cos(s);
    A[2][2] = cos(x)*cos(y);

    //Convert rotation matrix to quaternions
    out = rotMatrix_To_Quat(A);

    return out;
}

psSphereRot *psSpherePrecess(const psTime *fromTime,
                             const psTime *toTime,
                             psPrecessMethod mode)
{
    // Check input for NULL pointers
    if (fromTime == NULL && toTime == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "Invalid time inputs.  fromTime & toTime cannot both be NULL.\n");
        return NULL;
    }
    PS_ASSERT_INT_WITHIN_RANGE(mode, PS_PRECESS_ROUGH, PS_PRECESS_IAU2000A, NULL);
    psF64 fromMJD, toMJD;
    // Calculate Julian centuries
    //If either input time is NULL, assume it to be J2000 -> from SDRS as of rev 18
    psTime *from = NULL;
    psTime *to = NULL;
    if (fromTime == NULL) {
        fromMJD = MJD_2000;
        from = psTimeFromMJD(fromMJD);
    } else {
        fromMJD = psTimeToMJD(fromTime);
        from = psTimeCopy(fromTime);
    }
    if (toTime == NULL) {
        toMJD = MJD_2000;
        to = psTimeFromMJD(toMJD);
    } else {
        toMJD = psTimeToMJD(toTime);
        to = psTimeCopy(toTime);
    }
    if (fromMJD > toMJD) {
        psWarning("From time is later than to time in psSpherePrecess.\n");
    }

    if (mode == PS_PRECESS_ROUGH) {
        //For PS_PRECESS_ROUGH, no time/earthpole corrections are used.  This is the
        //lowest level of detail mode.
        psF64 T = (toMJD - fromMJD) / JULIAN_CENTURY;

        // Calculate conversion constants
        //    psF64 alphaP = DEG_TO_RAD(90.0) - ((DEG_TO_RAD(0.6406161) * T) +
        psF64 alphaP = DEG_TO_RAD(180.0) + ((DEG_TO_RAD(0.6406161) * T) +
                                            (DEG_TO_RAD(0.0000839) * T * T) +
                                            (DEG_TO_RAD(0.000005) * T * T * T));

        psF64 deltaP = (DEG_TO_RAD(0.5567530) * T) -
                       (DEG_TO_RAD(0.0001185) * T * T) -
                       (DEG_TO_RAD(0.0000116) * T * T * T);

        //    psF64 phiP = DEG_TO_RAD(90.0) + ((DEG_TO_RAD(0.6406161) * T) +
        psF64 phiP = DEG_TO_RAD(180.0) + ((DEG_TO_RAD(0.6406161) * T) +
                                          (DEG_TO_RAD(0.0003041) * T * T) +
                                          (DEG_TO_RAD(0.0000051) * T * T * T));

        // Create transform with proper constants
        psSphereRot* tmpST = psSphereRotAlloc(alphaP, deltaP, phiP);
        psFree(from);
        psFree(to);
        return tmpST;
    } else if (mode == PS_PRECESS_IAU2000A) {
        //For IAU2000A mode, run psEOC_PrecessionModel and then psSphereRot_CEOtoGCRS for
        //each time.  Then difference the resulting rotations by adding the inverse
        //rotation corresponding to fromTime to the toTime rotation.

        //Calculate the earthpoles and quaternions corresponding to each time (from, to).
        //Combine the quaternions to produce the output psSphereRot.
        psEarthPole *fromEP = psEOC_PrecessionModel(from);
        psSphereRot *fromRot = psSphereRot_CEOtoGCRS(fromEP);
        psEarthPole *toEP = psEOC_PrecessionModel(to);
        psSphereRot *toRot = psSphereRot_CEOtoGCRS(toEP);
        psSphereRot *fromConj = psSphereRotConjugate(NULL, fromRot);
        psSphereRot *out = psSphereRotCombine(NULL, toRot, fromConj);
        psFree(from);
        psFree(to);
        psFree(fromEP);
        psFree(fromRot);
        psFree(toEP);
        psFree(toRot);
        psFree(fromConj);
        return out;
    } else if (mode == PS_PRECESS_COMPLETE_A) {
        //For PS_PRECESS_COMPLETE_A the same procedure as IAU2000A is used but with
        //additional earthpole corrections from psEOC_PrecessionCorr.  The corrections
        //for COMPLETE_A come from the IERS Bulletin A.

        //Calculate the earthpoles and quaternions corresponding to each time (from, to).
        //Add in the precession corrections from IERS bulletin A.
        //Combine the quaternions to produce the output psSphereRot.
        psEarthPole *fromEP = psEOC_PrecessionModel(from);
        psEarthPole *fromCorr = psEOC_PrecessionCorr(from, PS_IERS_A);
        fromEP->x += fromCorr->x;
        fromEP->y += fromCorr->y;
        psSphereRot *fromRot = psSphereRot_CEOtoGCRS(fromEP);
        psEarthPole *toEP = psEOC_PrecessionModel(to);
        psEarthPole *toCorr = psEOC_PrecessionCorr(to, PS_IERS_A);
        toEP->x += toCorr->x;
        toEP->y += toCorr->y;
        psSphereRot *toRot = psSphereRot_CEOtoGCRS(toEP);
        psSphereRot *fromConj = psSphereRotConjugate(NULL, fromRot);
        psSphereRot *out = psSphereRotCombine(NULL, toRot, fromConj);
        psFree(from);
        psFree(to);
        psFree(fromEP);
        psFree(fromCorr);
        psFree(fromRot);
        psFree(toEP);
        psFree(toCorr);
        psFree(toRot);
        psFree(fromConj);
        return out;
    } else {  //mode == PS_PRECESS_COMPLETE_B
        //For PS_PRECESS_COMPLETE_B the same procedure as IAU2000A is used but with
        //additional earthpole corrections from psEOC_PrecessionCorr.  The corrections
        //for COMPLETE_B come from the IERS Bulletin B.

        //Calculate the earthpoles and quaternions corresponding to each time (from, to).
        //Add in the precession corrections from IERS bulletin B.
        //Combine the quaternions to produce the output psSphereRot.
        psEarthPole *fromEP = psEOC_PrecessionModel(from);
        psEarthPole *fromCorr = psEOC_PrecessionCorr(from, PS_IERS_B);
        fromEP->x += fromCorr->x;
        fromEP->y += fromCorr->y;
        psSphereRot *fromRot = psSphereRot_CEOtoGCRS(fromEP);
        psEarthPole *toEP = psEOC_PrecessionModel(to);
        psEarthPole *toCorr = psEOC_PrecessionCorr(to, PS_IERS_B);
        toEP->x += toCorr->x;
        toEP->y += toCorr->y;
        psSphereRot *toRot = psSphereRot_CEOtoGCRS(toEP);
        psSphereRot *fromConj = psSphereRotConjugate(NULL, fromRot);
        psSphereRot *out = psSphereRotCombine(NULL, toRot, fromConj);
        psFree(from);
        psFree(to);
        psFree(fromEP);
        psFree(fromCorr);
        psFree(fromRot);
        psFree(toEP);
        psFree(toCorr);
        psFree(toRot);
        psFree(fromConj);
        return out;
    }
}
