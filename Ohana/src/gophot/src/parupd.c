# include "gophot.h"

parupd (float *instar, float *outstar, int ix, int iy) {

	outstar[0] = instar[0]*ufactor;
	outstar[1] = instar[1]*ufactor;
	outstar[2] = instar[2] + ix;
	outstar[3] = instar[3] + iy;
	outstar[4] = instar[4];
	outstar[5] = instar[5];
	outstar[6] = instar[6];
}

/* a legacy function 
twoupd (float *instar, float *outstar, int ix, int iy) {

	outstar[0] = instar[0];
	outstar[1] = instar[1];
	outstar[2] = instar[2] + ix;
	outstar[3] = instar[3] + iy;
	outstar[4] = instar[4];
	outstar[5] = instar[5] + ix;
	outstar[6] = instar[6] + iy;
}
*/
