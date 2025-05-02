/**
*  C Implementation: tap_psMetadataUpdate
*
* Description: Tests for psMetadataUpdate, psMetadataItemAdd for PS_META_REQUIRE_ENTRY and
* PS_META_REQUIRE_TYPE
*
* Author: Eugene Magnier
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
    plan_tests(16);
    // psMetadataUpdate Tests


    {
        psMemId id = psMemGetId();
	psMetadata *md = psMetadataAlloc();
	psMetadataAddS32 (md, PS_LIST_TAIL, "TEST1", 0, "", 1);
	bool status = psMetadataAddF32 (md, PS_LIST_TAIL, "TEST1", PS_META_REPLACE | PS_META_REQUIRE_ENTRY, "", 1);
	ok (status, "PS_META_REQUIRE_ENTRY : psMetadataAdd should not fail if item already exists");
	psFree (md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
	psMetadata *md = psMetadataAlloc();
	psMetadataAddS32 (md, PS_LIST_TAIL, "TEST1", 0, "", 1);
	bool status = psMetadataAddF32 (md, PS_LIST_TAIL, "TEST2", PS_META_REPLACE | PS_META_REQUIRE_ENTRY, "", 1);
	ok (!status, "PS_META_REQUIRE_ENTRY : psMetadataAdd should fail if item does not already exist");
	psFree (md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
	psMetadata *md = psMetadataAlloc();
	psMetadataAddS32 (md, PS_LIST_TAIL, "TEST1", 0, "", 1);
	bool status = psMetadataAddF32 (md, PS_LIST_TAIL, "TEST2", PS_META_REPLACE | PS_META_REQUIRE_TYPE, "", 1);
	ok (status, "PS_META_REQUIRE_TYPE : psMetadataAdd should not fail if item does not already exist");
	psFree (md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
	psMetadata *md = psMetadataAlloc();
	psMetadataAddS32 (md, PS_LIST_TAIL, "TEST1", 0, "", 1);
	bool status = psMetadataAddS32 (md, PS_LIST_TAIL, "TEST1", PS_META_REPLACE | PS_META_REQUIRE_TYPE, "", 1);
	ok (status, "PS_META_REQUIRE_TYPE : psMetadataAdd should not fail if existing item has the same type");
	psFree (md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
	psMetadata *md = psMetadataAlloc();
	psMetadataAddS32 (md, PS_LIST_TAIL, "TEST1", 0, "", 1);
	bool status = psMetadataAddF32 (md, PS_LIST_TAIL, "TEST1", PS_META_REPLACE | PS_META_REQUIRE_TYPE, "", 1);
	ok (!status, "PS_META_REQUIRE_TYPE : psMetadataAdd should fail if existing item does not match type");
	psFree (md);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
	psMetadata *input = psMetadataAlloc();
	psMetadata *output = psMetadataAlloc();

	psMetadataAddS32 (input, PS_LIST_TAIL, "TEST1", 0, "", 2);
	psMetadataAddS32 (input, PS_LIST_TAIL, "TEST2", 0, "", 2);
	psMetadataAddS32 (input, PS_LIST_TAIL, "TEST3", 0, "", 2);
	psMetadataAddS32 (output, PS_LIST_TAIL, "TEST1", 0, "", 1);
	psMetadataAddS32 (output, PS_LIST_TAIL, "TEST2", 0, "", 1);
	psMetadataAddS32 (output, PS_LIST_TAIL, "TEST3", 0, "", 1);

	bool status = psMetadataUpdate (output, input);
	ok (status, "psMetadataUpdate should not fail if items all exist and match type");
	psFree (input);
	psFree (output);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
	psMetadata *input = psMetadataAlloc();
	psMetadata *output = psMetadataAlloc();

	psMetadataAddS32 (input, PS_LIST_TAIL, "TEST1", 0, "", 2);
	psMetadataAddS32 (input, PS_LIST_TAIL, "TEST2", 0, "", 2);
	psMetadataAddS32 (input, PS_LIST_TAIL, "TEST3", 0, "", 2);
	psMetadataAddS32 (output, PS_LIST_TAIL, "TEST1", 0, "", 1);
	psMetadataAddS32 (output, PS_LIST_TAIL, "TEST3", 0, "", 1);

	bool status = psMetadataUpdate (output, input);
	ok (!status, "psMetadataUpdate should fail if any input items do not exist in output");
	psFree (input);
	psFree (output);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
	psMetadata *input = psMetadataAlloc();
	psMetadata *output = psMetadataAlloc();

	psMetadataAddS32 (input, PS_LIST_TAIL, "TEST1", 0, "", 2);
	psMetadataAddS32 (input, PS_LIST_TAIL, "TEST2", 0, "", 2);
	psMetadataAddS32 (input, PS_LIST_TAIL, "TEST3", 0, "", 2);

	psMetadataAddS32 (output, PS_LIST_TAIL, "TEST1", 0, "", 1);
	psMetadataAddF32 (output, PS_LIST_TAIL, "TEST2", 0, "", 1);
	psMetadataAddS32 (output, PS_LIST_TAIL, "TEST3", 0, "", 1);

	bool status = psMetadataUpdate (output, input);
	ok (!status, "psMetadataUpdate should fail if any input items do not match type in output");
	psFree (input);
	psFree (output);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
