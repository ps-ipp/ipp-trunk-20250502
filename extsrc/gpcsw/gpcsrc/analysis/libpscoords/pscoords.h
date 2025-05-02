/* Header file for pscoords.c */
#define NINT(x) (x<0?(int)((x)-0.5):(int)((x)+0.5))

#define OTA_CTE (5.0e-6)	/* Coefficient of thermal expansion of Si+Mo */
#define CFFP_CTE (0.5e-6)	/* Coefficient of thermal expansion of CFFP */
#define DELTA_T (-100.0)	/* Operating temp minus metrology temp */

#define OTA_SCALE (1.0+OTA_CTE*DELTA_T)	/* Room temp um to operating um of Si+Mo */
#define CFFP_SCALE ((1.0+CFFP_CTE*DELTA_T)*25400) /* Room temp inches to um on CFFP */

/* Silicon layout of OTAs (um) */
#define PSC_PIXEL     (10.0*OTA_SCALE)	/* Pixel size of an OTA */
#define PSC_HCELL   (5900.0*OTA_SCALE)	/* Horizontal cell size */
#define PSC_VCELL   (5980.0*OTA_SCALE)	/* Vertical cell size */
#define PSC_HSTREET  (120.0*OTA_SCALE)	/* Horizontal street between cells (HS) */
#define PSC_VSTREET  (180.0*OTA_SCALE)	/* Vertical street between cells (VS) */
#define PSC_LBORDER  (442.0*OTA_SCALE)	/* Left border (to left-most cell) (LB) */
#define PSC_RBORDER  (418.0*OTA_SCALE)	/* Right border (to right-most vertical street) (RB) */
#define PSC_TBORDER  (354.5*OTA_SCALE)	/* Top border (to top-most cell) (TB) */
#define PSC_BBORDER  (975.5*OTA_SCALE)	/* Bottom border (to lowest horizontal street) (BB) */
#define PSC_HDIE   (49500.0*OTA_SCALE)	/* Horizontal size of silicon die */
#define PSC_VDIE   (50130.0*OTA_SCALE)	/* Vertical size of silicon die */

/* Mechanical layout of OTAs (um) */
#define PSC_HMECH  (1.957*CFFP_SCALE)	/* Horizontal mech spacing of OTA placement */
#define PSC_VMECH  (2.025*CFFP_SCALE)	/* Vertical mech spacing of OTA placement */
#define PSC_VMOFF (PSC_VDIE-(1.568+0.375)*CFFP_SCALE) /* Vert die offset between sides */

#define PSC_REFRACT_CONST 60.0	/* Standard refraction ("/tanz) at STP */

/* Number of OTAs populating focal plane */
#define PSC_NX	     8  /* Number of OTAs in FP in x */
#define PSC_NY	     8  /* Number of OTAs in FP in y */

/* Cell layout, looking down on back illuminated cell:
 *  NOTE: center of pixel 0,0 is physical location 0.5,0.5 pixel;
 *        cell origin is lower left edge of P0,0.
 *
 *                +------------------+
 *                | P0,597  P589,597 |
 *                |  ...    ...      |
 *     Amplifier<<| P0,0    P589,0   |
 *                +------------------+
 */

/* OTA layout, looking down on back illuminated CCD:
 * NOTE:  OTA coordinates increase in x from Cell xy0n to Cell xy7n, y from
 *          Cell xyn0 to Cell xyny, origin is P0,0 in C00, units are pixels.
 *        "center" of OTA is the middle of the streets between cell 3 and 4
 *        therefore center is at (xota, yota) = (2423.0, 2434.0)
 *        Cell coordinates are flipped wrt OTA coordinates, so the x=589
 *        pixel is the left most one in OTA coordinates, i.e in OTA
 *        coordinates a cell looks like:    +---------------+
 *                                          | 589,597 0,597 |
 *                                          | ...      ...  |
 *                                          | 589,0   0,0   |> Amp
 *                                          +---------------+
 *    +--------Top----------+
 *    |         TB          |
 *    L  C07 VS ... C77 VS  R  Cnn is 590x598 10um pixels
 *    e   HS         HS     i
 *    fLB...         ...  RBg
 *    t  C00 VS ... C70 VS  h
 *    |   HS         HS     t
 *    |         BB          |
 *    +-------Bottom--------+ (amp side)
 */

