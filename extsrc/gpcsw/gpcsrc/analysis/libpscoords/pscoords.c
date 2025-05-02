/* Routine to convert microns in the focal plane to OTA/cell index and posn */
/* Syntax: pscoords [args] < infile > outfile
 *  args:
 *     in=[sky | tp | fp | ota | cell]
 *     out=[sky | tp | fp | ota | cell]
 *
 *   and other arguments required for various transformations: see syntax()
 */
/* See PSDC-730-003-01 (OTA FITS) and PSDC-730-005-00 (GPC Arch) for details */
/* 080930 Sidik added TEST_MAIN - must be defined if you want main() test program. */
/* 080904 Rev 1.1, renamed from fpota.c */
/* 071023 Rev 1.0 John Tonry */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "pscoords.h"

static char
#if defined(__GNUC__) && defined(__STDC__)
__attribute__ ((__unused__))
#endif
rcsid[] = "$Id: pscoords.c,v 1.2 2008/09/06 03:26:48 jt Exp jt $";

#define MRAD2 (0.001)	/* Convert from mrad to rad, used for PSC_OFFROT_T */

/* Overall focal plane offset and rotation (um, rad) */
PSC_OFFROT_T psc_psc_fpoff={0.,0.,0.};

/* Offsets and rotations of each OTA (um, millirad) */
PSC_OFFROT_T psc_otaoff[PSC_NX*PSC_NY]={	/* Offsets and rotations derived o4885 090516 */
   {   0.0,   0.0,  0.00}, {  -6.8, -32.4, -1.30},  /* OTA00 , OTA10 */
   {  15.7,-130.7,  0.07}, {  -9.4, -43.5, -1.34},  /* OTA20 , OTA30 */
   {  17.8,  71.7, -0.48}, {   7.2,  41.3, -0.82},  /* OTA40 , OTA50 */
   {  22.1, -48.3, -1.74}, {   0.0,   0.0,  0.00},  /* OTA60 , OTA70 */
   { -23.0, -12.9,  0.68}, { -52.0, -52.2,  0.88},  /* OTA01 , OTA11 */
   {  28.8, -10.9, -0.73}, { -22.9,  18.1, -0.89},  /* OTA21 , OTA31 */
   { -13.8,  45.9,  0.90}, { -12.2,  49.7, -0.32},  /* OTA41 , OTA51 */
   { -31.9,  -9.8, -1.60}, {   1.9,   8.0,  0.53},  /* OTA61 , OTA71 */
   { -27.2,  42.6,  2.73}, {   6.5, -52.6,  0.48},  /* OTA02 , OTA12 */
   {  10.1, -37.9, -1.53}, {   3.1,-139.0,  1.75},  /* OTA22 , OTA32 */
   { -21.4,  25.6, -1.12}, { -13.8,   0.9,  0.02},  /* OTA42 , OTA52 */
   {  -1.8, -16.1, -2.06}, {  76.0,  31.7,  1.70},  /* OTA62 , OTA72 */
   { -15.7,  24.1,  0.14}, {-104.2, -76.8,  1.97},  /* OTA03 , OTA13 */
   {  75.4,  31.8,  1.04}, { -18.7,  14.6,  0.51},  /* OTA23 , OTA33 */
   { -30.6,  53.4, -0.78}, {  12.6,  24.4,  0.43},  /* OTA43 , OTA53 */
   {   8.0, -34.9,  1.44}, {   6.4, -73.2, -0.93},  /* OTA63 , OTA73 */
   { -31.2,  11.2,  0.23}, { -10.3,  -2.1,  0.14},  /* OTA04 , OTA14 */
   {  33.2, 207.9, -0.40}, { -75.7,-105.9,  0.93},  /* OTA24 , OTA34 */
   {   8.1, -15.9, -0.03}, { -16.1, -13.3, -0.06},  /* OTA44 , OTA54 */
   {   4.1,  47.6,  1.26}, {  71.6,  -6.0,  0.42},  /* OTA64 , OTA74 */
   { -23.4,  46.7,  2.65}, {  31.1,   5.5, -0.64},  /* OTA05 , OTA15 */
   { -43.2,  -6.5,  1.68}, { -26.0, -53.2,  0.00},  /* OTA25 , OTA35 */
   { -43.0,   9.1, -0.69}, {  30.1,  69.7,  0.62},  /* OTA45 , OTA55 */
   { -48.5,  61.6, -0.51}, {  65.5, -30.1,  0.69},  /* OTA65 , OTA75 */
   {   2.5,  57.2,  0.17}, {   0.6,  25.4, -0.91},  /* OTA06 , OTA16 */
   { -70.8, -17.4,  1.36}, {  13.6, -24.2,  0.53},  /* OTA26 , OTA36 */
   {  -6.8,  28.1, -1.35}, {  20.9,  42.0,  0.19},  /* OTA46 , OTA56 */
   {   6.2,  13.2, -1.50}, {  36.5,  48.8, -0.36},  /* OTA66 , OTA76 */
   {   0.0,   0.0,  0.00}, {  20.4,   1.7,  0.09},  /* OTA07 , OTA17 */
   {  83.8,  11.8, -0.56}, { -26.5, -21.2,  0.56},  /* OTA27 , OTA37 */
   { -70.5,   7.2, -1.05}, { -41.2,  -3.6, -1.89},  /* OTA47 , OTA57 */
   { -35.0,  49.5, -0.40}, {   0.0,   0.0,  0.00}}; /* OTA67 , OTA77 */

