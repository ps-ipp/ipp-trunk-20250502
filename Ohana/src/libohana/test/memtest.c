# include "ohana.h"
# include "tap_ohana.h"

int main (void) {

  plan_tests (64);

  diag ("libohana memtest.c tests");

  {
    // int status;
    char *data;
    OhanaMemstats memstats;

    // does ALLOCATE work?
    ALLOCATE (data, char, 100);
    memstats = ohana_memstats(0);

    skip_start (!memstats.exists, 16, "skipping all tests : memory management not enabled");

    ok (memstats.Ntotal ==   1, "allocated a block");
    ok (memstats.Nbytes == 100, "allocated correct amount");
    ok (memstats.Ngood  ==   1, "block is good");
    ok (memstats.Nbad   ==   0, "no blocks are bad");
    
    // does REALLOCATE work?
    REALLOCATE (data, char, 1000);
    memstats = ohana_memstats(0);

    ok (memstats.Ntotal ==    1, "allocated a block");
    ok (memstats.Nbytes == 1000, "allocated correct amount");
    ok (memstats.Ngood  ==    1, "block is good");
    ok (memstats.Nbad   ==    0, "no blocks are bad");
    
    // does FREE work?
    free (data);
    memstats = ohana_memstats(0);

    ok (memstats.Ntotal ==   0, "freed block");
    ok (memstats.Nbytes ==   0, "kept correct amount");
    ok (memstats.Ngood  ==   0, "block is good");
    ok (memstats.Nbad   ==   0, "no blocks are bad");
    
    skip_end();
  }

  {
    // int status;
    char *data1, *data2, *data3, *data4;
    OhanaMemstats memstats;

    // does ALLOCATE work?
    ALLOCATE (data1, char, 100);
    ALLOCATE (data2, char, 100);
    ALLOCATE (data3, char, 100);
    ALLOCATE (data4, char, 100);
    memstats = ohana_memstats(0);

    skip_start (!memstats.exists, 16, "skipping all tests : memory management not enabled");

    ok (memstats.Ntotal ==   4, "allocated a block");
    ok (memstats.Nbytes == 400, "allocated correct amount");
    ok (memstats.Ngood  ==   4, "block is good");
    ok (memstats.Nbad   ==   0, "no blocks are bad");
    
    // does REALLOCATE work?
    REALLOCATE (data1, char, 1000);
    REALLOCATE (data2, char, 1000);
    REALLOCATE (data3, char, 1000);
    REALLOCATE (data4, char, 1000);
    memstats = ohana_memstats(0);

    ok (memstats.Ntotal ==    4, "allocated a block");
    ok (memstats.Nbytes == 4000, "allocated correct amount");
    ok (memstats.Ngood  ==    4, "block is good");
    ok (memstats.Nbad   ==    0, "no blocks are bad");
    
    // does FREE work?
    free (data1);
    memstats = ohana_memstats(0);

    ok (memstats.Ntotal ==    3, "freed block");
    ok (memstats.Nbytes == 3000, "kept correct amount");
    ok (memstats.Ngood  ==    3, "block is good");
    ok (memstats.Nbad   ==    0, "no blocks are bad");
    
    free (data2);
    memstats = ohana_memstats(0);

    ok (memstats.Ntotal ==    2, "freed block");
    ok (memstats.Nbytes == 2000, "kept correct amount");
    ok (memstats.Ngood  ==    2, "block is good");
    ok (memstats.Nbad   ==    0, "no blocks are bad");
    
    free (data3);
    memstats = ohana_memstats(0);

    ok (memstats.Ntotal ==    1, "freed block");
    ok (memstats.Nbytes == 1000, "kept correct amount");
    ok (memstats.Ngood  ==    1, "block is good");
    ok (memstats.Nbad   ==    0, "no blocks are bad");
    
    free (data4);
    memstats = ohana_memstats(0);

    ok (memstats.Ntotal ==    0, "freed block");
    ok (memstats.Nbytes ==    0, "kept correct amount");
    ok (memstats.Ngood  ==    0, "block is good");
    ok (memstats.Nbad   ==    0, "no blocks are bad");
    
    skip_end();
  }

  {
    // int status;
    char *data1, *data2, *data3, *data4;
    OhanaMemstats memstats;

    // does ALLOCATE work?
    ALLOCATE (data1, char, 100);
    ALLOCATE (data2, char, 100);
    ALLOCATE (data3, char, 100);
    ALLOCATE (data4, char, 100);
    memstats = ohana_memstats(0);

    skip_start (!memstats.exists, 16, "skipping all tests : memory management not enabled");

    ok (memstats.Ntotal ==   4, "allocated a block");
    ok (memstats.Nbytes == 400, "allocated correct amount");
    ok (memstats.Ngood  ==   4, "block is good");
    ok (memstats.Nbad   ==   0, "no blocks are bad");
    
    // damage some data
    data1[101] = 111;
    memstats = ohana_memstats(0);

    ok (memstats.Ntotal ==    4, "allocated a block");
    ok (memstats.Nbytes ==  400, "allocated correct amount");
    ok (memstats.Ngood  ==    3, "3 blocks are good");
    ok (memstats.Nbad   ==    1, "one block is bad");
    
    // damage some other data
    data2[-2] = 111;
    memstats = ohana_memstats(0);

    ok (memstats.Ntotal ==    4, "allocated a block");
    ok (memstats.Nbytes ==  400, "allocated correct amount");
    ok (memstats.Ngood  ==    2, "3 blocks are good");
    ok (memstats.Nbad   ==    2, "one block is bad");
    
    // does FREE work?
    free (data1);
    memstats = ohana_memstats(0);

    ok (memstats.Ntotal ==    3, "freed block");
    ok (memstats.Nbytes ==  300, "kept correct amount");
    ok (memstats.Ngood  ==    2, "block is good");
    ok (memstats.Nbad   ==    1, "no blocks are bad");
    
    free (data2);
    memstats = ohana_memstats(0);

    ok (memstats.Ntotal ==    2, "freed block");
    ok (memstats.Nbytes ==  200, "kept correct amount");
    ok (memstats.Ngood  ==    2, "block is good");
    ok (memstats.Nbad   ==    0, "no blocks are bad");
    
    free (data3);
    memstats = ohana_memstats(0);

    ok (memstats.Ntotal ==    1, "freed block");
    ok (memstats.Nbytes ==  100, "kept correct amount");
    ok (memstats.Ngood  ==    1, "block is good");
    ok (memstats.Nbad   ==    0, "no blocks are bad");
    
    free (data4);
    memstats = ohana_memstats(0);

    ok (memstats.Ntotal ==    0, "freed block");
    ok (memstats.Nbytes ==    0, "kept correct amount");
    ok (memstats.Ngood  ==    0, "block is good");
    ok (memstats.Nbad   ==    0, "no blocks are bad");
    
    skip_end();
  }

  return exit_status();
}
