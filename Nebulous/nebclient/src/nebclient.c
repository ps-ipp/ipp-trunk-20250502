/*
 * nebclient.c - nebulous client
 *
 * Copyright (C) 2005  Joshua Hoblitt
 *
 * $Id: nebclient.c,v 1.57 2009-02-13 02:45:47 jhoblitt Exp $
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include "xmalloc.h"
#include "soapH.h"
#include "SOAP.nsmap"
#include "nebclient.h"

#define ERRBUF_SIZE 2048
#define LOCK_TIMEOUT 10
#define LOCK_INTERVAL 1

#define nebSetErr(server,...) \
    p_nebSetErr(__FILE__, __LINE__, __func__, server,__VA_ARGS__)

// this is a slightly more useful error message than returned by assert()
#define REQUIRE_SERVER \
    if (!server) { \
        fprintf(stderr, "parameter 'server' may not be NULL"); \
        abort(); \
    }

#define NEBCOPYFILE 0

static void p_nebSetErr(const char * filename, unsigned long lineno, const char *function, nebServer *server, const char *format, ...);
static void nebSetServerErr(nebServer *server);
#if NEBCOPYFILE
static off_t nebCopyFile(nebServer *server, const char *source, const char *dest);
#endif
static off_t nebCopyFilehandle(nebServer *server, int sourceFH, int destFH);

static size_t nebParseURI(nebServer *server, const char *URI, char **filename);
static bool nebNukeFile(nebServer *server, const char *filename);
static char *nullstrncpy(char *dest, const char *src, size_t n);


nebServer *nebServerAlloc(const char *endpoint)
{
    nebServer *server = xmalloc(sizeof(nebServer));

    if (endpoint) {
        server->endpoint = xmalloc(strlen(endpoint) + 1);
        strcpy(server->endpoint, endpoint);
    } else {
        server->endpoint = NULL;
    }

    server->soap    = xmalloc(sizeof(struct soap));
    server->err_buf = xmalloc(ERRBUF_SIZE);
    server->err_buf_size = ERRBUF_SIZE;

    // init err buffer
    memset(server->err_buf, 0, server->err_buf_size);

    soap_init(server->soap);
    soap_set_namespaces(server->soap, namespaces); 

    return server;
}


void nebServerFree(nebServer *server)
{
    if (!server) {
        return;
    }

    soap_end(server->soap); // remove deserialized data and clean up
    soap_done(server->soap); // detach the gSOAP environment

    nebFree(server->endpoint);
    nebFree(server->soap);
    nebFree(server->err_buf);
    nebFree(server);
}


char *nebCreate(nebServer *server, const char *key, const char *volume, char **URI)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return NULL;
    }

    struct ns1__create_USCOREobjectResponse response;
    if (soap_call_ns1__create_USCOREobject(server->soap, server->endpoint, NULL,
            (char *)key, (char *)volume, (char **)&response) != SOAP_OK) {
        nebSetServerErr(server);

        return NULL;
    }

    if (!(char *)response.result) {
        nebSetErr(server, "server returned no result");
        return NULL;
    }

    char *filename;
    if (!nebParseURI(server, (char *)response.result, &filename)) {
        nebSetErr(server, "can not parse URI");
        nebFree(filename);

        return NULL;
    }

    // if URI is not NULL
    if (URI) {
        *URI = xmalloc(strlen((char *)response.result) + 1);
        strcpy(*URI, (char *)response.result);
    }

    return filename;
}


int nebOpenCreate(nebServer *server, const char *key, const char *volume, char **URI)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return -1;
    }

    char *filename = nebCreate(server, key, volume, URI);
    if (!filename) {
        return -1;
    }

    int file = open(filename, O_RDWR|O_TRUNC, 0660);
    if (file == -1) {
        nebSetErr(server, "can not open %s: %s", filename, strerror(errno));
        nebFree(filename);

        return -1;
    }

    nebFree(filename);

    return file;
}


bool nebReplicate(nebServer *server, const char *key, const char *volume, char **URI)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return false;
    }


    // We have to open the instance that we're going to copy from first.  If
    // we don't do this, it's possible that open will find & open the new
    // instance that we're in the process of creating
    int sourceFH = nebOpen(server, (char *)key, NEB_READ);
    if (sourceFH == -1) {
        nebSetErr(server, "can not open key %s", key);

        return false;
    }

    // ask the server for a new instance attached to our key
    struct ns1__replicate_USCOREobjectResponse response;
    if (soap_call_ns1__replicate_USCOREobject(server->soap, server->endpoint,
            NULL, (char *)key, (char *)volume, (char **)&response) != SOAP_OK) {
        nebSetServerErr(server);

        return false;
    }

    char *filename = NULL;
    if (!nebParseURI(server, (char *)response.result, &filename)) {
        nebSetErr(server, "can not parse URI");

        return false;
    }

    int destFH = open(filename, O_RDWR|O_TRUNC, 0660);
    if (destFH == -1) {
        nebSetErr(server, "can not open %s: %s\n", filename, strerror(errno));
        nebFree(filename);

        return false;
    }

    nebCopyFilehandle(server, sourceFH, destFH);

    if (close(sourceFH) == -1) {
        nebSetErr(server, "can not close file: %s", strerror(errno));
        nebFree(filename);

        return false;
    }

    if (close(destFH) == -1) {
        nebSetErr(server, "can not close %s: %s", filename, strerror(errno));
        nebFree(filename);

        return false;
    }

    nebFree(filename);

    // if URI is not NULL
    if (URI) {
        *URI = xmalloc(strlen((char *)response.result) + 1);
        strcpy(*URI, (char *)response.result);
    }

    return true;
}


bool nebCull(nebServer *server, const char *key)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return false;
    }

    nebObjectInstances *locations = nebFindInstances(server, key, NULL);
    if (!locations || locations->n <= 1) {
        nebSetErr(server, "can not cull - not enough instances");
        if (locations) {
            nebObjectInstancesFree(locations);
        }

        return false;
    }

    if (!nebDeleteInstance(server, key, locations->URI[0])) {
        nebObjectInstancesFree(locations);

        return false;
    }

    nebObjectInstancesFree(locations);

    return true;
}


bool nebLock(nebServer *server, const char *key, nebRW flag)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return false;
    }

    char *type = flag == NEB_READ ? "read" : "write";

    time_t endtime = time(NULL) + LOCK_TIMEOUT;

    while (time(NULL) < endtime) {
        int response;
        if (soap_call_ns1__lock_USCOREobject(server->soap, server->endpoint,
            NULL, (char *)key, type, &response) != SOAP_OK) {
            nebSetServerErr(server);
            return false;
        }

        if (response) {
            return true;
        }

        sleep(LOCK_INTERVAL);
    }

    // else, we never got a lock
    nebSetErr(server, "can not get a %s lock", type);

    return false;
}


bool nebUnlock(nebServer *server, const char *key, nebRW flag)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return false;
    }

    char *type = flag == NEB_READ ? "read" : "write";

    int response;
    if (soap_call_ns1__unlock_USCOREobject(server->soap, server->endpoint,
        NULL, (char *)key, type, &response) != SOAP_OK) {
        nebSetServerErr(server);
        return false;
    }

    if (response) {
        return true;
    }

    // else, we never got a lock
    nebSetErr(server, "can not release %s lock", type);

    return false;
}


bool nebSetXattr(nebServer *server, const char *key, const char *name, const char *value, nebCR flag)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");
        return false;
    }
    if (!name) {
        nebSetErr(server, "parameter 'name' may not be NULL");
        return false;
    }
    if (!value) {
        nebSetErr(server, "parameter 'value' may not be NULL");
        return false;
    }

    char *type = flag == NEB_CREATE ? "create" : "replace";

    int response;
    if (soap_call_ns1__setxattr_USCOREobject(server->soap, server->endpoint,
        NULL, (char *)key, (char *)name, (char *)value, type, &response) != SOAP_OK) {
        nebSetServerErr(server);
        return false;
    }

    if (!response) {
        nebSetErr(server, "can not set xattr");
        return false;
    }

    return true;
}


char *nebGetXattr(nebServer *server, const char *key, const char *name) 
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");
        return false;
    }
    if (!name) {
        nebSetErr(server, "parameter 'name' may not be NULL");
        return false;
    }

    struct ns1__getxattr_USCOREobjectResponse response;
    if (soap_call_ns1__getxattr_USCOREobject(server->soap, server->endpoint,
        NULL, (char *)key, (char *)name, (char **)&response) != SOAP_OK) {
        nebSetServerErr(server);
        return false;
    }

    if (!(char *)response.result) {
        nebSetErr(server, "server returned no result");
        return NULL;
    }

    char *value = xmalloc(strlen((char *)response.result) + 1);
    strcpy(value, (char *)response.result);

    return value;
}


int nebListXattr(nebServer *server, const char *key, char ***xattrs)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");
        return false;
    }

    struct ns1__listxattr_USCOREobjectResponse response;
    if (soap_call_ns1__listxattr_USCOREobject(server->soap, server->endpoint,
        NULL, (char *)key, &response) != SOAP_OK) {
        nebSetServerErr(server);
        return false;
    }

    int resultElements = response.result->__size;

    if (xattrs) {
        char **resultArray = response.result->__ptr;
        *xattrs = xmalloc(resultElements * sizeof(char *));
        if (resultElements) {
            for (int i = 0; i < resultElements; i++) {
                *xattrs[i] = xmalloc(strlen(resultArray[i]) + 1);
                strcpy(*xattrs[i], resultArray[i]);
            }

        }
    }

    return resultElements;
}


bool nebRemoveXattr(nebServer *server, const char *key, const char *name)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");
        return false;
    }
    if (!name) {
        nebSetErr(server, "parameter 'name' may not be NULL");
        return false;
    }

    int response;
    if (soap_call_ns1__removexattr_USCOREobject(server->soap, server->endpoint,
        NULL, (char *)key, (char *)name, &response) != SOAP_OK) {
        nebSetServerErr(server);
        return false;
    }

    if (!response) {
        nebSetErr(server, "can not remove xattr");
        return false;
    }

    return true;
}


nebObjectInstances *nebFindInstances(nebServer *server, const char *key, const char *volume)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return NULL;
    }

    struct ns1__find_USCOREinstancesResponse response;
    // FIXME is this leaking memory when response goes out of scope?  the gsoap
    // manual seems to 'suggest' that this is temporary data that gets cleaed
    // up on the next soap function call
    if (soap_call_ns1__find_USCOREinstances(server->soap, server->endpoint,
            NULL, (char *)key, (char *)volume, &response) != SOAP_OK) {
        nebSetServerErr(server);
        return NULL;
    }

    int resultElements = response.result->__size;

    nebObjectInstances *locations = NULL;
    if (resultElements) {
        locations = xmalloc(sizeof(nebObjectInstances));
        locations->n = resultElements;
        
        char **resultArray = response.result->__ptr;

        locations->URI = xmalloc(sizeof(char*) * resultElements);

        for (int i = 0; i < resultElements; i++) {
            locations->URI[i] = xmalloc(strlen(resultArray[i]) + 1);
            strcpy(locations->URI[i], resultArray[i]);
        }

    }

    return locations;
}


void nebObjectInstancesFree(nebObjectInstances *locations)
{
    if (!locations) {
        return;
    }

    for (int i = 0; i < locations->n; i++) {
        nebFree(locations->URI[i]);
    }

    nebFree(locations);
}


char *nebFind(nebServer *server, const char *key)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return NULL;
    }

    // Construct a dummy volume name to allow nebulous to find the
    // closest instance available.
    char hostname[256];
    char volname[260];

    int v;

    v = gethostname(hostname,256);
    //    fprintf(stderr, "%s %d\n", hostname, v);
/*     printf("%s %d\n",hostname,v); */
    if (v) {
      nebSetErr(server, "failed to construct hostname");
      return NULL;
    }
    snprintf(volname,260, "%s.0", hostname);
    
    nebObjectInstances *locations = nebFindInstances(server, key, volname);
    if (!locations) {
        if (!strstr(nebErr(server), "no instances on storage volume")) {
            nebSetErr(server, "no instances found");
            return NULL;
        }

        locations = nebFindInstances(server, key, "any");
        if (!locations) {
            nebSetErr(server, "no instances found");
            return NULL;
        }
    }

    char *filename;
    if (!nebParseURI(server, locations->URI[0], &filename)) {
        nebSetErr(server, "can not parse URI");
        nebObjectInstancesFree(locations);

        return NULL;
    }

    nebObjectInstancesFree(locations);

    return filename;
}


