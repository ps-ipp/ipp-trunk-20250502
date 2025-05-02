/*
 * xmalloc.h - malloc that can't fail
 *
 * Copyright (C) 2003-2005  Joshua Hoblitt, 2003  Robert Lupton
 *
 * $Id: xmalloc.h,v 1.5 2008-09-15 21:24:30 jhoblitt Exp $
 */

#ifndef XMALLOC_H
#define XMALLOC_H 1

#define xmalloc(size) \
x_xmalloc(__FILE__, __LINE__, __func__, size)

void * x_xmalloc(
    const char *file,                  ///< File of caller
    unsigned int lineno,               ///< Line number of caller
    const char *func,                  ///< Function name of caller
    size_t n
#ifdef __GNUC__
) __attribute__((malloc));
# else // ifdef __GNUC__
);
#endif // ifdef __GNUC__


#define xrealloc(ptr, size) \
x_xrealloc(__FILE__, __LINE__, __func__, ptr, size)

void * x_xrealloc(
    const char *file,                  ///< File of caller
    unsigned int lineno,               ///< Line number of caller
    const char *func,                  ///< Function name of caller
    void *ptr,
    size_t size
#ifdef __GNUC__
) __attribute__((malloc));
# else // ifdef __GNUC__
);
#endif // ifdef __GNUC__
 
#define xfree(ptr) \
x_xfree(__FILE__, __LINE__, __func__, ptr)

void x_xfree(
    const char *file,                  ///< File of caller
    unsigned int lineno,               ///< Line number of caller
    const char *func,                  ///< Function name of caller
    void *ptr
);

#endif // XMALLOC_H
