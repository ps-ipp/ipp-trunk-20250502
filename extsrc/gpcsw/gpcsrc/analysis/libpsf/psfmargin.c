/* Basic subroutines for computing psf profiles */
/*   algo:   1's digit: 0 for parabola fit, 1 for Gaussian fit to posn
 *          10's digit: 0 for quick, dumb flux sum, 1-3 for wing fit and
 *                      full flux sum, 1,2,3 = wauss, power law, exponential
 *         100's digit: 0 for normal operation, 1 to ignore right hand double
 *			(for calcite operations)
 *     1000000's digit: 1 for debug output
 */
/* 080703 JT - slim down border to BORDER instead of n/8 */
/* 031026 JT - mess around with better flux estimates ((algo/10)%10 = 1) */
/* 020420 JT - I'm generated NaNs somehow so test for them and remove them! */
/* 020322 JT - avoid edge pixels like the plague */
/* 020128 JT - change args to permit non-filled arrays */
/* 6/20/96 (JT) to return S/N and only one fwhm */
/* JT 6/25/93 */
/* Routine to find the biggest star in an image and return its
 * pixel center (ix,iy), fit center (x0,y0), width (fwhm) and S/N (sn)
 * The background level is returned as sky
 * Assumes unsigned short integers
 **************************CONVENTION******************************
 * The first element of the array is taken to be pixel 0.0 to 1.0
 ******************************************************************
 */

#include <stdio.h>
#include <math.h>
#include "psf.h"
// #define DEBUG	/* Engage debug code? */

#define MAXLINE 4096	/* Maximum storage for various purposes */
#define MAXDIM	1024	/* Maximum nx or ny */
#define EDGE  1		/* Number of edge pixels to ignore */
static int BORDER=2;	/* Closest a high pixel can be to edge */

#define DBLFRAC  0.6	/* Fraction of highest pixel to be a calcite double */

#define NGAUSS  5       /* Number of parameters for a Gaussian fit */
#define NWING   3       /* Number of parameters for a psf tail fit */

#define NODATA  0	/* Special value for *NO DATA* */

#define XERROR   (0x0001)
#define YERROR   (0x0002)
#define GXERROR  (0x0004)
#define GYERROR  (0x0008)
#define FWERROR  (0x0010)
#define WINGERROR  (0x0020)
#define DATAERROR  (0x0040)

static double strip[MAXLINE];
static double median(int n, double *key, int *idx);
int psfmargin_guts(int algo, int *ix, int *iy, int *big, double *x0, double *y0, 
	       double *fwhm, double *xfwhm, double *yfwhm, 
	       double *sky, double *flux, double *sn, 
	       int nx, int ny, int mx, unsigned short *im);
static int parabola(int nx, double *d, 
		    double *center, double *height, double *width);
static int gaussian(int *niter, int n, double *d, double *par);
static int dgauss(int n, double *d, 
		  double *par, double *chi, double *deriv, double *curv);
static int gchi(int n, double *d, double *par, double *chi);
static int wingfit(int *niter, int func, int n, 
		   double *d, double *r, double *par);
static int wingchi(int func, int n, double *r, double *d, 
	double dmin, double *par, double *chi);
static int dwingfit(int func, int n, double *r, double *d, double dmin, 
		    double *par, double *chi, double *deriv, double *curv);
static int linsolve(int n, double *y, double *a, double *x);
#ifdef FANCY_STUFF
static int exponential_bias(double *bias, double *ampl, double *efold, 
			    int nx,int ny,int mx, unsigned short *im);
static int video_bias_sub(double bias, double ampl, double efold, 
			  int nx,int ny,int mx, unsigned short *im);
#endif

/* Normal, Gaussian fit version */
int psfmargin(int sx, 			/* x offset of pixel 0,0 */
	     int sy,  			/* y offset of pixel 0,0 */
	     int binx,  		/* x bin factor */
	     int biny,   		/* y bin factor */
	     int bordx,  		/* x border */
	     int bordy,   		/* y border */
	     int nx,   			/* x size of image */
	     int ny,    		/* y size of image */
	     int NX,   			/* x stride of image */
	     unsigned short *data,	/* ushort image data: 0 = *NO_DATA* */
	     double *xu,		/* Fitted x position */
	     double *yu,		/* Fitted y position */
	     int *fmax,			/* Highest pixel in best bin */
	     double *fwhm,		/* FWHM of psf (binx,y corrected) */
	     double *xfwhm,		/* FWHM in x dir (binx corrected) */
	     double *yfwhm,		/* FWHM in y dir (biny corrected) */
	     double *bkgnd,		/* Sky level */
	     double *ftot,		/* Total flux */
	     double *weight,		/* Composite quality of star: <1 bad,*/
	     				/*    1-10 so-so, 10-30 OK, >30 fine */
	     double *snr)		/* S/N of (big-sky)/noise */
{
   int ix, iy, big, err;
   BORDER = MAX(bordx, bordy);
   err = psfmargin_guts(1, &ix,&iy, &big, 
       xu,yu, fwhm, xfwhm, yfwhm, bkgnd, ftot, snr, nx,ny,NX, data);

   *xu = sx + *xu * binx;
   *yu = sy + *yu * biny;
   *fwhm *= sqrt((double)(binx*biny));
   *xfwhm *= binx;
   *yfwhm *= biny;
   *fmax = big;
   *weight = *snr;
   return(err);
}