/* Offsets and rotations of each OTA (um, millirad) */
#ifdef RUN_THREE_V1
PSC_OFFROT_T psc_otaoff[PSC_NX*PSC_NY]={	/* Offsets and rotations derived o4885 090228 */
   {   0.0,   0.0,  0.00}, {  20.2, -59.5, -1.23},  /* OTA00 , OTA10 */
   {  28.1,-138.1,  0.18}, {   2.0, -43.6, -1.22},  /* OTA20 , OTA30 */
   {  33.6,  91.5, -0.38}, {  20.3,  48.8, -0.81},  /* OTA40 , OTA50 */
   {  22.5, -67.5, -1.48}, {   0.0,   0.0,  0.00},  /* OTA60 , OTA70 */
   {   4.8, -29.1,  0.83}, { -51.8, -51.7,  0.95},  /* OTA01 , OTA11 */
   {  31.8,  -8.6, -0.68}, { -13.7,  11.3, -0.82},  /* OTA21 , OTA31 */
   {  -2.3,  58.7,  0.92}, {   5.5,  67.5, -0.26},  /* OTA41 , OTA51 */
   { -11.8,   3.1, -1.55}, {  -3.0,   1.2,  0.36},  /* OTA61 , OTA71 */
   { -26.1,  40.6,  2.75}, {   0.4, -53.7,  0.53},  /* OTA02 , OTA12 */
   {  26.6, -56.8, -1.51}, {  19.4,-178.7,  1.78},  /* OTA22 , OTA32 */
   { -22.5,   4.8, -1.04}, { -14.4,  -1.9,  0.06},  /* OTA42 , OTA52 */
   {  19.2,  -7.8, -2.04}, {  89.9,  34.9,  1.79},  /* OTA62 , OTA72 */
   { -24.6,  20.4,  0.15}, {-103.2, -84.4,  1.98},  /* OTA03 , OTA13 */
   { 110.1,  11.1,  1.11}, {  12.6, -24.9,  0.53},  /* OTA23 , OTA33 */
   { -50.5,  34.5, -0.73}, { -11.0,  20.0,  0.49},  /* OTA43 , OTA53 */
   {  17.2, -29.8,  1.52}, {  27.9, -72.3, -0.84},  /* OTA63 , OTA73 */
   { -43.7,   1.5,  0.23}, { -21.4, -11.6,  0.23},  /* OTA04 , OTA14 */
   {  64.5, 208.5, -0.36}, { -48.1, -91.1,  0.98},  /* OTA24 , OTA34 */
   { -14.2,  16.8,  0.01}, { -41.3,   1.9,  0.01},  /* OTA44 , OTA54 */
   {  10.8,  49.3,  1.34}, {  89.7,  -7.2,  0.43},  /* OTA64 , OTA74 */
   { -29.2,  38.4,  2.74}, {  20.8,  -7.5, -0.59},  /* OTA05 , OTA15 */
   { -34.1,  -7.2,  1.70}, { -16.2, -36.1,  0.02},  /* OTA25 , OTA35 */
   { -51.9,  45.6, -0.60}, {  23.9,  82.4,  0.68},  /* OTA45 , OTA55 */
   { -34.9,  57.1, -0.47}, {  72.4, -32.0,  0.68},  /* OTA65 , OTA75 */
   {  16.2,  62.6,  0.17}, { -11.8,   7.5, -0.88},  /* OTA06 , OTA16 */
   { -82.3, -39.5,  1.37}, {  11.2, -40.0,  0.56},  /* OTA26 , OTA36 */
   {  -7.5,  31.1, -1.27}, {  26.4,  34.2,  0.26},  /* OTA46 , OTA56 */
   {  14.1,   3.6, -1.42}, {  20.1,  58.0, -0.23},  /* OTA66 , OTA76 */
   {   0.0,   0.0,  0.00}, {  50.4,  20.4, -0.47},  /* OTA07 , OTA17 */
   {  77.9,  -1.5, -0.60}, { -35.0, -47.0,  0.68},  /* OTA27 , OTA37 */
   { -74.8,   0.3, -0.92}, { -44.6,  -1.6, -1.78},  /* OTA47 , OTA57 */
   { -50.3,  72.1, -0.47}, {   0.0,   0.0,  0.00}}; /* OTA67 , OTA77 */
#endif

