# include <zlib.h>

/* functions defined in ricecomp.c */
int fits_rcomp(int a[], int nx,	unsigned char *c, int clen, int nblock);
int fits_rcomp_short(short a[], int nx, unsigned char *c, int clen, int nblock);
int fits_rcomp_byte(char a[], int nx, unsigned char *c, int clen, int nblock);
int fits_rdecomp (unsigned char *c, int clen, unsigned int array[], int nx, int nblock);
int fits_rdecomp_short (unsigned char *c, int clen, unsigned short array[], int nx, int nblock);
int fits_rdecomp_byte (unsigned char *c, int clen, unsigned char array[], int nx, int nblock);

/* functions defined in fits_hcompress.c */
# define LONGLONG long long
int fits_hcompress(int *a, int ny, int nx, int scale, char *output, long *nbytes, int *status);
int fits_hcompress64(LONGLONG *a, int ny, int nx, int scale, char *output, long *nbytes, int *status);

/* functions defined in fits_hdeccompress.c */
int fits_hdecompress(unsigned char *input, int smooth, int *a, int *ny, int *nx, int *scale, int *status);
int fits_hdecompress64(unsigned char *input, int smooth, LONGLONG *a, int *ny, int *nx, int *scale, int *status);

/* functions defined in pliocomp.c */
int pl_p2li (int *pxsrc, int xs, short *lldst, int npix);
int pl_l2pi (short *ll_src, int xs, int *px_dst, int npix);

/* functions defined in gzip.c */
int gfits_gz_stripheader (unsigned char *data, int *ndata);
int gfits_uncompress (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
int gfits_compress (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);

