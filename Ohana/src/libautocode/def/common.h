# include <ohana.h>
# include <gfitsio.h>

# ifndef AUTOCODE_H
# define AUTOCODE_H
/* matched endif is added by Makefile */

# ifndef TRUE
# define TRUE (1)
# define FALSE (0)
# endif

# define SWAP_BYTE(X) \
  tmp = byte[X+0]; byte[X+0] = byte[X+1]; byte[X+1] = tmp;
# define SWAP_WORD(X) \
  tmp = byte[X+0]; byte[X+0] = byte[X+3]; byte[X+3] = tmp; \
  tmp = byte[X+1]; byte[X+1] = byte[X+2]; byte[X+2] = tmp;
# define SWAP_DBLE(X) \
  tmp = byte[X+0]; byte[X+0] = byte[X+7]; byte[X+7] = tmp; \
  tmp = byte[X+1]; byte[X+1] = byte[X+6]; byte[X+6] = tmp; \
  tmp = byte[X+2]; byte[X+2] = byte[X+5]; byte[X+5] = tmp; \
  tmp = byte[X+3]; byte[X+3] = byte[X+4]; byte[X+4] = tmp;

# ifndef BYTE_SWAP
# ifndef NOT_BYTE_SWAP
# error "neither BYTE_SWAP not NOT_BYTE_SWAP is set"
# endif
# endif

# define e_time int
# define e_void long long
# define rawshort short

# define DVO_IMAGE_NAME_LEN 117

/*** rawshort is used to handle the broken pre-autocode photreg tables
     fix the tables and remove this
***/

/*** this file uses data types which must have fixed sizes regardless 
     of the platform.  It originally used the basic C primitives: 
       float, double, int, short int, unsigned long int, etc.
     this breaks under 64 bit (and probably on other systems).
     I should define internal data types which should be set by the 
     use of # define statements if needed.  I will cheat for now and use
     the time_t to replace unsigned long int in this file 
***/

