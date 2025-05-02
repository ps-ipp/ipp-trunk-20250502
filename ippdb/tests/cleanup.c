#include <pslib.h>
#include <ippdb.h>
#include <stdlib.h>

int main ()
{
    psDB            *dbh;

    dbh = psDBInit("localhost", "test", NULL, "test");
    if (!dbh) {
        exit(EXIT_FAILURE);
    }

    ippdbCleanup(dbh);

    exit(EXIT_SUCCESS);
}
