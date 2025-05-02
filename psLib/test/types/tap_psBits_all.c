/**
 *  C Implementation: tap_psBits_all
 *
 * Description:  Tests for psBitsAlloc, psBitsSet, psMemCheckBits, psBitsClear,
 *               psBitsTest, psBitsOp, psBitsNot, psBitsToString
 *
 * Author: dRob <David.Robbins@mhpcc.hpc.mil>, (C) 2006
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#include <pslib.h>
#include <string.h>
#include "tap.h"
#include "pstap.h"


int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(30);


    // testBitsBasics()
    {
        psMemId id = psMemGetId();
        psBits *noBits = NULL;
        psBits *bs = NULL;

        //Return properly allocated 0-size psBits
        {
            bs = psBitsAlloc(0);
            ok( bs != NULL && psMemCheckBits(bs) && bs->n == 0,
                "psBitsAlloc:         return properly allocated psBits.");
        }
        //Return properly allocated psBits
        {
            psFree(bs);
            bs = psBitsAlloc(8);
            ok( bs != NULL && psMemCheckBits(bs) && bs->n == 8,
                "psBitsAlloc:         return properly allocated psBits.");
        }
        //Make sure psMemCheckBits works correctly - return false
        if (0) {
            int j = 2;
            ok( !psMemCheckBits(&j),
                "psMemCheckBits:      return false for non-Bits input.");
        }

        //BitsSet Tests
        //Return FALSE for NULL input psBits
        {
            bool rc = psBitsSet(NULL, 0);
            ok(rc == false,
                "psBitsSet:           return FALSE for NULL Bits input.");
        }
        //Return FALSE for negative bit input
        {
            bool rc = psBitsSet(bs, -1);
            ok( rc == false,
                "psBitsSet:           return TRUE for negative bits input.");
            noBits = NULL;
        }
        //Return FALSE for out-of-range bits
        {
            psFree(bs);
            bs = psBitsAlloc(8);
            bool rc = psBitsSet(bs, 8);
            ok( rc == false,
                "psBitsSet:           return TRUE for out-of-range bits input.");
            noBits = NULL;
        }

        //Return set Bits for valid inputs
        {
            psBitsSet(bs, 2);
            ok( bs->bits[0] == 4,
                "psBitsSet:           return properly set Bits for valid inputs.");
        }

        //BitsClear Tests
        //Return FALSE for NULL input psBits
        {
            bool rc = psBitsClear(noBits, 0);
            ok( rc == false,
                "psBitsClear:         return FALSE for NULL Bits input.");
        }
        //Return FALSE for negative bit input
        {
            bool rc = psBitsClear(bs, -1);
            ok( rc == false,
                "psBitsClear:        return TRUE for negative bits input.");
            noBits = NULL;
        }
        //Return FALSE for out-of-range bits
        {
            bool rc = psBitsClear(bs, 8);
            ok( rc == false,
                "psBitsClear:        return FALSE for out-of-range bits input.");
            noBits = NULL;
        }

        //Return cleared Bits for valid inputs
        {
            psBitsClear(bs, 2);
            ok( bs->bits[0] == 0,
                "psBitsClear:        return properly cleared Bits for valid inputs.");
        }

        //BitsTest Tests
        //Return false for NULL input psBits
        {
            ok( !psBitsTest(noBits, 0),
                "psBitsTest:         return false for NULL Bits input.");
        }
        //Return false for negative bit input
        {
            ok( !psBitsTest(bs, -1),
                "psBitsTest:         return false for negative bits input.");
        }
        //Return false for out-of-range bits
        {
            ok( !psBitsTest(bs, 8),
                "psBitsTest:         return false for out-of-range bits input.");
        }
        //Return false for non-matching bit in Bits
        {
            ok( !psBitsTest(bs, 2),
                "psBitsTest:         return false for non-matching bit in Bits.");
        }
        //Return false for non-matching bit in Bits
        {
            psBitsSet(bs, 2);
            ok( psBitsTest(bs, 2),
                "psBitsTest:         return true for matching bit in Bits.");
        }

        psFree(bs);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testBitsOps()
    {
        psMemId id = psMemGetId();
        psBits *noBits = NULL;
        psBits *bs = NULL;
        bs = psBitsAlloc(8);
        psBitsSet(bs, 2);  // 0000 0100 == 4
        psBitsSet(bs, 3);  // 0000 1100 == 12
        psBitsSet(bs, 4);  // 0001 1100 == 28
        psBitsSet(bs, 5);  // 0011 1100 == 60
        psBitsSet(bs, 6);  // 0111 1100 == 124
        psBitsSet(bs, 7);  // 1111 1100 == 252
        psBits *out = NULL;

        //psBitsNot Tests
        //Return NULL for NULL Bits input
        {
            out = psBitsNot(out, noBits);
            ok( out == NULL,
                "psBitsNot:          return NULL for NULL Bits input.");
        }
        //Return correct Bits for valid Bits input
        {
            out = psBitsNot(out, bs);  //bs = 1111 1100  so out should = 0000 0011 = 3
            ok( out->bits[0] == 3,
                "psBitsNot:          return correct Bits for valid Bits input.");
        }

        //psBitsOp Tests   out = psBitsOp(out, bs1, "op", bs2);
        //Return NULL for NULL Bits input
        {
            psFree(out);
            out = NULL;
            out = psBitsOp(out, noBits, "op", noBits);
            ok( out == NULL,
                "psBitsOp:           return NULL for NULL Bits input.");
        }
        //Return NULL for NULL operator input
        {
            out = psBitsOp(out, bs, NULL, noBits);
            ok( out == NULL,
                "psBitsOp:           return NULL for NULL operator input.");
        }
        //Return NULL for invalid operator input
        {
            out = psBitsOp(out, bs, "XAND", noBits);
            ok( out == NULL,
                "psBitsOp:           return NULL for invalid operator input.");
        }
        //Return NULL for AND operator with NULL second Bits input
        {
            out = psBitsOp(out, bs, "AND", noBits);
            ok( out == NULL,
                "psBitsOp:           return NULL for AND operator with NULL second Bits input.");
        }
        //Return NULL for AND operator with Bits inputs of differing size.
        psBits *bs2 = psBitsAlloc(16);
        {
            out = psBitsOp(out, bs, "AND", bs2);
            ok( out == NULL,
                "psBitsOp:           return NULL for AND operator with Bits inputs of"
                " differing size.");
        }
        psFree(bs);
        bs = psBitsAlloc(16);
        psBitsSet(bs, 1);     // 0000 0010 == 2
        psBitsSet(bs2, 2);   // 0000 0100 == 4
        //Return correct psBits output for valid inputs with AND operator
        {
            out = psBitsOp(out, bs, "AND", bs2);
            ok( out->bits[0] == 0,
                "psBitsOp:           return correct psBits output for valid inputs"
                " with AND operator.");
        }
        //Return correct psBits output for valid inputs with OR operator
        {
            out = psBitsOp(out, bs, "OR", bs2);
            ok( out->bits[0] == 6,
                "psBitsOp:           return correct psBits output for valid inputs"
                " with OR operator.");
        }
        //Return correct psBits output for valid inputs with XOR operator
        psBitsSet(bs2, 1);     // 0000 0110 == 6
        {
            out = psBitsOp(out, bs, "XOR", bs2);
            ok( out->bits[0] == 4,
                "psBitsOp:           return correct psBits output for valid inputs"
                " with XOR operator.");
        }
        //Return correct psBits output for valid inputs with NOT operator
        {
            psFree(out);
            out = psBitsAlloc(0);
            psBitsSet(bs, 2);  // 0000 0110 == 4
            psBitsSet(bs, 3);  // 0000 1110 == 12
            psBitsSet(bs, 4);  // 0001 1110 == 28
            psBitsSet(bs, 5);  // 0011 1110 == 60
            psBitsSet(bs, 6);  // 0111 1110 == 124
            psBitsSet(bs, 7);  // 1111 1110 == 252
            out = psBitsOp(out, bs, "NOT", bs2);
            ok( out->bits[0] == 1,
                "psBitsOp:           return correct psBits output for valid inputs"
                " with NOT operator.");
        }

        //psBitsToString Tests
        //Return NULL for NULL Bits input
        psString bitStr = NULL;
        {
            bitStr = psBitsToString(noBits);
            ok( bitStr == NULL,
                "psBitsToString:     return NULL for NULL Bits input.");
        }
        //Return correct string for valid Bits input
        {
            psFree(bs);
            bs = psBitsAlloc(8);
            psBitsSet(bs, 2);  // 0000 0100 == 4
            bitStr = psBitsToString(bs);
            ok( !strncmp(bitStr, "00000100", 10),
                "psBitsToString:     return correct string for valid Bits input (%s).", bitStr);
        }

        psFree(bitStr);
        psFree(out);
        psFree(bs);
        psFree(bs2);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}