/* FP layout, looking down on FP:
 *
 * NOTE: edge to edge spacing of dice is HMECH,VMECH;
 *       RHS vertically offset by VMOFF from LHS
 *          to align pixel i,597 on LHS with pixel i,0 on RHS;
 *
 *       "center" is midpoint: OTA33 Cell00 Pix00 and OTA44 Cell00 Pix00
 *
 *       Physical coordinate convention runs right to left in x, in 
 *       order to comply with the "sky view" convention.
 *
 *          +--B--+ +--B--+ +--B--+ (VMOFF)
 *          |     | |     | |     | +--T--+ +--T--+ +--T--+
 *          R 17  L R 27  L R 37  L |     | |     | |     |
 *          |     | |     | |     | L 47  R L 57  R L 67  R
 *          +--T--+ +--T--+ +--T--+ |     | |     | |     |
 *  +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+
 *  |     | |     | |     | |     | +--T--+ +--T--+ +--T--+ +--T--+
 *  R 06  L R 16  L R 26  L R 36  L |     | |     | |     | |     |
 *  |     | |     | |     | |     | L 46  R L 56  R L 66  R L 76  R
 *  +--T--+ +--T--+ +--T--+ +--T--+ |     | |     | |     | |     |
 *  +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+
 *  |     | |     | |     | |     | +--T--+ +--T--+ +--T--+ +--T--+
 *  R 05  L R 15  L R 25  L R 35  L |     | |     | |     | |     |
 *  |     | |     | |     | |     | L 45  R L 55  R L 65  R L 75  R
 *  +--T--+ +--T--+ +--T--+ +--T--+ |     | |     | |     | |     |
 *  +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+
 *  |     | |     | |     | |     | +--T--+ +--T--+ +--T--+ +--T--+
 *  R 04  L R 14  L R 24  L R 34  L |     | |     | |     | |     |
 *  |     | |     | |     | |     | L 44  R L 54  R L 64  R L 74  R
 *  +--T--+ +--T--+ +--T--+ +--T--+ |     | |     | |     | |     |
 *  +--B--+ +--B--+ +--B--+ +--B--+X+--B--+ +--B--+ +--B--+ +--B--+
 *  |     | |     | |     | |     | +--T--+ +--T--+ +--T--+ +--T--+
 *  R 03  L R 13  L R 23  L R 33  L |     | |     | |     | |     |
 *  |     | |     | |     | |     | L 43  R L 53  R L 63  R L 73  R
 *  +--T--+ +--T--+ +--T--+ +--T--+ |     | |     | |     | |     |
 *  +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+
 *  |     | |     | |     | |     | +--T--+ +--T--+ +--T--+ +--T--+
 *  R 02  L R 12  L R 22  L R 32  L |     | |     | |     | |     |
 *  |     | |     | |     | |     | L 42  R L 52  R L 62  R L 72  R
 *  +--T--+ +--T--+ +--T--+ +--T--+ |     | |     | |     | |     |
 *  +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+
 *  |     | |     | |     | |     | +--T--+ +--T--+ +--T--+ +--T--+
 *  R 01  L R 11  L R 21  L R 31  L |     | |     | |     | |     |
 *  |     | |     | |     | |     | L 41  R L 51  R L 61  R L 71  R
 *  +--T--+ +--T--+ +--T--+ +--T--+ |     | |     | |     | |     |
 *          +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+ +--B--+
 *          |     | |     | |     | +--T--+ +--T--+ +--T--+
 *          R 10  L R 20  L R 30  L |     | |     | |     |           y
 *          |     | |     | |     | L 40  R L 50  R L 60  R           ^
 *          +--T--+ +--T--+ +--T--+ |     | |     | |     |           |
 *                                  +--B--+ +--B--+ +--B--+           |
 *                                                            x  <----+
 */

