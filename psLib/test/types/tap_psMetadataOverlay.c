/**
*  C Implementation: tap_psMetadataOverlay
*
* Description: Tests for psMetadataOverlay
*
* Author: Eugene Magnier
* Copyright: See COPYING file that comes with this distribution
*
*/
#include <pslib.h>
#include <string.h>
#include "tap.h"
#include "pstap.h"

#define DEBUG 0

int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(14);
    // psMetadataOverlay Tests

    // overlay two metadatas : output should contain union of all entries
    {
        psMemId id = psMemGetId();
	psMetadata *in = psMetadataAlloc();
	psMetadata *out = psMetadataAlloc();

	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN1", 0, "", 1);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN2", 0, "", 2);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN3", 0, "", 3);

	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT1", 0, "", 1);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT2", 0, "", 2);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT3", 0, "", 3);

	bool status = psMetadataOverlay (out, in);
	if (DEBUG) psMetadataPrint (stderr, out, 1);

	ok (status, "psMetadataOverlay : overlay two MDs");
	psFree (in);
	psFree (out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // overlay two metadatas, folder in output: output should contain union of all entries
    {
	psMemId id = psMemGetId();
	psMetadata *in = psMetadataAlloc();
	psMetadata *out = psMetadataAlloc();

	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN1", 0, "", 1);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN2", 0, "", 2);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN3", 0, "", 3);

	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT1", 0, "", 1);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT2", 0, "", 2);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT3", 0, "", 3);

	psMetadata *sub = psMetadataAlloc();
	psMetadataAddS32 (sub, PS_LIST_TAIL, "TEST.SUB1", 0, "", 1);
	psMetadataAddS32 (sub, PS_LIST_TAIL, "TEST.SUB2", 0, "", 2);
	psMetadataAddS32 (sub, PS_LIST_TAIL, "TEST.SUB3", 0, "", 3);

	psMetadataAddMetadata (out, PS_LIST_TAIL, "SUB", 0, "", sub);

	bool status = psMetadataOverlay (out, in);
	if (DEBUG) psMetadataPrint (stderr, out, 1);

	ok (status, "psMetadataOverlay : overlay two MDs");
	psFree (in);
	psFree (out);
	psFree (sub);
	ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // overlay two metadatas, folder in input: output should contain union of all entries
    {
	psMemId id = psMemGetId();
	psMetadata *in = psMetadataAlloc();
	psMetadata *out = psMetadataAlloc();

	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN1", 0, "", 1);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN2", 0, "", 2);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN3", 0, "", 3);

	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT1", 0, "", 1);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT2", 0, "", 2);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT3", 0, "", 3);

	psMetadata *sub = psMetadataAlloc();
	psMetadataAddS32 (sub, PS_LIST_TAIL, "TEST.SUB1", 0, "", 1);
	psMetadataAddS32 (sub, PS_LIST_TAIL, "TEST.SUB2", 0, "", 2);
	psMetadataAddS32 (sub, PS_LIST_TAIL, "TEST.SUB3", 0, "", 3);

	psMetadataAddMetadata (in, PS_LIST_TAIL, "SUB", 0, "", sub);

	bool status = psMetadataOverlay (out, in);
	if (DEBUG) psMetadataPrint (stderr, out, 1);

	ok (status, "psMetadataOverlay : overlay two MDs");
	psFree (in);
	psFree (out);
	psFree (sub);
	ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // overlay two metadatas, folder in input & output, no-matched elements
    {
	psMemId id = psMemGetId();
	psMetadata *in = psMetadataAlloc();
	psMetadata *out = psMetadataAlloc();

	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN1", 0, "", 1);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN2", 0, "", 2);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN3", 0, "", 3);

	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT1", 0, "", 1);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT2", 0, "", 2);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT3", 0, "", 3);

	psMetadata *inSub = psMetadataAlloc();
	psMetadataAddS32 (inSub, PS_LIST_TAIL, "TEST.INSUB1", 0, "", 1);
	psMetadataAddS32 (inSub, PS_LIST_TAIL, "TEST.INSUB2", 0, "", 2);
	psMetadataAddS32 (inSub, PS_LIST_TAIL, "TEST.INSUB3", 0, "", 3);
	psMetadataAddMetadata (in, PS_LIST_TAIL, "SUB", 0, "", inSub);

	psMetadata *outSub = psMetadataAlloc();
	psMetadataAddS32 (outSub, PS_LIST_TAIL, "TEST.OUTSUB1", 0, "", 1);
	psMetadataAddS32 (outSub, PS_LIST_TAIL, "TEST.OUTSUB2", 0, "", 2);
	psMetadataAddS32 (outSub, PS_LIST_TAIL, "TEST.OUTSUB3", 0, "", 3);
	psMetadataAddMetadata (out, PS_LIST_TAIL, "SUB", 0, "", outSub);

	bool status = psMetadataOverlay (out, in);
	if (DEBUG) psMetadataPrint (stderr, out, 1);

	ok (status, "psMetadataOverlay : overlay two MDs");
	psFree (in);
	psFree (out);
	psFree (inSub);
	psFree (outSub);
	ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // overlay two metadatas, folder and subfolders in input & output, no-matched elements
    {
	psMemId id = psMemGetId();
	psMetadata *in = psMetadataAlloc();
	psMetadata *out = psMetadataAlloc();

	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN1", 0, "", 1);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN2", 0, "", 2);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN3", 0, "", 3);

	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT1", 0, "", 1);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT2", 0, "", 2);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT3", 0, "", 3);

	psMetadata *inSub = psMetadataAlloc();
	psMetadataAddS32 (inSub, PS_LIST_TAIL, "TEST.INSUB1", 0, "", 1);
	psMetadataAddS32 (inSub, PS_LIST_TAIL, "TEST.INSUB2", 0, "", 2);
	psMetadataAddS32 (inSub, PS_LIST_TAIL, "TEST.INSUB3", 0, "", 3);

	psMetadata *inSubSub = psMetadataAlloc();
	psMetadataAddS32 (inSubSub, PS_LIST_TAIL, "TEST.INSUB1", 0, "", 1);
	psMetadataAddS32 (inSubSub, PS_LIST_TAIL, "TEST.INSUB2", 0, "", 2);
	psMetadataAddS32 (inSubSub, PS_LIST_TAIL, "TEST.INSUB3", 0, "", 3);
	psMetadataAddMetadata (inSub, PS_LIST_TAIL, "SUB", 0, "", inSubSub);

	psMetadataAddMetadata (in, PS_LIST_TAIL, "SUB", 0, "", inSub);

	psMetadata *outSub = psMetadataAlloc();
	psMetadataAddS32 (outSub, PS_LIST_TAIL, "TEST.OUTSUB1", 0, "", 1);
	psMetadataAddS32 (outSub, PS_LIST_TAIL, "TEST.OUTSUB2", 0, "", 2);
	psMetadataAddS32 (outSub, PS_LIST_TAIL, "TEST.OUTSUB3", 0, "", 3);

	psMetadata *outSubSub = psMetadataAlloc();
	psMetadataAddS32 (outSubSub, PS_LIST_TAIL, "TEST.OUTSUB1", 0, "", 1);
	psMetadataAddS32 (outSubSub, PS_LIST_TAIL, "TEST.OUTSUB2", 0, "", 2);
	psMetadataAddS32 (outSubSub, PS_LIST_TAIL, "TEST.OUTSUB3", 0, "", 3);
	psMetadataAddMetadata (outSub, PS_LIST_TAIL, "SUB", 0, "", outSubSub);

	psMetadataAddMetadata (out, PS_LIST_TAIL, "SUB", 0, "", outSub);

	bool status = psMetadataOverlay (out, in);
	if (DEBUG) psMetadataPrint (stderr, out, 1);

	ok (status, "psMetadataOverlay : overlay two MDs");
	psFree (in);
	psFree (out);
	psFree (inSub);
	psFree (outSub);
	psFree (inSubSub);
	psFree (outSubSub);
	ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // overlay two metadatas, folder in input & output, matched elements
    {
	psMemId id = psMemGetId();
	psMetadata *in = psMetadataAlloc();
	psMetadata *out = psMetadataAlloc();

	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN1", 0, "", 1);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN2", 0, "", 2);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN3", 0, "", 3);

	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT1", 0, "", 1);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT2", 0, "", 2);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT3", 0, "", 3);

	psMetadata *inSub = psMetadataAlloc();
	psMetadataAddS32 (inSub, PS_LIST_TAIL, "TEST.SUB1", 0, "", 1);
	psMetadataAddS32 (inSub, PS_LIST_TAIL, "TEST.SUB2", 0, "", 2);
	psMetadataAddS32 (inSub, PS_LIST_TAIL, "TEST.SUB3", 0, "", 3);
	psMetadataAddMetadata (in, PS_LIST_TAIL, "SUB", 0, "", inSub);

	psMetadata *outSub = psMetadataAlloc();
	psMetadataAddS32 (outSub, PS_LIST_TAIL, "TEST.SUB1", 0, "", 4);
	psMetadataAddS32 (outSub, PS_LIST_TAIL, "TEST.SUB2", 0, "", 5);
	psMetadataAddS32 (outSub, PS_LIST_TAIL, "TEST.SUB3", 0, "", 6);
	psMetadataAddMetadata (out, PS_LIST_TAIL, "SUB", 0, "", outSub);

	bool status = psMetadataOverlay (out, in);
	if (DEBUG) psMetadataPrint (stderr, out, 1);

	ok (status, "psMetadataOverlay : overlay two MDs");
	psFree (in);
	psFree (out);
	psFree (inSub);
	psFree (outSub);
	ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // overlay two metadatas, folder and subfolders in input & output, no-matched elements
    {
	psMemId id = psMemGetId();
	psMetadata *in = psMetadataAlloc();
	psMetadata *out = psMetadataAlloc();

	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN1", 0, "", 1);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN2", 0, "", 2);
	psMetadataAddS32 (in, PS_LIST_TAIL, "TEST.IN3", 0, "", 3);

	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT1", 0, "", 1);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT2", 0, "", 2);
	psMetadataAddS32 (out, PS_LIST_TAIL, "TEST.OUT3", 0, "", 3);

	psMetadata *inSub = psMetadataAlloc();
	psMetadataAddS32 (inSub, PS_LIST_TAIL, "TEST.SUB1", 0, "", 1);
	psMetadataAddS32 (inSub, PS_LIST_TAIL, "TEST.SUB2", 0, "", 2);
	psMetadataAddS32 (inSub, PS_LIST_TAIL, "TEST.SUB3", 0, "", 3);

	psMetadata *inSubSub = psMetadataAlloc();
	psMetadataAddS32 (inSubSub, PS_LIST_TAIL, "TEST.SUB1", 0, "", 1);
	psMetadataAddS32 (inSubSub, PS_LIST_TAIL, "TEST.SUB2", 0, "", 2);
	psMetadataAddS32 (inSubSub, PS_LIST_TAIL, "TEST.SUB3", 0, "", 3);
	psMetadataAddMetadata (inSub, PS_LIST_TAIL, "SUB", 0, "", inSubSub);

	psMetadataAddMetadata (in, PS_LIST_TAIL, "SUB", 0, "", inSub);

	psMetadata *outSub = psMetadataAlloc();
	psMetadataAddS32 (outSub, PS_LIST_TAIL, "TEST.SUB1", 0, "", 4);
	psMetadataAddS32 (outSub, PS_LIST_TAIL, "TEST.SUB2", 0, "", 5);
	psMetadataAddS32 (outSub, PS_LIST_TAIL, "TEST.SUB3", 0, "", 6);

	psMetadata *outSubSub = psMetadataAlloc();
	psMetadataAddS32 (outSubSub, PS_LIST_TAIL, "TEST.SUB1", 0, "", 4);
	psMetadataAddS32 (outSubSub, PS_LIST_TAIL, "TEST.SUB2", 0, "", 5);
	psMetadataAddS32 (outSubSub, PS_LIST_TAIL, "TEST.SUB3", 0, "", 6);
	psMetadataAddMetadata (outSub, PS_LIST_TAIL, "SUB", 0, "", outSubSub);

	psMetadataAddMetadata (out, PS_LIST_TAIL, "SUB", 0, "", outSub);

	bool status = psMetadataOverlay (out, in);
	if (DEBUG) psMetadataPrint (stderr, out, 1);

	ok (status, "psMetadataOverlay : overlay two MDs");
	psFree (in);
	psFree (out);
	psFree (inSub);
	psFree (outSub);
	psFree (inSubSub);
	psFree (outSubSub);
	ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

