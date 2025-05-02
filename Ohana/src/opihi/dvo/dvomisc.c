# include "dvoshell.h"

void cprecess (Average *average, off_t Naverage, double in_epoch, double out_epoch) {

  off_t i;
  double T;
  double A, D, RA, DEC, zeta, z, theta;
  double SA, CA, SD, CD;
  
  T = (out_epoch - in_epoch) / 100.0;
  
  zeta  = RAD_DEG*(0.6406161*T + 0.0000839*T*T + 0.0000050*T*T*T);
  theta = RAD_DEG*(0.5567530*T - 0.0001185*T*T - 0.0000116*T*T*T);
  z     =          0.6406161*T + 0.0003041*T*T + 0.0000051*T*T*T;
  
  for (i = 0; i < Naverage; i++) {
    A = average[i].R;
    D = average[i].D;
    SD =  cos(RAD_DEG*A + zeta)*sin(theta)*cos(RAD_DEG*D) + cos(theta)*sin(RAD_DEG*D);
    CD = sqrt (1 - SD*SD);
    SA =  sin(RAD_DEG*A + zeta)*cos(RAD_DEG*D)/CD;
    CA = (cos(RAD_DEG*A + zeta)*cos(theta)*cos(RAD_DEG*D) - sin(theta)*sin(RAD_DEG*D))/CD;
    
    DEC = DEG_RAD*asin(SD);
    RA  = DEG_RAD*atan2(SA, CA) + z;
    
    if (RA < 0)
      RA += 360;
    
    average[i].R = RA;
    average[i].D = DEC; 
  }

}