/* Basic PSF calculation routine */
int
psfmargin_guts(algo, ix,iy, big, x0,y0, fwhm, xfwhm, yfwhm, 
	       sky, flux, sn, nx,ny,mx, im)
   int algo;			/* What algorithm to use? */
   int *ix, *iy, *big;		/* Highest pixel */
   double *x0, *y0;		/* Fitted location of centroid */
   double *fwhm, *xfwhm, *yfwhm;/* Star fwhm */
   double *sn;			/* Star signal to noise */
   double *sky, *flux;		/* Sky level, total flux in star */
   int nx, ny, mx;		/* Size of image; x size of image storage */
   unsigned short *im;		/* Data array */
{
   int i, j, nsky, iwx, iwy, dblbig=0.0, dblx=0, dbly=0, err=0;
   int n, k[4];
   double mean[4], rms[4], var, medsky;
   double xhgt, yhgt;
   double gxpar[NGAUSS], gypar[NGAUSS], wingpar[NWING];
   int niter;

/* Locate the highest pixel in the central 3/4 of the image */
/* No, avoid pixels closer to the edge than BORDER */
   *big = -1;
//   for(j=ny/8; j<(7*ny)/8; j++) {
//      for(i=nx/8; i<(7*nx)/8; i++) {
   for(j=BORDER; j<ny-BORDER; j++) {
      for(i=BORDER; i<nx-BORDER; i++) {
	 if(im[i+mx*j] != NODATA && (int)im[i+mx*j] > (*big)) {
	    (*big) = im[i+mx*j];
	    *ix = i;
	    *iy = j;
	 }
      }
   }
/* No pixels found???  (data all *NODATA*) */
   if(*big == -1) return(DATAERROR);

/* Look in the corners for a sky value */
   for(i=0; i<4; i++) {
      mean[i] = rms[i] = 0.0;
      k[i] = 0;
   }
   i = MIN(nx, ny);		/* i = smaller dimension */
   j = MIN(6, i/8);		/* nsky = dim/8, but 3 <= nsky <= 6 */
   nsky = MAX(3, j);		/* nsky = dim/8, but 3 <= nsky <= 6 */
   if(nsky > i/2) nsky = i/3;
   if(nsky < 1) nsky = 1;
   for(j=EDGE; j<nsky+EDGE; j++) {
      for(i=EDGE; i<nsky+EDGE; i++) {
	 if(im[i+j*mx] != NODATA) {
	    mean[0] += im[i+j*mx];
	    rms[0]  += im[i+j*mx] * im[i+j*mx];
	    k[0] += 1;
	 }
	 if(im[nx-1-i+j*mx] != NODATA) {
	    mean[1] += im[nx-1-i+j*mx];
	    rms[1]  += im[nx-1-i+j*mx] * im[nx-1-i+j*mx];
	    k[1] += 1;
	 }
	 if(im[nx-1-i+(ny-1-j)*mx] != NODATA) {
	    mean[2] += im[nx-1-i+(ny-1-j)*mx];
	    rms[2]  += im[nx-1-i+(ny-1-j)*mx] * im[nx-1-i+(ny-1-j)*mx];
	    k[2] += 1;
	 }
	 if(im[i+(ny-1-j)*mx] != NODATA) {
	    mean[3] += im[i+(ny-1-j)*mx];
	    rms[3]  += im[i+(ny-1-j)*mx] * im[i+(ny-1-j)*mx];
	    k[3] += 1;
	 }
      }
   }
/* Normalize mean and rms, initialize index array */
   for(i=0; i<4; i++) {
      if(k[i] > 0) mean[i] /= k[i];
      if(k[i] > 0) rms[i] /= k[i];
      k[i] = i;
   }
/* Sort them via index array k[] */
   if(mean[k[2]] > mean[k[3]]) {i=k[2]; k[2]=k[3]; k[3]=i;}
   if(mean[k[1]] > mean[k[2]]) {i=k[1]; k[1]=k[2]; k[2]=i;}
   if(mean[k[2]] > mean[k[3]]) {i=k[2]; k[2]=k[3]; k[3]=i;}
   if(mean[k[0]] > mean[k[1]]) {i=k[0]; k[0]=k[1]; k[1]=i;}
   if(mean[k[1]] > mean[k[2]]) {i=k[1]; k[1]=k[2]; k[2]=i;}
   if(mean[k[2]] > mean[k[3]]) {i=k[2]; k[2]=k[3]; k[3]=i;}

/* Pick the second greatest as the sky level */
   i = 1;
   if(mean[k[i]] == NODATA) i = 2;
   if(mean[k[i]] == NODATA) i = 3;
   *sky = mean[k[i]];
   if(nsky > 1) var = rms[k[i]] - (*sky)*(*sky);
   else var = (mean[k[MIN(i+1,3)]]-mean[k[i]])*(mean[k[MIN(i+1,3)]]-mean[k[i]]);
   if(var < 1.0) var = 1.0;

/* Check to see whether there is another, nearly equivalent star to the left */
   if((algo/100)%10 == 1) {
//      for(j=ny/8; j<(7*ny)/8; j++) {
//	 for(i=nx/8; i<(7*nx)/8; i++) {
      for(j=BORDER; j<ny-BORDER; j++) {
	 for(i=BORDER; i<nx-BORDER; i++) {
/* Skip first star we found */
	    if(ABS(i-*ix) < 2 && ABS(j-*iy) < 2) continue;
/* Is it nearly as tall? */
	    if(im[i+mx*j] != NODATA &&
	       (int)im[i+mx*j] > ((*big)-(*sky))*DBLFRAC+(*sky)) {
/* Must be a local max */
	       if(im[i+mx*j]>im[i-1+mx*j] && im[i+mx*j]>=im[i+1+mx*j] &&
		  im[i+mx*j]>im[i+mx*(j-1)] && im[i+mx*j]>=im[i+mx*(j+1)]) {
		  dblbig = im[i+mx*j];
		  dblx = i;
		  dbly = j;
	       }
	    }
	 }
      }
/* Does a double star exist and is it to the left? */
      if(dblbig > 0 && dblx < *ix) {
	 *big = dblbig;
	 *ix = dblx;
	 *iy = dbly;
      }
   }

/*  printf("%d %d %d %d %.1f %.1f\n",nsky, i,i0,i1,*sky,sqrt(var)); */

/* "Signal to noise" is the ratio of the brightest pixel to the rms */
   *sn = ((*big) - *sky) / sqrt(var);

/* Estimate a width for the peak in x */
   for(i=(*ix)+1; i<nx; i++) 
      if(im[i+(*iy)*mx] != NODATA && im[i+(*iy)*mx] < ((*big)+(*sky))/2) break;
   iwx = i - (*ix);

/* Estimate a width for the peak in y */
   for(i=(*iy)+1; i<ny; i++)
      if(im[(*ix)+i*mx] != NODATA && im[(*ix)+i*mx] < ((*big)+(*sky))/2) break;
   iwy = i - (*iy);

/* Extract a strip in x */
   for(i=EDGE; i<nx-EDGE; i++) {
      strip[i] = 0.0;
      n = 0;
      for(j=MAX(EDGE,(*iy)-iwy); j<= MIN(ny-1-EDGE,(*iy)+iwy); j++) {
	 if(im[i+j*mx] != NODATA) {
	    strip[i] += im[i+j*mx];
	    n++;
	 }
      }
      if(n > 0) strip[i] =  strip[i] / n - (*sky);
   }

/* Find the peak in x */
   if((algo%10) == 0) {
      if(parabola(nx, strip, x0, &xhgt, xfwhm)) err |= XERROR ;
   } else if((algo%10) == 1) {
      niter = 20;
      if(gaussian(&niter, nx-2*EDGE, strip+EDGE, gxpar) == 0) {
	 *x0 = gxpar[3] + EDGE;
	 *xfwhm = 2.4*gxpar[4];
      } else {
	 err |= GXERROR;
	 if(parabola(nx, strip, x0, &xhgt, xfwhm)) err |= XERROR;
      }
   }

   if((algo/1000000)%10 == 1) {
      printf("%3d %3d %6.1f %8.3f %8.2f %8.2f %8.2f\n", 
	     *iy, iwy, gxpar[0], gxpar[1],gxpar[2], gxpar[3],gxpar[4]);
      for(i=0; i<nx-2*EDGE; i++) printf("%4d %8.1f\n", i, strip[EDGE+i]);
   }

/* Extract a strip in y */
   for(j=EDGE; j<ny-EDGE; j++) {
      strip[j] = 0.0;
      n = 0;
      for(i=MAX(EDGE,(*ix)-iwx); i<=MIN(nx-1-EDGE,(*ix)+iwx); i++) {
	 if(im[i+j*mx] != NODATA) {
	    strip[j] += im[i+j*mx];
	    n++;
	 }
      }
      if(n > 0) strip[j] =  strip[j] / n - (*sky);
   }

/* Find the peak in y */
   if((algo%10) == 0) {
      if(parabola(ny, strip, y0, &yhgt, yfwhm)) err |= YERROR;
   } else if((algo%10) == 1) {
      niter = 20;
      if(gaussian(&niter, ny-2*EDGE, strip+EDGE, gypar) == 0) {
	 *y0 = gypar[3] + EDGE;
	 *yfwhm = 2.4*gypar[4];
      } else {
	 err |= GYERROR;
	 if(parabola(ny, strip, y0, &yhgt, yfwhm)) err |= YERROR;
      }
   }

   if((algo/1000000)%10 == 1) {
      printf("%3d %3d %6.1f %8.3f %8.2f %8.2f %8.2f\n", 
	     *ix, iwx, gypar[0], gypar[1],gypar[2], gypar[3],gypar[4]);
      for(i=0; i<ny-2*EDGE; i++) printf("%4d %8.1f\n", i, strip[EDGE+i]);
   }

/* net FWHM = sqrt(fwx * fwy) */
   *fwhm = (*xfwhm) * (*yfwhm);

/* Sanity check on x0, y0 and FWHM, please! */
   if(*fwhm <= 0 || 
      *x0 < EDGE || *x0 > nx-1-EDGE || *y0 < EDGE || *y0 > ny-1-EDGE) {
      *x0 = *ix;
      *y0 = *iy;
      *fwhm = *xfwhm = *yfwhm = MIN(nx,ny) / 4;
      err |= FWERROR;
   } else {
      *fwhm = sqrt(*fwhm);
   }

/* Add up the total flux in excess of sky */
/* Quick, dumb sum of +/-2.5 FWHM less sky */
   if(((algo/10)%10) == 0) {
      for(j=-2.5*(*fwhm), *flux=0.0; j<=2.5*(*fwhm); j++) {
	 if(j+(*iy) >= EDGE && j+(*iy) <= ny-1-EDGE) {
	    for(i=-2.5*(*fwhm); i<=2.5*(*fwhm); i++) {
	       if(i+(*ix) >= EDGE && i+(*ix) <= nx-1-EDGE &&
		   im[i+(*ix)+mx*(j+(*iy))] != NODATA) {
		  *flux +=  im[i+(*ix)+mx*(j+(*iy))] - (*sky);
	       }
	    }
	 }
      }

   } else {
      int r, rmin, rmax, counts[2*MAXDIM], idx[2*MAXDIM], func;

      func = ((algo/10)%10) - 1;

/* This gets medsky from a square ring of outermost pixels */
      for(j=EDGE, n=0; j<ny-EDGE; j++) {
	 if(j >= EDGE+3 && j <= ny-1-EDGE-3) continue;
	 for(i=EDGE; i<nx-EDGE; i++) {
	    if(i >= EDGE+3 && i <= nx-1-EDGE-3) continue;
	    if(im[i+mx*j] != NODATA) {
	       strip[n++] =  im[i+mx*j];
	    }
	 }
      }
      medsky = median(n, strip, NULL);

/* Sum up medians as a function of radius */
      rmax = 0.65 * MIN(nx, ny) - EDGE;
      rmin = MAX(2, 1.5*(*fwhm));
      rmin = MIN(rmin, rmax-4);
      for(r=rmin; r<=rmax; r++) {
	 counts[r] = 0;
	 idx[r] = 3.2 * r*r;
      }
/* Accumulate the pixels in the skirt of the psf into arrays by radius */
      for(j=EDGE; j<ny-EDGE; j++) {
	 for(i=EDGE; i<nx-EDGE; i++) {
	    r = sqrt((i+0.5-(*x0))*(i+0.5-(*x0))+(j+0.5-(*y0))*(j+0.5-(*y0))) + 0.5;
	    if(r<rmin || r>rmax) continue;
	    strip[idx[r]+counts[r]] =  im[i+mx*j];
	    counts[r] += 1;
	 }
      }
/* Find the median as a function of radius*/
      for(r=rmin; r<=rmax; r++) {
	 strip[r] = r;
	 strip[r+idx[rmin]] = median(counts[r], &strip[idx[r]], NULL);
	 if((algo/1000000)%10 == 1) printf("%4d %4d %4d %6.1f\n", 
				       r, counts[r], idx[r], strip[r+idx[rmin]]);
      }

      niter = 20;
      if(wingfit(&niter, func, rmax-rmin+1, strip+idx[rmin]+rmin, strip+rmin, 
		 wingpar)) err |= WINGERROR;
#ifdef DEBUG
      printf("wing %3d %7.2f %7.2f %7.2f\n", 
	     niter, wingpar[0], wingpar[1], wingpar[2]);
#endif
/* power law fit sky */
      *sky = wingpar[0];
/* outermost circular ring */
//      *sky = strip[rmax+idx[rmin]];
/* outermost square pixels */
//      *sky = medsky;
/* Fixed sky at bias plus a bit */
//	      *sky = 202;

      for(j=EDGE, *flux=0.0; j<ny-EDGE; j++) {
	 for(i=EDGE; i<nx-EDGE; i++) {
	    *flux +=  im[i+mx*j] - *sky;
	 }
      }
/* Add in a bit more from extrapolation of the extended wings */
/*
      rpow = 0.65*nx;
      wing = -wingpar[1]*(2*3.14159) /(2+wingpar[2]) * pow(rpow,wingpar[2]+2);
      *big = wing / (*flux) * 10000;
      *flux +=  wing;
      *flux -= 0.61 * wing;
*/
   }

/*
  printf("(%d,%d) (%.1f,%.1f) (%.1f,%.1f) %.1f %.1f %.1f %.1f %.1f %.0f %d\n",
  *ix,*iy,*x0,*y0,x1,y1,*fwhm,r2,*sky,*sn,f0,*flux,MAX(iwx,iwy));
  */
   return(err);
}

