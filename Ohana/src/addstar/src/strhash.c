# include "addstar.h"

int strhash (char *string, int modulus) {

  myAssert (modulus < 0x100, "for the moment, (modulus) is limited to 255");

  int Nchar = strlen (string);

  int i;
  int Nsum = 0;
  for (i = 0; i < Nchar; i++) {
    Nsum += (string[i] % modulus);
  }
  int value = Nsum % modulus;
  return (value);
}