#ifdef RUN_TWO
/* Offsets and rotations of each OTA (um, millirad) */
const PSC_OFFROT_T psc_otaoff[PSC_NX*PSC_NY]={	/* Offsets and rotations derived o4699-o4724 */
   {   0.0,   0.0,  0.0 },  {  57.4, -71.7,  0.41},  /* OTA00 , OTA10 */
   {  35.0,-136.4,  0.29},  {  -7.6, -47.4,  2.20},  /* OTA20 , OTA30 */
   {  -4.1,  75.8,  0.52},  {  13.9,  19.4, -1.73},  /* OTA40 , OTA50 */
   {  27.3, -24.3,  0.62},  {   0.0,   0.0,  0.0 },  /* OTA60 , OTA70 */
   {  26.3, -36.1,  0.57},  { -54.7, -89.1,  1.64},  /* OTA01 , OTA11 */
   {  -1.1, -33.2, -1.09},  {  -4.7, -71.7, -0.25},  /* OTA21 , OTA31 */
   {  25.6,  51.8,  0.62},  {   2.1,  55.6,  0.32},  /* OTA41 , OTA51 */
   {  44.5,  41.5,  0.15},  {  53.8,  52.8,  0.33},  /* OTA61 , OTA71 */
   {   0.9,  25.3,  2.30},  { -33.1, -63.5,  2.85},  /* OTA02 , OTA12 */
   {  27.9, -87.0, -0.20},  {  62.6, -20.8,  0.50},  /* OTA22 , OTA32 */
   { -35.4, -14.7, -1.42},  {  24.6,  53.9, -0.82},  /* OTA42 , OTA52 */
   {  29.9,  36.7, -1.10},  { 100.5,  21.9,  1.27},  /* OTA62 , OTA72 */
   {  -1.1,  26.8, -1.16},  {  31.0, -46.0,  3.07},  /* OTA03 , OTA13 */
   { 113.2,  53.7,  0.39},  {  65.1, -17.3, -0.98},  /* OTA23 , OTA33 */
   { -54.9,  12.8,  0.10},  {  19.9,  30.6,  1.08},  /* OTA43 , OTA53 */
   {  35.3, -12.8,  1.68},  {  60.8,  30.3, -0.37},  /* OTA63 , OTA73 */
   { -79.8, -28.2,  0.53},  { -50.8, -63.1,  0.21},  /* OTA04 , OTA14 */
   {  33.4, -38.1, -0.18},  {   7.5, -89.4,  0.67},  /* OTA24 , OTA34 */
   {  -9.7,  56.5, -0.71},  {  -2.4,  42.5,  0.42},  /* OTA44 , OTA54 */
   {  -0.4,  32.8,  2.13},  {  23.1, -48.4,  0.41},  /* OTA64 , OTA74 */
   { -87.1,   7.8,  1.24},  { -42.1, -62.0,  2.01},  /* OTA05 , OTA15 */
   { -77.4,   2.4,  1.59},  { -38.4, -37.6,  1.00},  /* OTA25 , OTA35 */
   { -23.2,  50.4, -1.81},  {   1.0,  58.4,  0.33},  /* OTA45 , OTA55 */
   { -28.3,   7.1, -0.69},  {   3.8, -60.3, -0.15},  /* OTA65 , OTA75 */
   { -20.0,  28.7,  1.57},  { -26.0,  45.6,  3.08},  /* OTA06 , OTA16 */
   { -36.4, -69.0,  0.49},  {  26.1,  -4.9, -1.47},  /* OTA26 , OTA36 */
   { -24.2,  98.6,  0.09},  { -51.5,  14.7, -0.88},  /* OTA46 , OTA56 */
   {  16.6,  31.0, -1.34},  {   7.1,  12.4, -0.13},  /* OTA66 , OTA76 */
   {   0.0,   0.0,  0.0 },  {  -6.5,  39.8, -0.03},  /* OTA06 , OTA17 */
   { -15.7,   3.9,  0.65},  { -26.4,  16.5,  0.07},  /* OTA27 , OTA37 */
   { -41.1,  76.3,  0.56},  { -60.8,  43.0, -0.16},  /* OTA47 , OTA57 */
   { -52.6,  65.1,  0.58},  {   0.0,   0.0,  0.0 }}; /* OTA67 , OTA77 */
#endif