/* Subroutine to fit a parabola to some data (fast)
 * N         = n, where 2n+1 points are fitted
 * CENTER    = center of fit parabola
 * HEIGHT    = height of fit parabola
 * WIDTH     = width of fit parabola
 **************************CONVENTION******************************
 The first element of the array is taken to be pixel 0.0 to 1.0
 ******************************************************************
 */

static int
parabola(nx, d, center, height, width)
   int nx;
   double *center, *height, *width, *d;
{
   double sum0=0.0, sum1=0.0, sum2=0.0, big;
   float a, b, c;
   int i, j, k, n;

/* Find the center and estimate a width for the peak */
   for(i=EDGE, big=d[EDGE], k=0; i<nx-EDGE; i++) {
      if(d[i] > big) {
	 big = d[i];
	 k = i;
      }
   }
  
   for(i=k+1;  i<nx-EDGE; i++) if(d[i] < big/2) break;
   for(j=k-1; j>=EDGE; j--) if(d[j] < big/2) break;
   n = (i-j) / 2;
   if(k-n < EDGE || k+n > nx-1-EDGE) n = MIN(k-EDGE, nx-1-EDGE-k);

   if(n < 1) {
      *center = k + 0.5;
      *width = 0.0;
      *height = big;
      return(-1);
   }

   for(i=(-n); i<=n; i++) {
      sum0 = sum0 + d[k+i];
      sum1 = sum1 + i*d[k+i];
      sum2 = sum2 + i*i*d[k+i];
   }

   a = 45.*(sum2 - n*(n+1)/3.*sum0) / (n*(n+1)*(2*n-1)*(2*n+1)*(2*n+3));
   b = -3.*sum1 / (n*(n+1)*(2*n+1));
   c = sum0 / (2*n+1);

   if(a >= 0.0) {
      *center = k + 0.5;
      *width = 0.0;
      *height = big;
      return(-2);
   }
   *center = k + 0.5 + b/(2*a);
   *height = c - a * (n*(n+1)/3. + b*b/(4*a*a));
   *width = -2 * (*height) / a;

/*
  printf("\n");
  for(i=k-4; i<=k+4; i++)
  printf(" %8d", (int)d[i]);
  printf("\n%4d %10.3f %10.3f %10.3f %10.3f %10.3f %10.3f\n",
  k,a,b,c,*center,*width,*height);
*/
   if( *width > 0) *width = sqrt(*width);
   else *width = 0.0;

/* NaN sanity checks */
   if( !(*center < 0.0 || *center >= 0.0) ||
       !(*width  < 0.0 || *width  >= 0.0) ||
       !(*height < 0.0 || *height >= 0.0) ) {
      fprintf(stderr, "\nCorrecting a ctr NaN, nx = %d, k = %d, n = %d, max = %f\n", 
	      nx, k, n, big);
      *center = k + 0.5;
      *width = 0.0;
      *height = big;
   }

   return(0);
}

