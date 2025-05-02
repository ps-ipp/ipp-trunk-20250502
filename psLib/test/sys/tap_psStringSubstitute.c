#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

#define ORIGINAL "This is, a, test case, to check."
#define CORRECTED "This is a test case to check."

int main (void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(16);


    // Return input for NULL key
    {
        psMemId id = psMemGetId();
        psString input = psStringCopy(ORIGINAL);
        psStringSubstitute(&input, ",", NULL);
        ok(input && strcmp(input, ORIGINAL) == 0, "output = %s", input);
        psFree(input);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return input for empty key
    {
        psMemId id = psMemGetId();
        psString input = psStringCopy(ORIGINAL);
        psStringSubstitute(&input, "XXX", "");
        ok(input && strcmp(input, ORIGINAL) == 0, "output = %s", input);
        psFree(input);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return corrected version for NULL replace
    {
        psMemId id = psMemGetId();
        psString input = psStringCopy(ORIGINAL);
        psStringSubstitute(&input, NULL, ",");
        ok(input && strcmp(input, CORRECTED) == 0, "output = %s", input);
        psFree(input);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return corrected version for empty replace
    {
        psMemId id = psMemGetId();
        psString input = psStringCopy(ORIGINAL);
        psStringSubstitute(&input, "", ",");
        ok(input && strcmp(input, CORRECTED) == 0, "output = %s", input);
        psFree(input);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return NULL for NULL input
    {
        psMemId id = psMemGetId();
        int status = psStringSubstitute(NULL, "XXX", ",");
        ok(status == 0, "status = %d", status);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return emptry string for empty input
    {
        psMemId id = psMemGetId();
        psString input = psStringCopy("");
        psStringSubstitute(&input, "XXX", ",");
        ok(input && strcmp(input, "") == 0, "output = %s", input);
        psFree(input);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Change commas to bangs
    {
        psMemId id = psMemGetId();
        psString input = psStringCopy(ORIGINAL);
        psStringSubstitute(&input, "!", ",");
        ok(input && strcmp(input, "This is! a! test case! to check.") == 0, "output = %s", input);
        psFree(input);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Long replacement text --- should allocate new space
    {
        psMemId id = psMemGetId();
        psString input = psStringCopy(ORIGINAL);
        psStringSubstitute(&input, "; This string is too long to fit in input(35 chars)", ".");
        ok(input && strcmp(input, "This is, a, test case, to check; "
                           "This string is too long to fit in input(35 chars)") == 0,
           "output = %s", input);
        psFree(input);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}
