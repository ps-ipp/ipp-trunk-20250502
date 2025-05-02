#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "nebclient.h"
#include "tap.h"

#define neb_ok(server, ...) \
    if (!ok(__VA_ARGS__)) { \
        diag("nebErr: %s", nebErr(server)); \
    }

#define tst_nebFree(foo) \
    if (foo) nebFree(foo)

int show_instances (nebServer *server, char *key) {

  nebObjectInstances *locations = NULL;

  locations = nebFindInstances(server, key, NULL);

  for (int i = 0; i < locations->n; i++) {
    fprintf (stderr, "# loc %d : %s\n", i, locations->URI[i]);
  }
  nebObjectInstancesFree(locations);
  return 1;
}

int count_instances (nebServer *server, char *key) {

  nebObjectInstances *locations = NULL;
  locations = nebFindInstances(server, key, NULL);
  int Ninstances = locations ? locations->n : 0;
  nebObjectInstancesFree(locations);
  return Ninstances;
}

int main (int argc, char **argv) {
    nebServer       *server = NULL;
    char            *key = "foobarbaz";

    plan_tests(37);

    if (getenv("NEB_SERVER")) {
        server = nebServerAlloc(getenv("NEB_SERVER"));
    } else {
        server = nebServerAlloc(NULL);
    }
    ok(server, "server not NULL");

    printf("# server set to %s\n", server->endpoint);

    {
        char        *URI = NULL;
        char        *filename;
       
        filename = nebCreate(server, key, NULL, &URI);
        diag("filename is %s", filename);

	if (!filename) {
	  fprintf (stdout, "not ok: no files in test server, exiting\n");
	  exit (2);
	}

	show_instances (server, key);
	ok (count_instances (server, key) == 1, "correct instance count"); 

        neb_ok(server, filename, "create object");
        ok(URI, "URI not NULL");
        neb_ok(server, nebDelete(server, key), "delete object");

        tst_nebFree(URI);
        tst_nebFree(filename);
    }

    {
        char        *URI = NULL;
        int         fh;
       
        fh = nebOpenCreate(server, key, NULL, &URI);

        neb_ok(server, fh > -1, "create new object filehandle");
        ok(URI, "URI not NULL");
	ok (count_instances (server, key) == 1, "correct instance count"); 

        tst_nebFree(URI);
        close(fh);
    }

    show_instances (server, key);

    {
        int         fh;

        fh = nebOpen(server, key, NEB_READ);
        diag("fh is %d", fh);

        neb_ok(server, fh > 0, "open object for reading");

        close(fh);
    }

    {
        int         fh;

        fh = nebOpen(server, key, NEB_WRITE);
        diag("fh is %d", fh);

        neb_ok(server, fh > 0, "open object for writing");

        close(fh);
    }

    neb_ok(server, nebReplicate(server, key, NULL, NULL), "replicate object");
    show_instances (server, key);
    ok (count_instances (server, key) == 2, "correct instance count"); 

    neb_ok(server, nebOpen(server, key, NEB_WRITE) < 0, "write to object with multiple instances");
    show_instances (server, key);

    neb_ok(server, nebCull(server, key), "cull object");
    show_instances (server, key);
    ok (count_instances (server, key) == 1, "correct instance count"); 

    neb_ok(server, nebCull(server, key) == 0, "cannot cull object with one instance");
    show_instances (server, key);
    ok (count_instances (server, key) == 1, "correct instance count"); 

    neb_ok(server, nebStat(server, key), "stat object");
    show_instances (server, key);

    neb_ok(server, nebLock(server, key, NEB_WRITE), "lock object write");
    neb_ok(server, nebUnlock(server, key, NEB_WRITE), "unlock object write");

    neb_ok(server, nebLock(server, key, NEB_READ), "lock object read");
    neb_ok(server, nebUnlock(server, key, NEB_READ), "unlock object read");

    neb_ok(server, nebSetXattr(server, key, "user.copies", "2",  NEB_CREATE), "set user.copies xattr");

    char *copies = nebGetXattr(server, key, "user.copies");
    neb_ok(server, copies != NULL && strcmp(copies, "2") == 0, "get user.copies xattr");
    char **xattrs = NULL;
    int n_xattrs = nebListXattr(server, key, &xattrs);
    neb_ok(server, xattrs != NULL && n_xattrs == 1, "count of xattrs");
    neb_ok(server, xattrs != NULL && strcmp(xattrs[0], "user.copies") == 0, "user.copies xattr exists");
    neb_ok(server, nebRemoveXattr(server, key, "user.copies"), "remove user.copies xattr");

    {
        nebObjectInstances *locations = NULL;

        locations = nebFindInstances(server, key, NULL);

        neb_ok(server, locations, "locations not NULL");
        ok(locations && locations->n == 1, "find instances");

        nebObjectInstancesFree(locations);
    }

    {
        char        *filename = NULL;
       
        filename = nebFind(server, key);

        neb_ok(server, filename, "find file name");

        tst_nebFree(filename);
    }


    neb_ok(server, nebCopy(server, key, "copyiedfile"), "copy object");

    neb_ok(server, nebSwap(server, key, "copyiedfile"), "swap objects");

    neb_ok(server, nebDelete(server, key), "delete object");

    neb_ok(server, nebMove(server, "copyiedfile", "movedfile"), "move object");

    neb_ok(server, nebChmod(server, "movedfile", 0440) == 0, "chmod object");

    // no dead instances to remove
    neb_ok(server, nebPrune(server, "movedfile") == 0, "prune object");

    nebReplicate(server, key, NULL, NULL);
    neb_ok(server, nebThereCanBeOnlyOne(server, "movedfile", NULL) == 0, "reduce instances to only 1");

    if (!nebDelete(server, "movedfile")) {
        diag( "cleanup failed %s\n", nebErr(server));
    }

    nebServerFree(server);

    return exit_status();
}