#define SKY 0
#define SLP 1
#define HGT 2
#define CTR 3
#define SIG 4

#define TOL 0.1

static int
gaussian(int *niter, int n, double *d, double *par)
/* par[0-4] = sky, slope, height, center, width */
{
   double *dfit, deriv[NGAUSS], curv[NGAUSS*NGAUSS];
   double b[NGAUSS], delta[NGAUSS], a[NGAUSS*NGAUSS];
   double lambda, chi, chinew, sqrt10;
   int i, j, max, nfit, iter;

#ifdef DEBUG
   printf("%d", n);
   for(i=0; i<n; i++) printf(" %4.0f", d[i]);
   printf("\n");
#endif

/* Get some parameter estimates */
   par[SKY] = 0.5*(d[0] + d[1]);
   par[SLP] = (0.5*(d[n-1] + d[n-2]) - par[SKY]) / (n-2);
   for(i=1, max=0, par[HGT]=d[0]; i<n; i++) {
      if(d[i] > par[HGT]) {
	 max = i;
	 par[HGT] = d[i];
      }
   }
   par[HGT] -= par[SKY] + (max+0.5)*par[SLP];

   for(j=max+1; j<n; j++) {
      if(d[j] < par[SKY] + (j+0.5)*par[SLP] + 0.5*par[HGT]) break;
   }
   for(i=max-1; i>=0; i--) {
      if(d[i] < par[SKY] + (i+0.5)*par[SLP] + 0.5*par[HGT]) break;
   }
   par[SIG] = 0.25 * (j-i);
/* Restrict fit to just the peak; Gaussians suck otherwise */
   nfit = MAX(9, 2*(j-i)+1);
   if(max - (nfit-1)/2 < 0 || max + (nfit-1)/2 > n-1)
      nfit = MIN(2*max+1, 2*(n-1-max)+1);
   dfit = d + max - (nfit-1)/2;
   par[SKY] += (max-(nfit-1)/2) * par[SLP];
   par[CTR] = (nfit-1)/2;

   sqrt10 = sqrt(10.0);

/* Do some Marquardt iterations */
/*
   printf("\nnfit = %4d\n", nfit);
*/
   for(iter=0, lambda=0.1; iter<*niter; iter++) {
      dgauss(nfit, dfit, par, &chi, deriv, curv);
#ifdef DEBUG
      printf("%4d %7.4f %7.1f %7.1f %7.1f %7.2f %7.3f %7.1f\n", iter, lambda, 
	     par[SKY], par[SLP], par[HGT], par[CTR], par[SIG], chi);
#endif
      for(j=0; j<NGAUSS; j++) {
	 b[j] = deriv[j];
	 for(i=0; i<NGAUSS; i++) {
	    a[i+j*NGAUSS] = curv[i+j*NGAUSS];
	 }
	 a[j+j*NGAUSS] *= (1+lambda);
      }
      if(linsolve(NGAUSS, b, a, delta) != 0) { /* probably in very bad shape */
/* 2 means singular matrix -- these should be understood and debugged */
	 return(-1);
      }
      for(j=0; j<NGAUSS; j++) par[j] -= delta[j];
      gchi(nfit, dfit, par, &chinew);
      if(chi-chinew < TOL) {
	 par[CTR] += max - (nfit-1)/2;
	 *niter = iter;
/*	 printf("n = %d", *niter); */
	 return(0);
      } else if(chinew < chi) {
	 lambda /= sqrt10;
      } else {
	 lambda *= sqrt10;
	 for(j=0; j<NGAUSS; j++) par[j] += delta[j];
      }
   }
   *niter = iter;
   return(1);
}

