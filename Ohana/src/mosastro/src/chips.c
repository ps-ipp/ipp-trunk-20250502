# include "mosastro.h"

/* set chip model based on initial header info */

/** when the chips and field terms are treated independently, they must be of 
    ctype PLY.  When we reach a solution, and write out a coupled set of chip 
    and field terms, then we need to transition to WRP (chip) & DIS (field
**/

int init_chips () {

  int i, j;
  double R, D, P, Q, L, M, Scale, Det;

  if (CHIPS != (char *) NULL) {
    load_chips (CHIPS);
    return (1);
  }

  for (i = 0; i < Nchip; i++) {

    /* bore site center guess */
    InitCoords (&chip[i].map, "DEC--PLY");

    /* find (L,M) coords of reference pixel (0,0) */
    XY_to_RD (&R, &D, 0.0, 0.0, &chip[i].coords);
    RD_to_XY (&P, &Q, R, D, &field.project);
    RD_to_XY (&L, &M, P, Q, &field.distort);
    chip[i].map.crval1 = L;
    chip[i].map.crval2 = M;

    /** we preserve the rotation and parity of coords.pc_ij, but renormalize to unity scale **/
    Det = chip[i].coords.pc1_1*chip[i].coords.pc2_2 - chip[i].coords.pc1_2*chip[i].coords.pc2_1;
    Scale = 1.0 / sqrt(fabs(chip[i].coords.cdelt1*chip[i].coords.cdelt2*Det));

    /** test for NaN Scale **/

    // XXX : temporarily drop the re-scaling to compare with psastro
    # if (PSASTRO_MODE)
    chip[i].map.pc1_1  = chip[i].coords.pc1_1;
    chip[i].map.pc2_2  = chip[i].coords.pc2_2;
    chip[i].map.pc1_2  = chip[i].coords.pc1_2;
    chip[i].map.pc2_1  = chip[i].coords.pc2_1;
    # else
    chip[i].map.pc1_1  = Scale * chip[i].coords.pc1_1 * chip[i].coords.cdelt1;
    chip[i].map.pc2_2  = Scale * chip[i].coords.pc2_2 * chip[i].coords.cdelt2;
    chip[i].map.pc1_2  = Scale * chip[i].coords.pc1_2 * chip[i].coords.cdelt2;
    chip[i].map.pc2_1  = Scale * chip[i].coords.pc2_1 * chip[i].coords.cdelt1;
    # endif

    # if 0
    // XXX this is the wrong choice: re-scaling each chip ruins distortion measurement
    chip[i].map.pc1_1  = chip[i].coords.pc1_1 * chip[i].coords.cdelt1 / chip[0].coords.cdelt1;
    chip[i].map.pc2_2  = chip[i].coords.pc2_2 * chip[i].coords.cdelt2 / chip[0].coords.cdelt2;
    chip[i].map.pc1_2  = chip[i].coords.pc1_2 * chip[i].coords.cdelt1 / chip[0].coords.cdelt1;
    chip[i].map.pc2_1  = chip[i].coords.pc2_1 * chip[i].coords.cdelt2 / chip[0].coords.cdelt2;
    # endif

    fprintf (stderr, "chip: %f %f (%f,%f),(%f,%f)\n", 
	     chip[i].map.crval1, chip[i].map.crval2, 
	     chip[i].map.pc1_1, chip[i].map.pc1_2, 
	     chip[i].map.pc2_1, chip[i].map.pc2_2);

    chip[i].map.Npolyterms = 1;
    for (j = 0; j < 7; j++) {
      chip[i].map.polyterms[j][0] = 0;
      chip[i].map.polyterms[j][1] = 0;
    }
  }
  return (1);
}

int load_chips (char *filename) {

  fprintf (stderr, "not ready yet\n");
  exit (1);
}

