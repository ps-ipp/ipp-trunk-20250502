#include <pslib.h>

#include "tap.h"

#define done() ok(psMemCheckLeaks(0, NULL, stdout, false) == 0, "Memory Leaks"); return exit_status()

        # define mem() ok(psMemCheckLeaks(psMemGetLastId(), NULL, stdout, false) == 0, "Memory Leaks")

        # define checkLeaks false

        # define checkMem() if(checkLeaks) mem()

            # ifdef __GNUC__

            // write a comment which is counted as a test (and swallowed by prove)
            # define note(A, ...) _gen_result(1, __func__, __FILE__, __LINE__, A, ## __VA_ARGS__);

// use to test the value of a float
# define ok_float(VALUE,EXPECT,COMMENT, ...)\
ok((fabsf((VALUE)-(EXPECT)) < FLT_EPSILON), COMMENT, ## __VA_ARGS__);

// use to test the value of a double
# define ok_double(VALUE,EXPECT,COMMENT, ...)\
ok((fabs((VALUE)-(EXPECT)) < DBL_EPSILON), COMMENT, ## __VA_ARGS__);

// use to test the value of a float within a defined tolerance
# define ok_float_tol(VALUE,EXPECT,TOL,COMMENT, ...)\
ok((fabsf((VALUE)-(EXPECT)) < (TOL)), COMMENT, ## __VA_ARGS__);

// use to test the value of a double within a defined tolerance
# define ok_double_tol(VALUE,EXPECT,TOL,COMMENT, ...)\
ok((fabs((VALUE)-(EXPECT)) < (TOL)), COMMENT, ## __VA_ARGS__);

# define ok_str(VALUE,EXPECT,COMMENT, ...)\
ok(strcmp(VALUE, EXPECT) == 0, COMMENT, ## __VA_ARGS__);

#elif __STDC_VERSION__ >= 199901L /* __GNUC__ */

// write a comment which is counted as a test (and swallowed by prove)
# define note(A, ...) _gen_result(1, __func__, __FILE__, __LINE__, A, ...);

// use to test the value of a float
# define ok_float(VALUE,EXPECT, ...)\
ok((fabsf((VALUE)-(EXPECT)) < FLT_EPSILON), __VA_ARGS__);

// use to test the value of a double
# define ok_double(VALUE,EXPECT, ...)\
ok((fabs((VALUE)-(EXPECT)) < DBL_EPSILON), __VA_ARGS__);

// use to test the value of a float
# define ok_float_tol(VALUE,EXPECT,TOL, ...)\
ok((fabsf((VALUE)-(EXPECT)) < (TOL)), __VA_ARGS__);

// use to test the value of a double
# define ok_double_tol(VALUE,EXPECT,TOL, ...)\
ok((fabs((VALUE)-(EXPECT)) < )(TOL)), __VA_ARGS__);

# define ok_str(VALUE,EXPECT, ...)\
ok(strcmp(VALUE, EXPECT) == 0, __VA_ARGS__);

#else /* __STDC_VERSION__ */
# error "Needs gcc or C99 compiler for variadic macros."
#endif /* __STDC_VERSION__ */