static int
gchi(int n, double *d, double *par, double *chi)
/* par[0-4] = sky, slope, height, center, width */
{
   double z, expo, diff;
   int i;
   for(i=0, *chi = 0.0; i<n; i++) {
      z = i + 0.5;
      expo = exp(-0.5*(z-par[CTR])*(z-par[CTR])/(par[SIG]*par[SIG]));
      diff = par[SKY] + par[SLP]*z + par[HGT]*expo - d[i];
      *chi += diff * diff;
   }
   return(0);
}

static int
dgauss(int n, double *d, double *par, double *chi, double *deriv, double *curv)
/* par[0-4] = sky, slope, height, center, width */
{
   double z, expo, diff, df[NGAUSS];
   int i, j, k;
   *chi = 0.0;
   for(i=0; i<NGAUSS; i++) deriv[i] = 0.0;
   for(i=0; i<NGAUSS*NGAUSS; i++) curv[i] = 0.0;
   df[SKY] = 1.0;
   for(i=0; i<n; i++) {
      z = i + 0.5;
      expo = exp(-0.5*(z-par[CTR])*(z-par[CTR])/(par[SIG]*par[SIG]));
      diff = par[SKY] + par[SLP]*z + par[HGT]*expo - d[i];
      *chi += diff * diff;
      df[SLP] = z;
      df[HGT] = expo;
      df[CTR] = par[HGT] * expo * (z-par[CTR]) / (par[SIG]*par[SIG]);
      df[SIG] = par[HGT] * expo * (z-par[CTR])*(z-par[CTR]) / 
	 (par[SIG]*par[SIG]*par[SIG]);
      for(j=0; j<NGAUSS; j++) {
	 deriv[j] += diff * df[j];
	 for(k=j; k<NGAUSS; k++) {
	    curv[j+k*NGAUSS] += df[j] * df[k];
	 }
      }
   }
   for(j=1; j<NGAUSS; j++) {
      for(k=0; k<j; k++) {
	 curv[j+k*NGAUSS] = curv[k+j*NGAUSS];
      }
   }
   return(0);
}

