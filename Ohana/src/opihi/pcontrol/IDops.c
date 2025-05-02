# include "pcontrol.h"

static IDtype CurrentJobID  = 1;
static IDtype CurrentHostID = 1;

/* for now, no persistence between sessions : we could use the date/time to seed the upper
 * byte(s) if needed */
void InitIDs () {
  CurrentJobID = 1;
  CurrentHostID = 1;
}

IDtype NextJobID () {

  IDtype ID;

  ID = CurrentJobID;
  CurrentJobID ++;
  return (ID);
}

/* only used by the User thread */
IDtype NextHostID () {

  IDtype ID;

  ID = CurrentHostID;
  CurrentHostID ++;
  return (ID);
}

void PrintID (gpDest dest, IDtype ID) {

  unsigned short int word0, word1, word2, word3;

  word0 = 0xffff & ID;
  word1 = 0xffff & (ID >> 16);
  word2 = 0xffff & (ID >> 32);
  word3 = 0xffff & (ID >> 48);

  gprint (dest, "%x.%x.%x.%x", word3, word2, word1, word0);
}

IDtype GetID (char *IDword) {

  int Nargs;
  IDtype ID;
  unsigned int word0, word1, word2, word3;
  char *endptr;

  Nargs = sscanf (IDword, "%x.%x.%x.%x", &word3, &word2, &word1, &word0);
  if (Nargs == 4) {
      IDtype tmp;
    ID = 0;
    ID |= (word0 << 0);
    ID |= (word1 << 16);
    tmp = word2;
    ID |= (tmp << 32);
    tmp = word3;
    ID |= (tmp << 48);
    return ID;
  } 
    
  ID = strtoll (IDword, &endptr, 10);
  if (*endptr == 0) {
    return ID;
  }

  return 0;
}
