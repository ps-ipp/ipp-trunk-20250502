#ifndef PM_SOURCE_MASKS_H
#define PM_SOURCE_MASKS_H

// Bit flags to distinguish analysis results
// When adding to or subtracting from this list, please also modify pmSourceMaskHeader
typedef enum {
    PM_SOURCE_MODE_DEFAULT          	  = 0x00000000, ///< Initial value: resets all bits
    PM_SOURCE_MODE_PSFMODEL         	  = 0x00000001, ///< Source fitted with a psf model (linear or non-linear)
    PM_SOURCE_MODE_EXTMODEL         	  = 0x00000002, ///< Source fitted with an extended-source model
    PM_SOURCE_MODE_FITTED           	  = 0x00000004, ///< Source fitted with non-linear model (PSF or EXT; good or bad)
    PM_SOURCE_MODE_FAIL             	  = 0x00000008, ///< Fit (non-linear) failed (non-converge, off-edge, run to zero)
    PM_SOURCE_MODE_POOR             	  = 0x00000010, ///< Fit succeeds, but low-SN, high-Chisq, or large (for PSF -- drop?)
    PM_SOURCE_MODE_PAIR             	  = 0x00000020, ///< Source fitted with a double psf
    PM_SOURCE_MODE_PSFSTAR          	  = 0x00000040, ///< Source used to define PSF model
    PM_SOURCE_MODE_SATSTAR          	  = 0x00000080, ///< Source model peak is above saturation
    PM_SOURCE_MODE_BLEND            	  = 0x00000100, ///< Source is a blend with other sources
    PM_SOURCE_MODE_EXTERNAL         	  = 0x00000200, ///< Source based on supplied input position
    PM_SOURCE_MODE_BADPSF           	  = 0x00000400, ///< Failed to get good estimate of object's PSF
    PM_SOURCE_MODE_DEFECT           	  = 0x00000800, ///< Source is thought to be a defect
    PM_SOURCE_MODE_SATURATED        	  = 0x00001000, ///< Source is thought to be saturated pixels (bleed trail)
    PM_SOURCE_MODE_CR_LIMIT         	  = 0x00002000, ///< Source has crNsigma above limit
    PM_SOURCE_MODE_EXT_LIMIT        	  = 0x00004000, ///< Source has extNsigma above limit
    PM_SOURCE_MODE_MOMENTS_FAILURE  	  = 0x00008000, ///< could not measure the moments
    PM_SOURCE_MODE_SKY_FAILURE      	  = 0x00010000, ///< could not measure the local sky
    PM_SOURCE_MODE_SKYVAR_FAILURE   	  = 0x00020000, ///< could not measure the local sky variance
    PM_SOURCE_MODE_BELOW_MOMENTS_SN 	  = 0x00040000, ///< moments not measured due to low S/N
    PM_SOURCE_MODE_BIG_RADIUS       	  = 0x00100000, ///< poor moments for small radius, try large radius
    PM_SOURCE_MODE_AP_MAGS          	  = 0x00200000, ///< source has an aperture magnitude
    PM_SOURCE_MODE_BLEND_FIT        	  = 0x00400000, ///< source was fitted as a blend
    PM_SOURCE_MODE_EXTENDED_FIT     	  = 0x00800000, ///< full extended fit was used
    PM_SOURCE_MODE_EXTENDED_STATS   	  = 0x01000000, ///< extended aperture stats calculated
    PM_SOURCE_MODE_LINEAR_FIT       	  = 0x02000000, ///< source fitted with the linear fit
    PM_SOURCE_MODE_NONLINEAR_FIT    	  = 0x04000000, ///< source fitted with the non-linear fit
    PM_SOURCE_MODE_RADIAL_FLUX      	  = 0x08000000, ///< radial flux measurements calculated
    PM_SOURCE_MODE_SIZE_SKIPPED     	  = 0x10000000, ///< size could not be determined
    PM_SOURCE_MODE_ON_SPIKE         	  = 0x20000000, ///< peak lands on diffraction spike
    PM_SOURCE_MODE_ON_GHOST         	  = 0x40000000, ///< peak lands on ghost or glint
    PM_SOURCE_MODE_OFF_CHIP         	  = 0x80000000, ///< peak lands off edge of chip
} pmSourceMode;

