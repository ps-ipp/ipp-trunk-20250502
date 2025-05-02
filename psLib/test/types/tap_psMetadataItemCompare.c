/**
 *  C Implementation: tap_psMetadataItemCompare
 *
 * Description:  Tests for psMetadataItemCompare
 *
 * Author: dRob <David.Robbins@mhpcc.hpc.mil>, (C) 2006
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#include <pslib.h>
#include "tap.h"
#include "pstap.h"


int main(void)
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(22);

    // psMetadataItemCompare() tests
    {
        psMemId id = psMemGetId();
        psMetadataItem *compare = NULL;
        psMetadataItem *template = NULL;
        psMetadataItem *itemCopy = NULL;
        psMetadataItem *item = psMetadataItemAlloc("Snickers", PS_DATA_BOOL, "No Comment", true)
                               ;
    
        //Return false for NULL compare psMetadataItem
        {
            ok( !psMetadataItemCompare(compare, item),
                "psMetadataItemCompare:      return false for NULL compare input." );
        }
        //Return false for NULL template psMetadataItem
        {
            ok( !psMetadataItemCompare(item, template),
                "psMetadataItemCompare:      return false for NULL template input." );
        }
    
        //Return false for psMetadataItem's with different names
        {
            template = psMetadataItemAlloc("Milky Way", PS_DATA_BOOL, "No Comment", true)
                       ;
            compare = psMetadataItemAlloc("Snickers", PS_DATA_S32, "No Comment", 1);
            ok( !psMetadataItemCompare(template, item),
                "psMetadataItemCompare:      return false for input with different names.");
        }

        //Return true for an exact match (copied psMetadataItem)
        {
            itemCopy = psMetadataItemCopy(item);
            ok( psMetadataItemCompare(itemCopy, item),
                "psMetadataItemCompare:      return true for comparison of identical psMetadataItem's.");
        }
    
        //Return true for psMetadataItem's with different types but otherwise identical
        {
            ok( psMetadataItemCompare(compare, item),
                "psMetadataItemCompare:      return true for valid compare of different type.");
        }
    
        //Return true for comparing Bool with F64
        {
            psFree(compare);
            compare = psMetadataItemAlloc("Snickers", PS_DATA_F64, "No Comment", 1.0000);
            ok( psMetadataItemCompare(compare, item),
                "psMetadataItemCompare:      return false for comparison of F64 to bool." );
        }

        //Return true for comparing F64 with Bool
        {
            ok( psMetadataItemCompare(item, compare),
                "psMetadataItemCompare:      return false for comparison of bool to F64." );
        }
    
        //Return true for comparing S32 with F32 of same value
        {
            psFree(template);
            template = psMetadataItemAlloc("Snickers", PS_DATA_S32, "No Comment", 1);
            psFree(compare);
            compare = psMetadataItemAlloc("Snickers", PS_DATA_F32, "No Comment", 1.00);
            ok( psMetadataItemCompare(compare, template),
                "psMetadataItemCompare:      return true for comparison of S32 to F32." );
        }

        //Return true for comparing F32 with S32 of same value
        {
            ok( psMetadataItemCompare(template, compare),
                "psMetadataItemCompare:      return true for comparison of F32 to S32." );
        }
    
        //Return true for comparing U32 with S16 of same value
        {
            psFree(template);
            template = psMetadataItemAlloc("Snickers", PS_DATA_S16, "No Comment", 1);
            psFree(compare);
            compare = psMetadataItemAlloc("Snickers", PS_DATA_U32, "No Comment", 1);
            ok( psMetadataItemCompare(compare, template),
                "psMetadataItemCompare:     return true for comparison of S16 to U32." );
        }

        //Return true for comparing S16 with U32 of same value
        {
            ok( psMetadataItemCompare(template, compare),
                "psMetadataItemCompare:     return true for comparison of U32 to S16." );
        }
    
        //Return true for comparing U8 with U16 of same value
        {
            psFree(template);
            template = psMetadataItemAlloc("Snickers", PS_DATA_U8, "No Comment", 1);
            psFree(compare);
            compare = psMetadataItemAlloc("Snickers", PS_DATA_U16, "No Comment", 1);
            ok( psMetadataItemCompare(compare, template),
                "psMetadataItemCompare:     return true for comparison of U8 to U16." );
        }

        //Return true for comparing U16 with U8 of same value
        {
            ok( psMetadataItemCompare(template, compare),
                "psMetadataItemCompare:     return true for comparison of U16 to U8." );
        }
    
        //Return true for comparing U8 with S8 of same value
        {
            psFree(compare);
            compare = psMetadataItemAlloc("Snickers", PS_DATA_S8, "No Comment", 1);
            ok( psMetadataItemCompare(compare, template),
                "psMetadataItemCompare:     return true for comparison of U8 to S8." );
        }

        //Return true for comparing S8 with U8 of same value
        {
            ok( psMetadataItemCompare(template, compare),
               "psMetadataItemCompare:     return true for comparison of S8 to U8." );
        }
    
        //Return false for comparing U8 with S64 of same value
        {
            psFree(compare);
            compare = psMetadataItemAlloc("Snickers", PS_DATA_S64, "No Comment", 1);
            ok( !psMetadataItemCompare(compare, template),
                "psMetadataItemCompare:     return false for comparison of U8 to S64*.  "
                "*not yet implemented" );
        }

        //Return false for comparing S64 with U8 of same value
        {
            ok( !psMetadataItemCompare(template, compare),
                "psMetadataItemCompare:     return false for comparison of S64* to U8.  "
                "*not yet implemented" );
        }
    
        //Return false for comparing string of different value
        {
            psFree(compare);
            compare = psMetadataItemAlloc("Snickers", PS_DATA_STRING, "No Comment",
                                          "Not Going Anywhere?");
            psFree(template);
            template = psMetadataItemAlloc("Snickers", PS_DATA_STRING, "No Comment", "For a While?");
            ok( !psMetadataItemCompare(compare, template),
                "psMetadataItemCompare:     return false for comparison of different psString's.");
        }
    
        //Return true for comparing string of same value
        {
            psFree(template);
            template = psMetadataItemAlloc("Snickers", PS_DATA_STRING, "No Comment",
                                           "Not Going Anywhere?");
            ok( psMetadataItemCompare(compare, template),
                "psMetadataItemCompare:     return true for comparison of identical psString's.");
        }
    
        //Return false for comparing string with bool
        {
            psFree(template);
            template = psMetadataItemAlloc("Snickers", PS_DATA_BOOL, "No Comment", false);
            ok( !psMetadataItemCompare(compare, template),
                "psMetadataItemCompare:     return false for comparison of psString with bool.");
        }

        //Return false for comparing bool with string
        {
            psFree(template);
            template = psMetadataItemAlloc("Snickers", PS_DATA_BOOL, "No Comment", false);
            ok( !psMetadataItemCompare(template, compare),
                "psMetadataItemCompare:     return false for comparison of psString with bool.");
        }
    
        psFree(compare);
        psFree(template);
        psFree(itemCopy);
        psFree(item);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}
