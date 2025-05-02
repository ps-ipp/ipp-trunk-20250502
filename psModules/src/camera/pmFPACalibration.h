#ifndef PM_FPA_CALIBRATION_H
#define PM_FPA_CALIBRATION_H

/// Return the dark normalisation value
///
/// Unfortunately, dark current is not linear with the exposure time, but application of a polynomial
/// correction to the exposure time should make it linear.  This function returns the appropriate value with
/// which to normalise a dark frame.  The polynomial is obtained from DARK.NORM in the camera configuration.
/// The specific polynomial metadata to use is provided by DARK.NORM.KEY, which is keyword expanded in the
/// usual manner (e.g., try "{CHIP.NAME}").
float pmFPADarkNorm(const pmFPA *fpa,   ///< FPA for which to get the normalisation
                    const pmFPAview *view, ///< View to the FPA component of interest
                    float expTime       ///< The nominal exposure time
    );


#endif
