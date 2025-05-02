#include <math.h>


#include <pslib.h>

#include "tap.h"

#define done() ok(psMemCheckLeaks(0, NULL, stdout, false) == 0, "Memory Leaks"); \
return exit_status()

# define mem() ok(psMemCheckLeaks(psMemGetLastId(), NULL, stdout, false) == 0, "Memory Leaks")

# define checkLeaks false

# define checkMem() if(checkLeaks) mem()

// write a comment which is counted as a test (and swallowed by prove)
# define note(...)\
{ \
    fprintf(stdout, __VA_ARGS__); \
    fprintf(stdout, "\n[%s:%d in %s]\n", __FILE__, __LINE__, __func__); \
}

# define noted(...) _gen_result(1, __func__, __FILE__, __LINE__, __VA_ARGS__);

// use to test the value of a float
# define is_float(VALUE, EXPECT, ...) \
{ \
    bool status = false; \
    if (isnan(EXPECT)) { \
        status = isnan(VALUE); \
    } else { \
        status = (fabsf(VALUE - EXPECT) < FLT_EPSILON); \
    } \
    ok(status, __VA_ARGS__); \
    if (!status) { \
        diag("         got: '%f'", VALUE); \
        diag("    expected: '%f'", EXPECT); \
    } \
}


// use to test the value of a float within a defined tolerance
# define is_float_tol(VALUE,EXPECT,TOL,...)\
{ \
    bool status = false; \
    if (isnan(EXPECT)) { \
        status = isnan(VALUE); \
    } else { \
        status = (fabsf((VALUE) - (EXPECT)) < (TOL)); \
    } \
    ok(status, __VA_ARGS__); \
    if (!status) { \
        diag("         got: '%f'", VALUE); \
        diag("    expected: '%f' +/- %f", EXPECT, TOL); \
    } \
}


// use to test the value of a double
# define is_double(VALUE,EXPECT,...)\
{ \
    bool status = false; \
    if (isnan(EXPECT)) { \
        status = isnan(VALUE); \
    } else { \
        status = (fabs(VALUE - EXPECT) < DBL_EPSILON); \
    } \
    ok(status, __VA_ARGS__); \
    if (!status) { \
        diag("         got: '%.10f'", VALUE); \
        diag("    expected: '%.10f'", EXPECT); \
    } \
}


// use to test the value of a double
# define is_double_tol(VALUE,EXPECT,TOL, ...)\
{ \
    bool status = false; \
    if (isnan(EXPECT)) { \
        status = isnan(VALUE); \
    } else { \
        status = (fabs((VALUE) - (EXPECT)) < (TOL)); \
    } \
    ok(status, __VA_ARGS__); \
    if (!status) { \
        diag("         got: '%.10f'", VALUE); \
        diag("    expected: '%.10f' +/- %.10f", EXPECT, TOL); \
    } \
}


# define is_str(VALUE, EXPECT, ...) \
{ \
    int cmp = (strcmp(VALUE, EXPECT) == 0); \
    ok(cmp, __VA_ARGS__); \
    if (!cmp) { \
        diag("         got: '%s'", VALUE); \
        diag("    expected: '%s'", EXPECT); \
    } \
}


# define is_strn(VALUE, EXPECT, N, ...)\
{ \
    int cmp = (strncmp(VALUE, EXPECT, N) == 0); \
    ok(cmp, __VA_ARGS__); \
    if (!cmp) { \
        diag("         got: '%s'", VALUE); \
        diag("    expected: '%s'", EXPECT); \
    } \
}


# define is_int(VALUE, EXPECT, ...)\
{ \
    int cmp = (VALUE == EXPECT); \
    ok(cmp, __VA_ARGS__); \
    if (!cmp) { \
        diag("         got: '%d'", VALUE); \
        diag("    expected: '%d'", EXPECT); \
    } \
}


# define is_long(VALUE, EXPECT, ...)\
{ \
    int cmp = (VALUE == EXPECT); \
    ok(cmp, __VA_ARGS__); \
    if (!cmp) { \
        diag("         got: '%ld'", VALUE); \
        diag("    expected: '%ld'", EXPECT); \
    } \
}

# define is_bool(VALUE, EXPECT, ...)\
{ \
    int cmp = (VALUE == EXPECT); \
    ok(cmp, __VA_ARGS__); \
    if (!cmp) { \
        diag("         got: '%s'", VALUE ? "true" : "false"); \
        diag("    expected: '%s'", EXPECT ? "true" : "false"); \
    } \
}

# define ok_float      is_float     
# define ok_float_tol  is_float_tol 
# define ok_double     is_double    
# define ok_double_tol is_double_tol
# define ok_str	       is_str	      
# define ok_strn       is_strn      
# define ok_int	       is_int	      
# define ok_long       is_long      
# define ok_bool       is_bool      
