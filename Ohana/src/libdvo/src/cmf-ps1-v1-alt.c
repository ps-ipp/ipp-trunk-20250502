# include "dvo.h"

/* if we are not correctly including the ohana headers, this will fail */
# ifndef BYTE_SWAP
# ifndef NOT_BYTE_SWAP
# error "neither BYTE_SWAP not NOT_BYTE_SWAP is set"
# endif
# endif

CMF_PS1_V2 *gfits_table_get_CMF_PS1_V1_Alt (FTable *ftable, off_t *Ndata, char *swapped) {

  off_t i, nitems;
  unsigned char *byte, *inbyte, *otbyte, tmp;
  CMF_PS1_V2 *output;

  /* provide initial values to avoid compiler warnings for non-BYTE_SWAP arch */
  i = tmp = 0;
  byte = NULL;

  // this function is a special case : it must have Nx = 136
  if (ftable[0].header[0].Naxis[0] != 136) { 
    fprintf (stderr, "ERROR: wrong format for CMF_PS1_V1_Alt: "OFF_T_FMT" vs %d\n",  ftable[0].header[0].Naxis[0], 136);
    return (NULL);
  }

  *Ndata = ftable[0].header[0].Naxis[1];
  nitems = ftable[0].header[0].Naxis[1];

  if ((swapped == NULL) || (*swapped == FALSE)) {

# ifdef BYTE_SWAP
      // we need to do the byte swap before applying the table scaling:
      byte = (unsigned char *) ftable[0].buffer;
      for (i = 0; i < nitems; i++, byte += 136) {
	  /** BYTE SWAP **/
	  SWAP_WORD (0); // IPP_IDET
	  SWAP_WORD (4); // X_PSF
	  SWAP_WORD (8); // Y_PSF
	  SWAP_WORD (12); // X_PSF_SIG
	  SWAP_WORD (16); // Y_PSF_SIG
	  SWAP_DBLE (20); // RA_PSF
	  SWAP_DBLE (28); // DEC_PSF
	  SWAP_WORD (36); // POSANGLE
	  SWAP_WORD (40); // PLTSCALE
	  SWAP_WORD (44); // PSF_INST_MAG
	  SWAP_WORD (48); // PSF_INST_MAG_SIG
	  SWAP_WORD (52); // AP_MAG_STANDARD
	  SWAP_WORD (56); // AP_MAG_RADIUS
	  SWAP_WORD (60); // PEAK_FLUX_AS_MAG
	  SWAP_WORD (64); // CAL_PSF_MAG
	  SWAP_WORD (68); // CAL_PSF_MAG_SIG
	  SWAP_WORD (72); // SKY
	  SWAP_WORD (76); // SKY_SIG
	  SWAP_WORD (80); // PSF_CHISQ
	  SWAP_WORD (84); // CR_NSIGMA
	  SWAP_WORD (88); // EXT_NSIGMA
	  SWAP_WORD (92); // PSF_MAJOR
	  SWAP_WORD (96); // PSF_MINOR
	  SWAP_WORD (100); // PSF_THETA
	  SWAP_WORD (104); // PSF_QF
	  SWAP_WORD (108); // PSF_NDOF
	  SWAP_WORD (112); // PSF_NPIX
	  SWAP_WORD (116); // MOMENTS_XX
	  SWAP_WORD (120); // MOMENTS_XY
	  SWAP_WORD (124); // MOMENTS_YY
	  SWAP_WORD (128); // FLAGS
	  SWAP_BYTE (132); // N_FRAMES
	  SWAP_BYTE (134); // PADDING
      }
# endif  

      gfits_table_scale_data (ftable);
      if (swapped != NULL) *swapped = TRUE;
  }

  byte = (unsigned char *) ftable[0].buffer;

  // allocate a new output data buffer
  ALLOCATE (output, CMF_PS1_V2, nitems);
  inbyte = (unsigned char *) byte;
  otbyte = (unsigned char *) output;

  // the data in the input table does not line up with the output structure: copy carefully.
  for (i = 0; i < nitems; i++, inbyte += 136, otbyte += 136) {
    memcpy (&otbyte[0],  &inbyte[0],  20);
    memcpy (&otbyte[20], &inbyte[36], 36);
    memcpy (&otbyte[56], &inbyte[20], 16);
    memcpy (&otbyte[72], &inbyte[72], 62);
  }

  free (ftable[0].buffer);
  ftable[0].buffer = (char *) output;

  // XXX other mods to make ftable consistent with CMF_PS1_V2? (Nx, EXTNAME?)

  return (output);
} 

// data organization (input vs output)
//              FITS                       struct
// WORD  0      IPP_IDET              0    IPP_IDET                
// WORD  4      X_PSF                 4    X_PSF                   
// WORD  8      Y_PSF                 8    Y_PSF                   
// WORD  12     X_PSF_SIG             12   X_PSF_SIG               
// WORD  16     Y_PSF_SIG             16   Y_PSF_SIG               
// DBLE  20     RA_PSF                20   POSANGLE                
// DBLE  28     DEC_PSF               24   PLTSCALE                
// WORD  36     POSANGLE              28   PSF_INST_MAG            
// WORD  40     PLTSCALE              32   PSF_INST_MAG_SIG        
// WORD  44     PSF_INST_MAG          36   AP_MAG_STANDARD         
// WORD  48     PSF_INST_MAG_SIG      40   AP_MAG_RADIUS           
// WORD  52     AP_MAG_STANDARD       44   PEAK_FLUX_AS_MAG        
// WORD  56     AP_MAG_RADIUS         48   CAL_PSF_MAG             
// WORD  60     PEAK_FLUX_AS_MAG      52   CAL_PSF_MAG_SIG         
// WORD  64     CAL_PSF_MAG           56   RA_PSF                  
// WORD  68     CAL_PSF_MAG_SIG       64   DEC_PSF                 
// WORD  72     SKY                   72   SKY                     
// WORD  76     SKY_SIG               76   SKY_SIG                 
// WORD  80     PSF_CHISQ             80   PSF_CHISQ               
// WORD  84     CR_NSIGMA             84   CR_NSIGMA               
// WORD  88     EXT_NSIGMA            88   EXT_NSIGMA              
// WORD  92     PSF_MAJOR             92   PSF_MAJOR               
// WORD  96     PSF_MINOR             96   PSF_MINOR               
// WORD  100    PSF_THETA             100  PSF_THETA               
// WORD  104    PSF_QF                104  PSF_QF                  
// WORD  108    PSF_NDOF              108  PSF_NDOF                
// WORD  112    PSF_NPIX              112  PSF_NPIX                
// WORD  116    MOMENTS_XX            116  MOMENTS_XX              
// WORD  120    MOMENTS_XY            120  MOMENTS_XY              
// WORD  124    MOMENTS_YY            124  MOMENTS_YY              
// WORD  128    FLAGS                 128  FLAGS                   
// BYTE  132    N_FRAMES              132  N_FRAMES                
// BYTE  134    PADDING               134  PADDING                 
