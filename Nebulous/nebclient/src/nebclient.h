/*
 * nebclient.h -  nebulous client header
 * 
 * Copyright (C) 2005  Joshua Hoblitt
 *
 * $Id: nebclient.h,v 1.40 2009-02-13 02:49:00 jhoblitt Exp $
 */

#include <stdbool.h>

#ifndef NEBCLIENT_H
#define NEBCLIENT_H 1

// #include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @addtogroup nebclient
/// @{

typedef enum { NEB_READ, NEB_WRITE } nebRW;
typedef enum { NEB_CREATE, NEB_REPLACE } nebCR;

/** Represents a "connection" to a Nebulous server
 */

typedef struct {
    void            *soap;
    char            *err_buf;
    size_t          err_buf_size;
    char            *endpoint;
} nebServer;

/** The properties of a storage object
 */

typedef struct {
    long            so_id;              ///< storage object ID
    long            ext_id;             ///< storage object key (name)
    int             read_lock;          ///< read lock value
    char            write_lock[256];    ///< write lock value
    char            epoch[256];         ///< creation time stamp
    char            mtime[256];         ///< modification time stamp
    int             available;          ///< number of available instances
    int             instances;          ///< total number of instances
} nebObjectStat;

/** The storage location of a storage object's instances
 */

typedef struct {
    unsigned int    n;                  ///< number of instances
    char            **URI;              ///< URLs of instances 
} nebObjectInstances;

/** Allocates a nebServer object
 *
 * Structure used for communicating with a Nebulous server.  If endpoint is
 * NULL then the default value of "http://localhost:80/nebulous" is used.
 *
 * @return A nebServer object or NULL on failure.
 */

nebServer *nebServerAlloc(
    const char *endpoint                ///< URL of server
);

/** Deallocates a nebServer object
 */

void nebServerFree(
    nebServer *server                   ///< nebServer object
);

/** Creates a new storage object
 *
 * @return A local filename
 */

char *nebCreate(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    const char *volume,                 ///< preferred storage location of initial instance
    char **URI                          ///< URL of initial instance, can be NULL
);

/** Creates and opens new storage object
 *
 * @return A filehandle on success or a negative value on failure.
 */

int nebOpenCreate(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    const char *volume,                 ///< preferred storage location of initial instance
    char **URI                          ///< URL of initial instance, can be NULL
);

/** Adds an instance to a storage object
 *
 * @return true on success
 */

bool nebReplicate(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    const char *volume,                 ///< preferred storage location of new instance
    char **URI                          ///< URL of new instance, can be NULL
);

/** Removes an instance from a storage object
 *
 * This function can not remove the last instance of a storage object.
 *
 * @return true on success
 */

bool nebCull(
    nebServer *server,                  ///< nebServer object
    const char *key                     ///< storage object key (name)
);

/** Trys to acquire a lock on a storage object
 *
 * This function times out after a default interval of 10s (compile time
 * configurable).
 *
 * @return true on success
 */
bool nebLock(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    nebRW flag                          ///< read only or write lock
);

/** Trys to release a lock on a storage object
 *
 * @return true on success
 */
bool nebUnlock(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    nebRW flag                          ///< read only or write lock
);

/** Set an Xattr on a storage object
 *
 * @return true on success
 */
bool nebSetXattr(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    const char *name,                   ///< xattr name
    const char *value,                  ///< xattr value
    nebCR flag                          ///< flag: "create"|"replace"
);

/** Get an Xattr on a storage object
 *
 * @return true on success
 */
char *nebGetXattr(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    const char *name                    ///< xattr name
);

/** List all Xattrs on a storage object
 *
 * This function returns the count of Xattrs on an object and the actual names
 * are returned in the (char **) pointed to by the xattrs param.  The xattrs
 * param may be NULL.
 *
 * @return int
 */
int nebListXattr(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    char ***xattrs                      ///< array of xattr names to return
);

/** Remove an Xattr from a storage object
 *
 * @return true on success
 */
bool nebRemoveXattr(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    const char *name                    ///< xattr name
);

/** Lists all instances of a storage object
 *
 * @return A nebObjectInstances object or NULL on failure.
 */

nebObjectInstances *nebFindInstances(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    const char *volume                  ///< restrict search to this storage volume (non-working)
);

/** Deallocates a nebObjectInstances object
 */

void nebObjectInstancesFree(
    nebObjectInstances *locations       ///< nebObjectInstances object to free 
);

/** Find any instance of a storage object
 *
 * @return A string of the files storage path (not a URL) or NULL on failure.
 */

char *nebFind(
    nebServer *server,                  ///< nebServer object
    const char *key                     ///< storage object key (name)
);

/** Open a storage object for read or write
 *
 * @return A filehandle on success or a negative value on failure.
 */

int nebOpen(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    nebRW flag                          ///< read only or write lock
);

/** Delete a storage object and all of it's instances
 *
 * @return true on success
 */

bool nebDelete(
    nebServer *server,                  ///< nebServer object
    const char *key                     ///< storage object key (name)
);

/** Copy a storage object
 *
 * The new storage object will be created with only one instance.
 *
 * @return true on success
 */

bool nebCopy(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    const char *newKey                  ///< new storage object key (name)
);

/** Rename a storage object
 *
 * Currently this operation will remove all but one instance of the storage
 * object and may change it's storage location.
 */
bool nebMove(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    const char *newKey                  ///< new storage object key (name)
);

/** Atomically swap two storage objects
 *
 * @return true on success
 */
bool nebSwap(
    nebServer *server,                  ///< nebServer object
    const char *key1,                   ///< storage object key (name)
    const char *key2                    ///< storage object key (name)
);

/** Remove a storage object instance
 *
 * Removing the last instance of a storage object with destroy the storage
 * object as well.
 *
 * @return true on success
 */

bool nebDeleteInstance(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    const char *URI                     ///< URL of instance to remove
);

/** View the properties of a storage object
 *
 * @return A nebObjectStat object or NULL on failure.
 */

nebObjectStat *nebStat(
    nebServer *server,                  ///< nebServer object
    const char *key                     ///< storage object key (name)
);


/** Deallocates a stat object
 */
void nebObjectStatFree(
    nebObjectStat *stat                 ///< nebObjectStat object
);

/** Change permissions of a storage objects
 *
 * This function will change the file permissions of all currently on disk
 * instances of a storage object.
 *
 * @return the new mode or NULL on failure.
 */

int nebChmod(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    mode_t mode                         ///< chmod(2) compatible mode (mode_t)
);

/** Removes all of the inaccessible instances from an object
 *
 * @return the number of inaccessible instances removed
 */

int nebPrune(
    nebServer *server,                  ///< nebServer object
    const char *key                     ///< storage object key (name)
);

/** Removes all but one instances of an object
 *
 * @return the number of available instances removed
 */

int nebThereCanBeOnlyOne(
    nebServer *server,                  ///< nebServer object
    const char *key,                    ///< storage object key (name)
    const char *volume                  ///< leave remain instance on this volume
);

/** Returns the error message from the last nebclient function that failed.
 *
 * This function may not be called unless an error has occurred.  The string
 * return by this function does not need to be freed but the value will change
 * at the next nebclient function failure.
 *
 * @return A string
 */

char *nebErr(
    nebServer *server                   ///< nebServer object
);

/** Free nebclient allocated memory
 */
void nebFree(void *ptr);

/// @}

#ifdef __cplusplus
}
#endif

#endif // NEBLIENT_H
