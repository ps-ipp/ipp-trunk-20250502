#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>

#include "tap.h"

int main (void)
{
    plan_tests(46);

    diag("pmShutterCorrection tests");

    // test allocation, free of pmShutterCorrPars
    diag("pmShutterCorrParsAlloc tests");
    {
        psMemId id = psMemGetId();
        pmShutterCorrection *pars = pmShutterCorrectionAlloc ();

        ok(pars != NULL, "pmShutterCorrPars successfully allocated");
        skip_start(pars == NULL, 0, "Skipping tests because pmShutterCorrParsAlloc() failed");
        skip_end();

        psFree(pars);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test parameter guess (linearly spaced exptimes, TK/TO < 1)
    diag("pmShutterCorrectionGuess tests : coarse linear-spaced exptimes");
    {
        psMemId id = psMemGetId();

        int NPTS = 10;
        float AK = 5.0;
        float TK = 0.1;
        float TO = 0.2;
        psVector *exptime = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *counts  = psVectorAlloc (NPTS, PS_TYPE_F32);
        exptime->n = counts->n = NPTS;

        for (int i = 0; i < exptime->n; i++) {
            exptime->data.F32[i] = i*0.25;
            counts->data.F32[i] = AK*(exptime->data.F32[i] + TK) / (exptime->data.F32[i] + TO);
        }

        pmShutterCorrection *pars = pmShutterCorrectionGuess (exptime, counts);

        ok(pars != NULL, "pmShutterCorrPars successfully allocated");
        skip_start(pars == NULL, 0, "Skipping tests because pmShutterCorrParsAlloc() failed");

        // with coarse linearly-spaced times large compared to TO and TK,
        // we can't expect very accurate guesses.  the exptime guess is fairly good because
        // the largest exptime is much longer than TK or TO
        ok(fabs(pars->scale  - AK) < 0.5, "scale guess is close enough (got %f vs %f)",  pars->scale, AK);
        ok(fabs(pars->offset - TK) < 0.5, "offset guess is close enough (got %f vs %f)", pars->offset, TK);
        ok(fabs(pars->offref - TO) < 0.5, "offref guess is close enough (got %f vs %f)", pars->offref, TO);
        skip_end();

        psFree(pars);
        psFree(exptime);
        psFree(counts);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test parameter guess (linearly spaced exptimes, TK/TO < 1)
    diag("pmShutterCorrectionGuess tests : fine linear-spaced exptimes");
    {
        psMemId id = psMemGetId();

        int NPTS = 20;
        float AK = 5.0;
        float TK = 0.1;
        float TO = 0.2;
        psVector *exptime = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *counts  = psVectorAlloc (NPTS, PS_TYPE_F32);
        exptime->n = counts->n = NPTS;

        for (int i = 0; i < exptime->n; i++) {
            exptime->data.F32[i] = i*0.1;
            counts->data.F32[i] = AK*(exptime->data.F32[i] + TK) / (exptime->data.F32[i] + TO);
        }

        pmShutterCorrection *pars = pmShutterCorrectionGuess (exptime, counts);

        ok(pars != NULL, "pmShutterCorrection successfully allocated");
        skip_start(pars == NULL, 0, "Skipping tests because pmShutterCorrectionAlloc() failed");

        // with fine linearly-spaced times large compared to TO and TK,
        // we get a good guess to TK and TO, but since the largest exptime is not
        // many times larger than TO, we don't do very well with the AK
        ok(fabs(pars->scale  - AK) < 0.5, "scale guess is close enough (got %f vs %f)",  pars->scale, AK);
        ok(fabs(pars->offset - TK) < 0.05, "offset guess is close enough (got %f vs %f)", pars->offset, TK);
        ok(fabs(pars->offref - TO) < 0.05, "offref guess is close enough (got %f vs %f)", pars->offref, TO);
        skip_end();

        psFree(pars);
        psFree(exptime);
        psFree(counts);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test parameter guess (log spaced exptimes, TK/TO < 1)
    diag("pmShutterCorrectionGuess tests : log-spaced exptimes");
    {
        psMemId id = psMemGetId();

        int NPTS = 40;
        float AK = 5.0;
        float TK = 0.1;
        float TO = 0.2;
        psVector *exptime = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *counts  = psVectorAlloc (NPTS, PS_TYPE_F32);
        exptime->n = counts->n = NPTS;

        for (int i = 0; i < exptime->n; i++) {
            exptime->data.F32[i] = pow(10.0, -2 + i*0.1);
            counts->data.F32[i] = AK*(exptime->data.F32[i] + TK) / (exptime->data.F32[i] + TO);
        }

        pmShutterCorrection *pars = pmShutterCorrectionGuess (exptime, counts);

        // with fine log-spaced times well-sampling TO and TK,
        // we can expect accurate guesses
        ok(pars != NULL, "pmShutterCorrectionsuccessfully allocated");
        skip_start(pars == NULL, 0, "Skipping tests because pmShutterCorrectionAlloc() failed");
        ok(fabs(pars->scale  - AK) < 0.01, "scale guess is close enough (got %f vs %f)",  pars->scale, AK);
        ok(fabs(pars->offset - TK) < 0.01, "offset guess is close enough (got %f vs %f)", pars->offset, TK);
        ok(fabs(pars->offref - TO) < 0.01, "offref guess is close enough (got %f vs %f)", pars->offref, TO);
        skip_end();

        psFree(pars);
        psFree(exptime);
        psFree(counts);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test parameter guess (linearly spaced exptimes, TK/TO > 1)
    diag("pmShutterCorrectionGuess tests : coarse linear-spaced exptimes, TK/TO > 1");
    {
        psMemId id = psMemGetId();

        int NPTS = 10;
        float AK = 5.0;
        float TK = 0.2;
        float TO = 0.1;
        psVector *exptime = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *counts  = psVectorAlloc (NPTS, PS_TYPE_F32);
        exptime->n = counts->n = NPTS;

        for (int i = 0; i < exptime->n; i++) {
            exptime->data.F32[i] = i*0.25;
            counts->data.F32[i] = AK*(exptime->data.F32[i] + TK) / (exptime->data.F32[i] + TO);
        }

        pmShutterCorrection *pars = pmShutterCorrectionGuess (exptime, counts);

        ok(pars != NULL, "pmShutterCorrection successfully allocated");
        skip_start(pars == NULL, 0, "Skipping tests because pmShutterCorrectionAlloc() failed");

        // with coarse linearly-spaced times large compared to TO and TK,
        // we can't expect very accurate guesses.  the exptime guess is fairly good because
        // the largest exptime is much longer than TK or TO
        ok(fabs(pars->scale  - AK) < 0.5, "scale guess is close enough (got %f vs %f)",  pars->scale, AK);
        ok(fabs(pars->offset - TK) < 0.5, "offset guess is close enough (got %f vs %f)", pars->offset, TK);
        ok(fabs(pars->offref - TO) < 0.5, "offref guess is close enough (got %f vs %f)", pars->offref, TO);
        skip_end();

        psFree(pars);
        psFree(exptime);
        psFree(counts);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test parameter guess (linearly spaced exptimes, TK/TO > 1)
    diag("pmShutterCorrectionGuess tests : fine linear-spaced exptimes, TK/TO > 1");
    {
        psMemId id = psMemGetId();

        int NPTS = 20;
        float AK = 5.0;
        float TK = 0.2;
        float TO = 0.1;
        psVector *exptime = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *counts  = psVectorAlloc (NPTS, PS_TYPE_F32);
        exptime->n = counts->n = NPTS;

        for (int i = 0; i < exptime->n; i++) {
            exptime->data.F32[i] = i*0.1;
            counts->data.F32[i] = AK*(exptime->data.F32[i] + TK) / (exptime->data.F32[i] + TO);
        }

        pmShutterCorrection *pars = pmShutterCorrectionGuess (exptime, counts);

        ok(pars != NULL, "pmShutterCorrection successfully allocated");
        skip_start(pars == NULL, 0, "Skipping tests because pmShutterCorrectionsAlloc() failed");

        // with fine linearly-spaced times large compared to TO and TK,
        // we get a good guess to TK and TO, but since the largest exptime is not
        // many times larger than TO, we don't do very well with the AK
        ok(fabs(pars->scale  - AK) < 0.5, "scale guess is close enough (got %f vs %f)",  pars->scale, AK);
        ok(fabs(pars->offset - TK) < 0.05, "offset guess is close enough (got %f vs %f)", pars->offset, TK);
        ok(fabs(pars->offref - TO) < 0.05, "offref guess is close enough (got %f vs %f)", pars->offref, TO);
        skip_end();

        psFree(pars);
        psFree(exptime);
        psFree(counts);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test parameter guess (log spaced exptimes, TK/TO > 1)
    diag("pmShutterCorrectionGuess tests : log-spaced exptimes, TK/TO > 1");
    {
        psMemId id = psMemGetId();

        int NPTS = 40;
        float AK = 5.0;
        float TK = 0.2;
        float TO = 0.1;
        psVector *exptime = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *counts  = psVectorAlloc (NPTS, PS_TYPE_F32);
        exptime->n = counts->n = NPTS;

        for (int i = 0; i < exptime->n; i++) {
            exptime->data.F32[i] = pow(10.0, -2 + i*0.1);
            counts->data.F32[i] = AK*(exptime->data.F32[i] + TK) / (exptime->data.F32[i] + TO);
        }

        pmShutterCorrection *pars = pmShutterCorrectionGuess (exptime, counts);

        // with fine log-spaced times well-sampling TO and TK,
        // we can expect accurate guesses
        ok(pars != NULL, "pmShutterCorrection successfully allocated");
        skip_start(pars == NULL, 0, "Skipping tests because pmShutterCorrectionAlloc() failed");
        ok(fabs(pars->scale  - AK) < 0.01, "scale guess is close enough (got %f vs %f)",  pars->scale, AK);
        ok(fabs(pars->offset - TK) < 0.01, "offset guess is close enough (got %f vs %f)", pars->offset, TK);
        ok(fabs(pars->offref - TO) < 0.01, "offref guess is close enough (got %f vs %f)", pars->offref, TO);
        skip_end();

        psFree(pars);
        psFree(exptime);
        psFree(counts);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test non-linear fitting
    diag("pmShutterCorrectionFullFit tests : linear-spaced exptimes");
    {
        psMemId id = psMemGetId();

        int NPTS = 20;
        float FL = 10000.0;
        float AK = 5.0;
        float TK = 0.2;
        float TO = 0.1;
        psVector *exptime = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *counts  = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *cntErr  = psVectorAlloc (NPTS, PS_TYPE_F32);
        exptime->n = counts->n = cntErr->n = NPTS;

        for (int i = 0; i < exptime->n; i++) {
            exptime->data.F32[i] = i*0.1;
            counts->data.F32[i] = AK*(exptime->data.F32[i] + TK) / (exptime->data.F32[i] + TO);
            cntErr->data.F32[i] = AK*sqrt(FL*(exptime->data.F32[i] + TK)) / (exptime->data.F32[i] + TO);
        }

        pmShutterCorrection *guess = pmShutterCorrectionGuess (exptime, counts);
        skip_start(guess == NULL, 0, "Skipping tests because pmShutterCorrectionGuess() failed");
        pmShutterCorrection *pars = pmShutterCorrectionFullFit (exptime, counts, cntErr, guess);

        // with fine log-spaced times well-sampling TO and TK,
        // we can expect accurate guesses
        ok(pars != NULL, "pmShutterCorrection successfully allocated by FullFit");
        skip_start(pars == NULL, 0, "Skipping tests because pmShutterCorrectionsAlloc() failed");
        ok(fabs(pars->scale  - AK) < 0.01, "scale fit is close enough (got %f vs %f)",  pars->scale, AK);
        ok(fabs(pars->offset - TK) < 0.01, "offset fit is close enough (got %f vs %f)", pars->offset, TK);
        ok(fabs(pars->offref - TO) < 0.01, "offref fit is close enough (got %f vs %f)", pars->offref, TO);
        skip_end();

        psFree(pars);
        skip_end();

        psFree(guess);
        psFree(exptime);
        psFree(counts);
        psFree(cntErr);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test non-linear fitting
    diag("pmShutterCorrectionFullFit tests : log-spaced exptimes");
    {
        psMemId id = psMemGetId();

        int NPTS = 40;
        float FL = 10000.0;
        float AK = 1.0;
        float TK = 0.2;
        float TO = 0.1;
        psVector *exptime = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *counts  = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *cntErr  = psVectorAlloc (NPTS, PS_TYPE_F32);
        exptime->n = counts->n = cntErr->n = NPTS;

        for (int i = 0; i < exptime->n; i++) {
            exptime->data.F32[i] = pow(10.0, -2 + i*0.1);
            counts->data.F32[i] = AK*(exptime->data.F32[i] + TK) / (exptime->data.F32[i] + TO);
            cntErr->data.F32[i] = AK*sqrt(FL*(exptime->data.F32[i] + TK)) / (exptime->data.F32[i] + TO);
        }

        pmShutterCorrection*guess = pmShutterCorrectionGuess (exptime, counts);
        skip_start(guess == NULL, 0, "Skipping tests because pmShutterCorrectionGuess() failed");
        pmShutterCorrection *pars = pmShutterCorrectionFullFit (exptime, counts, cntErr, guess);

        // with fine log-spaced times well-sampling TO and TK,
        // we can expect accurate guesses
        ok(pars != NULL, "pmShutterCorrection successfully allocated by FullFit");
        skip_start(pars == NULL, 0, "Skipping tests because pmShutterCorrectionAlloc() failed");
        ok(fabs(pars->scale  - AK) < 0.01, "scale fit is close enough (got %f vs %f)",  pars->scale, AK);
        ok(fabs(pars->offset - TK) < 0.01, "offset fit is close enough (got %f vs %f)", pars->offset, TK);
        ok(fabs(pars->offref - TO) < 0.01, "offref fit is close enough (got %f vs %f)", pars->offref, TO);
        skip_end();

        psFree(pars);
        skip_end();

        psFree(guess);
        psFree(exptime);
        psFree(counts);
        psFree(cntErr);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // XXX should add tests with the input counts scattered with GaussDev...

    // test linear fitting
    diag("pmShutterCorrectionLinFit tests : linear-spaced exptimes");
    {
        psMemId id = psMemGetId();

        int NPTS = 20;
        float FL = 10000.0;
        float AK = 5.0;
        float TK = 0.2;
        float TO = 0.1;
        psVector *exptime = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *counts  = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *cntErr  = psVectorAlloc (NPTS, PS_TYPE_F32);
        exptime->n = counts->n = cntErr->n = NPTS;

        for (int i = 0; i < exptime->n; i++) {
            exptime->data.F32[i] = i*0.1;
            counts->data.F32[i] = AK*(exptime->data.F32[i] + TK) / (exptime->data.F32[i] + TO);
            cntErr->data.F32[i] = AK*sqrt(FL*(exptime->data.F32[i] + TK)) / (exptime->data.F32[i] + TO);
        }

        pmShutterCorrection*guess = pmShutterCorrectionGuess (exptime, counts);
        skip_start(guess == NULL, 0, "Skipping tests because pmShutterCorrectionGuess() failed");
        pmShutterCorrection *full = pmShutterCorrectionFullFit (exptime, counts, cntErr, guess);
        pmShutterCorrection *pars = pmShutterCorrectionLinFit (exptime, counts, cntErr, NULL, full->offref, 5, 0);

        // with fine log-spaced times well-sampling TO and TK,
        // we can expect accurate guesses
        ok(pars != NULL, "pmShutterCorrection successfully allocated by FullFit");
        skip_start(pars == NULL, 0, "Skipping tests because pmShutterCorrectionAlloc() failed");
        ok(fabs(pars->scale  - AK) < 0.01, "scale fit is close enough (got %f vs %f)",  pars->scale, AK);
        ok(fabs(pars->offset - TK) < 0.01, "offset fit is close enough (got %f vs %f)", pars->offset, TK);
        ok(fabs(pars->offref - TO) < 0.01, "offref fit is close enough (got %f vs %f)", pars->offref, TO);
        skip_end();

        psFree(pars);
        psFree(full);
        skip_end();

        psFree(guess);
        psFree(exptime);
        psFree(counts);
        psFree(cntErr);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test linear fitting
    diag("pmShutterCorrectionLinFit tests : log-spaced exptimes");
    {
        psMemId id = psMemGetId();

        int NPTS = 40;
        float FL = 10000.0;
        float AK = 1.0;
        float TK = 0.2;
        float TO = 0.1;
        psVector *exptime = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *counts  = psVectorAlloc (NPTS, PS_TYPE_F32);
        psVector *cntErr  = psVectorAlloc (NPTS, PS_TYPE_F32);
        exptime->n = counts->n = cntErr->n = NPTS;

        for (int i = 0; i < exptime->n; i++) {
            exptime->data.F32[i] = pow(10.0, -2 + i*0.1);
            counts->data.F32[i] = AK*(exptime->data.F32[i] + TK) / (exptime->data.F32[i] + TO);
            cntErr->data.F32[i] = AK*sqrt(FL*(exptime->data.F32[i] + TK)) / (exptime->data.F32[i] + TO);
        }

        pmShutterCorrection *guess = pmShutterCorrectionGuess (exptime, counts);
        skip_start(guess == NULL, 0, "Skipping tests because pmShutterCorrectionGuess() failed");
        pmShutterCorrection *full = pmShutterCorrectionFullFit (exptime, counts, cntErr, guess);
        pmShutterCorrection *pars = pmShutterCorrectionLinFit (exptime, counts, cntErr, NULL, full->offref, 5, 0);

        // with fine log-spaced times well-sampling TO and TK,
        // we can expect accurate guesses
        ok(pars != NULL, "pmShutterCorrection successfully allocated by FullFit");
        skip_start(pars == NULL, 0, "Skipping tests because pmShutterCorrectionAlloc() failed");
        ok(fabs(pars->scale  - AK) < 0.01, "scale fit is close enough (got %f vs %f)",  pars->scale, AK);
        ok(fabs(pars->offset - TK) < 0.01, "offset fit is close enough (got %f vs %f)", pars->offset, TK);
        ok(fabs(pars->offref - TO) < 0.01, "offref fit is close enough (got %f vs %f)", pars->offref, TO);
        skip_end();

        psFree(pars);
        psFree(full);
        skip_end();

        psFree(guess);
        psFree(exptime);
        psFree(counts);
        psFree(cntErr);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    return exit_status();
}
