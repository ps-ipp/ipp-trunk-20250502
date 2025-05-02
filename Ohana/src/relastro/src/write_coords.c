# include "relastro.h"

# define CD_COORDS 1

void write_coords (Header *header, Coords *coords) {

  gfits_modify (header, "CTYPE1",   "%s",  1, "RA---TAN");
  gfits_modify (header, "CTYPE2",   "%s",  1, "DEC--TAN");

  gfits_modify (header, "CRVAL1",   "%lf", 1, coords[0].crval1);
  gfits_modify (header, "CRVAL2",   "%lf", 1, coords[0].crval2);  

  gfits_modify (header, "CRPIX1",   "%lf", 1, coords[0].crpix1);
  gfits_modify (header, "CRPIX2",   "%lf", 1, coords[0].crpix2);

# if (CD_COORDS)  
  gfits_modify (header, "CD1_1",    "%le", 1, coords[0].pc1_1 * coords[0].cdelt1);
  gfits_modify (header, "CD2_1",    "%le", 1, coords[0].pc2_1 * coords[0].cdelt1);
  gfits_modify (header, "CD1_2",    "%le", 1, coords[0].pc1_2 * coords[0].cdelt2);
  gfits_modify (header, "CD2_2",    "%le", 1, coords[0].pc2_2 * coords[0].cdelt2);
# else
  gfits_modify (header, "CDELT1",   "%le", 1, coords[0].cdelt1); 
  gfits_modify (header, "CDELT2",   "%le", 1, coords[0].cdelt2);
  gfits_modify (header, "PC001001", "%le", 1, coords[0].pc1_1);
  gfits_modify (header, "PC001002", "%le", 1, coords[0].pc1_2);
  gfits_modify (header, "PC002001", "%le", 1, coords[0].pc2_1);
  gfits_modify (header, "PC002002", "%le", 1, coords[0].pc2_2);
# endif
}
