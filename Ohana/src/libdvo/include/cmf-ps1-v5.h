
/** STRUCT DEFINITION **/
typedef struct {
  unsigned int     detID;                // detection ID                     
  float            X;                    // x coord (pixels)
  float            Y;                    // y coord (pixels)
  float            dX;                   // x coord error (pixels)
  float            dY;                   // y coord error (pixels)
  float            posangle;             // Posangle at source (degrees)
  float            pltscale;             // Plate Scale at source (arcsec/pixel)
  float            M;                    // inst mags (mags)
  float            dM;                   // inst mag error (mags)
  float            Flux;                 // psf flux (counts)
  float            dFlux;                // psf flux error (counts      )
  float            Map;                  // standard aperture mag (mags)
  float            MapRaw;               // raw aperture mag (mags)
  float            apRadius;             // radius used for aper (pixels)
  float            apFlux;               // aperture flux (counts)
  float            apFluxErr;            // error on ap flux (counts)
  int              apNpix;               // pixels used by aper (pixels)
  float            Mcalib;               // calibrated psf mag (mags)
  float            dMcal;                // zero point scatter (mags)
  float            Mpeak;                // peak flux as a mag (mags)
  double           RA;                   // PSF RA coord (degrees)
  double           DEC;                  // PSF DEC coord (degrees)
  float            sky;                  // sky flux (cnts/sec)
  float            dSky;                 // sky flux error (cnts/sec)
  float            psfChisq;             // psf fit chisq
  float            crNsigma;             // Nsigma deviations from PSF to CF
  float            extNsigma;            // Nsigma deviations from PSF to EXT
  float            fx;                   // psf fit major axis (pixels)
  float            fy;                   // psf fit minor axis (pixels)
  float            df;                   // ellipse angle (degrees)
  float            k;                    // extra PSF parameter (unitless)
  float            fwhmMaj;              // true fwhm of psf (pixels)
  float            fwhmMin;              // true fwhm (minor) (pixels)
  float            psfQF;                // quality factor
  float            psfQFperf;            // quality factor perfect
  int              psfNdof;              // psf degrees of freedom
  int              psfNpix;              // psf number of pixels
  float            Mxx;                  // second moment X (pixels^2)
  float            Mxy;                  // second moment Y (pixels^2)
  float            Myy;                  // second moment XY (pixels^2)
  float            M3c;                  // third moment cos(t) (pixels^3)
  float            M3s;                  // third moment sin(t) (pixels^3)
  float            M4c;                  // fourth moment cos(t) (pixels^4)
  float            M4s;                  // fourth moment sin(t) (pixels^4)
  float            Mr1;                  // first radial moment (pixels)
  float            Mrh;                  // half radial moment (pixels^1/2)
  float            kronFlux;             // kron flux (counts)
  float            kronFluxErr;          // kron flux error (counts)
  float            kronInner;            // kron flux 1<R<2.5 (counts)
  float            kronOuter;            // kron flux 2.5<R<4 (counts)
  float            skyLimitRad;          // profile to sky limit (radius)
  float            skyLimitFlux;         // profile to sky limit (flux)
  float            skyLimitSlope;        // profile to sky limit (slope)
  int              flags;                // analysis flags
  int              flags2;               // analysis flags (2)
  short            nFrames;              // images overlapping peak
  short            padding;              // padding for 8byte records
} CMF_PS1_V5;

CMF_PS1_V5 *gfits_table_get_CMF_PS1_V5 (FTable *table, off_t *Ndata, char *swapped);
int      gfits_table_set_CMF_PS1_V5 (FTable *ftable, CMF_PS1_V5 *data, off_t Ndata);
int      gfits_table_mkheader_CMF_PS1_V5 (Header *header);
int      gfits_convert_CMF_PS1_V5 (unsigned char *data, off_t size, off_t nitems, char toStruct);
int      Send_CMF_PS1_V5 (int device, CMF_PS1_V5 *data, int Ndata, int copy);
int      Recv_CMF_PS1_V5 (int device, CMF_PS1_V5 **data, int *Ndata);
