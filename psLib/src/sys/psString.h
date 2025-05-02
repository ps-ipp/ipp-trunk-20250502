/* @file  psString.h
 *
 * @brief string utility functions
 *
 * String utility functions defined shall provide basic string copying
 * capabilities.
 *
 * @author EAM, IfA
 * @author Eric Van Alst, MHPCC
 * @author David Robbins, MHPCC
 * @author Joshua Hoblitt, University of Hawaii
 *
 * @version $Revision: 1.44 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-11-09 00:47:41 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_STRING_H
#define PS_STRING_H

/// @addtogroup SysUtils System Utilities
/// @{

#include <sys/types.h>
#include <stdarg.h>

#include "psType.h"
#include "psList.h"

/** This macro will convert the argument to a quoted string */
#define PS_STRING(S)  #S

/** This macro returns a (static buffer containing) "file:line" */
const char *p_psFileLine(const char *file, int line)
#ifdef __GNUC__
__attribute__((deprecated))
#endif // ifdef __GNUC__
;
#define PS_FILE_LINE p_psFileLine(__FILE__,__LINE__)

// gcc (since 8.1) warns if the output may be truncated by snprint.
// since that is sometimes a desired behavior, this version of snprintf
// is defined to fool the compiler about this warning:
// the following NOOP function is used to fool the compiler
// see snprintf_nowarn in ohana.h
#define ps_snprintf_nowarn(...) (snprintf(__VA_ARGS__) < 0 ? psNOOP() : 0)
void psNOOP (void);

char *ps_strncpy_nowarn (char *dest, const char *src, size_t n);

// some constants to use in snprintf statements and variable definitions to ensure
// consistency
# define PS_SMALLWORD 16
# define PS_BIGWORD 128

/** Allocates a new psString.
 *
 *  @return psString:       Newly allocated string of length n.
 */
#ifdef DOXYGEN
psString psStringAlloc(
    size_t nChar                        ///< Size of psString to allocate.
);
#else // ifdef DOXYGEN
psString p_psStringAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    size_t nChar                        ///< Size of psString to allocate.
) PS_ATTR_MALLOC;
#define psStringAlloc(nChar) \
      p_psStringAlloc(__FILE__, __LINE__, __func__, nChar)
#endif // ifdef DOXYGEN

/** Reallocate an existing psString (or alloc if not existent)
 *
 *  @return psString:       string of length n.
 */
#ifdef DOXYGEN
psString psStringRealloc(
    psString string,
    size_t nChar                        ///< Size of psString to allocate.
);
#else // ifdef DOXYGEN
psString p_psStringRealloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psString string,			///< supplied string or NULL
    size_t nChar                        ///< Size of psString to allocate.
) PS_ATTR_MALLOC;
#define psStringRealloc(string, nChar)				\
    p_psStringRealloc(__FILE__, __LINE__, __func__, string, nChar)
#endif // ifdef DOXYGEN

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr
 *  datatype.
 *
 *  @return bool:       True if the pointer matches a psString structure, false otherwise.
 */
bool psMemCheckString(
    psPtr ptr                          ///< the pointer whose type to check
);


/** Copies the input string
 *
 *  This function shall allocate memory to the length of the input string plus
 *  one and copy the input string to the newly allocated memory.  If 'string'
 *  is 'NULL' then 'NULL' is returned.
 *
 *  @return psString:      Copy of input string
 */
#ifdef DOXYGEN
psString psStringCopy(
    const char *string                  ///< Input string of characters to copy
);
#else // ifdef DOXYGEN
psString p_psStringCopy(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    const char *string                  ///< Input string of characters to copy
);
#define psStringCopy(string) \
      p_psStringCopy(__FILE__, __LINE__, __func__, string)
#endif // ifdef DOXYGEN


/** Copies the input string up to the specified number of characters
 *
 *  This function shall allocate memory to the length specified by nChar
 *  plus one and copy the input string to the newly allocated memory.
 *  This function will only copy nChar bytes from the input to new string,
 *  so if the input string is larger than nChar characters the copied
 *  string will be a substring of the input string.  If the input string
 *  is smaller than nChar bytes then the remaining bytes allocated will
 *  be set to NULL.  If 'string' is 'NULL' then 'NULL' is returned.
 *
 *  @return  psString:   Copy of input string
 */
#ifdef DOXYGEN
psString psStringNCopy(
    const char *string,                 ///< Input string of characters to copy
    size_t nChar                        ///< Number of bytes to allocate for string copy
);
#else // ifdef DOXYGEN
psString p_psStringNCopy(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    const char *string,                 ///< Input string of characters to copy
    size_t nChar                        ///< Number of bytes to allocate for string copy
);
#define psStringNCopy(string, nChar) \
      p_psStringNCopy(__FILE__, __LINE__, __func__, string, nChar)
#endif // ifdef DOXYGEN


/** Appends a format onto a string
 *
 * This function shall allocate a new string if dest is NULL.  dest shall be
 * automatically extended to the size of the new string.
 *
 * @return ssize_t:     The length of the new string (excluding '\0')
 */
#ifdef DOXYGEN
ssize_t psStringAppend(
    psString *dest,                     ///< existing string
    const char *format,                 ///< format to append
    ...                                 ///< format arguments
);
#else // ifdef DOXYGEN
ssize_t p_psStringAppend(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psString *dest,                     ///< existing string
    const char *format,                 ///< format to append
    ...                                 ///< format arguments
) PS_ATTR_FORMAT(printf, 5, 6);
#define psStringAppend(dest, ...) \
      p_psStringAppend(__FILE__, __LINE__, __func__, dest, __VA_ARGS__)