int nebOpen(nebServer *server, const char *key, nebRW flag)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return -1;
    }

    nebObjectInstances *locations = nebFindInstances(server, key, NULL);
    if (!locations) {
        nebSetErr(server, "no instances found");

        return -1;
    }

    char *filename;
    if (!nebParseURI(server, locations->URI[0], &filename)) {
        nebSetErr(server, "can not parse URI");
        nebObjectInstancesFree(locations);

        return -1;
    }

    int fh = 0;
    if (flag == NEB_WRITE) {
        if (locations->n > 1) {
            nebSetErr(server, "write not allowed with multiple instances");
            nebFree(filename);
            nebObjectInstancesFree(locations);

            return -1;
        }
        fh = open(filename, O_RDWR);
    } else {
        fh = open(filename, O_RDONLY);
    }

    nebFree(filename);
    nebObjectInstancesFree(locations);

    if (fh < 0) {
        nebSetErr(server, "open: %s", strerror(errno));

        return -1;
    }

    return fh;
}


bool nebDelete(nebServer *server, const char *key)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return false;
    }

    nebObjectInstances *locations = nebFindInstances(server, key, "any");
    if (!locations) {
        nebSetErr(server, "no instances found");

        return false;
    }

    for (int i = 0; i < locations->n; i++) {
      if (!nebDeleteInstance(server, key, locations->URI[i])) {
            nebSetErr(server, "can not delete instance");
            nebObjectInstancesFree(locations);

            return false;
        }
    }

    nebObjectInstancesFree(locations);

    return true;
}


