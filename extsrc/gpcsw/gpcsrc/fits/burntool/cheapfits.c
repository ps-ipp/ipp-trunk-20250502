#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define BZERO 32768
#define NFITS 2880
typedef signed short int IMTYPE;	/* Data type of image */

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

char fitsbuf[NFITS];
static int _ENDIAN_TEST = 1;
#define LOWENDIAN() (*(char*)&_ENDIAN_TEST)

void swab(const void *from, void *to, ssize_t n);

/****************************************************************/
/* write_2dfits() writes a single 2D FITS file */
int write_2dfits(int nx, int ny, int sx, int sy, IMTYPE *data, int fd)
{
   int i, n;
   for(n=0; n<NFITS; n++) fitsbuf[n] = ' ';
   n = 0;
   sprintf(fitsbuf+80*n++, "%-8s= %20s", "SIMPLE", "T");
   sprintf(fitsbuf+80*n++, "%-8s= %20d", "BITPIX", 16);
   sprintf(fitsbuf+80*n++, "%-8s= %20d", "NAXIS", 2);
   sprintf(fitsbuf+80*n++, "%-8s= %20d", "NAXIS1", nx);
   sprintf(fitsbuf+80*n++, "%-8s= %20d", "NAXIS2", ny);
   sprintf(fitsbuf+80*n++, "%-8s= %20d", "CNPIX1", sx);
   sprintf(fitsbuf+80*n++, "%-8s= %20d", "CNPIX2", sy);
   sprintf(fitsbuf+80*n++, "%-8s= %20.1f", "BSCALE", 1.0);
   sprintf(fitsbuf+80*n++, "%-8s= %20.1f", "BZERO", (double)BZERO);
   sprintf(fitsbuf+80*n++, "%-8s",       "END");
   for(n=0; n<NFITS; n++) if(fitsbuf[n]=='\0') fitsbuf[n] = ' ';
   if(write(fd, fitsbuf, NFITS) < 0) return(-1);
   for(i=0; i<nx*ny; i+=NFITS/sizeof(IMTYPE)) {
      n = MIN(NFITS, (nx*ny-i) * sizeof(IMTYPE));
      if(LOWENDIAN()) swab(data+i, fitsbuf, n);
      else            memcpy(fitsbuf, data+i, n);
      if(n < NFITS) bzero(fitsbuf+n, NFITS-n);
      if(write(fd, fitsbuf, NFITS) < 0) return(-1);
   }
   return(0);
}

/****************************************************************/
/* write_3dhdr() writes a bare 3D FITS header */
int write_3dhdr(int nx, int ny, int nz, int ncmt, char *cmt[], int fd)
{
   int i, n;
   for(n=0; n<NFITS; n++) fitsbuf[n] = ' ';
   n = 0;
   sprintf(fitsbuf+80*n++, "%-8s= %20s", "SIMPLE", "T");
   sprintf(fitsbuf+80*n++, "%-8s= %20d", "BITPIX", 16);
   sprintf(fitsbuf+80*n++, "%-8s= %20d", "NAXIS", 3);
   sprintf(fitsbuf+80*n++, "%-8s= %20d", "NAXIS1", nx);
   sprintf(fitsbuf+80*n++, "%-8s= %20d", "NAXIS2", ny);
   sprintf(fitsbuf+80*n++, "%-8s= %20d", "NAXIS3", nz);
   sprintf(fitsbuf+80*n++, "%-8s= %20.1f", "BSCALE", 1.0);
   sprintf(fitsbuf+80*n++, "%-8s= %20.1f", "BZERO", (double)BZERO);
   for(i=0; i<ncmt; i++) sprintf(fitsbuf+80*n++, "COMMENT %s", cmt[i]);
   sprintf(fitsbuf+80*n++, "%-8s",       "END");
   for(n=0; n<NFITS; n++) if(fitsbuf[n]=='\0') fitsbuf[n] = ' ';
   if(write(fd, fitsbuf, NFITS) < 0) return(-1);
   return(0);
}

/****************************************************************/
/* write_2ddata() writes a single data chunk as part of a 3D fits file */
int write_2ddata(int nx, int ny, int *ntot, IMTYPE *data, int fd)
{
   int i, n=0;
   for(i=0; i<nx*ny; i+=NFITS/sizeof(IMTYPE)) {
      n = MIN(NFITS, (nx*ny-i) * sizeof(IMTYPE));
      if(LOWENDIAN()) swab(data+i, fitsbuf, n);
      else            memcpy(fitsbuf, data+i, n);
      if(write(fd, fitsbuf, n) < 0) return(-1);
   }
   *ntot = *ntot + n;
   return(0);
}

/****************************************************************/
/* write_3dend() writes a single data chunk as part of a 3D fits file */
int write_3dend(int *ntot, int fd)
{
   if((*ntot%NFITS) != 0) {
      bzero(fitsbuf, NFITS-(*ntot%NFITS));
      if(write(fd, fitsbuf, NFITS-(*ntot%NFITS)) < 0) return(-1);
   }

   return(0);
}
