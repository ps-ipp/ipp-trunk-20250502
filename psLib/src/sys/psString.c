
/** @file  psString.c
 *
 * -*- mode: C; c-basic-indent: 4; tab-width: 8; indent-tabs-mode: nil -*-
 * vim: set cindent ts=8 sw=4 expandtab:
 *
 *  @brief Contains the definition of string utility functions
 *
 *  String utility functions defined shall provide basic string copying
 *  capabilities while using the preferred memory management utilities.
 *
 *  @author Eric Van Alst, MHPCC
 *  @author David Robbins, MHPCC
 *
 *  @version $Revision: 1.61 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-08-15 00:45:18 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "psString.h"
#include "psError.h"
#include "psAssert.h"
#include "psAbort.h"
#include "psMemory.h"
#include "psConstants.h"

static void stringFree(psString string)
{
    // this function is only necessary so psMemCheckString has a function
    // pointer address to test against
}


// gcc (Ubuntu 20.04) complains when strncpy is used to copy a fraction of a buffer,
// potentially skipping the ending NULL.  To avoid the error, manually copy
// and (to ensure the buffer ends in a NULL) set the last + 1 byte to NUL: 
// WARNING : len(dest) must be >= n + 1
char *ps_strncpy_nowarn (char *dest, const char *src, size_t n) {

  size_t i;
  
  char *d = dest;
  char *s = (char *) src;
  for (i = 0; i < n && *s != 0; i++, d++, s++) { *d = *s; }
  for ( ; i <= n; i++, d++) { *d = 0; }
  
  return dest;
}

psString p_psStringAlloc(const char *file,
                         unsigned int lineno,
                         const char *func,
                         size_t nChar)
{
    psString string = p_psAlloc(file, lineno, func, nChar + 1);
    psMemSetDeallocator(string, (psFreeFunc)stringFree);

    return string;
}


psString p_psStringRealloc(const char *file,
			   unsigned int lineno,
			   const char *func,
			   psString string,
			   size_t nChar)
{
    if (!string) {
	string = p_psAlloc(file, lineno, func, nChar + 1);
	psMemSetDeallocator(string, (psFreeFunc)stringFree);
    } else {
        string = p_psRealloc(file, lineno, func, string, nChar + 1);
    }

    return string;
}


bool psMemCheckString(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return (psMemGetDeallocator(ptr) == (psFreeFunc)stringFree);
}


psString p_psStringCopy(const char *file,
                        unsigned int lineno,
                        const char *func,
                        const char *string)
{
    // Pass through NULL values!
    if (!string) {
        return NULL;
    }

    // Output string
    psString output = p_psStringAlloc(file, lineno, func, strlen(string));

    // Copy input string to memory just allocated
    return strcpy(output, string);
}


psString p_psStringNCopy(const char *file,
                         unsigned int lineno,
                         const char *func,
                         const char *string,
                         size_t nChar)
{
    // Pass through NULL values
    if (!string) {
        return NULL;
    }

    // Check the number of characters to copy is non-negative
    // XXX need to limit nChar to < memtotal
    if (nChar < 0) {
        // Log error message and return NULL
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Can not copy a negative number of characters (%zd)."),
                nChar);
        return NULL;
    }

    // Copy input string to memory allocated up to nChar characters
    psString output = p_psStringAlloc(file, lineno, func, nChar);
    output = strncpy(output, string, nChar);

    // Ensure the last byte is NULL character
    output[nChar] = '\0';

    // Return the string pointer
    return output;
}


ssize_t p_psStringAppend(const char *file,
                         unsigned int lineno,
                         const char *func,
                         psString *dest,
                         const char *format,
                         ...)
{
    PS_ASSERT_PTR_NON_NULL(dest, 0);
    PS_ASSERT_PTR(*dest, 0);
    PS_ASSERT_STRING_NON_EMPTY(format, 0);

    va_list ap;
    va_start(ap, format);
    ssize_t length = p_psStringAppendV(file, lineno, func, dest, format, ap);
    va_end(ap);

    return length;
}

ssize_t p_psStringAppendV(const char *file,
                          unsigned int lineno,
                          const char *func,
                          psString *dest,
                          const char *format,
                          va_list ap)
{
    PS_ASSERT_PTR_NON_NULL(dest, 0);
    PS_ASSERT_PTR(*dest, 0);
    PS_ASSERT_STRING_NON_EMPTY(format, 0);

    size_t          length = 0;             // complete string length (sans \0)
    size_t          oldLength = 0;          // original string length (sans \0)
    ssize_t         tailLength = 0;         // length of string to append

    if (*dest && psMemGetRefCounter(*dest) > 1) {
        psWarning("Appending to a string with multiple reference counts may corrupt memory!");
    }

    if (*dest) {
        oldLength = strlen(*dest);
    } else {
        oldLength = 0;
    }

    va_list         apCopy;

    // find the size of the string to append
    // C99 guarentees vsnprintf() to work as expected with size = 0
    // BUT: the output cannot be a NULL string
    va_copy(apCopy, ap);
    char tmp = 0;
    tailLength = vsnprintf(&tmp, 0, format, apCopy);
    va_end(apCopy);

    // if the new tail is zero length, return the length of the old string.  if
    // it's a format error, return the error code.
    if (tailLength < 1) {
        return tailLength == 0 ? oldLength : tailLength;
    }

    // realloc string to string + tail + \0
    if (*dest) {
        *dest = p_psRealloc(file, lineno, func, *dest, oldLength + tailLength + 1);
    } else {
        *dest = p_psStringAlloc(file, lineno, func, oldLength + tailLength + 1);
    }

    // append tail + \0
    // XXX this second call to va_copy() isn't strictly nessicary as the
    // calling function can't assume we won't modify the va_list.  However, we
    // have decided to error on the side of caution.
    va_copy(apCopy, ap);
    vsnprintf(*dest + oldLength, tailLength + 1, format, apCopy);
    va_end(apCopy);

    return length;
}


ssize_t p_psStringPrepend(const char *file,
                          unsigned int lineno,
                          const char *func,
                          psString *dest,
                          const char *format,
                          ...)
{
    PS_ASSERT_PTR_NON_NULL(dest, 0);
    PS_ASSERT_PTR(*dest, 0);
    PS_ASSERT_STRING_NON_EMPTY(format, 0);

    va_list ap;
    va_start(ap, format);
    ssize_t length = p_psStringPrependV(file, lineno, func, dest, format, ap);
    va_end(ap);

    return length;
}


ssize_t p_psStringPrependV(const char *file,
                           unsigned int lineno,
                           const char *func,
                           psString *dest,
                           const char *format,
                           va_list ap)
{
    PS_ASSERT_PTR_NON_NULL(dest, 0);
    PS_ASSERT_PTR(*dest, 0);
    PS_ASSERT_STRING_NON_EMPTY(format, 0);

    size_t          length;             // complete string length (sans \0)
    ssize_t         headLength;         // length of string to prepend
    char            *oldDest;           // copy of original string

    if (*dest && psMemGetRefCounter(*dest) > 1) {
        psWarning("Appending to a string with multiple reference counts may corrupt memory!");
    }

    if (!*dest) {
        // makes the string backup and concatination pointless
        *dest = p_psStringCopy(file, lineno, func, "");
        length = 0;
    } else {
        // size of existing string
        length = strlen(*dest);
    }

    va_list apCopy;

    // find the size of the string to prepend
    // C99 guarentees vsnprintf() to work as expected with size = 0
    va_copy(apCopy, ap);
    char tmp = 0;
    headLength = vsnprintf(&tmp, 0, format, apCopy);
    va_end(apCopy);

    // if the new head is zero length, return the length of the old string.  if
    // it's a format error, return the error code.
    if (headLength < 1) {
        return headLength == 0 ? length : headLength;
    }

    // backup original string
    oldDest = p_psStringCopy(file, lineno, func, *dest);

    // new string length (sans \0)
    length += headLength;

    // realloc string to head + string + \0
    *dest = p_psRealloc(file, lineno, func, *dest, length + 1);

    // copy the new head to the beginning of string
    // XXX this second call to va_copy() isn't strictly nessicary as the
    // calling function can't assume we won't modify the va_list.  However, we
    // have decided to error on the side of caution.
    va_copy(apCopy, ap);
    vsnprintf(*dest, length + 1, format, apCopy);
    va_end(apCopy);

    // append the original string
    strncat(*dest, oldDest, length + 1);

    psFree(oldDest);

    return length;
}


// split the string by the given splitters
// NULL input string returns empty (not NULL) list
// NULL splitters is an error
psList *p_psStringSplit(const char *file,
                        unsigned int lineno,
                        const char *func,
                        const char *string,
                        const char *splitters,
                        bool multipleAreSignificant)
{
    PS_ASSERT_STRING_NON_EMPTY(splitters, NULL);

    psList *values = p_psListAlloc(file, lineno, func, NULL); // The list of values to return
    // An input NULL string should not generate an error: it is a valid case
    if (string == NULL) {
        return values;
    }

    char *next = NULL;
    char *current = (char *) string;
    while ((next = strpbrk (current, splitters)) != NULL) {

        // are multiple splitters in-a-row significant?
        if ((next == current) && !multipleAreSignificant) {
            current ++;
            continue;
        }

        // Copy the current word
        psString word = p_psStringNCopy(file, lineno, func, current, next - current);
        psListAdd(values, PS_LIST_TAIL, word);
        psFree(word);

        current = next + 1;
    }

    if (strlen(current) > 0) {
        // Copy the last word
        psString word = p_psStringCopy(file, lineno, func, current);
        (void)psListAdd(values, PS_LIST_TAIL, word);
        psFree(word);
    }

    return values;
}


// given the input string, search for all copies of the key, and replace with the replacement value
// the input string may be freed if not needed
ssize_t psStringSubstitute(psString *input,
                           const char *replace,
                           const char *key)
{
    PS_ASSERT_PTR_NON_NULL(input, 0);

    // Empty input gives empty output
    if (!*input || strlen(*input) == 0) {
        return 0;
    }
    // No key gives the same as the input
    if (!key || strlen(key) == 0) {
        return strlen(*input);
    }
    if (!psMemCheckString(*input)) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "This function requires a psString as input.\n");
        return 0;
    }


    // replace == NULL is valid: it just means that we strip out the key
    // without putting anything else in its place
    size_t replaceLength;               // Size of "replace" string
    if (replace) {
        replaceLength = strlen(replace);
    } else {
        replaceLength = 0;
    }

    char *search = *input;              // Search this string
    while (true) {
        char *found = strstr(search, key); // Found occurence of the key
        if (!found) {
            return strlen(*input);
        }

        // we have input = xxxkeyxxx
        // we want output = xxxreplacexxx

        // this is safe since we will subtract strlen(key) elements from input
        psString output = psStringAlloc(strlen(*input) + replaceLength + 1); // Output string
        int numChar = found - *input;    // Number of characters to copy

        // copy the first segement into 'output'
        strncpy(output, *input, numChar);

        // copy the key replacement to the start of the key
        if (replace && replaceLength > 0) {
            strcpy(output + numChar, replace);
            numChar += replaceLength;
        }

        // copy the remainder to the end of the replacement
        strcpy(output + numChar, found + strlen(key));

        psFree(*input);
        *input = output;
        search = output + numChar;
    }

    psAbort("Should never get here.\n");
    return 0;
}


psArray *psStringSplitArray(const char *string, const char *splitters, bool multi)
{

    psList *list = psStringSplit(string, splitters, multi);
    psArray *array = psListToArray(list);
    psFree (list);
    return array;
}


#ifndef whitespace
#define whitespace(c) (((c) == ' ') || ((c) == '\t'))
#endif

/* Strip whitespace from the start and end of STRING. */
size_t psStringStrip(char *string)
{
    long i;

    if (!string || strlen(string) == 0) {
        return 0;
    }

    for (i = 0; i < strlen(string) && whitespace(string[i]); i++)
        ; // No action
    if (i) {
        memmove (string, string + i, strlen(string+i)+1);
    }
    for (i = strlen (string) - 1; (i > 0) && whitespace(string[i]); i--)
        ; // No action
    string[++i] = 0;

    return i;
}