bool nebCopy(nebServer *server, const char *key, const char *newKey)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return false;
    }

    if (!newKey) {
        nebSetErr(server, "parameter 'newKey' may not be NULL");

        return false;
    }

    int sourceFH = nebOpen(server, (char *)key, NEB_READ);
    if (sourceFH < 0) {
        // propigate nebOpen() error

        return false;
    }

    int destFH = nebOpenCreate(server, newKey, NULL, NULL);
    if (destFH < 0) {
        // propigate nebCreate() error

        return false;
    }

    nebCopyFilehandle(server, sourceFH, destFH);

    if (close(sourceFH) == -1) {
        nebSetErr(server, "can not close file: %s", strerror(errno));

        return false;
    }

    if (close(destFH) == -1) {
        nebSetErr(server, "can not close file: %s", strerror(errno));

        return false;
    }

    return true;
}


bool nebMove(nebServer *server, const char *key, const char *newKey)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return false;
    }

    if (!newKey) {
        nebSetErr(server, "parameter 'newKey' may not be NULL");

        return false;
    }

    if (!nebCopy(server, key, newKey)) {
        return false;
    }

    if (!nebDelete(server, key)) {
        return false;
    }

    return true;
}


bool nebSwap(nebServer *server, const char *key1, const char *key2)
{
    REQUIRE_SERVER;

    if (!key1) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return false;
    }

    if (!key2) {
        nebSetErr(server, "parameter 'newKey' may not be NULL");

        return false;
    }

    struct ns1__create_USCOREobjectResponse response;
    if (soap_call_ns1__swap_USCOREobjects(server->soap, server->endpoint,
            NULL, (char *)key1, (char *)key2, (char **)&response) != SOAP_OK) {
        return false;
    }

    return true;
}


