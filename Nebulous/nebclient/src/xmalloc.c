/*
 * xmalloc.c - malloc that can't fail
 *
 * Copyright (C) 2003-2005  Joshua Hoblitt, 2003  Robert Lupton
 *
 * $Id: xmalloc.c,v 1.4 2008-09-15 21:24:30 jhoblitt Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include "xmalloc.h"

//#define USE_EFF 1
#ifdef USE_EFF
#include "efence.h"
#endif

/*****************************************************************************/
/*
 * Wrappers for malloc/free.  xmalloc cannot fail.
 */
void * x_xmalloc(
                const char *file,
                unsigned int lineno,
                const char *func,
                size_t n)
{
#ifdef USE_EFF
   void *ptr = _eff_malloc(n, file, lineno);
#else
   void *ptr = malloc(n);
#endif
    
   if (ptr == NULL) {
      perror("malloc");
      exit(EXIT_FAILURE);
   }
 
   return(ptr);
}

void * x_xrealloc(
                const char *file,
                unsigned int lineno,
                const char *func,
                void *ptr,
                size_t size)
{
#ifdef USE_EFF
    void *newptr = _eff_realloc(ptr, size, file, lineno);
#else
    void *newptr = realloc(ptr, size);
#endif

    if (!newptr) {
        perror("realloc");
        exit(EXIT_FAILURE);
    } 

    return(newptr);
}
 
void x_xfree(
            const char *file,
            unsigned int lineno,
            const char *func,
            void *ptr)
{
#ifdef USE_EFF
    _eff_free(ptr, file, lineno);
#else
   free(ptr);
#endif
}