/*
 * Used by PS_FILE_LINE
 */
const char *p_psFileLine(const char *file, int line)
{
    static char msg[100];
    sprintf(msg, "%s:%d", file, line);
    return msg;
}


psString psStringFileBasename (const char *fullname) {
 
    char *file;

    const char *ptr = strrchr (fullname, '/');
    if (ptr) {
	file = psStringCopy(ptr + 1);
    } else {
	file = psStringCopy(fullname);
    }
  return (file);
}

psString psStringStripCVS(const char *string, const char *tagName)
{
    PS_ASSERT_STRING_NON_EMPTY(string, NULL);
    PS_ASSERT_STRING_NON_EMPTY(tagName, NULL);

    psString tagString = NULL;          // Tag string, e.g., "$Name:" from "Name"
    psStringAppend(&tagString, "$%s:", tagName);
    char *p = strstr(string, tagString); // The start of the real tag
    psFree(tagString);
    if (!p) {
        return psStringCopy("UNKNOWN");
    }

    psString tag = psStringCopy(string + 6); // The tag, without the leading tagString
    p = strrchr(tag, '$');              // The closing dollar sign
    if (p) {
        *p = 0;
    }
    psStringStrip(tag);
    if (*tag == 0) {
        psFree(tag);
        return psStringCopy("UNKNOWN");
    }

    return tag;
}

// several snprintf statements below my truncate their output
// gcc (since 8.1) warns if the output may be truncated.
// the following NOOP function is used to fool the compiler
// see ps_snprintf_nowarn in psString.h
void psNOOP (void) { }