#ifdef FIRST_GUESS
/* Offsets and rotations of each OTA (um, rad) */
PSC_OFFROT_T psc_otaoff[PSC_NX*PSC_NY]={	/* Offsets and rotations derived o4678 */
   {   0.0,   0.0, 0.00000},  {  46.8, -82.5, 0.00102},  /* OTA00 , OTA10 */
   {  29.1,-130.8, 0.00145},  {   1.7, -46.3, 0.00315},  /* OTA20 , OTA30 */
   {  14.4,  70.8, 0.00095},  {  20.0,  30.3,-0.00145},  /* OTA40 , OTA50 */
   {   4.0,  -1.2, 0.00121},  {   0.0,   0.0, 0.00000},  /* OTA60 , OTA70 */
   {  -2.9, -67.9, 0.00090},  { -98.0,-105.0, 0.00107},  /* OTA01 , OTA11 */
   { -22.2, -23.0,-0.00171},  {  12.0, -72.2,-0.00056},  /* OTA21 , OTA31 */
   {  57.1,  33.0, 0.00030},  {  18.6,  54.9,-0.00055},  /* OTA41 , OTA51 */
   {  22.0,  80.7,-0.00063},  {  21.4,  81.2, 0.00027},  /* OTA61 , OTA71 */
   { -16.9,  23.8, 0.00248},  { -32.3, -46.8, 0.00181},  /* OTA02 , OTA12 */
   {  58.7, -45.0,-0.00045},  { 118.1,  -3.0, 0.00076},  /* OTA22 , OTA32 */
   {  32.3, -48.0,-0.00113},  {  76.6,  25.5,-0.00131},  /* OTA42 , OTA52 */
   {  46.2,  48.4,-0.00198},  { 104.5,  39.2, 0.00182},  /* OTA62 , OTA72 */
   {  -1.2,  42.8,-0.00095},  {  43.2, -10.4, 0.00274},  /* OTA03 , OTA13 */
   { 139.5, 114.3, 0.00086},  {  96.9,   7.9, 0.00011},  /* OTA23 , OTA33 */
   { -18.8, -22.2, 0.00117},  {  43.1, -23.5, 0.00148},  /* OTA43 , OTA53 */
   {  44.9, -33.1, 0.00138},  {  69.1,  25.9, 0.00006},  /* OTA63 , OTA73 */
   { -87.3, -12.1, 0.00112},  { -48.1, -43.0, 0.00014},  /* OTA04 , OTA14 */
   {  10.9,  20.4, 0.00041},  { -29.1, -59.1, 0.00185},  /* OTA24 , OTA34 */
   { -44.8,  30.6, 0.00053},  { -25.0, -15.0, 0.00084},  /* OTA44 , OTA54 */
   { -15.0,  -1.9, 0.00182},  {  18.0, -58.7, 0.00067},  /* OTA64 , OTA74 */
   { -92.8,   9.3, 0.00202},  { -55.1, -63.4, 0.00168},  /* OTA05 , OTA15 */
   {-125.6,  35.0, 0.00142},  {-112.7,  -6.6, 0.00137},  /* OTA25 , OTA35 */
   { -90.2,  36.4,-0.00160},  { -34.4,  20.9, 0.00005},  /* OTA45 , OTA55 */
   { -25.9, -11.9,-0.00160},  {  19.8, -59.5,-0.00020},  /* OTA65 , OTA75 */
   {   1.3,  43.8, 0.00208},  {   0.2,  21.7, 0.00234},  /* OTA06 , OTA16 */
   { -47.8, -61.4,-0.00023},  { -13.6,  21.8,-0.00201},  /* OTA26 , OTA36 */
   { -50.1, 104.1,-0.00061},  { -34.3,  14.9,-0.00148},  /* OTA46 , OTA56 */
   {  72.3,  53.8,-0.00213},  {  58.0,  47.6,-0.00012},  /* OTA66 , OTA76 */
   {   0.0,   0.0, 0.00000},  {  31.1,  44.6, 0.00113},  /* OTA07 , OTA17 */
   {  -8.2,   5.3, 0.00087},  { -39.8,  35.0, 0.00031},  /* OTA27 , OTA37 */
   { -47.8,  85.1, 0.00135},  { -28.8,  51.0, 0.00037},  /* OTA47 , OTA57 */
   {  -2.6,  91.5, 0.00071},  {   0.0,   0.0, 0.00000}}; /* OTA67 , OTA77 */
#endif

static int DOOFFSET=1;	/* Do these offsets or no? */

#if 0
#define IRAS	/* Gnomonic? irsa.ipac.edu/IRASdocs/surveys/coordproj.html */
#define PSC_REFRACT_CONST 55.7	/* Standard refraction ("/tanz) at STP */
#endif

/* Return default values for current fit*/
int psc_defaults(double *pscale, double *d2, double *d3, double *airdens)
{
   *pscale = PS_scale;
   *d2 = PS_d2;
   *d3 = PS_d3;
   *airdens = PS_airdens;
   return(0);
}

/* Enable application of chip offsets (um, mrad)? */
int psc_load_otaoff(const char *fname)
{
   int i;
   char rbuf[1024];
   FILE *fp;
   if( (fp=fopen(fname, "r")) == NULL) {
      fprintf(stderr, "error: Error opening OTA offset file '%s'\n", fname);
      return(-1);
   }
   for(i=0; i<PSC_NX*PSC_NY; i++) {
      if(fgets(rbuf, 1024, fp) == NULL || 
	 sscanf(rbuf, "%lf %lf %lf", &psc_otaoff[i].dx, 
		&psc_otaoff[i].dy, &psc_otaoff[i].rot) != 3) {
	 fprintf(stderr, "error: Error reading OTA offset file '%s' line %d\n",
		 fname, i);
	 fclose(fp);
	 return(-1);
      }
   }
   fclose(fp);
   return(0);
}

/* Load up a new offset table? */
int psc_do_chipoff(int doit)
{
   DOOFFSET = doit;
   return(0);
}

