STRUCT       Coords
EXTNAME      COORDS
TYPE         BINTABLE
SIZE         120
DESCRIPTION  DVO Coordinate Transformation Table

# elements of the Coords structure
FIELD  crval1,           CRVAL1,               double,   	 coordinate at reference pixel
FIELD  crval2,           CRVAL2,               double,  	 coordinate at reference pixel
FIELD  crpix1,           CRPIX1,               float,   	 coordinate of reference pixel
FIELD  crpix2,           CRPIX2,               float,   	 coordinate of reference pixel
FIELD  cdelt1,           CDELT1,               float,   	 degrees per pixel
FIELD  cdelt2,           CDELT2,               float,    	 degrees per pixel
FIELD  pc1_1,            PC1_1,                float,    	 rotation matrix
FIELD  pc1_2,            PC1_2,                float,    	 rotation matrix
FIELD  pc2_1,            PC2_1,                float,    	 rotation matrix
FIELD  pc2_2,            PC2_2,                float,    	 rotation matrix
FIELD  polyterms,        POLYTERMS,            float[7][2],	 higher order warping terms
FIELD  ctype,            CTYPE,                char[15],         coordinate type
FIELD  Npolyterms,       NPOLYTERMS,           char,     	 order of polynomial
FIELD  astrom_map_id,    ASTROM_MAP_ID,        unsigned int,   reference to 2D astrometry map