#define SKY 0
#define AMP 1
#define XPO 2
#define PWRTOL 0.0001
#define B4 1.0
#define B6 0.5

static int
wingfit(int *niter, int func, int n, double *d, double *r, double *par)
/* func = 0,1,2 for waussian, power law, exponential */
/* par[0-2] = sky, amplitude, exponent */
{
   double deriv[NWING], curv[NWING*NWING];
   double b[NWING], delta[NWING], a[NWING*NWING];
   double lambda, chi, chinew, sqrt10, dmin;
   int i, j, iter;

#ifdef DEBUG
   printf("%d\n", n);
   for(i=0; i<n; i++) printf(" %4.0f", r[i]);
   printf("\n");
   for(i=0; i<n; i++) printf(" %4.0f", d[i]);
   printf("\n");
#endif

/* Get some parameter estimates */
   par[SKY] = d[n-1];
   if(func == 0) {		/* Waussian */
      par[XPO] = 5;
      par[AMP] = (d[0]-d[n-1]) / exp(-0.5*r[0]*r[0]/(par[XPO]*par[XPO]));
   } else if(func == 1) {	/* power law */
      par[XPO] = -3.0;
      par[AMP] = (d[0]-d[n-1]) / pow(r[0], par[XPO]);
   } else if(func == 2) {	/* exponential */
      par[XPO] = -0.1;
      par[AMP] = (d[0]-d[n-1]) / exp(r[0]*par[XPO]);
   }

   for(i=1, dmin=d[0]; i<n; i++) dmin = MIN(dmin, d[i]);
   dmin = 0.5 * dmin;

   sqrt10 = sqrt(10.0);

/* Do some Marquardt iterations */
   for(iter=0, lambda=0.1; iter<*niter; iter++) {
      dwingfit(func, n, r, d, dmin, par, &chi, deriv, curv);
#ifdef DEBUG
      printf("%4d %7.4f %7.1f %7.1f %7.3f %7.1f\n", iter, lambda, 
	     par[SKY], par[AMP], par[XPO], chi);
#endif
      for(j=0; j<NWING; j++) {
	 b[j] = deriv[j];
	 for(i=0; i<NWING; i++) {
	    a[i+j*NWING] = curv[i+j*NWING];
	 }
	 a[j+j*NWING] *= (1+lambda);
      }
      if(linsolve(NWING, b, a, delta) != 0) return(-1);
      for(j=0; j<NWING; j++) par[j] -= delta[j];
      wingchi(func, n, r, d, dmin, par, &chinew);

      if(chi < chinew) {
	 lambda *= sqrt10;
	 for(j=0; j<NWING; j++) par[j] += delta[j];
      } else if(chi-chinew < PWRTOL) {
	 *niter = iter;
	 return(0);
      } else if(chinew < chi) {
	 lambda /= sqrt10;
      }
   }
   *niter = iter;
   return(1);
}

static int
wingchi(int func, int n, double *r, double *d, 
	double dmin, double *par, double *chi)
/* par[0-2] = sky, amplitude, exponent */
{
   double diff, z2, expo=0.0;
   int i;
   for(i=0, *chi = 0.0; i<n; i++) {
      if(func == 0) {		/* Waussian */
	 z2 = 0.5 * r[i]*r[i] / (par[XPO]*par[XPO]);
	 expo = 1/(1+z2*(1+z2*(B4/2+z2*B6/6)));
      } else if(func == 1) {	/* power law */
	 expo = pow(r[i], par[XPO]);
      } else if(func == 2) {	/* exponential */
	 expo = exp(r[i]*par[XPO]);
      }
      diff = par[SKY] + par[AMP]*expo - d[i];
      *chi += diff * diff / ((d[i]-dmin)*(d[i]-dmin));
   }
   return(0);
}

static int
dwingfit(int func, int n, double *r, double *d, double dmin, double *par, 
	  double *chi, double *deriv, double *curv)
