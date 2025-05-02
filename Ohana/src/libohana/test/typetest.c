# include "ohana.h"
# include "tap_ohana.h"

enum
  {
    OHANA_LITTLE_ENDIAN = 0x03020100ul,
    OHANA_BIG_ENDIAN    = 0x00010203ul,
  };

static const union { 
  unsigned char bytes[4]; 
  int value; 
} ohana_host_order = { { 0, 1, 2, 3 } };

# define OHANA_HOST_ORDER (ohana_host_order.value)

// we need to verify some type-related issues:
int main (void) {

  plan_tests (4);

  diag ("libohana typetest.c tests");

  int status = 0;

  // 1) have we defined BYTE_SWAP correctly?

# ifdef BYTE_SWAP
  ok (OHANA_HOST_ORDER == OHANA_LITTLE_ENDIAN, "BYTE_SWAP defined on small endian hardware (see libohana/include/ohana.h)");
# else
  ok (OHANA_HOST_ORDER == OHANA_BIG_ENDIAN, "BYTE_SWAP NOT defined on big endian hardware (see libohana/include/ohana.h)");
# endif 

  // 2) have we defined NAN correctly?
  {
    float fvalue;
    double dvalue;
    
    fvalue = NAN;
    dvalue = NAN;

    ok (isnan(fvalue), "float  NAN correctly defined"); 
    ok (isnan(dvalue), "double NAN correctly defined"); 

# ifdef __STDC_VERSION__
    diag ("STDC_VERSION: %ld\n", __STDC_VERSION__);
# else
    diag ("STDC_VERSION is not set");
# endif
  }

  // 3) have we defined OFF_T_FMT correctly?  this test should actually raise a compile-time error
  off_t big_value = 10;

  char line[80];
  status = snprintf (line, 80, "this is a big int: "OFF_T_FMT" n'est pas?", big_value);
  //                            1234567890123456789         0123456789012
  ok (status == 32, "correctly formatted an off_t int with %d chars", status);

  // we could move the % out of the FMT and allow constructions like this:
  // # define OFF_T_FMT "jd"
  // fprintf (stderr, "this is a big int: %06"OFF_T_FMT_ALT" n'est pas?\n", big_value);
  // still kind of ugly...

  fprintf (stderr, "max float:  %e\n", FLT_MAX);
  fprintf (stderr, "max double: %e\n", DBL_MAX);
  fprintf (stderr, "min float:  %e\n", FLT_MIN);
  fprintf (stderr, "min double: %e\n", DBL_MIN);

  return exit_status();
}
