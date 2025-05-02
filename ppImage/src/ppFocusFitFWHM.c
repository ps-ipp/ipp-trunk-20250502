#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "ppImage.h"

bool ppFocusFitFWHM (pmConfig *config, psVector *focus, psVector *fwhm) {

    float minFocus;

    psPolynomial1D *poly = psPolynomial1DAlloc (PS_POLYNOMIAL_ORD, 2);

    if (!psVectorFitPolynomial1D (poly, NULL, 0, fwhm, NULL, focus)) {
	psError (PS_ERR_UNKNOWN, false, "failed to fit focus/fwhm trend; invalid image data?");
	return false;
    }

    if (poly->coeff[2] <= 0.0) {
	psLogMsg ("ppFocus", 3, "poor focus fit: zero or negative curvature\n");
	psLogMsg ("ppFocus", 3, "fit coeffs: %f  %f  %f\n", 
		  poly->coeff[0], poly->coeff[1], poly->coeff[2]);
    }
    
    minFocus = -0.5 * poly->coeff[1] / poly->coeff[2];
    psLogMsg ("ppFocus", 3, "best fit focus: %f\n", minFocus);
    psLogMsg ("ppFocus", 3, "fwhm @ min: %f\n", psPolynomial1DEval (poly, minFocus));
    
    psFree (poly);
    return true;
}