/* par[0-2] = sky, amplitude, exponent */
{
   double expo=0.0, diff, df[NWING], z2;
   int i, j, k;
   *chi = 0.0;
   for(i=0; i<NWING; i++) deriv[i] = 0.0;
   for(i=0; i<NWING*NWING; i++) curv[i] = 0.0;
   df[SKY] = 1.0;
   for(i=0; i<n; i++) {
      if(func == 0) {	/* Waussian */
	 z2 = 0.5 * r[i]*r[i] / (par[XPO]*par[XPO]);
	 expo = 1 / (1+z2*(1+z2*(B4/2+z2*B6/6)));
	 df[XPO] = par[AMP] * expo*expo * (1+z2*(B4+z2*B6/2)) * 
	    r[i]*r[i] / (par[XPO]*par[XPO]*par[XPO]);
      } else if(func == 1) {	/* power law */
	 expo = pow(r[i], par[XPO]);
	 df[XPO] = par[AMP] * log(r[i]) * expo;
      } else if(func == 2) {	/* exponential */
	 expo = exp(par[XPO]*r[i]);
	 df[XPO] = par[AMP] * r[i] * expo;
      }
      diff = par[SKY] + par[AMP]*expo - d[i];
      *chi += diff * diff / ((d[i]-dmin)*(d[i]-dmin));
      df[AMP] = expo;
      for(j=0; j<NWING; j++) {
	 deriv[j] += diff * df[j] / ((d[i]-dmin)*(d[i]-dmin));
	 for(k=j; k<NWING; k++) {
	    curv[j+k*NWING] += df[j] * df[k] / ((d[i]-dmin)*(d[i]-dmin));
	    if(j==XPO && k==XPO) { /* second deriv, probably not nec */
	       if(func == 1) {		/* power law */
		  curv[j+k*NWING] += diff * df[XPO] * log(r[i]) / 
		     ((d[i]-dmin)*(d[i]-dmin)); 
	       } else if(func == 2) {	/* exponential */
		  curv[j+k*NWING] += diff * df[XPO] * r[i] / 
		     ((d[i]-dmin)*(d[i]-dmin)); 
	       }
	    }
	    if(j==AMP && k==XPO) { /* second deriv, probably not nec */
	       if(func == 1) {		/* power law */
		  curv[j+k*NWING] += diff * expo * log(r[i]) / 
		     ((d[i]-dmin)*(d[i]-dmin)); 
	       } else if(func == 2) {	/* exponential */
		  curv[j+k*NWING] += diff * expo * r[i] / 
		     ((d[i]-dmin)*(d[i]-dmin)); 
	       }
	    }
	 }
      }
   }
   for(j=1; j<NWING; j++) {
      for(k=0; k<j; k++) {
	 curv[j+k*NWING] = curv[k+j*NWING];
      }
   }
   return(0);
}

#ifdef FANCY_STUFF
static int
exponential_bias(bias, ampl, efold, nx,ny,mx, im)
   double *bias;
   double *ampl, *efold;	/* Sky level at bottom * exp(efold*y) */
   int nx, ny, mx;		/* Size of image; x size of image storage */
   unsigned short *im;		/* Data array */
{
   int i, j, k, nsort, n;
   double a, b, c, yave, eave, yeave, e2ave, chimin=0.0;

/* Assemble medians in x as a function of y, fit exponential + constant */
   for(j=EDGE, n=0; j<ny-EDGE; j++, n++) {
      for(i=EDGE, nsort=0; i<=nx-1-EDGE; i++) {
	 strip[ny+nsort++] = im[i+j*mx];
      }
      strip[n] = median(nsort, strip+ny, NULL);
/*
      fprintf(stderr,"%2d %6.2f\n",j, strip[n]);
*/
   }

/* Evaluate chi^2 as a function of exponential arg */
   for(k=0; k<100; k++) {
      b = 0.002 * (k-50);	/* b = -0.1:+0.1 efold test */
      for(i=0, j=EDGE, yave=eave=yeave=e2ave=0.0; i<n; i++, j++) {
	 yave += strip[i];
	 eave += exp(b*j);
	 yeave += strip[i] * exp(b*j);
	 e2ave += exp(2*b*j);
      }
      a = (yeave - yave*eave/n) / MAX(1e-10,e2ave-eave*eave/n);
      c = (yave - a*eave)/n;
/* Stash chi^2 off end of strip array */
      for(i=0, j=EDGE, strip[k+n]=0.0; i<n; i++, j++) {
	 strip[k+n] += (strip[i]-c-a*exp(b*j)) * (strip[i]-c-a*exp(b*j));
      }
/*
      fprintf(stderr,"%.3f %6.2f ",b, strip[n+k]);
*/
/* Find the best fit as a function of k */
      if(k == 0 || strip[n+k] <= chimin) {
	 *ampl = a;
	 *efold = b;
	 *bias = c;
	 chimin = strip[n+k];
      }
   }
/*
   fprintf(stderr,"\n");
   fprintf(stderr,"%.3f %.3f %.3f\n", *ampl, *efold, *bias);
*/
   return(0);
}

static int
video_bias_sub(bias, ampl, efold, nx,ny,mx, im)
   double bias;
   double ampl, efold;		/* Sky level at bottom * exp(efold*y) */
   int nx, ny, mx;		/* Size of image; x size of image storage */
   unsigned short *im;		/* Data array */
{
   int i, j;
   double t;
   for(j=EDGE; j<ny-EDGE; j++) {
      for(i=EDGE; i<nx-EDGE; i++) {
	 t = im[i+j*mx] - bias - ampl * exp(efold*j);
	 if(t >= 0)  im[i+j*mx] = (int)(t+0.5);
	 else	     im[i+j*mx] = 0;
/*
	 if(j<EDGE || j>=ny-EDGE || i<EDGE || i>=nx-EDGE) im[i+j*mx] = 0;
*/
      }
   }
   return(0);
}
#endif

