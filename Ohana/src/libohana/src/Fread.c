# include <ohana.h>

static int ByteSwap (char *ptr, off_t size, off_t nitems, char *type);

# define BYTE_EXCHANGE(X,Y) tmp = byte[X]; byte[X] = byte[Y]; byte[Y] = tmp;
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

/* add other architectures here, ie dec, alpha, etc */ 

off_t Fread (void *ptr, off_t size, off_t nitems, FILE *f, char *type) {

  int valid;
  off_t status;

  status = fread (ptr, size, nitems, f);

  valid = ByteSwap (ptr, size, nitems, type);

  if (!valid) return (FALSE);
  return (status);
}

off_t Fwrite (void *ptr, off_t size, off_t nitems, FILE *f, char *type) {

  int valid;
  off_t status;

  valid = ByteSwap (ptr, size, nitems, type);

  if (!valid) return (FALSE);

  status = fwrite (ptr, size, nitems, f);
  return (status);
}

static int ByteSwap (char *ptr, off_t size, off_t nitems, char *type) {
  OHANA_UNUSED_PARAM(size);

# ifndef BYTE_SWAP
  OHANA_UNUSED_PARAM(ptr);
  OHANA_UNUSED_PARAM(nitems);
  OHANA_UNUSED_PARAM(type);
  return (TRUE);

# else

  off_t i;
  unsigned char *byte0, *byte1, *byte2, *byte3, *byte4, *byte5, *byte6, *byte7, tmp;

  if (!strcmp (type, "char")) return (TRUE);

  if (!strcmp (type, "short")) {
    byte0 = (unsigned char *)ptr;
    byte1 = (unsigned char *)ptr + 1;
    for (i = 0; i < nitems; i++, byte0 += 2, byte1 += 2) {
      tmp = *byte0;
      *byte0 = *byte1;
      *byte1 = tmp;
    }
    return (TRUE);
  }

  if (!strcmp (type, "int") || !strcmp (type, "float")) {
    byte0 = (unsigned char *)ptr;
    byte1 = (unsigned char *)ptr + 1;
    byte2 = (unsigned char *)ptr + 2;
    byte3 = (unsigned char *)ptr + 3;
    for (i = 0; i < nitems; i++, byte0 += 4, byte1 += 4, byte2 += 4, byte3 += 4) {
      tmp = *byte0;
      *byte0 = *byte3;
      *byte3 = tmp;
      tmp = *byte1;
      *byte1 = *byte2;
      *byte2 = tmp;
    }
    return (TRUE);
  }

  if (!strcmp (type, "double")) {
    byte0 = (unsigned char *)ptr;
    byte1 = (unsigned char *)ptr + 1;
    byte2 = (unsigned char *)ptr + 2;
    byte3 = (unsigned char *)ptr + 3;
    byte4 = (unsigned char *)ptr + 4;
    byte5 = (unsigned char *)ptr + 5;
    byte6 = (unsigned char *)ptr + 6;
    byte7 = (unsigned char *)ptr + 7;
    for (i = 0; i < nitems; i++, byte0 += 8, byte1 += 8, byte2 += 8, byte3 += 8, byte4 += 8, byte5 += 8, byte6 += 8, byte7 += 8) {
      tmp = *byte0;
      *byte0 = *byte7;
      *byte7 = tmp;
      tmp = *byte1;
      *byte1 = *byte6;
      *byte6 = tmp;
      tmp = *byte1;
      *byte2 = *byte5;
      *byte5 = tmp;
      tmp = *byte1;
      *byte3 = *byte4;
      *byte4 = tmp;
    }
    return (TRUE);
  }

  fprintf (stderr, "unknown type %s\n", type);
  return (FALSE);

# endif 

}
