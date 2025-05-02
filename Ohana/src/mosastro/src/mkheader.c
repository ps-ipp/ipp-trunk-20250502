# include "mosastro.h"

Header *mkheader (int Nx, int Ny, int Nstars, Coords *coords) {

  Header *header;

  ALLOCATE (header, Header, 1);

  gfits_init_header (header);
  header[0].bitpix = -32;
  header[0].Naxes = 2;
  header[0].Naxis[0] = Nx;
  header[0].Naxis[1] = Ny;

  gfits_create_header (header);

  gfits_modify (header, "NSTARS",   "%d", 1, Nstars);
  gfits_modify (header, "PHOTCODE", "%s", 1, "STD.R");
  gfits_modify (header, "DATE-OBS", "%s", 1, "2004-04-22");
  gfits_modify (header, "UTC-OBS",  "%s", 1, "14:27:45.30");
  gfits_modify (header, "ZERO_PT", "%lf", 1, 25.0);
  gfits_modify (header, "EXPTIME", "%lf", 1, 2.0);

  PutCoords (coords, header);

  gfits_modify (header, "NASTRO",   "%d", 1, 1); 

  return (header);
}