#if 0
static int
old_video_bias_sub(bias, ampl, efold, nx,ny,mx, im)
   double bias;
   double ampl, efold;		/* Sky level at bottom * exp(efold*y) */
   int nx, ny, mx;		/* Size of image; x size of image storage */
   unsigned short *im;		/* Data array */
{
   int i, j;
   double median;
/*
   for(j=EDGE; j<ny-EDGE; j++) {
      for(i=EDGE; i<=nx-1-EDGE; i++) {
*/
   for(j=0; j<ny; j++) {
      for(i=0; i<nx; i++) {
	 median = exp(ampl + efold*j);
	 im[i+j*mx] -= median + bias;
      }
   }
   return(0);
}

static int
old_exponential_bias(bias, ampl, efold, nx,ny,mx, im)
   double *bias;
   double *ampl, *efold;	/* Sky level at bottom * exp(efold*y) */
   int nx, ny, mx;		/* Size of image; x size of image storage */
   unsigned short *im;		/* Data array */
{
   int i, j, nsort, n;
   double sx, sx2, sy, sy2, sxy, med;
   double delx, dely, delxy;

/* Assemble medians in x as a function of y, fit linear function */
   sx = sx2 = sy = sy2 = sxy = 0;
   for(j=EDGE, n=0; j<ny-EDGE; j++) {
      for(i=EDGE, nsort=0; i<=nx-1-EDGE; i++) {
	 strip[nsort++] = log(MAX(1.0, im[i+j*mx]-*bias));
      }
      med = median(nsort, strip, NULL);
      n++;
      sx += j;
      sx2 += j*j;
      sy += med;
      sy2 += med*med;
      sxy += j*med;
/*      fprintf(stderr,"%2d %6.2f",n, med); */
   }
/*   fprintf(stderr,"\n"); */

/* Fit the profile now with a linear (i.e. exponential) function */
   delx = n*sx2 - sx*sx;
   dely = n*sy2 - sy*sy;
   delxy = n*sxy - sx*sy;
   if(delx != 0.0) {
      *ampl = (sx2*sy - sx*sxy) / delx;
      *efold = delxy / delx;
   } else {
      *ampl = *bias;
      *efold = 0.0;
   }

   return(0);
}
#endif

/* median.c : simple bubble sort, carry another array, return median
 *
 * 020130 John Tonry
 */
static
double median(int n, double *key, int *idx)
{
   int i, j, k, itmp=0;
   double tmp;

   for(j=n-2; j>=0; j--) {
      for(i=j+1, k=j; i<n; i++) {
	 if(key[j] <= key[i]) break;
	 k = i;
      }
      if(k == j) continue;
      tmp = key[j];
      if(idx != NULL) itmp = idx[j];
      for(i=j+1; i<=k; i++) {
	 key[i-1] = key[i];
	 if(idx != NULL) idx[i-1] = idx[i];
      }
      key[k] = tmp;
      if(idx != NULL) idx[k] = itmp;
   }
   tmp = 0.5 * (key[n/2]+key[(n-1)/2]);		/* Median */
   return(tmp);
}

/* linsolve.c - solve a set of linear equations */
/*
 * Solves the matrix equation   Y = A X,   returns X.
 *        where y[j=0,n-1], x[i=0,n-1], a[i+j*n].
 */
/* 020211 - John Tonry */
#define MAXEQ 100

static int
linsolve(int n, double *y, double *a, double *x)
{
   int i, j, k;
   int rowstatus[MAXEQ], row[MAXEQ];
   double rat;

   if(n > MAXEQ) {
      fprintf(stderr, "error: too many equations %d req %d max\n", n, MAXEQ);
      return(1);
   }

   for(i=k=0; i<n; i++) rowstatus[i] = 0;

/* Solve matrix by Gaussian elimination */
   for(j=0; j<n; j++) {		/* j = column to be zero'ed out */

/* Find a good equation to work on (pivot row) */
      for(i=0, rat=0.0; i<n; i++) {
	 if(rowstatus[i]) continue;
	 if(ABS(a[j+i*n]) > rat) {
	    k = i;
	    rat = ABS(a[j+k*n]);
	 }
      }
      rowstatus[k] = 1;
      row[j] = k;
      if(rat == 0.0) {
/*
	 fprintf(stderr, "WHOA: singular matrix, col %d, row %d\n", j, k);
*/
	 return(2);
      }
/* Subtract away matrix below diagonal */
      for(i=0; i<n; i++) {	/* i = subsequent equations */
	 if(rowstatus[i]) continue;
	 rat = a[j+i*n] / a[j+row[j]*n];
	 y[i] -= rat*y[row[j]];
 	 for(k=j+1; k<n; k++) a[k+i*n] -= rat*a[k+row[j]*n];
      }
   }

/* Back substitute into upper diagonal matrix to solve for parameters */
   for(j=n-1; j>=0; j--) {
      x[j] = y[row[j]];
      for(k=j+1; k<n; k++) x[j] -= a[k+row[j]*n]*x[k];
      x[j] = x[j] / a[j+row[j]*n];
   }
   return(0);
}