/* Convert a focal plane position to OTA otax,otay, Cell cellx,celly, Pixel */
int psc_fp_to_pixel(double xfp, double yfp, 	/* FP position (um) */
		int *ota_xid, int *ota_yid,	/* OTA ID (0:7) */
		double *xota, double *yota)	/* OTA position (pix) */
{
   double mechgap=0.5*(PSC_HMECH-PSC_HDIE);	/* Assume equal L/R/T chip gap */
   int idx;				/* Index of this OTA in otaoff */
   int lhs;				/* Left/right hand side = +1/-1 */
   double xc, yc, x, y, xp, yp;

/* Position in FP after removing offset and rotation */
   if(DOOFFSET) {
      x =  (xfp-psc_fpoff.dx)*cos(psc_fpoff.rot) + (yfp-psc_fpoff.dy)*sin(psc_fpoff.rot);
      y = -(xfp-psc_fpoff.dx)*sin(psc_fpoff.rot) + (yfp-psc_fpoff.dy)*cos(psc_fpoff.rot);
   } else {
      x = xfp;
      y = yfp;
   }

/* Guess at which side we are on */
   lhs = x > 0 ? +1 : -1;

/* Guess at which OTA this is */
   *ota_xid = NINT(PSC_NX/2-0.5-x/PSC_HMECH);
   *ota_yid = NINT((y-lhs*0.5*(PSC_VMOFF+PSC_VMECH-PSC_VDIE-2*mechgap))/PSC_VMECH+(PSC_NY/2-0.5));
/* Stop at the edges of the focal plane */
   if (*ota_xid < 0)       *ota_xid = 0;
   if (*ota_xid >= PSC_NX) *ota_xid = PSC_NX-1;
   if (*ota_yid < 0)       *ota_yid = 0;
   if (*ota_yid >= PSC_NY) *ota_yid = PSC_NY-1;
   idx = *ota_xid+*ota_yid*PSC_NX;

/* Center of nominal mechanical PSC_HMECH,PSC_VMECH envelope wrt FP center */
   xc = ((PSC_NX/2-0.5)-*ota_xid) * PSC_HMECH;
   yc = (*ota_yid-(PSC_NY/2-0.5)) * PSC_VMECH + lhs*0.5*(PSC_VMOFF+PSC_VMECH-PSC_VDIE-2*mechgap);

/* Center of OTA (8x8 set of cells) within mechanical envelope wrt FP center */
   xc -= lhs*(0.5*PSC_HMECH - mechgap - PSC_LBORDER - PSC_NX/2*PSC_HCELL - (PSC_NX/2-0.5)*PSC_VSTREET);
   yc -= lhs*(0.5*PSC_VMECH - mechgap - PSC_TBORDER - PSC_NY/2*PSC_VCELL - (PSC_NY/2-0.5)*PSC_HSTREET);

/* Position wrt nominal OTA center prior to offset and rotation correction */
   xp = x - xc;
   yp = y - yc;

/* Position wrt OTA center after correction for offset and rotation */
   if(DOOFFSET) {
      x =  (xp-psc_otaoff[idx].dx)*cos(psc_otaoff[idx].rot*MRAD2) + 
	 (yp-psc_otaoff[idx].dy)*sin(psc_otaoff[idx].rot*MRAD2);
      y = -(xp-psc_otaoff[idx].dx)*sin(psc_otaoff[idx].rot*MRAD2) + 
	 (yp-psc_otaoff[idx].dy)*cos(psc_otaoff[idx].rot*MRAD2);
   } else {
      x = xp;
      y = yp;
   }

/* Position in pixels relative to OTA origin */
   *xota =  (lhs*x + (PSC_NX/2*PSC_HCELL + (PSC_NX/2-0.5)*PSC_VSTREET)) / PSC_PIXEL;
   *yota = -(lhs*y - (PSC_NY/2*PSC_VCELL + (PSC_NY/2-0.5)*PSC_HSTREET)) / PSC_PIXEL;

   return(0);
}

