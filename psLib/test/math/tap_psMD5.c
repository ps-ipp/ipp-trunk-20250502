#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

// The official MD5 test cases
#define NUM_TESTS 7
static const struct
{
    const char *input;                  // The input string
    const char result[16];              // The hash
    const char string[34];              // The stringified hash
}
tests[] =
    {
        { "",
          "\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e",
          "d41d8cd98f00b204e9800998ecf8427e" },
        { "a",
          "\x0c\xc1\x75\xb9\xc0\xf1\xb6\xa8\x31\xc3\x99\xe2\x69\x77\x26\x61",
          "0cc175b9c0f1b6a831c399e269772661" },
        { "abc",
          "\x90\x01\x50\x98\x3c\xd2\x4f\xb0\xd6\x96\x3f\x7d\x28\xe1\x7f\x72",
          "900150983cd24fb0d6963f7d28e17f72" },
        { "message digest",
          "\xf9\x6b\x69\x7d\x7c\xb7\x93\x8d\x52\x5a\x2f\x31\xaa\xf1\x61\xd0",
          "f96b697d7cb7938d525a2f31aaf161d0" },
        { "abcdefghijklmnopqrstuvwxyz",
          "\xc3\xfc\xd3\xd7\x61\x92\xe4\x00\x7d\xfb\x49\x6c\xca\x67\xe1\x3b",
          "c3fcd3d76192e4007dfb496cca67e13b" },
        { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
          "\xd1\x74\xab\x98\xd2\x77\xd9\xf5\xa5\x61\x1c\x2c\x9f\x41\x9d\x9f",
          "d174ab98d277d9f5a5611c2c9f419d9f" },
        { "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
          "\x57\xed\xf4\xa2\x2b\xe3\xc9\x55\xac\x49\xda\x2e\x21\x07\xb6\x7a",
          "57edf4a22be3c955ac49da2e2107b67a" }
    };


int main(void)
{
    plan_tests(NUM_TESTS * 3 +          // Strings
               NUM_TESTS * 2 +          // Vectors
               (NUM_TESTS - 1) * 2 +    // Images (except the empty string test)
               (NUM_TESTS - 2) * 3 +    // Sub-images (except the empty and "a" string test)
               1                        // Memory leaks
              );

//    diag("psMD5 tests");

    // Strings
    for (int i = 0; i < NUM_TESTS; i++) {
        psString string = psStringCopy(tests[i].input); // String to test
        psVector *hash = psStringMD5(string); // The MD5 hash
        ok(hash, "hash generated");

        skip_start(!hash, 2, "Skipping tests because hash calculation failed");

        ok(memcmp(hash->data.U8, tests[i].result, 16) == 0, "hash matches");

        psString hashString = psMD5toString(hash); // String of the hash
        ok(strcmp(hashString, tests[i].string) == 0, "hash string matches");
        psFree(hashString);

        skip_end();

        psFree(hash);
        psFree(string);
    }

    // Vectors
    for (int i = 0; i < NUM_TESTS; i++) {
        psVector *vector = psVectorAlloc(strlen(tests[i].input), PS_TYPE_U8); // Vector to test
        memcpy(vector->data.U8, tests[i].input, strlen(tests[i].input));
        psVector *hash = psVectorMD5(vector); // The MD5 hash
        ok(hash, "hash generated");

        skip_start(!hash, 1, "Skipping test because hash calculation failed");
        ok(memcmp(hash->data.U8, tests[i].result, 16) == 0, "hash matches");
        skip_end();

        psFree(hash);
        psFree(vector);
    }

    // Images
    for (int i = 0; i < NUM_TESTS; i++) {
        if (strlen(tests[i].input) == 0) {
            // We don't like images with size = 0,0
            continue;
        }

        psImage *image = psImageAlloc(1, strlen(tests[i].input), PS_TYPE_U8); // Image to test
        for (int j = 0; j < strlen(tests[i].input); j++) {
            image->data.U8[j][0] = tests[i].input[j];
        }

        {
            psVector *hash = psImageMD5(image);
            ok(hash, "hash generated");

            skip_start(!hash, 1, "Skipping test because hash calculation failed");
            ok(memcmp(hash->data.U8, tests[i].result, 16) == 0, "hash matches");
            skip_end();

            psFree(hash);
        }

        if (strlen(tests[i].input) > 1) {
            // Generate a subImage (so the source can't assume that everything's contiguous)
            psRegion region = { 0, 1, 1, image->numRows }; // Region of interest --- all but the first char
            psImage *subImage = psImageSubset(image, region);

            // We're going to test against something that we calculate ourselves.  Not the most robust of
            // tests, but we've already verified psStringMD5, so we cross our fingers and hope.
            psVector *reference = psStringMD5(tests[i].input + 1);
            ok(reference, "reference hash generated");
            skip_start(!reference, 2, "Skipping tests because hash calculation failed");

            psVector *hash = psImageMD5(subImage);
            ok(hash, "hash generated");

            skip_start(!hash, 1, "Skipping test because hash calculation failed");
            ok(memcmp(reference->data.U8, hash->data.U8, 16) == 0, "hash matches");
            skip_end();

            psFree(hash);

            skip_end();

            psFree(reference);

            psFree(subImage);
        }

        psFree(image);
    }

    done();
}
