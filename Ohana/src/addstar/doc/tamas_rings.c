
/* 
 *  Prototype implementation of the optimal rings 
 *  by Tamas Budavari <budavari@pha.jhu.edu>
 *  2012-07-23
 *
 *  NOTES:
 *  - Does only one hemisphere (re-run or mirror)
 *  - Will leave a hole on top to be covered later
 *  - Based on PS1 internal draft and the C# code
 *  - Centers seem to match previous output
 *  - No further testing has beed done
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char* argv[])
{
    double d2r = M_PI / 180;

    // parameter 'a' is the cell size in degrees
    double adeg = 3.955;

    // half of 'a' in radians and its atan
    double halfa = adeg / 2 * d2r;
    double halftheta = atan(halfa);

    // loop init
    double d = 0; // starting Decl. - could change this...
    int ring = 0; // ring ID

    while (d < M_PI / 2 - halftheta) 
    {
        double dm = d - halftheta; // eq.5
        if (d == 0) dm = 0; // initial

	int m = (int)ceil(M_PI * cos(dm) / halftheta);  // eq.6        
	double ip = 2 * M_PI / m; // eq.7
        double dp = atan(tan(d + halftheta) * cos(ip / 2)); // eq.9

	// printf("%d  %d\n", ring, m); // ring & # of cells in that ring
	int i; // dump centers of the cells
	for (i=0; i<m; i++)
	{
	    // R.A. can use different phase per ring 
	    double a = i * ip; // + phase (watch wraparound) 
	    // print Decl. and R.A. in deg
	    printf(" \t %d   %25.20f   %25.20f\n", i, d/d2r, a/d2r);
	}

	// advance to next ring
	d = halftheta + dp;
	ring++;
    }
    return 0;
}

