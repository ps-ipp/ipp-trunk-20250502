// gcc `pkg-config nebclient --cflags --libs` example.c -o example

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <nebclient.h>

#define NEB_SERVER  "http://podb1.ifa.hawaii.edu:80/nebulous"
#define NEB_KEY     "foobarbaz"

int main (int argc, char **argv) {
    nebServer       *server = NULL;
    char            *key = NEB_KEY;

    server = nebServerAlloc(NEB_SERVER);
    if (!server) {
        printf("nebServerAlloc() failed\n");
        exit(EXIT_FAILURE);
    }

    // make sure there isn't already a file named "foobarbaz" so this example
    // doesn't cause an error
    nebDelete(server, key);

    int fh = nebCreate(server, key, 0, NULL, NULL, NULL);
    if (fh < 0) {
        printf( "nebCreate() failed: %s\n", nebErr(server));
        exit(EXIT_FAILURE);
    }

    // fh is a file descriptor
    close(fh);

    if (!nebReplicate(server, key, NULL, NULL)) {
        printf( "nebReplicate() failed: %s\n", nebErr(server));
        exit(EXIT_FAILURE);
    }

    if (!nebDelete(server, key)) {
        printf( "nebDelete() failed: %s\n", nebErr(server));
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
