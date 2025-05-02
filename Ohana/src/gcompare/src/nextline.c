# include "gcompare.h"

char *nextline (line)
char line[];
{

  char *ret;
  
  ret = strchr (line, '\n');
  if (ret != NULL) { 
    *ret = 0;
    ret += 1;
  }

  return (ret);
}
  
