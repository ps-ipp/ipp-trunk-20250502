#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pstampint.h"
#include "pstampROI.h"
#include "ohana.h"

static bool get2Angles(int argnum, int *pArgc, char **argv, bool bothDegrees, bool makePositive, double *p1, double *p2);
static bool get2Ints(int argnum, int *pArgc, char **argv, bool makePositive, int *p1, int *p2);


bool pstampGetROI(pstampROI *roip, int *pArgc, char **argv, bool *gotCenter, bool *gotRange)
{
    int argnum;                         // Argument number of interest

    if ((argnum = psArgumentGet(*pArgc, argv, "-pixcenter"))) {
        *gotCenter = true;
        roip->celestialCenter = false;
        psArgumentRemove(argnum, pArgc, argv);

        roip->center[0] = argv[argnum];
        roip->center[1] = argv[argnum+1];

        if (!get2Ints(argnum, pArgc, argv, false, &roip->centerX, &roip->centerY)) {
            psError(PSTAMP_ERR_ARGUMENTS, true, "invalid pixcenter specification: %s %s\n",
                argv[argnum], argv[argnum+1]);
            return false;
        }
        psArgumentRemove(argnum, pArgc, argv);
        psArgumentRemove(argnum, pArgc, argv);
    }

    if ((argnum = psArgumentGet(*pArgc, argv, "-skycenter"))) {
        if (*gotCenter) {
            psError(PSTAMP_ERR_ARGUMENTS, true, "can't specify both -pixcenter and -skycenter\n");;
            return false;
        }
        psArgumentRemove(argnum, pArgc, argv);

        roip->center[0] = argv[argnum];
        roip->center[1] = argv[argnum+1];

        double raDeg, decDeg;
        if (!get2Angles(argnum, pArgc, argv, false, false, &raDeg, &decDeg)) {
            psError(PSTAMP_ERR_ARGUMENTS, true, "invalid skycenter specification: %s %s\n",
                argv[argnum], argv[argnum+1]);
            return false;
        }
        psArgumentRemove(argnum, pArgc, argv);
        psArgumentRemove(argnum, pArgc, argv);
        roip->centerRA  = DEG_TO_RAD(raDeg);
        roip->centerDEC = DEG_TO_RAD(decDeg);
        *gotCenter = true;
        roip->celestialCenter = true;
    }

    if ((argnum = psArgumentGet(*pArgc, argv, "-pixrange"))) {
        *gotRange = true;
        roip->celestialRange = false;
        psArgumentRemove(argnum, pArgc, argv);
        roip->range[0] = argv[argnum];
        roip->range[1] = argv[argnum+1];

        if (!get2Ints(argnum, pArgc, argv, true, &roip->dX, &roip->dY)) {
            psError(PSTAMP_ERR_ARGUMENTS, true, "invalid pixrange specification: %s %s\n",
                argv[argnum], argv[argnum+1]);
            return false;
        }
        psArgumentRemove(argnum, pArgc, argv);
        psArgumentRemove(argnum, pArgc, argv);
    }

    if ((argnum = psArgumentGet(*pArgc, argv, "-arcrange"))) {
        if (*gotRange) {
            psError(PSTAMP_ERR_ARGUMENTS, true, "specify only one of -pixrange or -arcrange\n");;
            return false;
        }
        psArgumentRemove(argnum, pArgc, argv);
        *gotRange = true;
        roip->celestialRange = true;

        roip->range[0] = argv[argnum];
        roip->range[1] = argv[argnum+1];

        // arcrange values are seconds of arc
        roip->dRA  = SEC_TO_RAD(fabs(atof(argv[argnum])));
        roip->dDEC = SEC_TO_RAD(fabs(atof(argv[argnum+1])));

        psArgumentRemove(argnum, pArgc, argv);
        psArgumentRemove(argnum, pArgc, argv);
    }
    // I'm leaving in the -celrange option (HH:MM:SS DD:MM:SS), but not publicizing it
    if ((argnum = psArgumentGet(*pArgc, argv, "-celrange"))) {
        if (*gotRange) {
            psError(PSTAMP_ERR_ARGUMENTS, true, "specify one of -pixrange, -arcrange, or -celrange\n");;
            return false;
        }
        psArgumentRemove(argnum, pArgc, argv);
        *gotRange = true;
        roip->celestialRange = true;

        roip->range[0] = argv[argnum];
        roip->range[1] = argv[argnum+1];

        double deg1, deg2;
        if (!get2Angles(argnum, pArgc, argv, false, true, &deg1, &deg2)) {
            psError(PSTAMP_ERR_ARGUMENTS, true, "invalid celrange specification: %s %s\n",
                argv[argnum], argv[argnum+1]);
            return false;
        }
        psArgumentRemove(argnum, pArgc, argv);
        psArgumentRemove(argnum, pArgc, argv);
        roip->dRA = DEG_TO_RAD(deg1);
        roip->dDEC = DEG_TO_RAD(deg2);
    }

    if (!*gotCenter) {
        psError(PSTAMP_ERR_ARGUMENTS, true, "must specify center");
        return false;
    }
    if (!*gotRange) {
        psError(PSTAMP_ERR_ARGUMENTS, true, "must specify range for stamp");
        return false;
    }

    return true;
}

static bool validNumber(char *string)
{
    char *p = string;

    if ((*p == '+') || (*p == '-')) {
        p++;
    }
    return isdigit(*p);
}

static bool get2Ints(int argnum, int *pArgc, char **argv, bool makePositive, int *p1, int *p2)
{
    if (*pArgc < 2) {
        return false;
    }

    if (!validNumber(argv[argnum])) {
        return false;
    }
    *p1 = atoi(argv[argnum]);

    if (!validNumber(argv[argnum+1])) {
        return false;
    }
    *p2 = atoi(argv[argnum+1]);

    if (makePositive) {
        *p1 = abs(*p1);
        *p2 = abs(*p2);
    }

    return true;
}

static bool get2Angles(int argnum, int *pArgc, char **argv, bool bothInDegrees, bool makePositive,
    double *p1, double *p2)
{
    bool rval;

    if (*pArgc < 2) {
        return false;
    }

    if (bothInDegrees) {
        // both values are angles of arc DD:MM:SS or decimal degrees
        rval   = ohana_dms_to_ddd(p1, argv[argnum]);
        if (rval) {
            rval  = ohana_dms_to_ddd(p2, argv[argnum+1]);
        }
    } else {
        // first value may be in HH:MM:SS
        rval = ohana_str_to_radec(p1, p2, argv[argnum], argv[argnum+1]);
    }

    if (rval && makePositive) {
        *p1 = abs(*p1);
        *p2 = abs(*p2);
    }

    return rval;
}