#endif // ifdef DOXYGEN


/** Appends a format onto a string
 *
 * This function shall allocate a new string if dest is NULL.  dest shall be
 * automatically extended to the size of the new string.
 *
 * @return ssize_t:     The length of the new string (excluding '\0')
 */
#ifdef DOXYGEN
ssize_t psStringAppendV(
    psString *dest,                     ///< existing string
    const char *format,                 ///< format to append
    va_list ap                          ///< va_list of format arguments
);
#else // ifdef DOXYGEN
ssize_t p_psStringAppendV(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psString *dest,                     ///< existing string
    const char *format,                 ///< format to append
    va_list ap                          ///< va_list of format arguments
);
#define psStringAppendV(dest, format, ap) \
      p_psStringAppendV(__FILE__, __LINE__, __func__, dest, format, ap)
#endif // ifdef DOXYGEN


/** Prepends a format onto a string
 *
 * This function shall allocate a new string if dest is NULL.  dest shall be
 * automatically extended to the size of the new string.
 *
 * @return ssize_t:     The length of the new string (excluding '\0')
 */
#ifdef DOXYGEN
ssize_t psStringPrepend(
    psString *dest,                     ///< existing string
    const char *format,                 ///< format to append
    ...                                 ///< format arguments
);
#else // ifdef DOXYGEN
ssize_t p_psStringPrepend(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psString *dest,                     ///< existing string
    const char *format,                 ///< format to append
    ...                                 ///< format arguments
) PS_ATTR_FORMAT(printf, 5, 6);
#define psStringPrepend(dest, ...) \
      p_psStringPrepend(__FILE__, __LINE__, __func__, dest, __VA_ARGS__)
#endif // ifdef DOXYGEN


/** Prepends a format onto a string
 *
 * This function shall allocate a new string if dest is NULL.  dest shall be
 * automatically extended to the size of the new string.
 *
 * @return ssize_t:     The length of the new string (excluding '\0')
 */
#ifdef DOXYGEN
ssize_t psStringPrependV(
    psString *dest,                     ///< existing string
    const char *format,                 ///< format to append
    va_list ap                          ///< va_list of format arguments
);
#else // ifdef DOXYGEN
ssize_t p_psStringPrependV(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psString *dest,                     ///< existing string
    const char *format,                 ///< format to append
    va_list ap                          ///< va_list of format arguments
);
#define psStringPrependV(dest, format, ap) \
      p_psStringPrependV(__FILE__, __LINE__, __func__, dest, format, ap)
#endif // ifdef DOXYGEN


/** Procedure to split the input string into a psList of psStrings.
 *
 *  The string is split at any one of the characters in splitters.  Split
 *  strings of zero length should not be included in the output list.
 *
 *  @return psList*:    The list of (split) psStrings.
 */
#ifdef DOXYGEN
psList *psStringSplit(
    const char *string,                ///< String to split
    const char *splitters,             ///< Characters on which to split
    bool multipleAreSignificant        ///< Are multiple occurences significant?
);
#else // ifdef DOXYGEN
psList *p_psStringSplit(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    const char *string,                ///< String to split
    const char *splitters,             ///< Characters on which to split
    bool multipleAreSignificant        ///< Are multiple occurences significant?
    );
#define psStringSplit(string, splitters, multiple) \
      p_psStringSplit(__FILE__, __LINE__, __func__, string, splitters, multiple)
#endif // ifdef DOXYGEN

/** Procedure to split the input string into a psArray of psStrings.
 *
 *  The string is split at any one of the characters in splitters.  Split
 *  strings of zero length should not be included in the output list.
 *
 *  @return psArray*:       The array of (split) psStrings.
 */
psArray *psStringSplitArray(
    const char *string,                ///< String to split
    const char *splitters,             ///< Characters on which to split
    bool multipleAreSignificant        ///< Are multiple occurences significant?
);

// Given the input string, search for all copies of the key, and replace with
// the replacement value the input string may be freed if not needed
/** Procedure to search an input string and substitute strings where desired.
 *
 *  The input string is searched for all instances of the key, which is then
 *  replaced with the replacement value wherever found.  The input string may
 *  be freed if not needed.
 *
 *  @return ssize_t:      the length of the new string (excluding '\0')
 */
ssize_t psStringSubstitute (
    psString *input,                    ///< ptr to input string to be modified
    const char *replace,                ///< replacement value
    const char *key                     ///< string to be replaced in input
);


// strip whitespace from head and tail of string
/** Procedure to strip the whitespace from the head and tail of a string.
 *
 *  @return size_t:         Number of whitespaces removed.
 */
size_t psStringStrip(
    char *string                       ///< input string to be stripped.
);


/// Given a CVS keyword string, strip off the CVS-specific keyword to get the value
psString psStringStripCVS(const char *string, ///< The string, something like "$CVSKEYWORD: Value$"
                          const char *tagName ///< The name of the tag to remove, something like "CVSKEYWORD"
                         );

#define PS_ASSERT_STRING_NON_EMPTY(NAME, RVAL) \
if (!(NAME) || strlen(NAME) == 0) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: String %s is NULL or empty.", \
            #NAME); \
    return RVAL; \
}

char *psStrcasestr (const char *haystack, const char *needle);

psString psStringFileBasename (const char *fullname);

/// @}
#endif // #ifndef PS_STRING_H
