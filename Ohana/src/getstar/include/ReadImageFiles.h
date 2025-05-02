# include <glob.h>

char   DateKeyword[64];
char   DateMode[64];
char   UTKeyword[64];
char   MJDKeyword[64];
char   JDKeyword[64];
char   ExptimeKeyword[64];

int parse_time (Header *header);
int ReadImageHeader (Header *header, Image *image);
Image *ReadImageFiles (char *filename, off_t *Nimages);