/* Convert a pixel position in OTA ota_xid,ota_yid to FP */
int psc_pixel_to_fp(int ota_xid, int ota_yid,	/* OTA ID (0:7) */
		double xota, double yota,	/* OTA position (pix) */
		double *xfp, double *yfp)	/* FP position (um) */
{
   int idx=ota_xid+ota_yid*PSC_NX;		/* Index of this OTA in otaoff */
   double mechgap=0.5*(PSC_HMECH-PSC_HDIE);	/* Assume equal L/R/T chip gap */
   int lhs=ota_xid < PSC_NX/2 ? +1 : -1;	/* Left/right hand side = +1/-1 */
   double xc, yc, x, y, xp, yp;

/* Pixel position in um relative to OTA center */
   x = lhs * ( xota*PSC_PIXEL - (PSC_NX/2*PSC_HCELL + (PSC_NX/2-0.5)*PSC_VSTREET));
   y = lhs * (-yota*PSC_PIXEL + (PSC_NY/2*PSC_VCELL + (PSC_NY/2-0.5)*PSC_HSTREET));

/* Position wrt nominal OTA center after its offset and rotation */
   if(DOOFFSET) {
      xp = psc_otaoff[idx].dx + x*cos(psc_otaoff[idx].rot*MRAD2) - y*sin(psc_otaoff[idx].rot*MRAD2);
      yp = psc_otaoff[idx].dy + x*sin(psc_otaoff[idx].rot*MRAD2) + y*cos(psc_otaoff[idx].rot*MRAD2);
   } else {
      xp = x;
      yp = y;
   }

/* Center of nominal mechanical PSC_HMECH,PSC_VMECH envelope wrt FP center */
   xc = ((PSC_NX/2-0.5)-ota_xid) * PSC_HMECH;
   yc = (ota_yid-(PSC_NY/2-0.5)) * PSC_VMECH + lhs*0.5*(PSC_VMOFF+PSC_VMECH-PSC_VDIE-2*mechgap);

/* Center of OTA (8x8 set of cells) within mechanical envelope wrt FP center */
   xc -= lhs*(0.5*PSC_HMECH - mechgap - PSC_LBORDER - PSC_NX/2*PSC_HCELL - (PSC_NX/2-0.5)*PSC_VSTREET);
   yc -= lhs*(0.5*PSC_VMECH - mechgap - PSC_TBORDER - PSC_NY/2*PSC_VCELL - (PSC_NY/2-0.5)*PSC_HSTREET);

/* Position within FP */
   x = xp + xc;
   y = yp + yc;

/* Final position from FP offset and rotation */
   if(DOOFFSET) {
      *xfp = psc_fpoff.dx + x*cos(psc_fpoff.rot) - y*sin(psc_fpoff.rot);
      *yfp = psc_fpoff.dy + x*sin(psc_fpoff.rot) + y*cos(psc_fpoff.rot);
   } else {
      *xfp = x;
      *yfp = y;
   }

   return(0);
}

/* Convert an OTA pixel position to cell ID and cell coords */
int psc_pixel_to_cell(double xota, double yota,	/* OTA position (pix) */
		  int *cell_xid, int *cell_yid,	/* Cell ID (0:7) or -1 */
		  double *xcell, double *ycell)	/* Cell position (pix) */
{
   double x, y;
   x = xota*PSC_PIXEL / (PSC_HCELL+PSC_VSTREET);
   *cell_xid = (int)x;
   *xcell = (x - (int)x) * (PSC_HCELL+PSC_VSTREET) / PSC_PIXEL;
   if(x < 0 || *cell_xid > PSC_NX-1 || 
      (x-*cell_xid) > 1-PSC_VSTREET/(PSC_HCELL+PSC_VSTREET)) *cell_xid = -1;
   *xcell = PSC_HCELL/PSC_PIXEL - *xcell;

   y = yota*PSC_PIXEL / (PSC_VCELL+PSC_HSTREET);
   *cell_yid = (int)y;
   *ycell = (y - (int)y) * (PSC_VCELL+PSC_HSTREET) / PSC_PIXEL;
   if(y < 0 || *cell_yid > PSC_NY-1 ||
      (y-*cell_yid) > 1-PSC_HSTREET/(PSC_VCELL+PSC_HSTREET)) *cell_yid = -1;

   return(0);
}

/* Convert a cell position and ID to OTA coords */
int psc_cell_to_pixel(int cell_xid, int cell_yid,	/* Cell ID (0:7) */
		  double xcell, double ycell,	/* Cell position (pix) */
		  double *xota, double *yota)	/* OTA position (pix) */
{
   *xota = (PSC_HCELL/PSC_PIXEL - xcell) + cell_xid*(PSC_HCELL+PSC_VSTREET) / PSC_PIXEL;
   *yota = ycell + cell_yid*(PSC_VCELL+PSC_HSTREET) / PSC_PIXEL;
   return(0);
}

/* Evaluate an optical model: arcsec -> microns notionally */
/* Offset, scale with quadratic and cubic distortion, rotate */
int psc_psoptics2(double x, double y, double dx, double dy, double dpa, 
	 double pscale, double d2, double d3, double *xfp, double *yfp)
{
   double w2=((x-dx)*(x-dx)+(y-dy)*(y-dy)), w;
   w = sqrt(w2);
   x = pscale * (1.0 + d2*w + d3*w2) * (x-dx);
   y = pscale * (1.0 + d2*w + d3*w2) * (y-dy);
/* Apply any extra TP-FP rotation */
   *xfp =  x * cos(dpa) + y * sin(dpa);
   *yfp = -x * sin(dpa) + y * cos(dpa);
   return(0);
}

/* Evaluate an inverse optical model: microns -> arcsec notionally */
/* Offset, scale with *small* quadratic and cubic distortion, rotate */
int psc_invoptics2(double xfp, double yfp, double dx, double dy, double dpa, 
	  double pscale, double d2, double d3, double *x, double *y)
{
   double xp, yp, w;
/* Undo any extra TP-FP rotation */
   xp = (xfp * cos(dpa) - yfp * sin(dpa)) / pscale;
   yp = (xfp * sin(dpa) + yfp * cos(dpa)) / pscale;
   w = sqrt(xp*xp+yp*yp);
   *x = xp / (1.0 + d2*w + d3*(xp*xp+yp*yp)) + dx;
   *y = yp / (1.0 + d2*w + d3*(xp*xp+yp*yp)) + dy;
   return(0);
}


