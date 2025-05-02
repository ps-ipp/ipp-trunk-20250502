#include <pslib.h>
#include <string.h>
#include <unistd.h>     // needed for unlink()

#include "tap.h"
#include "pstap.h"

int main(void)

{
    plan_tests(15);

    psArray *table = psArrayAllocEmpty (10);
    for (int i = 0; i < table->nalloc; i++) {
        psMetadata *row = psMetadataAlloc ();
        psMetadataAdd (row, PS_LIST_TAIL, "X_PIX",   PS_DATA_F32, "", 2.0*i);
        psMetadataAdd (row, PS_LIST_TAIL, "Y_PIX",   PS_DATA_F32, "", 4.0*i);
        psArrayAdd (table, 10, row);
    }

    psImage *image = psImageAlloc (512, 512, PS_DATA_F32);

    psFits *fits = psFitsOpen ("test.fits", "w");

    psMetadata *header = psMetadataAlloc ();

    for (int i = 0; i < 5; i++) {
        char name[80];

        sprintf (name, "blank.%02d", i);
        psMetadataAddStr (header, PS_LIST_TAIL, "TESTNAME", PS_META_REPLACE, "test name", name);
        // psFitsMoveLast (fits);
        psFitsWriteBlank (fits, header, "testext");

        sprintf (name, "image.%02d", i);
        psMetadataAddStr (header, PS_LIST_TAIL, "TESTNAME", PS_META_REPLACE, "test name", name);
        psFitsWriteImage (fits, header, image, 1, "testext");

        sprintf (name, "table.%02d", i);
        psMetadataAddStr (header, PS_LIST_TAIL, "TESTNAME", PS_META_REPLACE, "test name", name);
        psMetadataAddStr (header, PS_LIST_TAIL, "EXTTYPE", PS_META_REPLACE, "type", "SMPDATA");
        psMetadataAddStr (header, PS_LIST_TAIL, "EXTHEAD", PS_META_REPLACE, "head", "image.00");
        psMetadataAddStr (header, PS_LIST_TAIL, "EXTDATA", PS_META_REPLACE, "data", "blank.00");
        psFitsWriteTable (fits, header, table, "testext");
    }

    psFitsClose (fits);

    fits = psFitsOpen ("test.fits", "r");
    for (int i = 0; i < 5; i++) {
        char *name, expect[80];

        psMetadata *outhead = psMetadataAlloc ();

        fprintf (stderr, "top: %d\n", psFitsGetExtNum (fits));

        psFitsReadHeader (outhead, fits);
        sprintf (expect, "blank.%02d", i);
        name = psMetadataLookupStr (NULL, outhead, "TESTNAME");
        ok (!strcmp(name, expect), "got TESTNAME = %s, expected %s", name, expect);
        psFitsMoveExtNum (fits, 1, true);
        fprintf (stderr, "curr: %d\n", psFitsGetExtNum (fits));

        psFitsReadHeader (outhead, fits);
        sprintf (expect, "image.%02d", i);
        name = psMetadataLookupStr (NULL, outhead, "TESTNAME");
        ok (!strcmp(name, expect), "got TESTNAME = %s, expected %s", name, expect);
        psFitsMoveExtNum (fits, 1, true);
        fprintf (stderr, "curr: %d\n", psFitsGetExtNum (fits));

        psFitsReadHeader (outhead, fits);
        sprintf (expect, "table.%02d", i);
        name = psMetadataLookupStr (NULL, outhead, "TESTNAME");
        ok (!strcmp(name, expect), "got TESTNAME = %s, expected %s", name, expect);
        psFitsMoveExtNum (fits, 1, true);
        fprintf (stderr, "curr: %d\n", psFitsGetExtNum (fits));
    }
    psFitsClose (fits);
    unlink ("test.fits");

    exit (0);
}