// Bit flags to distinguish analysis results
// When adding to or subtracting from this list, please also modify pmSourceMaskHeader
typedef enum {
    PM_SOURCE_MODE2_DEFAULT          	  = 0x00000000, ///< Initial value: resets all bits
    PM_SOURCE_MODE2_DIFF_WITH_SINGLE 	  = 0x00000001, ///< diff source matched to a single positive detection
    PM_SOURCE_MODE2_DIFF_WITH_DOUBLE 	  = 0x00000002, ///< diff source matched to positive detections in both images
    PM_SOURCE_MODE2_MATCHED          	  = 0x00000004, ///< source generated based on another image
    PM_SOURCE_MODE2_ON_SPIKE         	  = 0x00000008, ///< > 25% of (PSF-weighted) pixels land on diffraction spike
    PM_SOURCE_MODE2_ON_STARCORE      	  = 0x00000010, ///< > 25% of (PSF-weighted) pixels land on starcore
    PM_SOURCE_MODE2_ON_BURNTOOL      	  = 0x00000020, ///< > 25% of (PSF-weighted) pixels land on burntool
    PM_SOURCE_MODE2_ON_CONVPOOR      	  = 0x00000040, ///< > 25% of (PSF-weighted) pixels land on convpoor
    PM_SOURCE_MODE2_PASS1_SRC             = 0x00000080, ///< source detected in first pass analysis
    PM_SOURCE_MODE2_HAS_BRIGHTER_NEIGHBOR = 0x00000100, ///< peak is not the brightest in its footprint
    PM_SOURCE_MODE2_BRIGHT_NEIGHBOR_1     = 0x00000200, ///< flux_n / (r^2 flux_p) > 1
    PM_SOURCE_MODE2_BRIGHT_NEIGHBOR_10    = 0x00000400, ///< flux_n / (r^2 flux_p) > 10
    PM_SOURCE_MODE2_DIFF_SELF_MATCH  	  = 0x00000800, ///< positive detection match is probably this source 
    PM_SOURCE_MODE2_SATSTAR_PROFILE       = 0x00001000, ///< saturated source is modeled with a radial profile

    PM_SOURCE_MODE2_ECONTOUR_FEW_PTS      = 0x00002000, ///< too few points to measure the elliptical contour
    PM_SOURCE_MODE2_RADBIN_NAN_CENTER     = 0x00004000, ///< radial bins failed with too many NaN center bin
    PM_SOURCE_MODE2_PETRO_NAN_CENTER      = 0x00008000, ///< petrosian radial bins failed with too many NaN center bin
    PM_SOURCE_MODE2_PETRO_NO_PROFILE      = 0x00010000, ///< petrosian not build because radial bins missing

    PM_SOURCE_MODE2_PETRO_INSIG_RATIO     = 0x00020000, ///< insignificant measurement of petrosian ratio
    PM_SOURCE_MODE2_PETRO_RATIO_ZEROBIN   = 0x00040000, ///< petrosian ratio in the 0th bin (likely bad)
    
    PM_SOURCE_MODE2_EXT_FITS_RUN          = 0x00080000, ///< we attempted to run extended fits on this source
    PM_SOURCE_MODE2_EXT_FITS_FAIL         = 0x00100000, ///< at least one of the model fits failed
    PM_SOURCE_MODE2_EXT_FITS_RETRY        = 0x00200000, ///< one of the model fits was re-tried with new window
    PM_SOURCE_MODE2_EXT_FITS_NONE         = 0x00400000, ///< ALL of the model fits failed
    
    PM_SOURCE_MODE2_ON_GHOST      	  = 0x00800000, ///< > 25% of (PSF-weighted) pixels land on ghost
    PM_SOURCE_MODE2_ON_CROSSTALK      	  = 0x01000000, ///< peaks land on electronic crostalk
    PM_SOURCE_MODE2_ON_CTE      	  = 0x02000000, ///< peaks land on CTE region

    
} pmSourceMode2;

/// Populate header with mask values
bool pmSourceMasksHeader(
    psMetadata *header                  ///< Header to populate
    );



#endif
