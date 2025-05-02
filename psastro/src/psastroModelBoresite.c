/** @file psastroModelBoresite.c
 *
 *  @brief
 *
 *  @ingroup psastroModel
 *
 *  @author IfA
 *  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"

/**
 * the full chisq is built of two associated sums over coordinates:
 * chisq = sum ((X_obs - X_fit(t))^2 + (Y_obs - Y_fit(t))^2)
 * we use split this into a 2x long vector and use coord[1] to distinguish the X and Y terms:
 * coord[0] = measured X or measured Y
 * coord[1] =          0 or          1
 */
psF32 psastroModelBoresite (psVector *deriv, const psVector *params, const psVector *coord) {

    psF32 *PAR = params->data.F32;

    float csphi = cos(PAR[PAR_P0]);
    float snphi = sin(PAR[PAR_P0]);

    float dtheta = coord->data.F32[0] - PAR[PAR_T0];
    float cstht = cos(dtheta);
    float sntht = sin(dtheta);

    // value is X
    if (coord->data.F32[1] == 0) {

	float value = PAR[PAR_X0] + PAR[PAR_RX]*cstht*csphi + PAR[PAR_RY]*sntht*snphi;

	if (deriv) {
	    psF32 *dPAR = deriv->data.F32;
	    dPAR[PAR_X0] = 1.0;
	    dPAR[PAR_Y0] = 0.0;
	    dPAR[PAR_RX] = +cstht*csphi;
	    dPAR[PAR_RY] = +sntht*snphi;
	    dPAR[PAR_P0] = -PAR[PAR_RX]*cstht*snphi + PAR[PAR_RY]*sntht*csphi;
	    dPAR[PAR_T0] =  PAR[PAR_RX]*sntht*csphi - PAR[PAR_RY]*cstht*snphi;
	}
	return (value);
    }  

    // value is Y
    if (coord->data.F32[1] == 1) {
	float value = PAR[PAR_Y0] + PAR[PAR_RY]*sntht*csphi - PAR[PAR_RX]*cstht*snphi;

	if (deriv) {
	    psF32 *dPAR = deriv->data.F32;
	    dPAR[PAR_X0]  = 0.0;
	    dPAR[PAR_Y0]  = 1.0;
	    dPAR[PAR_RX]  = -cstht*snphi;
	    dPAR[PAR_RY]  = +sntht*csphi;
	    dPAR[PAR_P0]  = -PAR[PAR_RY]*sntht*snphi - PAR[PAR_RX]*cstht*csphi;
	    dPAR[PAR_T0]  = -PAR[PAR_RY]*cstht*csphi - PAR[PAR_RX]*sntht*snphi;
	}
	return (value);
    }  
    psAbort ("programming error: invalid coordinate");
}
