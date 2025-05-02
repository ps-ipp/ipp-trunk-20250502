
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
  float            apRadius;             // radius used for fit (pixels)
  float            apFlux;               // ap flux
  float            apFluxErr;            // ap flux err
  float            Mpeak;                // peak flux as a mag (mags)
  float            Mcalib;               // calibrated psf mag (mags)
  float            dMcal;                // zero point scatter (mags)
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
  float            psfQF;                // quality factor
  float            psfQFperf;            // quality factor perfect
  int              psfNdof;              // psf degrees of freedom
  int              psfNpix;              // psf number of pixels
  float            Mxx;                  // second moment X (pixels^2)
  float            Mxy;                  // second moment Y (pixels^2)
  float            Myy;                  // second moment XY (pixels^2)
  float            Mr1;                  // first radial moment (pixels)
  float            Mrh;                  // half radial moment (pixels^1/2)
  float            kronFlux;             // kron flux (counts)
  float            kronFluxErr;          // kron flux error (counts)
  float            kronInner;            // kron flux 1<R<2.5 (counts)
  float            kronOuter;            // kron flux 2.5<R<4 (counts)
  int              D_Npos;               // diff param
  float            D_Fratio;             // diff param
  float            D_Nratio_bad;         // diff param
  float            D_Nratio_mask;        // diff param
  float            D_Nratio_all;         // diff param
  float            D_Rp;                 // diff param
  float            D_SNp;                // diff param
  float            D_Rm;                 // diff param
  float            D_SNm;                // diff param
  int              flags;                // analysis flags
  int              flags2;               // analysis flags (2)
  short            nFrames;              // images overlapping peak
  short            padding;              // padding for 8byte records
} CMF_PS1_V3;

CMF_PS1_V3 *gfits_table_get_CMF_PS1_V3 (FTable *table, off_t *Ndata, char *swapped);
int      gfits_table_set_CMF_PS1_V3 (FTable *ftable, CMF_PS1_V3 *data, off_t Ndata);
int      gfits_table_mkheader_CMF_PS1_V3 (Header *header);
int      gfits_convert_CMF_PS1_V3 (CMF_PS1_V3 *data, off_t size, off_t nitems);
int      Send_CMF_PS1_V3 (int device, CMF_PS1_V3 *data, int Ndata, int copy);
int      Recv_CMF_PS1_V3 (int device, CMF_PS1_V3 **data, int *Ndata);
