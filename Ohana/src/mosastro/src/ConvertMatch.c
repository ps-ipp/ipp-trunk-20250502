# include "mosastro.h"

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

# define MATCH_SIZE 104

int ConvertMatch (MatchData *data, int size, int nitems) {

  int i;
  unsigned char *byte, tmp;

# ifdef BYTE_SWAP

  if (size != MATCH_SIZE) {
    fprintf (stderr, "mismatch in type sizes (MatchData) %d vs %d\n", size, MATCH_SIZE);
    return (FALSE);
  }

  byte = (unsigned char *) data;
  for (i = 0; i < nitems; i++, byte += size) {
    SWAP_DBLE (0);   /* R */
    SWAP_DBLE (8);   /* D */
    SWAP_WORD (16);  /* P */
    SWAP_WORD (20);  /* Q */
    SWAP_WORD (24);  /* L */
    SWAP_WORD (28);  /* M */
    SWAP_WORD (32);  /* X */
    SWAP_WORD (36);  /* Y */

    SWAP_DBLE (40);  /* R */
    SWAP_DBLE (40);  /* D */
    SWAP_WORD (56);  /* P */
    SWAP_WORD (60);  /* Q */
    SWAP_WORD (64);  /* L */
    SWAP_WORD (68);  /* M */
    SWAP_WORD (72);  /* X */
    SWAP_WORD (76);  /* Y */

    SWAP_WORD (80);  /* Mcat */
    SWAP_WORD (84);  /* dMcat */
    SWAP_WORD (88);  /* Minst */
    SWAP_WORD (92);  /* dMinst */
  }
  return (TRUE);

# else
  return (TRUE);
# endif  
} 
