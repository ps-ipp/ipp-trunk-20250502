
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pslib.h>
#include "psIOBuffer.h"

static void psIOBufferFree (psIOBuffer *buffer)
{
    if (buffer == NULL)
        return;

    psFree (buffer->data);
    return;
}

psIOBuffer *psIOBufferAlloc (int nBuffer)
{

    psIOBuffer *buffer = (psIOBuffer *)psAlloc(sizeof(psIOBuffer));
    psMemSetDeallocator(buffer, (psFreeFunc) psIOBufferFree);

    buffer->data = (char *) psAlloc (nBuffer + 1);

    buffer->nAlloc = nBuffer + 1;
    buffer->nReset = nBuffer;
    buffer->nBlock = nBuffer / 2;
    buffer->n = 0;
    return (buffer);
}

bool psIOBufferFlush (psIOBuffer *buffer)
{

    if (buffer == NULL)
        return false;
    buffer->n = 0;
    buffer->nAlloc = buffer->nReset;
    buffer->data = psRealloc (buffer->data, buffer->nAlloc);
    memset(buffer->data, '\0', buffer->nAlloc);

    return true;
}

int psIOBufferRead (psIOBuffer *buffer, int fd)
{

    int Nread, Nfree;

    if (fd == 0) {
        /* pipe is closed */
        return (0);
    }

    Nfree = buffer->nAlloc - buffer->n - 1;

    // extend the data block if needed
    if (Nfree < buffer->nBlock) {
        buffer->nAlloc += 2*buffer->nBlock + 1;
        buffer->data = psRealloc (buffer->data, buffer->nAlloc);
        Nfree = buffer->nAlloc - buffer->n;
        memset(buffer->data + buffer->n, '\0', Nfree);
    }

    // attempt to read from the fd into the buffer
    Nread = read (fd, &buffer->data[buffer->n], buffer->nBlock);

    if (Nread >= 0) {
        buffer->n += Nread;
        buffer->data[buffer->n] = 0;
        return (Nread);
    }

    // check on exit status (try again if waiting for non-blocking fd)
    if (Nread == -1) {
        switch (errno) {
        case EAGAIN:
        case EIO:
            /** no data available in pipe **/
            return (-1);
        default:
            /** error reading from pipe **/
            psError (PS_ERR_IO, true, "error on psIOBufferRead");
            return (-2);
        }
    }
    return (Nread);
}

/* read until buffer is empty (Nmax retries) */
int psIOBufferReadEmpty (psIOBuffer *buffer, int Nmax, int fd)
{

    int i, status;

    status = -1;
    for (i = 0; (status != 0) && (i < Nmax); i++) {
        status = psIOBufferRead (buffer, fd);
        if (status == -2)
            return false;
        if (status == -1)
            usleep (10000);
        if (status > 0)
            i = 0;
    }
    if (status == -1)
        return false;
    return true;
}