bool nebDeleteInstance(nebServer *server, const char *key, const char *URI)
{
    REQUIRE_SERVER;

    if (!URI) {
        nebSetErr(server, "parameter 'URI' may not be NULL");

        return false;
    }

    char *filename;
    if (!nebParseURI(server, URI, &filename)) {
        nebSetErr(server, "can not parse URI");

        return false;
    }

    int response;
    if (soap_call_ns1__delete_USCOREinstance(server->soap, server->endpoint,
        NULL, (char *)key, (char *)URI, &response) != SOAP_OK) {
        nebFree(filename);

        return false;
    }

    bool status;
    if (response > 0) {
        status = nebNukeFile(server, filename);
    } else {
        status = false;
    }

    nebFree(filename);

    return status;
}


nebObjectStat *nebStat(nebServer *server, const char *key)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return NULL;
    }

    nebObjectStat *stat = xmalloc(sizeof(nebObjectStat));

    // FIXME is this leaking memory when response goes out of scope?  the gsoap
    // manual seems to 'suggest' that this is temporary data that gets cleaed
    // up on the next soap function call
    struct ns1__stat_USCOREobjectResponse response;
    if (soap_call_ns1__stat_USCOREobject(server->soap, server->endpoint,
            NULL, (char *)key, &response) != SOAP_OK) {
        nebSetServerErr(server);
        return NULL;
    }

    int resultElements = response.result->__size;
    char **resultArray    = response.result->__ptr;

    if (resultElements != 8) {
        nebSetErr(server, "server didn't return the proper number of stat elements");
        return NULL;
    }

    stat->so_id     = atol(resultArray[0]);
    stat->ext_id    = atol(resultArray[1]);
    stat->read_lock = atoi(resultArray[2]);
    nullstrncpy(stat->write_lock, resultArray[3], 256);
    nullstrncpy(stat->epoch, resultArray[4], 256);
    nullstrncpy(stat->mtime, resultArray[5], 256);
    stat->available = atoi(resultArray[6]);
    stat->instances = atoi(resultArray[7]);

    return stat;
}