/* Structure for tweaks of OTA positions wrt mechanical */
typedef struct {
      double dx;	/* OTA x origin found at dx wrt mechanical (um) */
      double dy;	/* OTA y origin found at dy wrt mechanical (um) */
      double rot;	/* OTA rotated CCW by rot wrt mechanical (rad) */
} PSC_OFFROT_T;

/* Overall focal plane offset and rotation (um, rad) */
PSC_OFFROT_T psc_fpoff;

/* Offsets and rotations of each OTA (um, millirad) */
PSC_OFFROT_T psc_otaoff[PSC_NX*PSC_NY];


/* This is from Run 3, 090516 */
#define PS_scale 38.7932	/* Default PS plate scale [um/arcsec] */
#define PS_d2  9.78e-7	/* Default quadratic PS distortion [arcsec^-1] */
#define PS_d3  3.16e-11	/* Default cubic PS distortion [arcsec^-2] */


#ifdef RUN_THREE_V1	/* This is from Run 3, 090228 */
#define PS_scale 38.860	/* Default PS plate scale [um/arcsec] */
#define PS_d3  1.49e-10	/* Default cubic PS distortion [arcsec^-2] */
#endif


#define PS_airdens 0.71	/* Default PS1 qir density (Haleakala) */

/**********************/
/* Normal Application */
/**********************/
/*
 * To go from a position (a,d) relative to (a0,d0,pa) pointing to cell coords:
 * ---------------------------------------------------------------------------
 *   psc_tproject(a, d, a0, d0, pa, density, pi/2-alt, vertical, &x, &y);
 *   psc_psoptics2(x/sec2rad, y/sec2rad, dx, dy, dpa, pscale, d2, d3, &xfp, &yfp);
 *   psc_fp_to_pixel(xfp, yfp, &ota_xid, &ota_yid, &xota, &yota);
 *   psc_pixel_to_cell(xota, yota, &cell_xid, &cell_yid, &xcell, &ycell);
 *
 *  can also use
 *   psc_psoptics(x/sec2rad, y/sec2rad, dx, dy, dpa, pscale, d3, &xfp, &yfp);
 *
 *
 * To go from cell coords to a position (a,d) relative to (a0,d0,pa) pointing:
 * ---------------------------------------------------------------------------
 *   psc_cell_to_pixel(cell_xid,cell_yid, xcell,ycell, &xota,&yota);
 *   psc_pixel_to_fp(ota_xid, ota_yid, xota, yota, &xfp, &yfp);
 *   psc_invoptics2(xfp, yfp, dx, dy, dpa, pscale, d2, d3, &x, &y);
 *   psc_tplonglat(x*sec2rad, y*sec2rad, a0, d0, pa, density,  pi/2-alt, vertical, &a, &d);
 *
 *  can also use
 *   psc_invoptics(xfp, yfp, dx, dy, dpa, pscale, d3, &x, &y);
 */

/**************/
/* Prototypes */
/**************/
/* Enable application of chip offsets? */
int psc_do_chipoff(int doit); 	/* Apply chip offsets? (0/1, default=1) */

/* Convert a focal plane position to OTA otax,otay, Cell cellx,celly, Pixel */
int psc_fp_to_pixel(double xfp_um, double yfp_um, 	/* FP position (um) */
		int *ota_xid, int *ota_yid,		/* OTA ID (0:7) */
		double *xota_pix, double *yota_pix);	/* OTA position (pix) */

