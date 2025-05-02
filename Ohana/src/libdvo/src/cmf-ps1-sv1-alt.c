# include "dvo.h"

/* if we are not correctly including the ohana headers, this will fail */
# ifndef BYTE_SWAP
# ifndef NOT_BYTE_SWAP
# error "neither BYTE_SWAP not NOT_BYTE_SWAP is set"
# endif
# endif

CMF_PS1_SV1 *gfits_table_get_CMF_PS1_SV1_Alt (FTable *ftable, off_t *Ndata, char *swapped) {

  off_t i, nitems;
  unsigned char *byte, *inbyte, *otbyte, tmp;
  CMF_PS1_SV1 *output;

  /* provide initial values to avoid compiler warnings for non-BYTE_SWAP arch */
  i = tmp = 0;
  byte = NULL;

  // this function is a special case : it must have Nx = 196
  if (ftable[0].header[0].Naxis[0] != 196) { 
    fprintf (stderr, "ERROR: wrong format for CMF_PS1_SV1_Alt: "OFF_T_FMT" vs %d\n",  ftable[0].header[0].Naxis[0], 196);
    return (NULL);
  }

  *Ndata = ftable[0].header[0].Naxis[1];
  nitems = ftable[0].header[0].Naxis[1];

  if ((swapped == NULL) || (*swapped == FALSE)) {

# ifdef BYTE_SWAP
      // we need to do the byte swap before applying the table scaling:
      byte = (unsigned char *) ftable[0].buffer;
      for (i = 0; i < nitems; i++, byte += 196) {
	  /** BYTE SWAP **/
	  SWAP_WORD (0  ); // IPP_IDET
	  SWAP_WORD (4  ); // X_PSF
	  SWAP_WORD (8  ); // Y_PSF
	  SWAP_WORD (12 ); // X_PSF_SIG
	  SWAP_WORD (16 ); // Y_PSF_SIG
	  SWAP_WORD (20 ); // POSANGLE
	  SWAP_WORD (24 ); // PLTSCALE
	  SWAP_WORD (28 ); // PSF_INST_MAG    
	  SWAP_WORD (32 ); // PSF_INST_MAG_SIG
	  SWAP_WORD (36 ); // PSF_INST_FLUX    
	  SWAP_WORD (40 ); // PSF_INST_FLUX_SIG
	  SWAP_WORD (44 ); // AP_MAG_STANDARD
	  SWAP_WORD (48 ); // AP_MAG_RAW
	  SWAP_WORD (52 ); // AP_MAG_RADIUS  
	  SWAP_WORD (56 ); // PEAK_FLUX_AS_MAG
	  SWAP_WORD (60 ); // CAL_PSF_MAG     
	  SWAP_WORD (64 ); // CAL_PSF_MAG_SIG 
	  SWAP_DBLE (68 ); // RA_PSF
	  SWAP_DBLE (76 ); // DEC_PSF
	  SWAP_WORD (84 ); // SKY
	  SWAP_WORD (88 ); // SKY_SIG
	  SWAP_WORD (92 ); // PSF_CHISQ
	  SWAP_WORD (96 ); // CR_NSIGMA
	  SWAP_WORD (100); // EXT_NSIGMA
	  SWAP_WORD (104); // PSF_MAJOR
	  SWAP_WORD (108); // PSF_MINOR
	  SWAP_WORD (112); // PSF_THETA
	  SWAP_WORD (116); // PSF_QF
	  SWAP_WORD (120); // PSF_QF_PERFECT
	  SWAP_WORD (124); // PSF_NDOF	
	  SWAP_WORD (128); // PSF_NPIX	
	  SWAP_WORD (132); // MOMENTS_XX
	  SWAP_WORD (136); // MOMENTS_XY
	  SWAP_WORD (140); // MOMENTS_YY
	  SWAP_WORD (144); // MOMENTS_M3C    
	  SWAP_WORD (148); // MOMENTS_M3S    
	  SWAP_WORD (152); // MOMENTS_M4C    
	  SWAP_WORD (156); // MOMENTS_M4S    
	  SWAP_WORD (160); // MOMENTS_R1     
	  SWAP_WORD (164); // MOMENTS_RH     
	  SWAP_WORD (168); // KRON_FLUX      
	  SWAP_WORD (172); // KRON_FLUX_ERR  
	  SWAP_WORD (176); // KRON_FLUX_INNER
	  SWAP_WORD (180); // KRON_FLUX_OUTER
	  SWAP_WORD (184); // FLAGS
	  SWAP_WORD (188); // FLAGS2
	  SWAP_BYTE (192); // N_FRAMES
	  SWAP_BYTE (194); // PADDING
      }
# endif  

      gfits_table_scale_data (ftable);
      if (swapped != NULL) *swapped = TRUE;
  }

  byte = (unsigned char *) ftable[0].buffer;

  // allocate a new output data buffer
  ALLOCATE (output, CMF_PS1_SV1, nitems);
  inbyte = (unsigned char *) byte;
  otbyte = (unsigned char *) output;

  // the data in the input table does not line up with the output structure: copy carefully.
  for (i = 0; i < nitems; i++, inbyte += 196, otbyte += 200) {
    memcpy (&otbyte[  0], &inbyte[  0],  56); // IPP_IDET to AP_MAG_RADIUS
    memcpy (&otbyte[ 56], &inbyte[ 60],  24); // CAL_PSF_MAG to DEC_PSF
    memcpy (&otbyte[ 80], &inbyte[ 56],   4); // PEAK_FLUX_AS_MAG
    memcpy (&otbyte[ 84], &inbyte[ 84], 108); // SKY to FLAGS2
    memcpy (&otbyte[194], &inbyte[192],   2); // N_FRAMES
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
