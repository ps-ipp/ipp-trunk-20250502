#include <pslib.h>
#include <ippdb.h>
#include <stdlib.h>

int main ()
{
    psDB            *dbh;

    dbh = ippdbInit("localhost", "test", NULL, "test");
    if (!dbh) {
        exit(EXIT_FAILURE);
    }

    psDBCleanup(dbh);

    exit(EXIT_SUCCESS);
}
