STRUCT  AstromOffsetMap_Disk_6x6
EXTNAME ASTROM_OFFSET_MAP_DISK_6x6
TYPE    BINTABLE
SIZE    312

# we have one row per correction image, with the max correction dimensions hard-wired in the structure

# ID is the ID of this correction image, image ID is the image being corrected
FIELD   tableID,   TABLE_ID,       unsigned int
FIELD   imageID,   IMAGE_ID,       unsigned int

# 6x6 is the max size; Nx,Ny give the actual size of the array
FIELD   Nx,	   NX,             int
FIELD   Ny,	   NY,             int

# for an image of size (NxBig,NyBig) and a map of size (Nx,Ny), the relationship between
# real and map pixels is given by

# Ix = ix * (Nx / NxBig)
# dX = (Nx / NxBig)
FIELD   dX,	   DX_SCALE,       float
FIELD   dY,	   DY_SCALE,       float

FIELD   dXv,       DX_VALUE,       float[6][6]
FIELD   dYv,       DY_VALUE,       float[6][6]