void nebObjectStatFree(nebObjectStat *stat)
{
    if (!stat) {
        return;
    }

    nebFree(stat);
}


int nebChmod(nebServer *server, const char *key, mode_t mode)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return -1;
    }

    // FIXME is this leaking memory when response goes out of scope?  the gsoap
    // manual seems to 'suggest' that this is temporary data that gets cleaed
    // up on the next soap function call
    int response;
    if (soap_call_ns1__chmod_USCOREobject(server->soap, server->endpoint,
            NULL, (char *)key, mode, &response) != SOAP_OK) {
        nebSetServerErr(server);
        return -1;
    }

    return 0;
}


int nebPrune(nebServer *server, const char *key)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return -1;
    }

    // FIXME is this leaking memory when response goes out of scope?  the gsoap
    // manual seems to 'suggest' that this is temporary data that gets cleaed
    // up on the next soap function call
    int response;
    if (soap_call_ns1__prune_USCOREobject(server->soap, server->endpoint,
            NULL, (char *)key, &response) != SOAP_OK) {
        nebSetServerErr(server);
        return -1;
    }

    return response;
}


int nebThereCanBeOnlyOne(nebServer *server, const char *key, const char *volume)
{
    REQUIRE_SERVER;

    if (!key) {
        nebSetErr(server, "parameter 'key' may not be NULL");

        return -1;
    }

    // first - strip off any inaccesible instances
    if (nebPrune(server, key) < 0 ) {
        // use the prune() method to also determine if $key is valid
        return -1;
    }

    // check to see if there is an instance on $vol_name that should be the
    // sole survivor
    nebObjectInstances *locations = NULL;
    if (volume) {
        locations = nebFindInstances(server, key, NULL);
    }

    long removed = 0;
    if (locations && locations->n) {
        nebObjectInstances *instances = nebFindInstances(server, key, NULL);
        for (long i = 0; i < instances->n; i++) {
            if (strcmp(locations->URI[0], instances->URI[i]) == 0) {
                continue;
            }
            if (!nebDeleteInstance(server, key, instances->URI[i])) {
                nebObjectInstancesFree(instances);
                nebObjectInstancesFree(locations);
                return -1;
            }
            removed++;
        }
        nebObjectInstancesFree(instances);
        nebObjectInstancesFree(locations);
    } else {
        nebObjectInstancesFree(locations);
        // nuke whatever
        nebObjectStat *stat = nebStat(server, key);
        if (!stat) {
            return -1;
        }

        int available = stat->available;
        nebObjectStatFree(stat);

        // start at one so cull() is called one less time then the # of
        // instances
        for (long i = 1; i < available; i++) {
            if (!nebCull(server, key)) {
                return -1;
            }
            removed++;
        }
    }

    return removed;
}


char *nebErr(nebServer *server)
{
    return server->err_buf;
}


void nebFree(void *ptr) {
    if (!ptr) { return; }
    xfree(ptr);
}


static void p_nebSetErr(const char * filename, unsigned long lineno, const char *function, nebServer *server, const char *format, ...)
{
    char err_location[ERRBUF_SIZE];
    size_t err_location_size = snprintf(err_location, ERRBUF_SIZE, "%s:%lu %s() - ",
            filename, lineno, function);

    if (err_location_size < 0) {
        fprintf(stderr, "failed to set error info");

        abort();
    }

    // copy error location info to start of error string
    strncpy(server->err_buf, err_location, ERRBUF_SIZE);

    while (1) {
        va_list args;
        va_start(args, format);
        // append error string after error location info
        size_t err_size = vsnprintf(
                server->err_buf + err_location_size,
                server->err_buf_size - err_location_size,
                format, args);
        va_end(args);

        if (err_size < 0) {
            fprintf(stderr, "failed to set error message");

            abort();
        }

        if (err_size < server->err_buf_size) {
            break;
        }

        // else: expand err_buf
        // account for '\0'
        server->err_buf_size = err_size + 1;
        xrealloc(server->err_buf, server->err_buf_size);
    }

}


static void nebSetServerErr(nebServer *server)
{
    REQUIRE_SERVER;

    const char **code   = soap_faultcode(server->soap);
    const char **string = soap_faultstring(server->soap);

    nebSetErr(server, "%s - %s", *code, *string);
}