/* Convert a pixel position in OTA ota_xid,ota_yid to FP */
int psc_pixel_to_fp(int ota_xid, int ota_yid,		/* OTA ID (0:7) */
		double xota_pix, double yota_pix,	/* OTA position (pix) */
		double *xfp_um, double *yfp_um);	/* FP position (um) */

/* Convert an OTA pixel position to cell ID and cell coords */
int psc_pixel_to_cell(double xota_pix, double yota_pix,	/* OTA position (pix) */
		  int *cell_xid, int *cell_yid,		/* Cell ID (0:7) or -1 */
		  double *xcell_pix, double *ycell_pix);/* Cell position (pix) */

/* Convert a cell position and ID to OTA coords */
int psc_cell_to_pixel(int cell_xid, int cell_yid,	/* Cell ID (0:7) */
		  double xcell_pix, double ycell_pix,	/* Cell position (pix) */
		  double *xota_pix, double *yota_pix);	/* OTA position (pix) */

/* Convert tangent plane (x,y) at (a0,d0,pa) to RA,Dec (a,d) [radians] */
/* y is rotated CCW by pa from N; NOTE: x is west when PA = 0 (pos parity) */
/* Set density=0 to apply no correction for differential refraction, otherwise P/T in STP */
int psc_tplonglat(double x_rad, double y_rad, double a0_rad, double d0_rad, double pa_rad,
		  double density/*0.71 for HK*/, double zenith_rad/*pi/2-ALT*/, 
		  double vertical_rad/*ROT*/, double *a_rad, double *d_rad);

/* Project RA,Dec (a,d) to the tangent plane (x,y) at (a0,d0,pa) [radians] */
/* y is rotated CCW by pa from N; NOTE: x is west when PA = 0 (pos parity) */
/* Set density=0 to apply no correction for differential refraction, otherwise P/T in STP */
int psc_tproject(double a_rad, double d_rad, double a0_rad, double d0_rad, double pa_rad,
		 double density/*0.71 for HK*/, double zenith_rad/*pi/2-ALT*/, 
		 double vertical_rad/*ROT*/, double *x_rad, double *y_rad);

/* Evaluate an inverse optical model: microns -> arcsec notionally */
/* Offset, scale with *small* distortion, rotate */
int psc_invoptics(double xfp_um, double yfp_um, double dx_sec, double dy_sec, double dpa_rad, 
		  double pscale/*38.86um/sec*/, double d3/*1.49e-10sec^-2*/, 
		  double *x_sec, double *y_sec);

/* Evaluate an optical model: arcsec -> microns notionally */
/* Offset, scale with distortion, rotate */
int psc_psoptics(double x_sec, double y_sec, double dx_sec, double dy_sec, double dpa_rad, 
		 double pscale/*38.86um/sec*/, double d3/*1.49e-10sec^-2*/, 
		 double *xfp_um, double *yfp_um);

/* Evaluate an inverse optical model: microns -> arcsec notionally */
/* Offset, scale with *small* distortion, rotate */
int psc_invoptics2(double xfp_um, double yfp_um, double dx_sec, double dy_sec, double dpa_rad, 
		   double pscale/*38.793um/sec*/,
		   double d2/*9.8e-7sec^-2*/, double d3/*3.1e-11sec^-2*/, 
		   double *x_sec, double *y_sec);

/* Evaluate an optical model: arcsec -> microns notionally */
/* Offset, scale with distortion, rotate */
int psc_psoptics2(double x_sec, double y_sec, double dx_sec, double dy_sec, double dpa_rad, 
		  double pscale/*38.793um/sec*/,
		  double d2/*9.8e-7sec^-2*/, double d3/*3.1e-11sec^-2*/, 
		  double *xfp_um, double *yfp_um);

/* Enable application of chip offsets (um, mrad)? */
int psc_load_otaoff(const char *fname);

/* Return default values for current fit*/
int psc_defaults(double *pscale, double *d2, double *d3, double *airdens);