/* Evaluate an optical model: arcsec -> microns notionally */
/* Offset, scale with distortion, rotate */
int psc_psoptics(double x, double y, double dx, double dy, double dpa, 
	 double pscale, double d3, double *xfp, double *yfp)
{
   double w2=((x-dx)*(x-dx)+(y-dy)*(y-dy));
   x = pscale * (1.0 + d3*w2) * (x-dx);
   y = pscale * (1.0 + d3*w2) * (y-dy);
/* Apply any extra TP-FP rotation */
   *xfp =  x * cos(dpa) + y * sin(dpa);
   *yfp = -x * sin(dpa) + y * cos(dpa);
   return(0);
}

/* Evaluate an inverse optical model: microns -> arcsec notionally */
/* Offset, scale with *small* distortion, rotate */
int psc_invoptics(double xfp, double yfp, double dx, double dy, double dpa, 
	  double pscale, double d3, double *x, double *y)
{
   double xp, yp;
/* Undo any extra TP-FP rotation */
   xp = (xfp * cos(dpa) - yfp * sin(dpa)) / pscale;
   yp = (xfp * sin(dpa) + yfp * cos(dpa)) / pscale;
   *x = xp / (1.0 + d3*(xp*xp+yp*yp)) + dx;
   *y = yp / (1.0 + d3*(xp*xp+yp*yp)) + dy;
   return(0);
}

/* Project RA,Dec (a,d) to the tangent plane (x,y) at (a0,d0,pa) [radians] */
/* y is rotated CCW by pa from N; NOTE: x is west when PA = 0 (pos parity) */
int psc_tproject(double a, double d, double a0, double d0, double pa,
	 double density, double zenith, double vertical, 
	 double *x, double *y)
{
#ifndef IRAS
   double pdotx, pdoty, pdotz, E, N, dx, dy, z, Up, Horiz, sec2rad=atan(1.0)/(45*3600);
   pdotx = cos(a0)*cos(d0)*cos(a)*cos(d) + 
      sin(a0)*cos(d0)*sin(a)*cos(d) + sin(d0)*sin(d);
   pdoty = -sin(a0)*cos(a)*cos(d) + cos(a0)*sin(a)*cos(d);
   pdotz = -cos(a0)*sin(d0)*cos(a)*cos(d) -
      sin(a0)*sin(d0)*sin(a)*cos(d) + cos(d0)*sin(d);
   E = pdoty / pdotx;
   N = pdotz / pdotx;
   *x = -E*cos(pa) + N*sin(pa);
   *y =  E*sin(pa) + N*cos(pa);
/* Differential refraction? */
   if(density > 0) {
/* Rotate to vertical */
      Horiz =  *x * cos(vertical) + *y * sin(vertical);
      Up = -*x * sin(vertical) + *y * cos(vertical);
      z = sqrt(Horiz*Horiz+(zenith-Up)*(zenith-Up));
      dx = -density*sec2rad*PSC_REFRACT_CONST*tan(z)*Horiz/z;
      dy = density*sec2rad*PSC_REFRACT_CONST*(tan(z)*(zenith-Up)/z - tan(zenith));
      Horiz += dx;
      Up += dy;
/* Rotate back */
      *x =  Horiz * cos(vertical) - Up * sin(vertical);
      *y =  Horiz * sin(vertical) + Up * cos(vertical);
   }

#else
   double C, F, E, N;
   C = cos(d)*cos(a-a0);
   F = 1/(sin(d0)*sin(d)+C*cos(d0));
   N = -F*(cos(d0)*sin(d) - C*sin(d0));
   E = -F*cos(d)*sin(a-a0);
   *x =  E*cos(pa) + N*sin(pa);
   *y = -E*sin(pa) + N*cos(pa);
#endif
   return(0);
}