#if NEBCOPYFILE
static off_t nebCopyFile(nebServer *server, const char *source, const char *dest)
{
    REQUIRE_SERVER;

    int sourceFH = open(source, O_RDONLY);
    if (sourceFH == -1) {
        nebSetErr(server, "can not open %s: %s", source, strerror(errno));

        return -1;
    }

    int destFH = open(dest, O_WRONLY|O_CREAT|O_CREAT, 0600);
    if (destFH == -1) {
        nebSetErr(server, "can not open %s: %s", dest, strerror(errno));

        return -1;
    }

    off_t bytesCopied = nebCopyFilehandle(server, sourceFH, destFH);
    if (bytesCopied < 0) {
        return -1;
    }

    if (close(sourceFH) == -1) {
        nebSetErr(server, "can not close %s: %s", source, strerror(errno));

        return -1;
    }

    if (close(destFH) == -1) {
        nebSetErr(server, "can not close %s: %s", dest, strerror(errno));

        return -1;
    }

    return bytesCopied;
}
#endif


static off_t nebCopyFilehandle(nebServer *server, int sourceFH, int destFH)
{
    REQUIRE_SERVER;

    struct stat     sourceStat;
    if(fstat(sourceFH, &sourceStat)) {
        nebSetErr(server, "can not stat filehandles: %s", strerror(errno));

        return -1;
    }

    // the size of the file to copied
    off_t bytesTotal = sourceStat.st_size;

    /*
     * If the file size is <= the buffer size call write(2) with the
     * file size.  If the file size is > the buffer call write with the
     * buffer size and loop until file size <= buffer size is true.
    */

    off_t bytesRemaining = bytesTotal;

    while (bytesRemaining) {
        off_t writeSize;
        char writeBuf[64 * 1024];
        if (bytesRemaining <= sizeof(writeBuf)) {
            writeSize = bytesRemaining;
        } else {
            writeSize = sizeof(writeBuf);
        }

        if (read(sourceFH, writeBuf, writeSize) != writeSize) {
            nebSetErr(server, "can not read filehandle: %s", strerror(errno));

            return -1;
        }

        if (write(destFH, writeBuf, writeSize) != writeSize) {
            nebSetErr(server, "can not write filehandles: %s", strerror(errno));

            return -1;
        }

        bytesRemaining -= writeSize;
    }

    return bytesTotal;
}


static size_t nebParseURI(nebServer *server, const char *URI, char **filename)
{
    REQUIRE_SERVER;

    if (!URI) {
        nebSetErr(server, "parameter 'URI' may not be NULL");

        return 0;
    }

    if (!filename) {
        nebSetErr(server, "parameter 'filename' may not be NULL");

        return 0;
    }

    regex_t myregex;
    regmatch_t mymatch[2];
    char *pattern = "^file://(.*)$";
    char errbuf[ERRBUF_SIZE];

    int status = regcomp(&myregex, pattern, REG_EXTENDED);
    if (status != 0) {
        regerror(status, &myregex, errbuf, ERRBUF_SIZE);
        nebSetErr(server, "regcomp error: %s", errbuf);

        return 0;
    }

    status = regexec(&myregex, URI, 2, mymatch, 0);
    if (status != 0) {
        regerror(status, &myregex, errbuf, ERRBUF_SIZE);
        nebSetErr(server, "regexec error: %s", errbuf);

        return 0;
    }

    regfree(&myregex);

    size_t matchStart = (size_t)mymatch[1].rm_so;
    size_t matchEnd   = (size_t)mymatch[1].rm_eo;
    size_t matchLength = matchEnd - matchStart;

    if (matchStart == -1) {
        return 0;
    }

    size_t filename_size = matchLength + 1;
    *filename = xmalloc(filename_size);

    snprintf(*filename, filename_size, "%.*s", (int)matchLength, URI + matchStart);

    return strlen(*filename);
}


static bool nebNukeFile(nebServer *server, const char *filename)
{
    REQUIRE_SERVER;

    if (!filename) {
        nebSetErr(server, "parameter 'filename' may not be NULL");

        return false;
    }

    if (unlink(filename) < 0) {
        nebSetErr(server, "unlink: %s", strerror(errno));

        return false;
    }

    return true;
}


static char *nullstrncpy(char *dest, const char *src, size_t n) {
    if (!src) {
        dest = NULL;
        return dest;
    }

    if (!dest) {
        dest = xmalloc(n + 1);
    }

    return strncpy(dest, src, n);
}
