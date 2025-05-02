# include <ohana.h>
# include <gfitsio.h>
# include "tap_ohana.h"

# define VERBOSE 0
# define NSRC 10
# define NTGT 20
# define NOUT 125

# define SWAP_DBLE { \
  char tmp; \
  tmp = Pout[0]; Pout[0] = Pout[7]; Pout[7] = tmp; \
  tmp = Pout[1]; Pout[1] = Pout[6]; Pout[6] = tmp; \
  tmp = Pout[2]; Pout[2] = Pout[5]; Pout[5] = tmp; \
  tmp = Pout[3]; Pout[3] = Pout[4]; Pout[4] = tmp; }

int main (void) {
  
  plan_tests (14);

  diag ("libfits keyword tests");

  { 
    
    // create an empty header
    char *ptr;
    Header header;
    gfits_init_header (&header);
    gfits_create_header (&header);

    // note that the compiler forces format elements and arguments to match
    ok (!gfits_modify (&header, "TEST", "as %s", 1, "test line 7"), "skipped the bad mode");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // set a few string keywords
    ok (gfits_modify (&header, "S",        "%s", 1, "test line 0"), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "S1",       "%s", 1, "test line 1"), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "S12",      "%s", 1, "test line 2"), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "S123",     "%s", 1, "test line 3"), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "S1234",    "%s", 1, "test line 4"), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "S12345",   "%s", 1, "test line 5"), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "S123456",  "%s", 1, "test line 6"), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "S1234567", "%s", 1, "test line 7"), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // set a few numerical keywords
    ok (gfits_modify (&header, "D",        "%d", 1, 1), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "D1",       "%d", 1, 2), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "D12",      "%d", 1, 3), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "D123",     "%d", 1, 4), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "D1234",    "%d", 1, 5), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "D12345",   "%d", 1, 6), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "D123456",  "%d", 1, 7), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "D1234567", "%d", 1, 8), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // set one of each of the few numerical types (ints)
    ok (gfits_modify (&header, "D_INT", "%d", 1, 0x100), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "D_LONG", "%ld", 1, (long) 0x100), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "D_LONG2", "%lld", 1, (long long) 0x100), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // this is not ISO C99:
    // ok (gfits_modify (&header, "D_LONG3", "%Ld", 1, (long long) 0x100), "wrote a keyword");
    // ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // set one of each of the few numerical types (unsigned ints)
    ok (gfits_modify (&header, "DU_INT", "%u", 1, 0x100), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "DU_LONG", "%lu", 1, (long) 0x100), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "DU_LONG2", "%llu", 1, (long long) 0x100), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // ok (gfits_modify (&header, "DU_LONG3", "%Lu", 1, (long long) 0x100), "wrote a keyword");
    // ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "D_SHORT", "%hd", 1, 0x20), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "D_INTMAX", "%jd", 1, (intmax_t) 0x20000), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // set one of ehac of the few numerical types (floats)
    ok (gfits_modify (&header, "F_FLT", "%f", 1, 5.5), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "F_DBL", "%lf", 1, 5.5), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // note that the %e, %g varients are only valid for write not read
    ok (gfits_modify (&header, "E_FLT", "%e", 1, 5.5), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "E_DBL", "%le", 1, 5.5), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "G_FLT", "%g", 1, 5.5), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_modify (&header, "G_DBL", "%lg", 1, 5.5), "wrote a keyword");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // now attempt to read the things we wrote
    char line[128];
    int Ivalue;
    long Jvalue;
    long long Kvalue;
    unsigned int Uvalue;
    unsigned long Ulong;
    unsigned long long Ulonglong;
    short Ishort;
    intmax_t Imax;
    float Fvalue;
    double Dvalue;

    ok (gfits_scan (&header, "S",        "%s", 1, line), "read a keyword");
    ok (!strcmp (line, "test line 0"), "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "S1",       "%s", 1, line), "read a keyword");
    ok (!strcmp (line, "test line 1"), "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "S12",      "%s", 1, line), "read a keyword");
    ok (!strcmp (line, "test line 2"), "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "S123",     "%s", 1, line), "read a keyword");
    ok (!strcmp (line, "test line 3"), "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "S1234",    "%s", 1, line), "read a keyword");
    ok (!strcmp (line, "test line 4"), "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "S12345",   "%s", 1, line), "read a keyword");
    ok (!strcmp (line, "test line 5"), "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "S123456",  "%s", 1, line), "read a keyword");
    ok (!strcmp (line, "test line 6"), "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "S1234567", "%s", 1, line), "read a keyword");
    ok (!strcmp (line, "test line 7"), "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // set a few numerical keywords
    ok (gfits_scan (&header, "D",        "%d", 1, &Ivalue), "read a keyword");
    ok (Ivalue == 1, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "D1",       "%d", 1, &Ivalue), "read a keyword");
    ok (Ivalue == 2, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "D12",      "%d", 1, &Ivalue), "read a keyword");
    ok (Ivalue == 3, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "D123",     "%d", 1, &Ivalue), "read a keyword");
    ok (Ivalue == 4, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "D1234",    "%d", 1, &Ivalue), "read a keyword");
    ok (Ivalue == 5, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "D12345",   "%d", 1, &Ivalue), "read a keyword");
    ok (Ivalue == 6, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "D123456",  "%d", 1, &Ivalue), "read a keyword");
    ok (Ivalue == 7, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "D1234567", "%d", 1, &Ivalue), "read a keyword");
    ok (Ivalue == 8, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // set one of each of the few numerical types (ints)
    ok (gfits_scan (&header, "D_INT", "%d", 1, &Ivalue), "read a keyword");
    ok (Ivalue == 0x100, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "D_LONG", "%ld", 1, &Jvalue), "read a keyword");
    ok (Jvalue == 0x100, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "D_LONG2", "%lld", 1, &Kvalue), "read a keyword");
    ok (Kvalue == 0x100, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // ok (gfits_scan (&header, "D_LONG3", "%Ld", 1, &Kvalue), "read a keyword");
    // ok (Kvalue == 0x100, "got the right value");
    // ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // set one of each of the few numerical types (unsigned ints)
    ok (gfits_scan (&header, "DU_INT", "%u", 1, &Uvalue), "read a keyword");
    ok (Uvalue == 0x100, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "DU_LONG", "%lu", 1, &Ulong), "read a keyword");
    ok (Ulong == 0x100, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "DU_LONG2", "%llu", 1, &Ulonglong), "read a keyword");
    ok (Ulonglong == 0x100, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // ok (gfits_scan (&header, "DU_LONG3", "%Lu", 1, &Ulonglong), "read a keyword");
    // ok (Ulonglong == 0x100, "got the right value");
    // ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "D_SHORT", "%hd", 1, &Ishort), "read a keyword");
    ok (Ishort == 0x20, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "D_INTMAX", "%jd", 1, &Imax), "read a keyword");
    ok (Imax == 0x20000, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    // set one of ehac of the few numerical types (floats)
    ok (gfits_scan (&header, "F_FLT", "%f", 1, &Fvalue), "read a keyword");
    ok (Fvalue == 5.5, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "F_DBL", "%lf", 1, &Dvalue), "read a keyword");
    ok (Dvalue == 5.5, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "E_FLT", "%f", 1, &Fvalue), "read a keyword");
    ok (Fvalue == 5.5, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "E_DBL", "%lf", 1, &Dvalue), "read a keyword");
    ok (Dvalue == 5.5, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "G_FLT", "%f", 1, &Fvalue), "read a keyword");
    ok (Fvalue == 5.5, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");

    ok (gfits_scan (&header, "G_DBL", "%lf", 1, &Dvalue), "read a keyword");
    ok (Dvalue == 5.5, "got the right value");
    ptr = gfits_header_field (&header, "END", 1); ok (ptr, "END still good");
  }

  { 
    // test some error conditions:

    // header without "END"
  }

  return exit_status();
}