/* Convert tangent plane (x,y) at (a0,d0,pa) to RA,Dec (a,d) [radians] */
/* y is rotated CCW by pa from N; NOTE: x is west when PA = 0 (pos parity) */
int psc_tplonglat(double x, double y, double a0, double d0, double pa,
	  double density, double zenith, double vertical, 
	  double *a, double *d)
{
#ifndef IRAS
   double px, py, pz, E, N, pi=4*atan(1.0), dx, dy, z, sec2rad=atan(1.0)/(45*3600);
   double Horiz, Up;
/* Undo differential refraction? */
   if(density > 0) {
/* Rotate to vertical */
      Horiz =  x * cos(vertical) + y * sin(vertical);
      Up = -x * sin(vertical) + y * cos(vertical);
      z = sqrt(Horiz*Horiz+(zenith-Up)*(zenith-Up));
      dx = -density*sec2rad*PSC_REFRACT_CONST*tan(z)*Horiz/z;
      dy = density*sec2rad*PSC_REFRACT_CONST*(tan(z)*(zenith-Up)/z - tan(zenith));
      Horiz -= dx;
      Up -= dy;
/* Rotate back */
      x =  Horiz * cos(vertical) - Up * sin(vertical);
      y =  Horiz * sin(vertical) + Up * cos(vertical);
   }
   E = -x*cos(pa) + y*sin(pa);
   N =  x*sin(pa) + y*cos(pa);
   px = cos(a0)*cos(d0) - E*sin(a0) - N*cos(a0)*sin(d0);
   py = sin(a0)*cos(d0) + E*cos(a0) - N*sin(a0)*sin(d0);
   pz = sin(d0) + N*cos(d0);
   *a = atan2(py, px);
   if(pz >= 1)       *d =  pi/2;
   else if(pz <= -1) *d = -pi/2;
   else              *d = asin(pz);
#else
   double DD, B, XX, YY, E, N;
   E = -x*cos(pa) + y*sin(pa);
   N =  x*sin(pa) + y*cos(pa);
   DD = atan(sqrt(x*x+y*y));
   B = atan2(-x, y);
   XX = sin(d0)*sin(DD)*cos(B) + cos(d0)*cos(DD);
   YY = sin(DD)*sin(B);
   *a = a0 + atan2(YY, XX);
   *d = asin(sin(d0)*cos(DD)-cos(d0)*sin(DD)*cos(B));
#endif
   return(0);
}

#ifdef TEST_MAIN
static
void bomb(char *msg)
{
   fprintf(stderr, "%s", msg);
   exit(1);
}

static
void syntax(char *prog)
{
   printf("Syntax: %s [options] < input_coords > output coords\n", prog);
   printf("\nOptions:\n");
   printf("  in=[sky | tp | fp | ota | cell]     Select input coord type\n");
   printf("  out=[sky | tp | fp | ota | cell]    Select output coord type\n");
   printf("\nRequired extra arguments:\n");
   printf("  sky:   requires ra=R dec=D pa=P for boresight [deg]\n");
   printf("  tp-fp: requires dx=X dy=Y [\"] dpa=P [deg] sc=S [um/\"] [d3=D [\"^-2]] for optical model [deg]\n");
   printf("\nOptional extra arguments:\n");
   printf("  offset=[t|f]    Apply OTA/FP offsets (default f)\n");
   printf("\nInputs and outputs:\n");
   printf("  sky:   RA, Dec [deg]\n");
   printf("  tp:    x,y rotated by PA from N [arcsec]\n");
   printf("  fp:    xfp,yfp [um] (aka camera coords)\n");
   printf("  ota:   OTAxy ID and xota,yota [pix]\n");
   printf("  cell:  OTAxy Cellxy IDs and xcell,ycell [pix]\n");
   printf("   (OTAxy can be omitted for OTA-Cell conversions only\n");
   printf("\nSynonyms:\n");
   printf("  ra=R    or    -ra R     Boresight pointing (deg)\n");
   printf("  dec=D   or    -dec D    Boresight pointing (deg)\n");
   printf("  pa=P    or    -pa P     PA of image on sky (deg)\n");
   printf("  dx=X    or    -dx X     FP offset wrt boresight (arcsec)\n");
   printf("  dy=Y    or    -dy Y     FP offset wrt boresight (arcsec)\n");
   printf("  dpa=P   or    -dpa P    FP rotation wrt PA (deg)\n");
   printf("  sc=S    or    -sc S     Plate scale (um/arcsec)\n");
   printf("  d3=D    or    -d3 D     Cubic distortiont (arcsec^-2)\n");
   printf("  offset=t         or    -dooff    Apply OTA offsets? (def=f)\n");
   printf("  offset=f         or    -nooff    Apply OTA offsets? (def=f)\n");
   printf("  in=sky out=tp    or    -skytp\n");
   printf("  in=tp out=fp     or    -tpfp\n");
   printf("  in=fp out=ota    or    -fpota\n");
   printf("  in=cell out=ota  or    -clota\n");
   printf("  in=ota out=fp    or    -otafp\n");
   printf("\nTo implement differential refraction in Sky-TP:\n");
   printf("  alt=A   or    -alt A     Altitude (deg)\n");
   printf("  dens=D  or    -dens D    P/T (wrt STP, 0.71 for HK)\n");
   printf("  vert=V  or    -vert V    Rotator angle (deg)\n");
   printf("\nExamples:\n");
   printf(" echo 33 13 277.36 61.79 | pscoords out=sky in=cell ra=105 dec=7 pa=10 dx=0 dy=112 dpa=20 sc=38.856\n");
   printf(" -> 105.100000   7.100022\n");
   printf(" echo 105.1 7.1 | pscoords out=cell in=sky ra=105 dec=7 pa=10 dx=0 dy=112 dpa=20 sc=38.856\n");
   printf(" -> 33 13    277.36     61.79\n");
   printf("(note slight bug in Sky<->TP projections depending on distance from pole)\n");
   printf("\nVersion:  %s\n", rcsid);

   exit(0);
}
#endif /* TEST_MAIN */
